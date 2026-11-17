# AGENTS.md — Byggeplads Safety System

Primær kontekst- og regelbog for AI-agenter i IDE (Rider, Cursor, Copilot, Antigravity) samt teknisk reference for **Oleh & Roman**.

## 1. Projektoversigt & Milepæle
* **Formål:** IoT-baseret adgangs- og farezoneovervågning på byggepladser uden GPS og uden tunge native mobilapps.
* **Projektperiode:** 16. november 2026 — 10. december 2026
* **Milepæl 1 (`v0.1-mvp`):** 20. november 2026 — Grundlæggende End-to-End alarmflow på fysisk hardware.
* **Milepæl 2 (`v0.2-core`):** 27. november 2026 — **100 % af `Must have` er færdigt og testet.**
* **Milepæl 3 (`v0.3-security` / Feature Freeze):** 4. december 2026 — `Should have` (Cybersecurity & DMI) integreret.
* **Milepæl 4 (`v1.0-release` / Forsvar):** 10. december 2026 — Afsluttende eksamen og live demonstration.

## 2. Teknologistak & Mappestruktur (Monorepo)
* **Hardware (`firmware/` — `2 × Arduino MKR WiFi 1010 + IoT Carrier`):** C++ med `NINA-W102 Firmware 3.0.1+` (samtidig Wi-Fi og BLE), `WiFiNINA 2.0+`, `ArduinoBLE 2.0+`, `Arduino_MKRIoTCarrier`, `ArduinoJson`. Opdelt i:
  * `firmware/gate-node-01/` (`Arduino #1: Gate Node`)
  * `firmware/hazard-radar-02/` (`Arduino #2: Hazard Radar`)
  * `firmware/shared/` (Fælles headers, konstanter og hjælpefunktioner)
* **Backend (`backend/`):** **C# (.NET 8 Web API)**, **SignalR** (realtids WebSockets), **Entity Framework Core**, **Web Push API** (`Lib.Net.Http.WebPush` med VAPID-nøgler). *(Ingen separate Go-mikroservices — al serverlogik og vejr-polling ligger samlet i C#).*
* **Frontend (`frontend/`):** **Vanilla JavaScript (ES6+)**, HTML5, CSS3 og **Service Worker (`sw.js`)** *(ingen Blazor)*. Opdelt i:
  * `frontend/pwa/` (Arbejder Mobil PWA)
  * `frontend/dashboard/` (Formand Dashboard)
* **Database (`docker-compose.yml`):** **PostgreSQL 16** i Docker (`Workers`, `Shifts`, `Incidents`, `DeviceStates`).

## 3. Scope-prioritering (MoSCoW)
AI-agenter må **kun** implementere `Should have` og `Nice to have`, når alle `Must have`-krav fungerer fejlfrit:
* **MUST HAVE (Kernesystem — Sprint 1 & 2):** Samtidig Wi-Fi/BLE på `NINA 3.0.1+`, `Arduino #2` BLE `RSSI`-radar (5-punkts glidende gennemsnit + $1.5\text{ s}$ debounce) + lokal sirene/LED + touch-ACK, `Arduino #1` QR-kode + $60\text{ s}$ dynamisk PIN + `HTS221` temperatur/fugtighed, C# API + PostgreSQL, `SignalR` + `Web Push` (baggrundsalarmer), Formand Dashboard, $5\text{ s}$ hardware heartbeat + $2\text{ s}$ Hardware Watchdog (`WDT`).
* **SHOULD HAVE (Cybersecurity & Vejr — Sprint 3):** `HMAC-SHA256` pakkesignering + $5\text{ s}$ NTP timestamp-vindue, anti-spoofing `Cross-Validation` (`SECURITY_SPOOF`), `LSM6DS3` accelerometer sabotage-alarm (`SECURITY_TAMPER`), PostgreSQL `Append-Only` audit-rolle (`INSERT` + `UPDATE` på kvitteringsfelter, ingen `DELETE`) + `.NET RateLimiter`, dansk **DMI Open Data API** vindovervågning ($>15\text{ m/s}$) som C# `BackgroundService`.
* **NICE TO HAVE (Ekstra — kun ved overskudstid):** Dedikeret iOS Safari *«Føj til hjemmeskærm»* onboarding-wizard, **USGS** jordskælvs-API integration, CSV/JSON-eksport af reaktionstider.

