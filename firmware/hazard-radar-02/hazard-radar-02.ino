#include <SPI.h>
#include <WiFiNINA.h>
#include <ArduinoBLE.h>
#include <Arduino_MKRIoTCarrier.h>
#include <ArduinoJson.h>
#include <safety_protocol.h>
#include <arduino_secrets.h>

MKRIoTCarrier carrier;
WiFiClient wifiClient;

// De 3 tilstande som radaren kan vaere i
enum RadarState {
  IDLE,         // Alt er roligt (groent lys, ingen lyd)
  DETECTING,    // Arbejderen er taet paa, vi taeller 1.5 sek (debounce)
  ALARM_ACTIVE  // Faren er bekraeftet: blinkende roedt lys + sirene!
};

RadarState currentState = IDLE;
unsigned long detectStartTime = 0;
unsigned long lastTagSeenTime = 0;
unsigned long lastPrintTime = 0;
unsigned long lastBlinkTime = 0;
unsigned long lastWifiAttemptTime = 0;
unsigned long ackCooldownUntil = 0;
bool alarmBlinkOn = false;

// Ringbuffer til de sidste 5 RSSI-maalinger (RSSI_WINDOW_SIZE = 5)
int rssiSamples[RSSI_WINDOW_SIZE];
int rssiIndex = 0;
int rssiCount = 0;
int lastAvgRssi = -100;

// Nulstiller RSSI-filteret efter en kvittering
void resetRssiFilter() {
  rssiIndex = 0;
  rssiCount = 0;
  lastAvgRssi = -100;
}

// Gemmer ny RSSI-maaling og returnerer gennemsnittet af op til 5 maalinger
int updateRssiAverage(int newRssi) {
  rssiSamples[rssiIndex] = newRssi;
  rssiIndex = (rssiIndex + 1) % RSSI_WINDOW_SIZE;

  if (rssiCount < RSSI_WINDOW_SIZE) {
    rssiCount++;
  }

  long sum = 0;
  for (int i = 0; i < rssiCount; i++) {
    sum += rssiSamples[i];
  }

  return (int)(sum / rssiCount);
}

// Styrer LED-farve og sirene ud fra om alarmen er aktiv
void setAlarmHardware(bool alarmOn) {
  if (alarmOn) {
    carrier.leds.fill(carrier.leds.Color(255, 0, 0), 0, 5); // Roed
    carrier.leds.show();
    carrier.Buzzer.sound(1800);
  } else {
    carrier.leds.fill(carrier.leds.Color(0, 255, 0), 0, 5); // Groed (sikker)
    carrier.leds.show();
    carrier.Buzzer.noSound();
  }
}

// Ikke-blokerende blink og bip-lyd under aktiv alarm
void updateAlarmBlink(unsigned long now) {
  if (now - lastBlinkTime >= ALARM_BLINK_INTERVAL_MS) {
    lastBlinkTime = now;
    alarmBlinkOn = !alarmBlinkOn;

    if (alarmBlinkOn) {
      carrier.leds.fill(carrier.leds.Color(255, 0, 0), 0, 5);
      carrier.Buzzer.sound(1800);
    } else {
      carrier.leds.fill(carrier.leds.Color(0, 0, 0), 0, 5);
      carrier.Buzzer.noSound();
    }
    carrier.leds.show();
  }
}

// Sender JSON-event til C# backend (POST /api/telemetry/event)
bool sendTelemetryEvent(const char* eventType, const char* bleUuid, int rssi) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Wi-Fi offline: kan ikke sende event (lokal Fail-Safe koerer videre).");
    return false;
  }

  JsonDocument doc;
  doc["eventType"] = eventType;
  if (bleUuid != nullptr) {
    doc["bleUuid"] = bleUuid;
  }
  if (rssi != 0) {
    doc["rssi"] = rssi;
  }
  doc["zoneState"] = ZONE_ACTIVE;

  char jsonBody[256];
  size_t bodyLen = serializeJson(doc, jsonBody, sizeof(jsonBody));

  wifiClient.stop();
  if (!wifiClient.connect(SERVER_HOST, SERVER_PORT)) {
    Serial.println("FEJL: Kunne ikke forbinde til C# backend!");
    return false;
  }

  char hostHeader[96];
  char contentLengthHeader[48];
  snprintf(hostHeader, sizeof(hostHeader), "Host: %s:%d", SERVER_HOST, SERVER_PORT);
  snprintf(contentLengthHeader, sizeof(contentLengthHeader), "Content-Length: %u", (unsigned int)bodyLen);

  wifiClient.print("POST ");
  wifiClient.print(API_PATH_TELEMETRY);
  wifiClient.println(" HTTP/1.1");
  wifiClient.println(hostHeader);
  wifiClient.print("X-Device-Id: ");
  wifiClient.println(DEVICE_ID_RADAR);
  wifiClient.println("Content-Type: application/json");
  wifiClient.println(contentLengthHeader);
  wifiClient.println("Connection: close");
  wifiClient.println();
  wifiClient.println(jsonBody);

  // Vent kort paa svar-linjen (maks 1.5 sek), saa vi ikke blokerer loopen
  unsigned long waitStart = millis();
  while (wifiClient.connected() && !wifiClient.available() && (millis() - waitStart < 1500)) {
  }

  if (wifiClient.available()) {
    char statusLine[64];
    size_t len = wifiClient.readBytesUntil('\n', statusLine, sizeof(statusLine) - 1);
    statusLine[len] = '\0';
    Serial.print("HTTP svar: ");
    Serial.println(statusLine);
  }

  wifiClient.stop();
  return true;
}

