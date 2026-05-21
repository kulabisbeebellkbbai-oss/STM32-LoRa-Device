# External Firmware References

This directory contains shallow git submodules for full firmware stacks that the
device should support or remain compatible with:

- `meshtastic-firmware`: Meshtastic upstream firmware.
- `meshcore`: MeshCore upstream firmware.

These are not yet integrated into the native STM32WB55 PlatformIO build. Keep
ports and compatibility experiments isolated until the standalone hardware path
is stable.
