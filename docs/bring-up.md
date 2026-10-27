# Controlled bring-up and acceptance checklist

Record your board/module revisions, firmware commit, measured voltages, CAN bitrate and test setup. Do not commit identifying vehicle data.

## 1. Disconnected inspection

- [ ] Identify connector pin numbers by continuity; inspect for mirrored plug numbering.
- [ ] Verify transceiver is actually a 3.3 V model, with correct RS strap and no unwanted termination.
- [ ] Confirm GPIO11/13 are available on the exact board.
- [ ] Verify fuse, wire gauge, polarity, insulation and the protected supply design.
- [ ] Check that USB and buck power cannot backfeed one another.

## 2. USB demo, no vehicle

- [ ] Compile/upload default settings; join the AP and open 192.168.4.1.
- [ ] Confirm a prominent DEMO banner, changing RPM and zero simulated vehicle speed.
- [ ] Read simulated DTC and confirm it says SIMULATED.
- [ ] Disconnect Wi-Fi and confirm live cards are removed.

## 3. Current-limited power bench

- [ ] Test the power stage alone using a fused, current-limited supply; verify regulated 5 V.
- [ ] Connect the MCU; verify 3.3 V rail and Wi-Fi startup without brownouts.
- [ ] Measure input current, converter temperature and Wi-Fi peak-load behavior.
- [ ] Confirm safe startup/shutdown and transceiver recessive behavior.
- [ ] Do not perform destructive automotive transient experiments without suitable equipment and design review.

## 4. Isolated CAN simulator

- [ ] Use proper two-end termination and a known-good simulator at the configured bitrate.
- [ ] Passive mode: confirm no requests or ACKs from the prototype with an independent analyzer.
- [ ] Active mode: answer PID00/20/40 masks; verify only advertised metrics are polled.
- [ ] Return RPM frame on 0x7E8: `04 41 0C 1A F8 00 00 00`; expect 1726 rpm.
- [ ] Return coolant frame: `03 41 05 82 00 00 00 00`; expect 90 °C.
- [ ] Answer Mode03 with `04 43 01 03 00 00 00 00`; expect one P0300.
- [ ] Multi-frame Mode03: `10 0A 43 04 01 33 03 00`, then after FC `21 04 20 C1 23 00 00 00`; expect P0133, P0300, P0420, U0123.
- [ ] Test wrong ID, extended/RTR frames, short payload, oversized FF, wrong sequence and missing CF. No invalid reading or partial list may be shown as complete.
- [ ] Stop replies; confirm stale values and bounded shutdown. Force safe simulator-side CAN errors; verify no automatic restart.
- [ ] Observe at least 300 ms between requests; flow-control frames are separate protocol traffic.

## 5. Parked vehicle trial

- [ ] Confirm target vehicle supports classic CAN 11-bit diagnostics at the selected bitrate and ECU address.
- [ ] Use inspected protected hardware, vehicle stationary, parking brake applied, and an attended test.
- [ ] Connect with power off where practical; do not change wiring while powered.
- [ ] Compare readings against a trusted scanner under the same conditions. Do not run two active testers concurrently unless the test setup supports it.
- [ ] Confirm unsupported readings remain unavailable; do not infer a sensor fault from missing data.
- [ ] Disconnect immediately if vehicle warnings, communication anomalies or hardware heating appear.
- [ ] Disconnect physically after testing; the 15-minute software limit is not battery-drain protection.

No items here are pre-checked: these require physical hardware and independent observation.
