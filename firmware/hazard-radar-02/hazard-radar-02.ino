#include "../shared/safety_protocol.h"

// Gemmer tidspunktet (i ms) for sidste besked
unsigned long lastPrintTime = 0;

void setup() {
  // Starter seriel kommunikation med PC (115200 bits/sek)
  Serial.begin(115200);
  while (!Serial && millis() < 3000) {
    // Venter maks 3 sek paa at USB Serial bliver klar
  }

  Serial.print("Starter enhed: ");
  Serial.println(DEVICE_ID_RADAR);
}

void loop() {
  unsigned long now = millis();

  // Koerer hvert 5. sekund UDEN at blokere med delay()
  if (now - lastPrintTime >= HEARTBEAT_INTERVAL_MS) {
    lastPrintTime = now;
    Serial.println("Radar er klar (venter paa BLE & Wi-Fi)...");
  }
}
