# Arbejdsplan — Person B: C# Backend, PWA & Cloud Security

* **Ansvarsdomæne:** C# (.NET 8 Web API), SignalR, Web Push, Mobil PWA, Formand Dashboard & PostgreSQL (`backend/` & `frontend/`)
* **Git-branch:** `feat/person-b` (merges løbende til `dev`)
* **Periode:** 16. november 2026 — 10. december 2026
* **Milepæle:** `v0.1-mvp` (20. nov) $\rightarrow$ `v0.2-core` (`Must have` færdig 27. nov) $\rightarrow$ `v0.3-security` (4. dec) $\rightarrow$ `v1.0-release` (10. dec)
* **Integrationspartner:** Person A (Hardware, Embedded C++ & Edge Security)

---

## 1. Leverancer opdelt efter MoSCoW

| Prioritet | Tekniske leverancer for Person B |
| :--- | :--- |
| **MUST HAVE**<br>*(Sprint 1 & 2: 16. – 27. nov)* | 1. **Core Backend & DB:** `C# .NET 8 Web API` (`backend/`) + `EF Core` + `PostgreSQL 16` i Docker (`Workers`, `Shifts`, `Incidents`, `DeviceStates`) eksponeret via **Cloudflare Tunnel (`HTTPS`)**.<br>2. **Realtid & Baggrunds-Push:** `SignalR Hub` (øjeblikkelig rød alarmskærm i åben browser) + `Web Push API` (`Service Worker` `sw.js` i `frontend/pwa/` for alarm ved lukket browser/låst skærm).<br>3. **Check-in & Tovejs-ACK:** Validering af $60\text{ s}$ TOTP PIN fra `Arduino #1`, kobling af `BleBeaconUuid`, tvungen kvittering (`POST /ack` + håndtering af `LOCAL_ACK`), beregning af reaktionstid i sek. samt `GET /state` til at slukke sirenen på `Arduino #2`.<br>4. **Drift & Dashboard:** `BackgroundService` der overvåger $5\text{ s}$ Arduino-heartbeat (`RADAR OFFLINE` ved $>15\text{ s}$ udfald) + Formand Dashboard (`frontend/dashboard/`) med live status, kvitteringslog og global nødknap. |
| **SHOULD HAVE**<br>*(Sprint 3: 28. nov – 4. dec)* | 1. **Kryptografisk validering:** Middleware der verificerer `HMAC-SHA256` signatur og $5\text{ s}$ `X-Timestamp` på alle Arduino-requests.<br>2. **Anti-Spoofing (`Cross-Validation`):** Rejser `SECURITY_SPOOF`, hvis `Arduino #2` ser et BLE-tag uden aktivt check-in.<br>3. **Audit-integritet & Rate Limit:** PostgreSQL `Append-Only` audit-rolle på `Incidents` (`REVOKE DELETE`) + `.NET RateLimiter` mod PIN brute-force.<br>4. **Vejrintegration:** Letvægts C# `BackgroundService` der henter vindhastighed fra **DMI Open Data API** ($>15\text{ m/s}$ kranadvarsel). |
| **NICE TO HAVE**<br>*(Sprint 4: hvis tid)* | 1. **iOS Safari Onboarding-wizard:** Automatisk detektion af iPhone Safari (`standalone == false`) med visuel guide (*«Del $\rightarrow$ Føj til hjemmeskærm»*).<br>2. **USGS Jordskælvs-API** eller CSV/JSON-eksport af audit-loggen. |

---

## 2. API-endepunkter & Sikkerhedslogik

| Endepunkt / Hub | Kaldende part | Ansvar |
| :--- | :--- | :--- |
| `POST /api/telemetry/event` | Arduino #1 & #2 | Modtager `CRANE_RADAR`, `CLIMATE_DATA`, `SECURITY_TAMPER`, `LOCAL_ACK`, `HEARTBEAT`. |
| `GET /api/devices/{id}/state` | Arduino #2 | Returnerer `{"alarmActive": bool, "zoneState": "ACTIVE", "command": "CLEAR_ALARM" \| "NONE"}`. |
| `POST /api/shifts/checkin` | Arbejder PWA | Validerer $60\text{ s}$ PIN, kobler `BleBeaconUuid` og gemmer `PushSubscriptionJson`. |
| `POST /api/incidents/{id}/ack` | Arbejder PWA | Sætter `ACKNOWLEDGED`, beregner `ReactionTimeSeconds` og rydder alarmflaget på `DeviceStates`. |
| `POST /api/alerts/broadcast` | Formand / DMI | Udsender global alarm (`STRONG_WIND`, `SHELTER`) via SignalR + Web Push. |
| `/hubs/safety` (`SafetyHub`) | Backend $\leftrightarrow$ UI | Realtids-opdatering af mobil PWA og Formand Dashboard. |

### `HMAC-SHA256` & `Cross-Validation` (`Should Have`)

```csharp
// 1. HMAC-SHA256 + 5s Timestamp validering:
var canonical = $"{deviceId}:{timestamp}:{rawBody}";
var expected = ComputeHmacSha256Hex(sharedSecret, canonical);
if (Math.Abs(DateTimeOffset.UtcNow.ToUnixTimeSeconds() - timestamp) > 5 ||
    !CryptographicOperations.FixedTimeEquals(
        Encoding.UTF8.GetBytes(expected.ToLowerInvariant()),
        Encoding.UTF8.GetBytes(signature.ToLowerInvariant())))
{
    return Results.Unauthorized();
}

// 2. Anti-Spoofing Cross-Validation:
var activeShift = await db.Shifts
    .Include(s => s.Worker)
    .FirstOrDefaultAsync(s => s.Worker.BleBeaconUuid == bleUuid && s.CheckOutTime == null);

if (activeShift is null)
{
    await LogAndBroadcastIncident(EventType.SECURITY_SPOOF, workerId: null, bleUuid, rssi);
}
```

