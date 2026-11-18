#pragma once

// Identifikatorer for vores to Arduino-enheder
const char* DEVICE_ID_GATE = "ARDUINO-GATE-01";
const char* DEVICE_ID_RADAR = "ARDUINO-HAZARD-02";

// API-endepunkter paa C# backend
const char* API_PATH_TELEMETRY = "/api/telemetry/event";
const char* API_PATH_RADAR_STATE = "/api/devices/ARDUINO-HAZARD-02/state";

// Event-typer (skal matche AGENTS.md)
const char* EVENT_CRANE_RADAR = "CRANE_RADAR";
const char* EVENT_LOCAL_ACK = "LOCAL_ACK";
const char* EVENT_CLIMATE_DATA = "CLIMATE_DATA";
const char* EVENT_HEARTBEAT = "HEARTBEAT";
const char* EVENT_SECURITY_TAMPER = "SECURITY_TAMPER";

// Zonestatus
const char* ZONE_ACTIVE = "ACTIVE";
const char* ZONE_PAUSED = "PAUSED";

// Timere i millisekunder (1000 ms = 1 sekund)
const unsigned long HEARTBEAT_INTERVAL_MS = 5000;
const unsigned long CLIMATE_INTERVAL_MS = 30000;
const unsigned long STATE_POLL_INTERVAL_MS = 1500;
const unsigned long RSSI_DEBOUNCE_MS = 1500;
const int WATCHDOG_TIMEOUT_MS = 2000;

// BLE RSSI-filter
const int RSSI_WINDOW_SIZE = 5;
const int RSSI_ALARM_THRESHOLD = -65; // ~2 meter afstand (kalibreres senere)
