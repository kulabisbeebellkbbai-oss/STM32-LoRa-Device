# LoRa Regional / Legal Test Caveats

Before any over-the-air transmission, confirm regulatory constraints for your exact
country, state/province, and test environment.

## Global defaults you should verify

- Channel/frequency legality (for example 868 MHz / 902-928 MHz / 915 MHz availability).
- Maximum effective radiated power (EIRP) for your device class.
- Duty-cycle or dwell-time limits for the chosen sub-band.
- Spectrum occupancy requirements and fair-use restrictions.
- Whether your test setup requires lab approval or an amateur/private-network permit.

## During development

- Use point-to-point, bench-scale testing first (`TX` power reduced).
- Keep antennas physically present only for final, controlled RF checks.
- Prefer attenuated / short-path testing at the lowest legal profile.
- Stop immediately on suspected frequency conflict or unexpected external interference.
- Log every RF test: frequency, SF/BW (if used), TX power setting, date, and power source.

## Process requirement

- Record the local authority consulted (FCC/IC/ETSI/local equivalent) and date.
- Keep this record with test artifacts; treat it as part of the project release gate.
- If team policy requires legal review, complete that before field trials.

## Hard stop conditions

Do not continue over-the-air tests if any of these are missing:

- Verified regional authorization for the selected band,
- Confirmed peer station/channel plan,
- Stable antenna and power integrity checks,
- Documented emergency stop procedure in case of uncontrolled emissions.

For this repository, do not move from lab-only checks to open-air tests until this
document is signed off in your test log.

## Ontario, Canada project signoff

The current project test region is Ontario, Canada. Use
[Ontario, Canada RF Signoff](ontario-canada-rf-signoff.md) as the active regional
signoff worksheet.

Current firmware policy:

- DX-LR03-900T22D/ASR6601 AT-modem transmit remains locked pending RF authority.
- Receive-only 900 MHz lab work is the only approved path until the worksheet is
  completed.
- Canada 902-928 MHz LoRa/LoRaWAN work requires a matching 915 MHz-class module,
  matching antenna, and compliance review before transmit is enabled.
- Amateur-band experiments require a qualified operator/callsign and non-obscured
  traffic; do not enable encrypted mesh traffic under the amateur path.

### Ontario / Canada Reference Set (for review)

- RSS-210 Issue 11 (June 25, 2024)
- RSS-247
- RSS-Gen
- RIC-67

These references support planning and compliance review only; they are not treated as
in-firmware permission to transmit.
