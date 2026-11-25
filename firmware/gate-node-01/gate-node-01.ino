#include <SPI.h>
#include <WiFiNINA.h>
#include <Arduino_MKRIoTCarrier.h>
#include <ArduinoJson.h>
#include <Adafruit_SleepyDog.h>
#include <qrcode.h>
#include <safety_protocol.h>
#include <safety_crypto.h>
#include <arduino_secrets.h>

MKRIoTCarrier carrier;
WiFiClient wifiClient;

unsigned long lastWifiAttemptTime = 0;
unsigned long lastPinCheckTime = 0;
unsigned long lastHeartbeatTime = 0;
unsigned long lastClimateTime = 0;
unsigned long lastPinWindow = 0;
int currentPin = -1;
bool wdtEnabled = false;

// Tegner kun PIN-feltet under QR-koden, saa selve QR-koden aldrig blinker
void drawPinText(int pin) {
  carrier.display.fillRect(20, 164, 200, 36, ST77XX_BLACK);
  carrier.display.setTextColor(ST77XX_GREEN);
  carrier.display.setTextSize(3);
  carrier.display.setCursor(42, 170);

  if (pin < 1000) {
    carrier.display.print("PIN: ----");
  } else {
    char pinText[16];
    snprintf(pinText, sizeof(pinText), "PIN: %04d", pin);
    carrier.display.print(pinText);
  }
}

// Tegner en hoej-kontrast QR-kode (sort paa hvid baggrund) midt paa 240x240 skaermen
void drawCheckInQr(const char* url) {
  const uint8_t qrVersion = 4; // 33x33 felter (plads til op til 78 tegn)
  QRCode qrcode;
  uint8_t qrcodeData[qrcode_getBufferSize(qrVersion)];
  qrcode_initText(&qrcode, qrcodeData, qrVersion, ECC_LOW, url);

  const int scale = 3; // Hvert QR-felt bliver 3x3 pixels -> 99x99 px
  const int qrPixelSize = qrcode.size * scale;
  const int quietZone = 8; // Hvid ramme rundt om QR-koden (kraevet af mobilkameraer)
  const int boxSize = qrPixelSize + (quietZone * 2);

  const int boxX = (240 - boxSize) / 2;
  const int boxY = 38;
  const int qrStartX = boxX + quietZone;
  const int qrStartY = boxY + quietZone;

  carrier.display.fillScreen(ST77XX_BLACK);

  // Overskrift oeverst
  carrier.display.setTextColor(ST77XX_WHITE);
  carrier.display.setTextSize(2);
  carrier.display.setCursor(72, 14);
  carrier.display.print("CHECK-IN");

  // Hvid baggrund til QR-koden
  carrier.display.fillRect(boxX, boxY, boxSize, boxSize, ST77XX_WHITE);

  // Tegn de sorte firkanter i QR-koden
  for (uint8_t y = 0; y < qrcode.size; y++) {
    for (uint8_t x = 0; x < qrcode.size; x++) {
      if (qrcode_getModule(&qrcode, x, y)) {
        carrier.display.fillRect(
          qrStartX + (x * scale),
          qrStartY + (y * scale),
          scale,
          scale,
          ST77XX_BLACK
        );
      }
    }
  }

  drawPinText(currentPin);
}

// Sender JSON-event fra Gate Node til C# backend (POST /api/telemetry/event)
bool sendGateEvent(const char* eventType, bool includeClimate, float tempC, float humPct) {
  if (WiFi.status() != WL_CONNECTED) {
    return false;
  }

  JsonDocument doc;
  doc["eventType"] = eventType;
  if (includeClimate) {
    doc["temperature"] = tempC;
    doc["humidity"] = humPct;
  }
  doc["zoneState"] = ZONE_ACTIVE;

  char jsonBody[256];
  size_t bodyLen = serializeJson(doc, jsonBody, sizeof(jsonBody));

  Watchdog.reset();
  wifiClient.stop();
  if (!wifiClient.connect(SERVER_HOST, SERVER_PORT)) {
    Watchdog.reset();
    Serial.println("FEJL: Gate Node kunne ikke forbinde til C# backend!");
    return false;
  }
  Watchdog.reset();

  char hostHeader[96];
  char contentLengthHeader[48];
  snprintf(hostHeader, sizeof(hostHeader), "Host: %s:%d", SERVER_HOST, SERVER_PORT);
  snprintf(contentLengthHeader, sizeof(contentLengthHeader), "Content-Length: %u", (unsigned int)bodyLen);

  wifiClient.print("POST ");
  wifiClient.print(API_PATH_TELEMETRY);
  wifiClient.println(" HTTP/1.1");
  wifiClient.println(hostHeader);
  wifiClient.print("X-Device-Id: ");
  wifiClient.println(DEVICE_ID_GATE);
  wifiClient.println("Content-Type: application/json");
  wifiClient.println(contentLengthHeader);
  wifiClient.println("Connection: close");
  wifiClient.println();
  wifiClient.print(jsonBody);

  unsigned long waitStart = millis();
  while (wifiClient.connected() && !wifiClient.available() && (millis() - waitStart < 1500)) {
    Watchdog.reset();
  }

  if (wifiClient.available()) {
    char statusLine[64];
    size_t len = wifiClient.readBytesUntil('\n', statusLine, sizeof(statusLine) - 1);
    statusLine[len] = '\0';
    Serial.print("HTTP svar: ");
    Serial.println(statusLine);
  }

  wifiClient.stop();
  Watchdog.reset();
  return true;
}

