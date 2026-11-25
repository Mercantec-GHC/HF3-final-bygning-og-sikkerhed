#pragma once

// Kopier denne fil til "arduino_secrets.h" i samme mappe og udfyld dine egne vaerdier.
// "arduino_secrets.h" er i .gitignore og bliver ikke gemt i Git.

const char* WIFI_SSID = "DIT_WIFI_NAVN";
const char* WIFI_PASS = "DIT_WIFI_KODEORD";

const char* SERVER_HOST = "192.168.1.100";
const int SERVER_PORT = 5000;

// BLE UUID paa arbejderens hjelm-tag
const char* WORKER_BEACON_UUID = "f7826da6-4fa2-4e98-8024-bc5b71e0893e";

// URL til arbejderens check-in PWA (vises som QR-kode paa Arduino #1)
const char* CHECKIN_URL = "https://safety.byggeplads.dk/pwa";

// Delt hemmelighed til 60s TOTP PIN og HMAC-SHA256 (skal matche C# backend)
const char* SHARED_SECRET_KEY = "byggeplads-hmac-secret-key-2026-h3";
