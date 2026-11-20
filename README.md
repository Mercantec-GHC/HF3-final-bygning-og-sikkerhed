# Byggeplads Safety System (H3 Project)

IoT-baseret adgangs- og farezoneovervågning på byggepladser uden GPS og uden tunge native mobilapps.
Udviklet af **Oleh & Roman** (`Person A`: Hardware & Embedded C++ | `Person B`: C# .NET 8 Backend, PWA & Security).

---

## Projektstruktur (Monorepo)

```text
H3-project/
├── AGENTS.md                        # AI-agent regler, API-kontrakt og MoSCoW-scope
├── README.md                        # Projektoversigt og mappestruktur
├── docker-compose.yml               # PostgreSQL 16 database-container
├── docs/                            # Projekt- og arkitekturdokumentation
│   ├── architecture.md              # Systemarkitektur, Mermaid-diagrammer, DB-model & DoD
│   ├── plan-person-a.md             # Dag-for-dag arbejdsplan for Person A (Hardware & C++)
│   ├── plan-person-b.md             # Dag-for-dag arbejdsplan for Person B (C# Backend & Frontend)
│   ├── git-workflow.md              # Git-branching model (main, dev, feat/person-a, feat/person-b)
│   └── images/                      # Visuelle arkitekturdiagrammer fra projektgrundlaget
├── firmware/                        # [PERSON A] 2 × Arduino MKR WiFi 1010 + IoT Carrier (C++)
│   ├── gate-node-01/                # Arduino #1: QR-kode, 60s TOTP PIN, HTS221 Klima
│   ├── hazard-radar-02/             # Arduino #2: BLE RSSI-radar (5-pkt filter), Sirene, Touch0, LSM6DS3
│   └── shared/                      # Fælles C++ headers (protokol, HMAC-SHA256, TOTP, secrets)
├── backend/                         # [PERSON B] C# .NET 8 Web API, SignalR, EF Core & Web Push
│   ├── ByggepladsSafety.sln
│   └── src/
│       └── ByggepladsSafety.Api/
│           ├── ByggepladsSafety.Api.csproj
│           ├── appsettings.json
│           ├── Controllers/         # REST API controllers (Telemetry, Shifts, Incidents, Devices, Alerts)
│           ├── Data/                # EF Core SafetyDbContext & Migrations
│           ├── Hubs/                # SafetyHub (SignalR real-time WebSockets)
│           ├── Middleware/          # HmacAuthenticationMiddleware (Should Have)
│           ├── Models/              # Entities (Worker, Shift, Incident, DeviceState) & DTOs
│           └── Services/            # HeartbeatMonitor, WebPush, TotpPin, DmiWeather BackgroundService
└── frontend/                        # [PERSON B] Vanilla JS (ES6+), HTML5, CSS3 & Service Worker
    ├── pwa/                         # Arbejder Mobil PWA (Check-in, Grøn/Rød Alarm, sw.js, manifest.json)
    │   ├── css/
    │   ├── js/
    │   └── assets/
    └── dashboard/                   # Formand Dashboard (Live status, Nødknapper, Security Audit Log)
        ├── css/
        ├── js/
        └── assets/
```

---

## Dokumentation

* [AGENTS.md](./AGENTS.md) — Regler for AI-agenter og fælles API-kontrakt.
* [docs/architecture.md](./docs/architecture.md) — Systemarkitektur, MoSCoW DoD og PostgreSQL datamodel.
* [docs/plan-person-a.md](./docs/plan-person-a.md) — Arbejdsplan for **Person A** (`firmware/`).
* [docs/plan-person-b.md](./docs/plan-person-b.md) — Arbejdsplan for **Person B** (`backend/` & `frontend/`).
* [docs/git-workflow.md](./docs/git-workflow.md) — Git-strategi (`main`, `dev`, `feat/person-a`, `feat/person-b`).

---

## Hardware Pinout & Firmware Opsætning (`MKR WiFi 1010 + IoT Carrier`)

Begge `Arduino MKR WiFi 1010`-boards sidder direkte i `Arduino MKR IoT Carrier` (intern bus, ingen løse ledninger):

| Komponent | Pin / Interface | Anvendelse |
| :--- | :--- | :--- |
| **NINA-W102** (`Wi-Fi + BLE`) | Indbygget (`SPI` / `UART`) | Samtidig Wi-Fi (`WiFiNINA`) og BLE-scanner (`ArduinoBLE`) — kræver NINA firmware `3.0.1+` |
| **5 × RGB LED** (`DotStar`) | `D5` (`DATAPIN`), `D4` (`CLOCKPIN`) | Grøn (`IDLE`) / blinkende rød (`ALARM_ACTIVE`) |
| **Buzzer** (Sirene) | `D7` | Lokal Fail-Safe alarmsirene (`1800 Hz`) |
| **Touch-knapper** (`Touch0–4`) | `MCHPTouch` | `Touch0`: Lokal alarmkvittering (`LOCAL_ACK`) |
| **TFT Display** (`ST7789`) | `SPI` + `D3` (`Backlight`) | QR-kode og 60s TOTP PIN på `Arduino #1` |
| **HTS221 & LSM6DS3** | Intern `I2C` (`Wire`) | Klima (`Temp/Fugt`) og sabotage-accelerometer (`Tamper`) |

### Opsætning af fælles bibliotek (`firmware/shared/`)
1. Kopier eller symlink `firmware/shared/` ind i din lokale Arduino-biblioteksmappe som `ByggepladsShared` (fx `Documents/Arduino/libraries/ByggepladsShared`).
2. Kopier `firmware/shared/src/arduino_secrets.example.h` til `firmware/shared/src/arduino_secrets.h` og udfyld Wi-Fi + backend-IP (`arduino_secrets.h` ignoreres af Git).
3. Åbn `firmware/hazard-radar-02/hazard-radar-02.ino` i Arduino IDE med board **Arduino MKR WiFi 1010**.