## 4. Dokumentation i `docs/`
* `docs/architecture.md` — Systemarkitektur, MoSCoW DoD, Mermaid-diagrammer, databasemodel og sikkerhedsmatrix.
* `docs/plan-person-a.md` — Detaljeret dag-for-dag arbejdsplan for **Person A** (Hardware, Embedded C++ & Edge Security).
* `docs/plan-person-b.md` — Detaljeret dag-for-dag arbejdsplan for **Person B** (C# Backend, PWA & Cloud Security).
* `docs/git-workflow.md` — Produktions-Git-strategi (`main`, `dev`, `feat/person-a`, `feat/person-b`) og samarbejdsregler.

## 5. Fælles API-kontrakt (`Arduino -> C# Backend`)

### 5.1 Telemetry & Event Endpoint (`POST /api/telemetry/event`)
Understøttede `eventType`-værdier:
* `CRANE_RADAR` — Arbejder detekteret i farezone (`bleUuid`, `rssi`, `zoneState`).
* `LOCAL_ACK` — Arbejder/formand kvitterede fysisk via `Touch0` på `Arduino #2`.
* `CLIMATE_DATA` — `HTS221` måling fra `Arduino #1` (`temperature`, `humidity`).
* `HEARTBEAT` — $5\text{ s}$ `ALIVE`-puls fra `Arduino #1` og `Arduino #2` (`zoneState`).
* `SECURITY_TAMPER` — `LSM6DS3` rystelses-/flytningsalarm fra `Arduino #2`.

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

### 5.2 Polling af Alarmstatus (`GET /api/devices/{id}/state`)
Kaldes af `Arduino #2` hvert $1.5\text{ s}$ under aktiv alarm for at slukke lokal sirene ved PWA-kvittering:
```json
{
  "alarmActive": false,
  "zoneState": "ACTIVE",
  "command": "CLEAR_ALARM"
}
```

### 5.3 TOTP 60s PIN-algoritme (`Arduino #1` <-> `C# Backend`)
For at undgå ekstra HTTP-trafik beregner både `Arduino #1` og C# Backend den 4-cifrede PIN deterministisk ud fra NTP Unix-tid:
* `TimeWindow = CurrentUnixEpochSeconds / 60`
* `PIN = (HMAC-SHA256(TotpSecret, TimeWindow) % 9000) + 1000` (4 cifre: `1000–9999`).
* Backenden accepterer både `TimeWindow` og `TimeWindow - 1` ($\pm 1\text{ vindue}$) for at håndtere ur-drift ved skift.

## 6. Koderegler for AI-agenter
1. **Ikke-blokerende C++:** Brug aldrig `delay()` i `loop()`. Brug `millis()`-timere, faste `char[]`-buffere (ingen dynamisk `String`-fragmentering) og `2s Watchdog Timer` (husk `WDT.clear()` omkring netværkskald).
2. **Sikkerhed i C#:** Valider `X-Signature` med `CryptographicOperations.FixedTimeEquals` og afvis pakker ældre end $5\text{ s}$ (i `Must Have` Sprint 1–2 kan `RequireHmac` slås fra via `appsettings.json`).
3. **Uforanderlig Audit Log:** Eksponér aldrig `DELETE`-endpoints for `Incidents`; gem altid oprettelsestid (`CreatedAt`), kvitteringstid (`AcknowledgedAt`), kvitteringskilde (`AckSource`) og reaktionstid i sekunder (`ReactionTimeSeconds`).
