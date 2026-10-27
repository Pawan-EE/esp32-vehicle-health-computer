# Technical references

Reviewed during project creation on 2026-10-27. Consult the exact board and component revision you own.

- [Espressif ESP32-S3 TWAI documentation](https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-reference/peripherals/twai.html): controller capability, external transceiver requirement, classic-CAN limitation and listen-only semantics. New documentation presents a newer driver API; this project deliberately uses the bundled legacy API in Arduino-ESP32 3.3.12.
- [Arduino-ESP32 source and releases](https://github.com/espressif/arduino-esp32): board package, Wi-Fi and web-server libraries.
- [TI SN65HVD230 datasheet](https://www.ti.com/lit/ds/symlink/sn65hvd230.pdf): supply voltage, signal pins, RS behavior and electrical limits. The datasheet applies to the IC, not arbitrary marketplace modules.
- [CSS Electronics OBD-II decoding reference](https://www.csselectronics.com/pages/obd2-dbc-file): supported-PID variability and generic measurement decoding resources.
- [SAE J1979 standard](https://www.sae.org/standards/content/j1979/): diagnostic test modes; use the applicable licensed revision for conformance work.
- [Linux kernel ISO-TP documentation](https://docs.kernel.org/networking/iso15765-2.html): transport concepts, flow control and addressing context.

The repository contains original implementation and documentation, not a redistributed standards document. No formal standards-conformance claim is made.
