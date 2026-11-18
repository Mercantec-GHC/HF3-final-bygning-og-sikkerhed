#include <ArduinoBLE.h>
#include "../shared/safety_protocol.h"
#include "../shared/arduino_secrets.h"

unsigned long lastPrintTime = 0;

// Ringbuffer til de sidste 5 RSSI-maalinger (RSSI_WINDOW_SIZE = 5)
int rssiSamples[RSSI_WINDOW_SIZE];
int rssiIndex = 0;
int rssiCount = 0;

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

void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 3000) {}

  Serial.print("Starter enhed: ");
  Serial.println(DEVICE_ID_RADAR);

  // Starter Bluetooth-modulet paa Arduino
  if (!BLE.begin()) {
    Serial.println("FEJL: Kunne ikke starte BLE!");
    while (true) {} // Stopper her, hvis BLE-chippen ikke svarer
  }

  // Begynd at lytte efter BLE-enheder med vores arbejder-UUID
  BLE.scanForUuid(WORKER_BEACON_UUID);
  Serial.println("BLE-scanner startet. Lytter efter hjelm-tag...");
}

void loop() {
  unsigned long now = millis();

  // 1. Heartbeat-timer (hvert 5. sekund)
  if (now - lastPrintTime >= HEARTBEAT_INTERVAL_MS) {
    lastPrintTime = now;
    Serial.println("Radar er klar (HEARTBEAT)...");
  }

  // 2. Tjek om vi har fanget en BLE-pakke fra vores tag
  BLEDevice peripheral = BLE.available();
  if (peripheral) {
    int rawRssi = peripheral.rssi();
    int avgRssi = updateRssiAverage(rawRssi);

    Serial.print("Raw RSSI: ");
    Serial.print(rawRssi);
    Serial.print(" | Avg(5): ");
    Serial.println(avgRssi);
  }
}