// Sender et HEARTBEAT-signal til C# backend hvert 5. sekund (kun naar vi ikke midt i 1.5s debounce)
void sendHeartbeatIfDue(unsigned long now) {
  if (currentState == DETECTING) {
    return;
  }

  if (now - lastPrintTime >= HEARTBEAT_INTERVAL_MS) {
    lastPrintTime = now;
    Serial.println("Sender HEARTBEAT til server...");
    sendTelemetryEvent(EVENT_HEARTBEAT, nullptr, 0);
  }
}

// Forbinder til Wi-Fi uden at fryse systemet permanent, hvis routeren er nede
void connectWiFiOnce() {
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("ADVARSEL: Wi-Fi modul ikke fundet!");
    return;
  }

  Serial.print("Forbinder til Wi-Fi: ");
  Serial.println(WIFI_SSID);
  int status = WiFi.begin(WIFI_SSID, WIFI_PASS);

  if (status == WL_CONNECTED) {
    Serial.print("Wi-Fi forbundet! IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("Wi-Fi ikke klar endnu (Fail-Safe aktiv).");
  }
}

void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 3000) {}

  CARRIER_CASE = false;
  carrier.begin();
  setAlarmHardware(false);

  Serial.print("Starter enhed: ");
  Serial.println(DEVICE_ID_RADAR);

  // 1. Starter Bluetooth-modulet foerst (NINA-W102 samtidig BLE + Wi-Fi)
  if (!BLE.begin()) {
    Serial.println("FEJL: Kunne ikke starte BLE!");
    while (true) {} // Stopper her, hvis BLE-chippen ikke svarer
  }

  BLE.scanForUuid(WORKER_BEACON_UUID);
  Serial.println("BLE-scanner startet. Lytter efter hjelm-tag...");

  // 2. Forbinder til Wi-Fi bagefter (blokerer ikke permanent ved fejl)
  connectWiFiOnce();
  lastWifiAttemptTime = millis();
}

void loop() {
  unsigned long now = millis();

  // 1. Opdater touch-knapper paa Carrier og tjek Touch0 (LOCAL_ACK)
  carrier.Buttons.update();
  if (currentState == ALARM_ACTIVE && carrier.Buttons.onTouchDown(TOUCH0)) {
    currentState = IDLE;
    setAlarmHardware(false);
    resetRssiFilter();
    ackCooldownUntil = now + ACK_COOLDOWN_MS;
    Serial.println("LOCAL_ACK! Alarm kvitteret via Touch0.");
    sendTelemetryEvent(EVENT_LOCAL_ACK, WORKER_BEACON_UUID, lastAvgRssi);
  }

  // 2. Heartbeat-timer (hvert 5. sekund)
  sendHeartbeatIfDue(now);

  // 3. Forsoeg kun at genoprette Wi-Fi i IDLE, saa det ikke forsinker BLE-detektion eller alarm
  if (currentState == IDLE && WiFi.status() != WL_CONNECTED && (now - lastWifiAttemptTime >= WIFI_RETRY_INTERVAL_MS)) {
    lastWifiAttemptTime = now;
    connectWiFiOnce();
  }

  // 4. Blink LED og sirene hvis alarmen er aktiv
  if (currentState == ALARM_ACTIVE) {
    updateAlarmBlink(now);
    return;
  }

  // 5. Efter lokal kvittering venter vi kort, saa alarmen ikke genstarter med det samme
  if (now < ackCooldownUntil) {
    BLEDevice ignored = BLE.available();
    (void)ignored;
    return;
  }

  // 6. Tjek om vi har fanget en BLE-pakke fra vores tag
  BLEDevice peripheral = BLE.available();
  if (peripheral) {
    lastTagSeenTime = now;
    int rawRssi = peripheral.rssi();
    int avgRssi = updateRssiAverage(rawRssi);
    lastAvgRssi = avgRssi;

    Serial.print("Raw RSSI: ");
    Serial.print(rawRssi);
    Serial.print(" | Avg(5): ");
    Serial.println(avgRssi);

    if (avgRssi >= RSSI_ALARM_THRESHOLD) {
      if (currentState == IDLE) {
        currentState = DETECTING;
        detectStartTime = now;
        Serial.println("Status -> DETECTING (venter 1.5s)...");
      }
    } else if (currentState == DETECTING) {
      currentState = IDLE;
      Serial.println("Falsk alarm undgaaet -> tilbage til IDLE");
    }
  }

  // 7. Bekraeft alarm, hvis arbejderen har vaeret i farezonen i 1.5s (debounce)
  if (currentState == DETECTING) {
    if (now - lastTagSeenTime > 2000) {
      currentState = IDLE;
      Serial.println("Tag forsvandt -> tilbage til IDLE");
    } else if (now - detectStartTime >= RSSI_DEBOUNCE_MS) {
      currentState = ALARM_ACTIVE;
      alarmBlinkOn = true;
      lastBlinkTime = now;
      setAlarmHardware(true);
      Serial.println("ALARM_ACTIVE! Sirene og roede LED'er taendt!");
      sendTelemetryEvent(EVENT_CRANE_RADAR, WORKER_BEACON_UUID, lastAvgRssi);
    }
  }
}
