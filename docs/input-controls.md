# Input Control Recommendation

## Recommended Normal-Use Controls

Use a rotary encoder for primary navigation, two dedicated navigation buttons,
a keypad for direct entry, and two guarded-mode toggles.

| Control | Count | Role | Why |
| --- | --- | --- |
| Rotary encoder with push switch | 1 | Menu scroll, value adjust, select | Best primary control for the small ST7789 screen and nested app-mode menus. |
| Navigation buttons | 2 | Back/cancel and home/status | These remove the most common menu friction without duplicating the encoder press or keypad. |
| 12-button keypad, `0-9`, `*`, `#` | 1 | Direct channel, preset, PIN, address, or short numeric/text entry | Useful for LoRaWAN keys, presets, Meshtastic/MeshCore channel choices, and device configuration without USB. |
| Toggle switch, radio enable | 1 | Physical RF/app transmit enable | Makes radio activity an intentional state and helps bench testing. |
| Toggle switch, profile/power mode | 1 | Normal/low-power or field/setup profile | Gives a fast persistent mode choice without entering menus. |

This is the recommended full control set for normal standalone operation. It
keeps the device usable without USB while still leaving the encoder as the main
menu control.

## Excluded Inputs

| Control | Reason |
| --- | --- |
| Slide or thumb potentiometer | It does not add enough value for this device. Backlight, volume, and thresholds are better handled as saved menu settings; analog controls add drift, noise, ADC use, and enclosure openings. |
| Extra confirm/send buttons | The encoder push switch and keypad already cover these actions, so extra buttons would add clutter without a distinct workflow. |

## Minimal Bring-Up Controls

For early USB-powered board testing, wire only:

- Rotary encoder A/B/SW.
- Back button.
- Radio-enable toggle, if available.

Add the keypad, home/status button, and second toggle after display, sensors,
GPS, and DX-LR03 UART/AT radio tests are stable.

## Implementation Notes

- Prefer a keypad matrix or I2C GPIO expander if direct GPIO count gets tight.
- Debounce every button, encoder switch, and toggle in firmware.
- Treat toggles as state inputs at boot and during runtime.
- Keep the USB command interface enabled during bring-up as the diagnostic path,
  but do not make normal standalone operation depend on it.
