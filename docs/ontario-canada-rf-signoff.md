# Ontario, Canada RF Signoff

Date prepared: 2026-05-20
Prepared for: LoRa Device firmware bring-up in Ontario, Canada
Hardware baseline: STM32WB55CGU6 plus DX-SMART DX-LR03-900T22D (ASR6601 family)

This is an engineering signoff worksheet, not legal advice. It records the
project's current RF operating decision for Ontario, Canada testing.

Assumption used in this document:

- Module documentation for **DX-LR03-900T30D** lists `+28 dBm` as a maximum power
  candidate. The requested part for this build is **DX-LR03-900T22D**, and the
  exact max TX power value is not yet confirmed from a T22D primary source.

## Current Decision

RF transmit is not approved for the installed DX-LR03-900T22D module until legal and
project-specific gating is completed.

The approved firmware default for Ontario, Canada is:

| Profile | Status | Allowed activity |
| --- | --- | --- |
| `ca_902_928_rx_only` | Approved for receive-only workflow | Receive-only checks, serial diagnostics, display/UI work, and cabled/attenuated lab observations with TX disabled in firmware. |
| `ca_902_928_tx_locked` | Present but blocked | Stores candidate settings for 902-928 MHz operation, but firmware must refuse TX until a valid operating authority is documented. |
| `ca_902_928_tx_disabled` | Planned, not available for open-air work | Future Canada 902-928 MHz production path requires approved module certification, matching antenna, and documented policy for each endpoint use case. |

## Basis Checked

- ISED RSS-210 Issue 11 (June 25, 2024) for frequency use and certification context in the 902–928 MHz range.
- ISED RSS-247 for certification and transmission conditions.
- ISED RSS-Gen and ISED RIC-67 as companion references for licence-exempt and general station constraints.
- RSS family references are treated as regulatory references for review and approval planning only; they are not a software authorization to enable transmit.
- LoRaWAN Canada deployments normally use the US902-928 regional plan with
  902.3-914.9 MHz uplinks and 923.3-927.5 MHz downlinks.
- The current module target is a 900 MHz AT-modem family; no current test matrix has
  confirmed full legal operation for this specific part in every intended use case.

## Completion Checklist

Complete one of these paths before enabling any RF transmit profile.

### Path A: Receive-only 900 MHz lab work

- [ ] Firmware profile is `ca_902_928_rx_only`.
- [ ] `tx_enabled=false` is verified in firmware status output.
- [ ] No code path can call modem transmit commands in this profile.
- [ ] Test log records date, location, firmware build, antenna/cable state, and
  that no RF transmit was performed.

Signoff:

- Reviewer/operator:
- Callsign or authority if applicable:
- Date:
- Notes:

### Path B: Canada 902-928 MHz license-compliant path

- [ ] Confirm the module/device has the required ISED compliance route and any
  approval artifacts for the intended prototype/production model.
- [ ] Confirm 902-928 MHz profile table and channel settings are legal for this exact location and test use.
- [ ] Use a matching 902-928 MHz antenna and document antenna gain.
- [ ] Run low-power bench tests before open-air testing.

Signoff:

- Reviewer/operator:
- ISED module certification or compliance reference:
- Antenna:
- Date:
- Notes:

### Path C: Amateur experimental work (if used)

- [ ] A qualified amateur operator accepts responsibility for the station.
- [ ] Callsign is configured and transmitted as required.
- [ ] Non-obscured content is transmitted where amateur rules require it.
- [ ] Automatic relay/repeater behavior is disabled unless rules allow unattended
  operation and the operator approves.
- [ ] The test does not cause interference and does not claim protection from other
  services.
- [ ] The test log records frequency, bandwidth, TX power, antenna, message type,
  operator, and callsign.

Signoff:

- Amateur operator:
- Callsign:
- Certificate qualification:
- Date:
- Notes:

## Firmware Requirement

Until one checklist is completed, firmware must report Ontario/Canada profiles
as transmit-locked. The next firmware implementation should treat this document
as the profile source of truth for `ConfigService`.

Current firmware support:

- `config` prints the active `ca_902_928_tx_locked` profile, ISED reference set,
  signoff state, and `txAllowed=false`.
- `config signoff` records only a non-authorizing placeholder acknowledgement.
  It does not unlock transmit.
- Official references checked for this profile are ISED RSS-210 Issue 11
  (June 25, 2024), RSS-247, RSS-Gen, and RIC-67. These references identify the
  compliance path to review; they are not project permission to transmit.
