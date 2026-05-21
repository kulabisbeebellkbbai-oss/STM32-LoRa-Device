# LoRa Application Candidate Matrix

This matrix is for the current hardware target: STM32WB55CGU6 plus DX-SMART
DX-LR03-900T22D (DX-LR03-900 family, ASR6601) 900 MHz UART AT-command LoRa
module, ST7789 display, GPS, sensors, keypad, and USB serial diagnostics.

Status legend:

- `include-native`: implement directly in this firmware.
- `include-port-track`: keep in the product plan, but expect board-support and
  protocol-porting work before it can run on this MCU.
- `conditional`: useful only when the matching region, license, gateway, or
  operating model is available.
- `host-side-only`: belongs on a phone, PC, server, gateway, or companion host,
  while this device exposes a serial/radio endpoint.
- `not-suitable`: not a good fit for this board/radio.

Use this as a planning matrix, not a production compatibility guarantee. The
current baseline is native firmware around a UART modem service and AT command
profile layer; upstream app ports are not proven until STM32WB55 board support,
memory fit, and pin/protocol bindings are built and tested.

## Recommended Inclusion List

| Application / protocol | Status | Fit with this hardware | What is missing before real use |
| --- | --- | --- | --- |
| Standalone native LoRa telemetry | include-native | Good first product mode. DX-LR03 UART link is a practical base for telemetry beacon/receiver and local status text. | Config storage, channel/profile governance, receive path, packet framing, and RF-safe transmit gate. |
| Raw LoRa packet / P2P utility mode | include-native | AT modem supports transparent/fixed/broadcast framing and can expose frequency/channel settings through command-mode control. | Profile-level config commands, bounded payload checks, serial framing validation, and regional profile validation. |
| Simple LoRa text/chat mode | include-native | Practical local UI mode while upstream mesh ports are pending. | Addressing, contact/channel model, duplicate suppression, retries, message storage, receive UI, and channel profile consistency. |
| KISS / serial LoRa modem | include-native | Good fit for USB serial and host automation, but project-local modem profile rules still required. | Escape/framing rules, AT command contract, host integration tests, and documented radio command set. Do not assume this satisfies Meshtastic, MeshCore, RNode, or MQTT bridge clients without a matching adapter. |
| Native relay / store-and-forward mode | include-native | Natural middle step between raw P2P and full third-party mesh ports. It can reuse the same modem envelope and display local relay state. | Duplicate detection, hop limit, queue storage, duty-cycle/airtime controls, and loop prevention. |
| LoRaWAN Class A end device | conditional | ASR6601 AT modems are typically not a native substitute for upstream LoRaWAN stacks. Use native paths if explicit LoRaWAN firmware support is added. | Region selection, OTAA/ABP session handling, explicit gateway/path assumptions, and approved legal/radio profile confirmation. |
| Meshtastic | include-port-track | Product fit remains strong for off-grid text and telemetry, but this board currently has no native Meshtastic transport target for this modem family. | STM32WB55 board support, radio/profile binding, and full protocol/legal integration review for the selected band. |
| MeshCore | include-port-track | Fit depends on a modem abstraction rewrite to support UART AT profile control for this hardware. | Board support, app/USB/BLE companion decision, radio binding, and protocol policy review. |
| Reticulum / RNode / LXMF | include-port-track | Only if firmware intentionally implements an adapter protocol on top of the AT modem. | Choose one: RNode emulation path, Reticulum custom interface, or host-only LXMF bridge. Then implement protocol contract, encryption policy, and addressing rules. |
| LoRa APRS tracker / iGate-compatible endpoint | conditional | Hardware has GPS, display, keypad, and UART LoRa modem path. It can be a tracker or serial endpoint, not a full internet iGate without a host. | Amateur-radio license/callsign rules where applicable, APRS packet encoder, GPS validation, and host/network path for iGate use. |
| MySensors LoRa / RFM95 transport | conditional | Useful only if a custom transport mapping is built for the AT profile. | Message mapping, gateway/controller choice, and serial protocol contract. |
| TinyGS receiver | conditional | 900 MHz-capable antenna and profile selection may suit some TinyGS-style RX workflows, but firmware still needs explicit interface decisions. | Host/cloud path, supported frequency/profile choices, and legal/test gating. |
| OpenMQTTGateway raw LoRa bridge | host-side-only | Useful conceptually; the endpoint can expose a serial modem profile while a host publishes MQTT. | Host bridge script/service, schema, and defined serial modem command contract. |
| disaster.radio | include-port-track | Conceptually similar off-grid mesh, but this repository modem is AT/serial-first and should not be treated as a drop-in radio transport. | Major port or protocol reimplementation and adapter layer for this hardware stack. |
| SoftRF / OGN / aviation-style LoRa tracker | conditional | GPS/display + LoRa are relevant, but aviation tracking has specialized RF, safety, and protocol constraints. | Confirm protocol legality/use case, payload policy, GPS timing quality, and whether this project should enter that domain. |
| LoRa weather/balloon/tracker custom profiles | conditional | Hardware is a natural fit for low-rate telemetry, especially with GPS and environmental sensors. | Profile definitions, receive/forward mode, ID/preamble policy, and region-specific TX rules. |
| Helium miner / hotspot | not-suitable | Requires gateway/concentrator-class hardware and backhaul, not a single AT modem endpoint. | Use different hardware with SX130x-class concentrator and network backhaul. |
| LoRaWAN gateway | not-suitable | A single endpoint module is not a multi-channel LoRaWAN gateway. | Use SX1302/SX1303 concentrator gateway hardware instead. |
| ChirpStack / The Things Stack / TTN server | host-side-only | These are network-server/backend systems, not firmware modes for the endpoint. | Run on host/server infrastructure and connect through a true LoRaWAN endpoint path if needed. |

