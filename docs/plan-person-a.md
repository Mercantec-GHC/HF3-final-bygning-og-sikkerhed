# Arbejdsplan — Person A: Hardware, Embedded C++ & Edge Security

* **Ansvarsdomæne:** Embedded C++, `2 × Arduino MKR WiFi 1010 + IoT Carrier` & Edge Security (`firmware/`)
* **Git-branch:** `feat/person-a` (merges løbende til `dev`)
* **Periode:** 16. november 2026 — 10. december 2026
* **Milepæle:** `v0.1-mvp` (20. nov) $\rightarrow$ `v0.2-core` (`Must have` færdig 27. nov) $\rightarrow$ `v0.3-security` (4. dec) $\rightarrow$ `v1.0-release` (10. dec)
* **Integrationspartner:** Person B (C# Backend, PWA & Cloud)

---

## 1. Leverancer opdelt efter MoSCoW

| Prioritet | Tekniske leverancer for Person A |
| :--- | :--- |
| **MUST HAVE**<br>*(Sprint 1 & 2: 16. – 27. nov)* | 1. **Firmware-opgradering:** Begge `Arduino MKR WiFi 1010` opdateret til `NINA-W102 3.0.1+` med samtidig Wi-Fi og BLE.<br>2. **`Arduino #2` (`Hazard Radar`):** BLE-scanner med **5-punkts glidende gennemsnit** på `RSSI` + $1.5\text{ s}$ `Debounce`, lokal sirene + rød LED (*Hardware Fail-Safe*), lokal touch-kvittering (`Touch0`), fjern-slukning af sirene via `GET /state` samt $5\text{ s}$ `HEARTBEAT` og $2\text{ s}$ `Watchdog Timer`.<br>3. **`Arduino #1` (`Gate Node`):** QR-kode + $60\text{ s}$ dynamisk TOTP PIN-kode på displayet samt `HTS221` temperatur/fugtighedsmåling hvert $30\text{ s}$. |
| **SHOULD HAVE**<br>*(Sprint 3: 28. nov – 4. dec)* | 1. **Pakkesignering:** `HMAC-SHA256` signatur + NTP Unix `X-Timestamp` ($5\text{ s}$ vindue) på alle udgående JSON-pakker.<br>2. **Fysisk Sabotagedetektion:** `LSM6DS3` accelerometer/gyroskop på `Arduino #2` sender `SECURITY_TAMPER` ved stød/flytning.<br>3. **Zonestyring:** `Touch1` skifter zonen mellem `ACTIVE` (grøn) og `PAUSED` (gul); `Touch2` kører manuel alarmtest. |
| **NICE TO HAVE**<br>*(Sprint 4: hvis tid)* | 1. **Boot Splash & Diagnostik:** Opstartsskærm på Carrier-display med enheds-ID, IP og NTP-status.<br>2. **Visuel klimavarsling:** Rød ramme på `Arduino #1`-displayet ved temperatur $>30^\circ\text{C}$. |

---

## 2. Hardware- og Protokolspecifikation

| Enhed | Mappe i repo | Anvendte Carrier-moduler | Rolle |
| :--- | :--- | :--- | :--- |
| `ARDUINO-GATE-01` | `firmware/gate-node-01/` | TFT Display, `HTS221` (Temp/Fugt), RGB LED, `NINA-W102` Wi-Fi | Viser QR + $60\text{ s}$ PIN; sender klima hvert $30\text{ s}$ og heartbeat hvert $5\text{ s}$. |
| `ARDUINO-HAZARD-02` | `firmware/hazard-radar-02/` | `NINA-W102` (BLE + Wi-Fi), Buzzer, 5 × RGB LED, `LSM6DS3`, `Touch0–2` | Måler BLE `RSSI`; styrer sirene/LED; `Touch0` = Lokal ACK; `LSM6DS3` = Tamper. |
| Fælles C++ kode | `firmware/shared/` | Delt protokol, `HMAC-SHA256` og TOTP PIN-beregning | Deles mellem begge Arduino-boards. |

### JSON-kontrakt (`Arduino -> C# API`)

```http
POST /api/telemetry/event HTTP/1.1
Content-Type: application/json
X-Device-Id: ARDUINO-HAZARD-02
X-Timestamp: 1794908400
X-Signature: <hex(HMAC-SHA256(SecretKey, DeviceId + ":" + Timestamp + ":" + RawJsonBody))>

{
  "eventType": "CRANE_RADAR",
  "bleUuid": "f7826da6-4fa2-4e98-8024-bc5b71e0893e",
  "rssi": -58,
  "temperature": 22.4,
  "humidity": 48.0,
  "zoneState": "ACTIVE"
}
```

### Polling af alarmstatus (`Arduino #2 -> C# API` hvert $1.5\text{ s}$ under aktiv alarm)

```http
GET /api/devices/ARDUINO-HAZARD-02/state HTTP/1.1
```
Svar: `{"alarmActive": false, "zoneState": "ACTIVE", "command": "CLEAR_ALARM"}`

---

## 3. Dag-for-Dag Arbejdsplan (16. november — 10. december 2026)

### Sprint 1: `MUST HAVE` — Firmware & MVP Flow (16. — 20. november)
* **16. nov (Man):** Opgrader begge `MKR WiFi 1010` til `NINA 3.0.1+`; opdater `WiFiNINA 2.0+` og `ArduinoBLE 2.0+`; verificer samtidig Wi-Fi + BLE eksempelkode.
* **17. nov (Tir):** Skriv BLE-scanner i `firmware/hazard-radar-02/` (filtrerer på BLE-tag `UUID`) med **5-punkts glidende gennemsnit** på `RSSI` + HTTP POST-klient; send første test-JSON til Person B.
* **18. nov (Ons):** Tilføj lokal sirene (buzzer) og blinkende røde LED'er på `Arduino #2` via ikke-blokerende `millis()`-tilstandsmaskine (`IDLE`, `DETECTING`, `ALARM_ACTIVE`).
* **19. nov (Tor):** Programmer `Touch0` til lokal kvittering (`LOCAL_ACK`); gennemfør fuld End-to-End MVP-test sammen med Person B.
* **20. nov (Fre) — MILEPÆL 1 (`v0.1-mvp`):** Ryd op i C++-kode (`arduino_secrets.h` i `.gitignore`), dokumentér pinout i `README.md` og merge til `dev` -> `main` (`v0.1-mvp`).

### Sprint 2: `MUST HAVE` — Gate Node, Tovejs-kvittering & Stabilitet (21. — 27. november)
* **21.–22. nov (Lør–Søn):** Erstat dynamiske `String`-objekter med faste `char[]`-buffere og kør 2-timers stabilitetstest.
* **23. nov (Man):** Tilføj $5\text{ s}$ `HEARTBEAT` og $2\text{ s}$ hardware **Watchdog Timer (WDT)** på `Arduino #2`.
* **24. nov (Tir):** Opstart `Arduino #1 (Gate Node)` i `firmware/gate-node-01/`: initialiser Carrier-display og render høj-kontrast QR-kode til check-in URL.
* **25. nov (Ons):** Synkroniser NTP-tid (`WiFi.getTime()`), vis **$60\text{ s}$ dynamisk TOTP PIN-kode** under QR-koden (delt algoritme i `firmware/shared/`) og send `HTS221` temperatur/fugtighed hvert $30\text{ s}$.
* **26. nov (Tor):** Implementer `GET /api/devices/ARDUINO-HAZARD-02/state` polling på `Arduino #2`, så sirenen slukker automatisk ved tryk på telefonen; tilføj `HEARTBEAT` + WDT på `Arduino #1`.
* **27. nov (Fre) — MILEPÆL 2 (`v0.2-core`):** Kalibrér `RSSI`-tærskel på $2\text{ m}$ afstand og lås **$1.5\text{ s}$ Debounce**. *Alle `Must have`-krav på hardware er nu 100 % færdige.*

### Sprint 3: `SHOULD HAVE` — Edge Cybersecurity & Zonestyring (28. november — 4. december)
* **28.–29. nov (Lør–Søn):** Integrer SHA-256 bibliotek på SAMD21 i `firmware/shared/` og verificer eksakt `HMAC-SHA256` hash-match mod Person B's C#-kode.
* **30. nov (Man):** Tilføj `X-Device-Id`, `X-Timestamp` og `X-Signature` headere på alle requests fra `Arduino #1` og `#2`; test afvisning af ugyldige pakker (`HTTP 401`).
* **1. dec (Tir):** Aktiver `LSM6DS3` accelerometer/gyroskop på `Arduino #2` til detektion af fysisk flytning/stød (`SECURITY_TAMPER` alarm).
* **2. dec (Ons):** Programmer `Touch1` (skift zonestatus `ACTIVE` / `PAUSED`) og `Touch2` (manuel alarmtest) samt visning af status på Carrier-skærmen.
* **3. dec (Tor):** Samlet integrationstest af alle `Must have` og `Should have` scenarier med Person B.
* **4. dec (Fre) — MILEPÆL 3 (`v0.3-security`):** Kodegennemgang og **Feature Freeze** på begge firmware-projekter.

### Sprint 4: Stresstest, Live Demo & Forsvar (5. — 10. december)
* **5.–6. dec (Lør–Søn):** Fail-Safe stresstest: afbryd router og strøm under alarm; verificer autonom sirene og Watchdog-genstart.
* **7. dec (Man):** Opsæt testbane ($2\text{ m}$ farezone) og kør generalprøve på Tamper- og Spoofing-demo med Person B.
* **8. dec (Tir):** *(Nice to have)* Tilføj Boot Splash med IP/NTP-status på displayet og opdater hardware-diagrammer i `docs/`.
* **9. dec (Ons):** Optag backup-video af begge Arduino-boards i drift og pak udstyr til eksamen.
* **10. dec (Tor) — MILEPÆL 4 (`v1.0-release`):** Live demonstration og forsvar af hardwarearkitektur, samtidig Wi-Fi/BLE, RSSI-filtrering og edge-sikkerhed.