void connectWiFiOnce() {
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("ADVARSEL: Wi-Fi modul ikke fundet!");
    return;
  }

  Serial.print("Forbinder til Wi-Fi: ");
  Serial.println(WIFI_SSID);

  if (wdtEnabled) {
    Watchdog.disable();
  }
  int status = WiFi.begin(WIFI_SSID, WIFI_PASS);
  if (wdtEnabled) {
    Watchdog.enable(WATCHDOG_TIMEOUT_MS);
  }

  if (status == WL_CONNECTED) {
    Serial.print("Wi-Fi forbundet! IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("Wi-Fi ikke klar endnu.");
  }
}

// Henter NTP-tid fra WiFi-modulet og opdaterer 60s TOTP PIN, naar et nyt minut starter
void updateTotpPinIfDue(unsigned long now) {
  if (now - lastPinCheckTime < 1000) {
    return;
  }
  lastPinCheckTime = now;

  if (WiFi.status() != WL_CONNECTED) {
    return;
  }

  unsigned long unixEpoch = WiFi.getTime();
  if (unixEpoch == 0) {
    return; // NTP-tid er ikke synkroniseret endnu
  }

  unsigned long window = unixEpoch / 60UL;
  if (window != lastPinWindow) {
    lastPinWindow = window;
    currentPin = SafetyCrypto::computeTotpPin(SHARED_SECRET_KEY, unixEpoch);
    drawPinText(currentPin);

    Serial.print("Ny 60s TOTP PIN: ");
    Serial.println(currentPin);
  }
}

// Maaler temperatur og fugtighed (HTS221) hvert 30. sekund og sender CLIMATE_DATA
void sendClimateIfDue(unsigned long now) {
  if (now - lastClimateTime < CLIMATE_INTERVAL_MS) {
    return;
  }
  lastClimateTime = now;

  float temperature = carrier.Env.readTemperature();
  float humidity = carrier.Env.readHumidity();

  Serial.print("Klima -> Temp: ");
  Serial.print(temperature, 1);
  Serial.print(" C | Fugt: ");
  Serial.print(humidity, 1);
  Serial.println(" %");

  sendGateEvent(EVENT_CLIMATE_DATA, true, temperature, humidity);
}

// Sender HEARTBEAT hvert 5. sekund, saa dashboardet ved at Gate Node er online
void sendHeartbeatIfDue(unsigned long now) {
  if (now - lastHeartbeatTime < HEARTBEAT_INTERVAL_MS) {
    return;
  }
  lastHeartbeatTime = now;

  Serial.println("Gate Node sender HEARTBEAT...");
  sendGateEvent(EVENT_HEARTBEAT, false, 0.0f, 0.0f);
}

void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 3000) {}

  CARRIER_CASE = false;
  carrier.begin();

  Serial.print("Starter enhed: ");
  Serial.println(DEVICE_ID_GATE);

  // Groent statuslys paa Gate Node
  carrier.leds.fill(carrier.leds.Color(0, 255, 0), 0, 5);
  carrier.leds.show();

  drawCheckInQr(CHECKIN_URL);
  connectWiFiOnce();
  lastWifiAttemptTime = millis();

  // Aktiverer 2s hardware Watchdog Timer
  int actualWdtMs = Watchdog.enable(WATCHDOG_TIMEOUT_MS);
  wdtEnabled = true;
  Serial.print("Hardware Watchdog aktiv (ms): ");
  Serial.println(actualWdtMs);
}

void loop() {
  Watchdog.reset();
  unsigned long now = millis();

  if (WiFi.status() != WL_CONNECTED && (now - lastWifiAttemptTime >= WIFI_RETRY_INTERVAL_MS)) {
    lastWifiAttemptTime = now;
    connectWiFiOnce();
  }

  updateTotpPinIfDue(now);
  sendClimateIfDue(now);
  sendHeartbeatIfDue(now);
}
