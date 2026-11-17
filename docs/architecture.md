# Byggeplads Safety System — Systemarkitektur & Projektgrundlag

* **Team:** Oleh & Roman (`Person A`: Hardware & Embedded C++ | `Person B`: C# Backend, PWA & Security)
* **Periode:** 16. november 2026 — 10. december 2026
* **Milepæle:** `v0.1-mvp` (20. nov) $\rightarrow$ `v0.2-core` (27. nov) $\rightarrow$ `v0.3-security` (4. dec) $\rightarrow$ `v1.0-release` (10. dec)

---

## 1. Projektbeskrivelse & Afgrænsning

### Problemstilling
På byggepladser opstår kritiske ulykker, når medarbejdere træder ind i aktive farezoner (kranløft, højspænding, åbne skakter) eller arbejder under farlige klimaforhold. GPS fungerer ikke indendørs og mangler præcision på $1\text{–}3\text{ m}$ afstand, mens tunge native mobilapps skaber friktion ved opstart.

### Løsning
Et fokuseret, driftssikkert IoT-system bygget på to **Arduino MKR WiFi 1010**-enheder (`firmware/` med `NINA-W102 3.0.1+` firmware til samtidig Wi-Fi og BLE), én samlet **C# (.NET 8 Web API)** backend (`backend/`) med **SignalR** og **Web Push**, en **PostgreSQL 16**-database samt en letvægts **PWA** og **Formand Dashboard** (`frontend/` i Vanilla JS), der åbnes øjeblikkeligt via QR-kode uden app-installation.

---

## 2. Prioriteret Definition of Done (MoSCoW)

For at sikre et 100 % fungerende, gennemtestet og forklarligt kernesystem før udvidelser er kravene opdelt efter **MoSCoW-modellen**:

| Prioritet | Måldato | Konkrete krav (Definition of Done) |
| :--- | :--- | :--- |
| **MUST HAVE**<br>*(Kernesystemet — skal med)* | **20. nov** (`v0.1-mvp`)<br>&<br>**27. nov** (`v0.2-core`) | 1. **Samtidig Wi-Fi & BLE:** Begge Arduino-boards kører `NINA 3.0.1+` firmware stabilt uden pauser.<br>2. **Gate Node (`Arduino #1`):** Viser QR-kode + $60\text{ s}$ dynamisk TOTP PIN-kode på displayet og måler temperatur/fugtighed (`HTS221`) hvert $30\text{ s}$.<br>3. **Hazard Radar (`Arduino #2`):** Scanner BLE-tags med **5-punkts glidende gennemsnit** på `RSSI` og $1.5\text{ s}$ `Debounce`, tænder lokal sirene + rød LED (*Hardware Fail-Safe*) og understøtter lokal kvittering (`Touch0`).<br>4. **Backend & Realtid:** C# `.NET 8 Web API` + `PostgreSQL` gemmer vagter og hændelser; `SignalR` skifter åben PWA til rød alarmskærm.<br>5. **Baggrunds-Push:** `Web Push` (`Service Worker`) leverer alarm til låst skærm / lukket browser.<br>6. **Tvungen kvittering & Drift:** Alarm stopper først ved tryk på *«Forstået»* i PWA eller på Arduino; reaktionstid gemmes i DB; $5\text{ s}$ `Heartbeat` + $2\text{ s}$ `Watchdog` overvåger enhederne; Formand Dashboard viser live status og nødknap. |
| **SHOULD HAVE**<br>*(Cybersecurity & Vejr — hvis tiden holder)* | **4. dec** (`v0.3-security`) | 1. **Pakkesignering:** `HMAC-SHA256` signatur + $5\text{ s}$ NTP-timestamp på alle Arduino-requests.<br>2. **Anti-Spoofing (`Cross-Validation`):** Afviser/flagger klonede BLE-tags (`SECURITY_SPOOF`), hvis tagget ses uden aktivt check-in.<br>3. **Sabotagealarm:** Indbygget `LSM6DS3` accelerometer på `Arduino #2` udløser `SECURITY_TAMPER` ved fysisk flytning/stød.<br>4. **Audit-integritet:** PostgreSQL `Append-Only` audit-beskyttelse på `Incidents` (`REVOKE DELETE`) + `.NET RateLimiter` mod PIN brute-force.<br>5. **Vejrintegration:** Letvægts C# `BackgroundService` henter vindhastighed fra **DMI Open Data API** ($>15\text{ m/s}$ kranadvarsel). |
| **NICE TO HAVE**<br>*(Ekstra — kun ved overskud)* | **8. dec** (`v1.0-release`) | 1. **iOS Safari Onboarding-wizard:** Automatisk guide (*«Del $\rightarrow$ Føj til hjemmeskærm»*) for iPhone-brugere.<br>2. **USGS Jordskælvs-API:** Automatisk seismisk alarm (kan ellers udløses manuelt via Formand Dashboard).<br>3. **Dataeksport:** Download af hændelses- og reaktionstidslog som CSV/JSON. |

---

## 3. Systemarkitektur (Visuel og Tekstuel)

![Systemarkitektur](./images/system-architecture.jpg)

```mermaid
flowchart LR
    subgraph EDGE["Hardware & Edge Lag (firmware/)"]
        G["Arduino #1: Gate Node\n[MUST] QR + 60s PIN + HTS221 Klima"]
        R["Arduino #2: Hazard Radar\n[MUST] BLE RSSI + Sirene/LED + Touch\n[SHOULD] LSM6DS3 Tamper + HMAC"]
        T["BLE Worker Tag\n(Fast UUID Beacon på hjelm)"]
        T -. "BLE 2.4 GHz (RSSI)" .-> R
    end

    subgraph CORE["Samlet C# .NET 8 Backend & Data (backend/)"]
        API["C# .NET Web API\n[MUST] REST + Check-in + ACK\n[SHOULD] HMAC + Cross-Validation"]
        HUB["SignalR Hub\n[MUST] Realtids WebSockets"]
        PUSH["WebPush Service\n[MUST] VAPID / Service Worker"]
        BG["C# BackgroundServices\n[MUST] Heartbeat Monitor (5s/15s)\n[SHOULD] DMI Vind API (>15 m/s)"]
        DB[("PostgreSQL 16 (Docker)\nWorkers | Shifts | Incidents | DeviceStates\n[SHOULD] Append-Only Audit")]

        API <--> HUB
        API --> PUSH
        API <--> DB
        BG <--> DB
        BG --> HUB
    end

    subgraph CLIENTS["Klienter i frontend/ (Vanilla JS via Cloudflare HTTPS)"]
        PWA["Arbejder Mobil (frontend/pwa/)\n[MUST] Grøn/Rød Alarm + ACK + sw.js"]
        DASH["Formand Dashboard (frontend/dashboard/)\n[MUST] Live Status + Nødknapper"]
    end

    G -- "Wi-Fi HTTP/HTTPS POST" --> API
    R <-- "Wi-Fi HTTP/HTTPS POST & GET State" --> API
    HUB <--> PWA
    PUSH -. "Baggrunds-Push (HTTPS)" .-> PWA
    HUB <--> DASH
```

---

## 4. End-to-End Hændelses- og Kvitteringsflow (`Must Have`)

![End-to-End Flow](./images/end-to-end-flow.jpg)

```mermaid
sequenceDiagram
    autonumber
    actor W as Arbejder (PWA + BLE Tag)
    participant A1 as Arduino #1 (Gate)
    participant A2 as Arduino #2 (Radar)
    participant API as C# .NET Backend
    participant DB as PostgreSQL
    participant F as Formand Dashboard

    Note over W,A1: 1. Check-in (Must Have)
    A1->>W: Viser QR-kode + 60s TOTP PIN
    W->>API: POST /api/shifts/checkin (PIN + BLE UUID + PushSubscription)
    API->>DB: Validerer PIN & opretter aktiv vagt (Shift)

    Note over W,A2: 2. Detektion & Lokal Fail-Safe (Must Have)
    W->>A2: BLE Broadcast (UUID)
    A2->>A2: 5-pkt glidende gennemsnit RSSI < 2m i 1.5s
    A2->>A2: Tænder lokal sirene + rød LED
    A2->>API: POST /api/telemetry/event (CRANE_RADAR)

    Note over API,F: 3. Multikanals Alarmering (Must Have)
    API->>DB: Gemmer Incident (DETECTED -> ALERT_SENT)
    par Realtid & Baggrund
        API-->>W: SignalR TriggerAlarm (Rød skærm + lyd/vibration)
        API-->>W: Web Push (Vækker låst skærm / lukket browser)
        API-->>F: SignalR Live Alert på Dashboard
    end

    Note over W,DB: 4. Tvungen Kvittering (Must Have)
    alt Kvittering på Mobil (PWA)
        W->>API: POST /api/incidents/{id}/ack (AckSource = PHONE_PWA)
        API->>DB: Sætter ACKNOWLEDGED + ReactionTimeSeconds
        A2->>API: GET /api/devices/ARDUINO-HAZARD-02/state
        API-->>A2: alarmActive: false -> Slukker lokal sirene & LED
    else Kvittering på Hardware (Touch0)
        W->>A2: Trykker Touch0 på Arduino #2
        A2->>A2: Slukker lokal sirene & LED
        A2->>API: POST /api/telemetry/event (LOCAL_ACK)
        API->>DB: Sætter ACKNOWLEDGED (AckSource = ARDUINO_BUTTON)
        API-->>W: SignalR ClearAlarm (PWA skifter til grøn)
    end
    API-->>F: Opdaterer hændelse til grøn (Kvitteret)
```

---

## 5. Arkitektoniske Valg (Kort begrundelse)

| Komponent | Valg | Begrundelse |
| :--- | :--- | :--- |
| **Hardware (`firmware/`)** | `2 × Arduino MKR WiFi 1010` + `IoT Carrier` (C++) | SAMD21 har kun $32\text{ KB}$ RAM (udelukker Python). Med `NINA 3.0.1+` kører Wi-Fi og BLE parallelt i én C++-løkke. |
| **Backend (`backend/`)** | `C# (.NET 8 Web API)` + `SignalR` + `EF Core` | Ét samlet sprog på serversiden (inkl. `BackgroundService` til heartbeat og DMI vejr-polling) gør koden overskuelig, let at debugge og nem at forsvare til eksamen. |
| **Frontend (`frontend/`)** | `Vanilla JS` + `Service Worker` (`pwa/` & `dashboard/`) | Ingen tung Blazor WASM-download over mobilnetværk; øjeblikkelig opstart fra QR-kode og direkte adgang til `Web Push`. |
| **Transport** | `HTTPS` via **Cloudflare Tunnel** (+ LAN HTTP for Arduino) | Påkrævet af moderne mobilbrowsere for at tillade `Service Worker`, `Web Push` og `Vibration API`. |
| **Database** | `PostgreSQL 16` (Docker) | Hurtig opsætning i Docker, indbygget `JSONB` til push-nøgler og nem opsætning af `Append-Only` (`REVOKE DELETE`) rettigheder. |

---

## 6. IoT Cybersecurity (`Should Have` — Blue & Grey Team)

![IoT Cybersecurity](./images/security-architecture.jpg)

| Trussel | Team | Mekanisme | Effekt |
| :--- | :--- | :--- | :--- |
| **Check-in hjemmefra (foto af QR)** | Grey Team | $60\text{ s}$ dynamisk TOTP PIN på `Arduino #1` | Afviser udløbet PIN (`HTTP 401`) |
| **Kloning af BLE-tag (`Spoofing`)** | Grey Team | `Cross-Validation` mod aktiv vagt i DB | Udløser `SECURITY_SPOOF` alarm |
| **Falske sensorpakker (`Replay`)** | Grey Team | `HMAC-SHA256` signatur + $5\text{ s}$ `Timestamp` | Afviser manipulerede/gamle pakker |
| **Fysisk flytning af radar** | Blue Team | `LSM6DS3` accelerometer på `Arduino #2` | Udløser `SECURITY_TAMPER` alarm |
| **Netværksudfald / frossen kode** | Blue Team | $2\text{ s}$ Hardware Watchdog + $5\text{ s}$ Heartbeat | Auto-reboot / `RADAR OFFLINE` |
| **Sletning af log / Brute-force** | Blue Team | PostgreSQL `Append-Only` + `.NET RateLimiter` | Uforanderlig historik & PIN-beskyttelse |

---

## 7. Databasemodel (`PostgreSQL 16`)

```mermaid
erDiagram
    WORKERS ||--o{ SHIFTS : "har"
    WORKERS |o--o{ INCIDENTS : "registreres i (nullable ved TAMPER/SPOOF)"

    WORKERS {
        uuid Id PK
        string FullName
        string BleBeaconUuid UK
        text PushSubscriptionJson
    }

    SHIFTS {
        uuid Id PK
        uuid WorkerId FK
        timestamptz CheckInTime
        timestamptz CheckOutTime
        timestamptz LastHeartbeat
        boolean IsOnline
    }

    INCIDENTS {
        uuid Id PK
        uuid WorkerId FK "Nullable"
        string DeviceId
        string BleUuid "Nullable"
        string EventType
        int RssiValue "Nullable"
        double Temperature "Nullable"
        double Humidity "Nullable"
        string Status "DETECTED | ALERT_SENT | ACKNOWLEDGED"
        timestamptz CreatedAt
        timestamptz AcknowledgedAt "Nullable"
        int ReactionTimeSeconds "Nullable"
        string AckSource "PHONE_PWA | ARDUINO_BUTTON | DASHBOARD"
    }

    DEVICE_STATES {
        string DeviceId PK "ARDUINO-GATE-01 | ARDUINO-HAZARD-02"
        string ZoneState "ACTIVE | PAUSED"
        boolean AlarmActive
        string PendingCommand "NONE | CLEAR_ALARM"
        double LastTemperature "Nullable"
        double LastHumidity "Nullable"
        timestamptz LastSeenAt
        boolean IsOnline
    }
```