## First Firmware Additions

Implement these before attempting full app ports:

1. `ConfigService` with persisted radio/app profiles.
2. UART modem profile service with explicit AT command translation for frequency,
   ADR/air settings, region constraints, and power profile.
3. Serial commands for app profile, region, channel settings, node ID, and profile
   import/export.
4. Native packet receive path in `RadioService`.
5. A generic LoRa packet envelope shared by standalone, text/chat, and serial
   modem modes.
6. Native relay/store-and-forward mode using the same packet envelope.
7. Hard regional guardrails that prevent accidental over-the-air transmit on an
   unapproved profile.

## Initial Guardrail Source of Truth

Until a local legal/regulatory signoff is recorded in `docs/lora-regional-caveats.md`,
firmware should ship with `tx_enabled=false` for every profile. The initial
profile table should be explicit and conservative:

| Profile | Firmware default | Purpose |
| --- | --- | --- |
| `ca_902_928_rx_only` | Allowed without RF transmit | Receive-only and cabled/attenuated bench checks for the installed 900 MHz module in Ontario, Canada. |
| `ca_902_928_tx_locked` | Present but transmit-locked | Stores candidate 900 MHz settings, but refuses TX until project-local approval is recorded. |
| `ca_470_915_module_required` | Present but unavailable in current build | Placeholder for future non-LoRaWAN/legacy frequency experiments; requires matching certified hardware and antenna. |

The guardrail implementation should enforce at least: module band compatibility,
explicit `tx_enabled`, max TX power for the approved profile, payload size limits,
and an airtime/duty-cycle budget where the selected rule set requires it.

## Current Blockers

- STM32WB55 port status: no upstream Meshtastic, MeshCore, Reticulum/RNode, or
disaster.radio runtime is proven on this exact board in this repository.
- 900 MHz regional legal and channel constraints are not universal; all over-air
use must match local law and test context.
- Shared resource sequencing still depends on staged validation for UART command mode
and module power domain.
- GPS/display/input integration: GPS UART parsing, local message UI, and keypad
entry need stable firmware services before user-facing mesh modes are useful.
- Wi-Fi, Ethernet, BLE, and cloud dependencies: MQTT brokers, TTN/ChirpStack,
TinyGS, and several ESP32-first apps need host infrastructure or major porting.

## Regional Notes

The installed module is DX-LR03-900 family 850-931 MHz class hardware. In Ontario,
Canada, do not treat legacy 433 MHz assumptions as this project’s default path unless
the exact module profile and legal basis are confirmed. For Canadian LoRaWAN and
902-928 MHz work, use approved 902-928 profile settings, compliant antennas, and legal
authority before transmit.

Encrypted mesh applications need a policy decision before amateur-band use. If a path
falls under amateur-band rules, obscured/encrypted traffic is typically not acceptable
for that test path, and callsign/identity requirements may apply.

## Research Sources Checked

- DX-LR03/ASR6601 AT-command reference and module datasheet (vendor-provided family docs).
- LoRaWAN regional parameters overview: https://www.thethingsnetwork.org/docs/lorawan/regional-parameters/
- LoRa Alliance Regional Parameters resource: https://lora-alliance.org/resource_hub/rp2-102-lorawan-regional-parameters/
- Reticulum / RNode hardware notes: https://reticulum.community/manual/hardware.html
- TinyGS hardware notes: https://github.com/tinygs/tinyGS
- LoRa APRS tracker hardware notes: https://github.com/lora-aprs/LoRa_APRS_Tracker
- OpenMQTTGateway LoRa gateway notes: https://docs.openmqttgateway.com/use/lora.html
- disaster.radio firmware/hardware notes: https://disaster.radio/learn/firmware/
