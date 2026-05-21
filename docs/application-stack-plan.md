# Application Stack Plan

The device needs to support several LoRa application paths. Treat these as
separate firmware/application modes until memory, licensing, and upstream support
are proven on STM32WB55 plus DX-LR03-900 AT-modem.

## Mode Targets

| Mode | Initial repository treatment | First coding milestone |
| --- | --- | --- |
| Standalone | Native project mode | Display local telemetry and GPS state. |
| Native LoRa packet / P2P | Native project mode | Add configurable receive/transmit packet commands with regional guardrails. |
| LoRa text/chat | Native project mode | Add a simple local message UI before upstream mesh ports are ready. |
| KISS / serial LoRa modem | Native project mode | Expose a project-local USB serial modem profile; add upstream-specific adapters only after protocol requirements are chosen. |
| Native relay / store-and-forward | Native project mode | Add duplicate-safe forwarding with hop limits and airtime controls. |
| Meshtastic | Upstream firmware submodule in `external/meshtastic-firmware` | Track STM32WB55/modem feasibility and required upstream porting. |
| MeshCore | Upstream firmware submodule in `external/meshcore` | Track STM32WB55/modem feasibility and required upstream porting. |
| Reticulum / RNode / LXMF | Upstream compatibility investigation | Decide whether to emulate RNode, implement a Reticulum custom interface, or leave LXMF on a host. |
| LoRa APRS | Conditional native target | Add only after callsign/licensing, no-encryption, and regional rules are confirmed. |
| LoRaWAN | Native experiment-first compatibility investigation | Verify AT-modem support path first; do not assume native LoRaWAN stack replacement is available. |
| USB host interface | Deferred feature flag | Host protocol after standalone tests pass; the current serial diagnostics stay enabled for bring-up. |

See [LoRa Application Candidate Matrix](lora-application-candidates.md) for the
full include/defer/not-suitable list, including TinyGS, MySensors, OpenMQTTGateway,
disaster.radio, aviation-style trackers, LoRaWAN gateways, and network-server
systems.

## First Software Interfaces

- `RadioService`: owns modem UART setup, AT command state, packet send/receive,
  and protocol adapters.
- `SensorService`: owns BMP280, DHT11, ADXL345, and GPS readings.
- `DisplayService`: owns ST7789 rendering and backlight control.
- `InputService`: owns encoder/button events.
- `TelemetryModel`: normalizes environmental readings for every app mode.
- `UsbCommandService`: enabled for bring-up diagnostics (`help`, `status`,
  `telemetry`, `display`, `radio`, `sensors`, `i2cscan`, `pins`, `version`);
  keep normal standalone operation independent of USB.

## Upstream Sources

- Meshtastic firmware: https://github.com/meshtastic/firmware
- MeshCore firmware: https://github.com/meshcore-dev/MeshCore
- Native LoRa/LoRaWAN experiments: AT modem integration layer (`DX-LR03-900T22D`/ASR6601 profile docs)
- Full candidate list: `docs/lora-application-candidates.md`

Use the upstream projects as compatibility references and possible build targets.
Do not assume they will run unchanged on the STM32WB55CGU6 plus DX-LR03-900 board
until board support and modem pin/mode mapping are explicitly proven.
