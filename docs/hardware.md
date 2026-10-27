# Hardware and system design

## Design boundary

ESP32-S3 is the main MCU and Wi-Fi server. Its integrated TWAI controller connects to a 3.3 V CAN transceiver. The design deliberately removes the old MCP2515 controller and its SPI interface. SN65HVD230 is a prototype option; its supply range is 3.0–3.6 V. Do not substitute a 5 V-only module whose RX output exceeds MCU limits.

The SVG is a system wiring diagram, **not a PCB fabrication schematic or a fully specified surge-protection circuit**. Exact component choices, ratings, thermal design, layout, and automotive transient testing remain engineering work. It is suitable for planning and controlled bench assembly, not claiming a road-qualified product.

## Bill of materials

| Item | Prototype requirements | Qty |
|---|---|---:|
| ESP32-S3 development board | Exposes GPIO11/13, documented 5 V input, sufficient regulator capacity | 1 |
| SN65HVD230 transceiver module | 3.3 V supply and logic; identifiable D, R, CANH, CANL, GND; inspect RS | 1 |
| OBD-II male breakout/cable | Verified numbered pins 4, 5, 6, 14, 16; strain relief | 1 |
| Protected power front end | Fuse, reverse-polarity protection, automotive transient suppression and filtering | 1 assembly |
| Buck converter | Regulated 5 V; input range and absolute maximum compatible with protection clamp; suitable thermal margin | 1 |
| Fuse and holder | Near OBD supply takeoff; sized to wiring, inrush, converter demand and fault current | 1 |
| Decoupling | 100 nF ceramic close to transceiver VCC/GND; local bulk capacitance per module design | As needed |
| Enclosure and harness | Insulated, secured, twisted CAN pair, no loose conductive parts | 1 |
| Bench equipment | Current-limited supply, multimeter, known-good classic CAN simulator/interface | For validation |

Budget roughly 5 V / 1 A supply capacity for a development-board prototype, then **measure** peak load, input current, startup/inrush and temperature. This is a supply-design starting point, not a fuse selection. Wi-Fi causes current transients; the board's 3.3 V regulator must supply the MCU and transceiver without brownouts.

## Connections

Use [wiring.csv](../hardware/wiring.csv). GPIO11 is TX (to D); GPIO13 is RX (from R). These are GPIO numbers, not physical header positions. Check your board schematic for flash/PSRAM/peripheral conflicts and available pins. Avoid unreviewed strapping pins. The IC's SOIC pins are D=1, GND=2, VCC=3, R=4, Vref=5, CANL=6, CANH=7, RS=8; **module headers may have a completely different order**. Leave Vref unconnected unless a specific reviewed design needs it. RS low selects high-speed operation; do not leave it floating.

OBD connector illustrations can be mirrored between the vehicle socket face, plug mating face, and solder side. Use molded pin numbers and continuity measurements; do not infer left/right from a generic photograph. Pin 4 is chassis ground and pin 5 is signal ground. Both belong in a deliberate common-reference grounding arrangement for this nonisolated prototype; keep buck current return on the chassis-ground path and avoid routing converter current through a thin signal-ground lead. Confirm the breakout/vehicle documentation before bonding. This prototype is not galvanically isolated.

## Power front end

The intended order is OBD16 → fuse → reverse-polarity protection → surge/filter network → buck → board 5 V input. OBD4 is the primary power return; all signal electronics share the documented common reference. Never feed 12 V to 5V, 3V3, USB, or GPIO.

The surge network may use an automotive TVS and input filter, but selecting a TVS voltage alone does not validate the assembly. Coordinate its normal stand-off voltage, clamp voltage at the expected current, pulse energy, fuse clearing, converter absolute maximum, reverse-polarity device ratings and PCB parasitics. Validate cold crank, jump start, reverse battery and load-dump requirements appropriate to the target vehicle. An ordinary buck's “12 V input” listing is insufficient. A qualified protected automotive supply module is preferable to an improvised front end for vehicle trials.

Do not simultaneously power a development board from vehicle 5 V and USB unless its schematic explicitly supports this. For programming, disconnect OBD and use USB. Consider ground loops through a laptop, grounded bench instruments and the vehicle. An appropriately designed isolated interface may be needed for lab measurements.

## CAN physical layer

- Connect CANH to OBD6 and CANL to OBD14 using a short twisted pair. Keep the diagnostic stub short and away from buck switching loops.
- Inspect the module for a soldered 120-ohm resistor or jumper. Remove/disable its termination when attaching as a stub to an existing vehicle bus. Do not remove the vehicle's own termination.
- An isolated two-node bench bus needs termination at its two physical ends, typically 120 ohms each. With power removed, approximately 60 ohms across H/L can indicate two such resistors in parallel; vehicle topology/gateways can change the reading. Never measure resistance on an energized bus.
- Keep signal wiring short; place transceiver decoupling close to the IC. Add suitable CAN-line ESD/transient protection in a reviewed custom design. Do not attach LEDs or pull-down resistors directly to the bus lines.
- CAN FD-capable transceiver hardware does not make the ESP32-S3 controller support CAN FD.

## Future automotive PCB

Use a transceiver with appropriate automotive qualification, bus-fault protection, compatible 3.3 V logic and defined unpowered behavior. Add supervised power, EMC filtering, validated surge protection, thermal and reverse-polarity design, secure connectors and an enclosure. Review grounding, creepage where applicable, ESD, power consumption and sleep/wake behavior. Complete vehicle-specific compatibility and transient testing before deployment.