---

## 3. Dag-for-Dag Arbejdsplan (16. november — 10. december 2026)

### Sprint 1: `MUST HAVE` — Infrastruktur, Core API & MVP (16. — 20. november)
* **16. nov (Man):** Opret GitHub-repo, `docker-compose.yml` (`PostgreSQL 16`), C# `.NET 8 Web API` (`backend/`) og `frontend/` (`pwa/` + `dashboard/`) samt **Cloudflare Tunnel (`HTTPS`)**.
* **17. nov (Tir):** Byg `EF Core`-modeller (`Worker`, `Shift`, `Incident`, `DeviceState`), migreringer og `POST /api/telemetry/event`; modtag første test-POST fra Person A.
* **18. nov (Ons):** Opsæt `SafetyHub` (SignalR) og byg mobil PWA-grundskærm (`frontend/pwa/index.html` + `js/app.js`: grøn sikker $\leftrightarrow$ rød alarm med *«Forstået»*-knap).
* **19. nov (Tor):** Implementer `POST /api/incidents/{id}/ack` + basal alarmtabel i `frontend/dashboard/index.html`; gennemfør End-to-End MVP-test med Person A.
* **20. nov (Fre) — MILEPÆL 1 (`v0.1-mvp`):** Tilføj Swagger, database-seeding og arkitekturoversigt i `README.md`; tag `v0.1-mvp` på GitHub.

### Sprint 2: `MUST HAVE` — Web Push, Check-in, Heartbeat & Dashboard (21. — 27. november)
* **21.–22. nov (Lør–Søn):** Generer VAPID-nøgler, installer `Lib.Net.Http.WebPush` og opret `frontend/pwa/manifest.json`.
* **23. nov (Man):** Skriv `frontend/pwa/sw.js` (`Service Worker`) og `WebPushNotificationService` i C#; verificer push-alarm på låst Android/iOS-telefon.
* **24. nov (Tir):** Byg QR check-in formular i PWA'en (navn + PIN + BLE-tag valg) samt lydalarm og `navigator.vibrate()`.
* **25. nov (Ons):** Implementer $60\text{ s}$ TOTP PIN-validering i `POST /api/shifts/checkin` og lagring af `CLIMATE_DATA` fra `Arduino #1` (`HIGH_TEMP` alarm ved $>30^\circ\text{C}$).
* **26. nov (Tor):** Byg `GET /api/devices/{id}/state` til Person A's `Arduino #2` og `HeartbeatMonitorService` (`RADAR OFFLINE` ved $>15\text{ s}$ udfald).
* **27. nov (Fre) — MILEPÆL 2 (`v0.2-core`):** Færdiggør Formand Dashboard (`frontend/dashboard/`: live enhedsstatus, aktive vagter, reaktionstider og global nødknap). *Alle `Must have`-krav er nu 100 % færdige og testede.*

### Sprint 3: `SHOULD HAVE` — Cybersecurity & DMI Vejrdata (28. november — 4. december)
* **28.–29. nov (Lør–Søn):** Skriv `HmacAuthenticationMiddleware` i C# med `FixedTimeEquals` og $5\text{ s}$ replay-vindue.
* **30. nov (Man):** Aktivér HMAC-validering mod Person A's boards og implementer **Anti-Spoofing Cross-Validation** (`SECURITY_SPOOF`).
* **1. dec (Tir):** Tilføj håndtering af `SECURITY_TAMPER` fra `LSM6DS3`, `.NET RateLimiter` på check-in og PostgreSQL `Append-Only` rettigheder på `Incidents` (`REVOKE DELETE`).
* **2. dec (Ons):** Implementer letvægts C# `BackgroundService` mod **DMI Open Data API** (automatisk `STRONG_WIND`-advarsel ved vind $>15\text{ m/s}$).
* **3. dec (Tor):** Tilføj dedikeret *Security Audit Log*-visning på Formand Dashboard og gennemfør samlet sikkerhedstest med Person A.
* **4. dec (Fre) — MILEPÆL 3 (`v0.3-security`):** Kodeoprydning og **Feature Freeze**.

### Sprint 4: Stresstest, Live Demo & Forsvar (5. — 10. december)
* **5.–6. dec (Lør–Søn):** Belastnings- og sikkerhedstest: verificer `REVOKE DELETE` i PostgreSQL (`DELETE` afvises), rate limiting og baggrunds-WebPush.
* **7. dec (Man):** Klargør Grey Team demo-scripts (`curl` uden HMAC, gammelt timestamp, uregistreret BLE-tag) og kør generalprøve med Person A.
* **8. dec (Tir):** *(Nice to have)* Tilføj iOS Safari *«Føj til hjemmeskærm»*-guide i PWA og finpuds UI-kontrast på dashboardet.
* **9. dec (Ons):** Optag backup-video af softwareflowet, opdater `README.md` og opret `v1.0-release` på GitHub.
* **10. dec (Tor) — MILEPÆL 4 (`v1.0-release`):** Live demonstration og forsvar af C#/.NET backend, SignalR/WebPush, PostgreSQL datamodel og Grey/Blue Team sikkerhed.
