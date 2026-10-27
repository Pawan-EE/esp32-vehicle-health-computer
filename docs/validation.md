# Validation record

## Scope

Software verification is recorded separately from physical validation. No ESP32 board was flashed, no CAN simulator was connected, and no vehicle was tested during repository creation. The bring-up checklist remains mandatory before vehicle use.

## Automated checks

- Arduino IDE target: `esp32:esp32:esp32s3`, Arduino-ESP32 core 3.3.12.
- Local demo build passed on 2026-10-27: 901,696 bytes flash and 46,528 bytes static RAM before the small demo-MIL consistency adjustment. Final commit build results are tracked by CI.
- Local live-mode build passed on 2026-10-27: 907,172 bytes flash and 46,696 bytes static RAM. The verification copy enabled requests; the distributed configuration remains demo-only by default.
- Embedded dashboard JavaScript passed `node --check` syntax validation.
- CI builds demo, passive and live configurations independently.
- Host C++ tests cover PID conversion and length rejection, supported-mask boundaries, DTC character formatting, single and multi-frame ISO-TP, sequence rejection, oversize rejection and 128-byte sequence rollover.
- Host CI enables address and undefined-behavior sanitizers.
- The SVG was rendered to PNG and visually inspected for legibility and wiring consistency.

Consult the repository's Actions results for the actual status of a specific commit. A successful build is not a hardware or electrical safety test.

## Not yet verified on hardware

- CAN timing, TX/RX polarity, actual board GPIO routing and physical ECU compatibility.
- Real ISO-TP timing and ECU response variations, gateway behavior and multi-ECU scenarios.
- Actual Wi-Fi/captive-portal behavior across phone operating systems and long-term memory stability.
- Power consumption, transients, load dump, EMC, ESD, thermal performance and battery drain.
- Automotive suitability of any specific marketplace transceiver or buck module.

See `docs/bring-up.md` for acceptance cases and frame fixtures. No on-road, reliability, standards-conformance or automotive-qualification claims are made.
