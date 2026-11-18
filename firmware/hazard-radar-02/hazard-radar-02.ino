#include <ArduinoBLE.h>
#include "../shared/safety_protocol.h"
#include "../shared/arduino_secrets.h"

unsigned long lastPrintTime = 0;

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
    int rssi = peripheral.rssi();
    Serial.print("Arbejder-tag fundet! RSSI = ");
    Serial.println(rssi);
  }
}
