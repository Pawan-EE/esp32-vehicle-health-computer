# Firmware design

## Modes and startup

`DEMO_MODE=true` disables TWAI initialization and produces synthetic values every 500 ms. `DEMO_MODE=false` with `ENABLE_OBD_REQUESTS=false` installs listen-only TWAI: frames are counted but generic PID responses are not interpreted without a request context. With requests enabled, the sketch discovers the configured ECU's supported PIDs before polling. Configuration requires rebuilding; the web API cannot change modes or bitrate.

The runtime uses Arduino-ESP32's bundled legacy `driver/twai.h` API (not the newer `esp_twai_onchip.h` node API). A small synchronous web server and nonblocking CAN queue reads run cooperatively. Each loop processes at most 32 received frames and yields to Wi-Fi. Buffers are bounded. No heap allocation is driven by vehicle-reported ISO-TP lengths; the JSON response uses Arduino String and remains a prototype implementation.

## CAN/OBD scope

Classic CAN with 8-byte diagnostic frames and 11-bit IDs only. Default response ID `0x7E8`, physical request `0x7E0`. Configurable response range is 0x7E8–0x7EF, request=response−8. Responses from other IDs, extended frames, RTR frames and non-8-byte DLCs are discarded. There is no functional broadcast discovery or automatic ECU selection. Confirm the target ECU mapping externally.

The sole outgoing diagnostic services are **01** (allowlisted PIDs) and **03** (stored DTCs), plus ISO-TP flow control to finish a matching response. Mode 04 and UDS write/control services are absent. Normal-mode ACK/error frames are still possible. `ss=1` requests single-shot transmission. A successful transmit enqueue does not itself prove the ECU answered; response data is accepted separately.

Each request starts at least 300 ms after the preceding request. At most one is outstanding. The initial response timeout is 250 ms, and the entire segmented transaction has a 1 s deadline. ISO-TP receives a maximum of 128 payload bytes, verifies sequence numbers and rejects truncated or oversized first/consecutive frames. Flow control is CTS, block size 0, STmin 10 ms. At most 63 stored DTC records fit; a larger reply is rejected, never silently shown as a complete scan. Negative responses end the request; NRC 0x78 does not extend the deadline. Late/unrelated replies are ignored unless indistinguishable from the currently requested PID on the same ECU.

Supported masks are discovered at 00, then 20/40 only when continuation bits say they exist. A page gets at most three attempts. Timeouts do not become zeros. PID01 is polled about every five seconds when supported. Other supported metrics rotate at the global request interval. Three failed discovery requests, ten consecutive response timeouts, transmit failure, receive-queue overflow, error-passive or bus-off stop active polling. The 15-minute live session also stops the controller. No automatic bus recovery/restart is attempted; inspect and reboot. Session shutdown does not remove supply power.

## Implemented conversions

`A` and `B` are unsigned response data bytes after service and PID. Unsupported PIDs remain unavailable.

| Service 01 PID | Display | Formula |
|---|---|---|
| 04 | Calculated load (%) | A × 100 / 255 |
| 05 | Coolant (°C) | A − 40 |
| 0C | RPM | (256A + B) / 4 |
| 0D | Speed (km/h) | A |
| 0F | Intake air (°C) | A − 40 |
| 10 | MAF (g/s) | (256A + B) / 100 |
| 11 | Throttle (%) | A × 100 / 255 |
| 2F | Fuel level (%) | A × 100 / 255 |
| 42 | ECU supply voltage (V) | (256A + B) / 1000 |

PID01 uses A bit7 for MIL and bits6:0 for ECU-reported DTC count. Readiness bytes are length-checked but not decoded in V1. Mode03 on ISO15765 is decoded as `43, count, DTC-high, DTC-low, ...`; transport padding is not a code. The two-byte DTC encoding produces P/C/B/U categories and four remaining characters. No manufacturer-specific description database is included.

## Dashboard and local API

| Route | Method | Behavior |
|---|---|---|
| `/` | GET | Embedded HTML/CSS/JS; no external dependencies |
| `/api/status` | GET | Metrics, freshness, mode, counters, selected ECU, scan state, session token |
| `/api/dtc` | POST | Queue one stored-code read; `X-Session-Token` required |
| Other paths | Any | Redirect to local landing page for captive-portal assistance |

JSON is polled every second. Values become stale at 10 seconds and immediately unavailable after a halted session; browser network failure clears the metric cards. DTC results are snapshots from the last scan, not continuous monitoring. A fresh scan clears old code results before requesting new ones. Demo's P0300 result is explicitly simulated.

The AP has a configured WPA password and maximum two stations. A random per-boot token reduces cross-origin blind POST requests; it is not a substitute for authentication. Any connected client can read telemetry and obtain the token. HTTP is unencrypted inside the Wi-Fi network; no internet bridge or cloud upload is implemented. Do not use the sample password around real vehicles. Captive portal popups are OS-dependent; HTTPS interception is neither attempted nor supported. Use the plain HTTP local address.

## Extending the firmware

Add a PID only after reviewing its support-mask range, payload length, formula, units and expected vehicle behavior. Keep transmission allowlisting, rate limiting and tests intact. Do not add arbitrary CAN transmission to the dashboard. The demo is a UI demonstration, not an ECU emulator. Use an independent simulator to validate on-wire protocol behavior.
