#pragma once
// Start on USB with no vehicle attached. Live operation requires BOTH settings.
constexpr bool DEMO_MODE = true;
constexpr bool ENABLE_OBD_REQUESTS = false;
constexpr int CAN_TX_PIN = 11;
constexpr int CAN_RX_PIN = 13;
constexpr int CAN_KBIT = 500; // 250 or 500; no automatic bitrate probing
constexpr unsigned ECU_RESPONSE_ID = 0x7E8; // Engine ECU; physical request = response - 8
constexpr char AP_SSID[] = "VehicleHealth";
constexpr char AP_PASSWORD[] = "ChangeMe-OBD32"; // Change before vehicle use (8-63 characters)
constexpr unsigned POLL_MS = 300;
constexpr unsigned RESPONSE_MS = 250;
constexpr unsigned STALE_MS = 10000;
constexpr unsigned SESSION_MS = 15 * 60 * 1000; // Reboot required after parked test session
