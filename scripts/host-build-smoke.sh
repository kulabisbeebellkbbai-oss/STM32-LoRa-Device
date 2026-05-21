#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="${HOST_BUILD_PROJECT_ROOT:-$(cd "${SCRIPT_DIR}/.." && pwd)}"
PIO_BIN="${PIO_BIN:-$(command -v pio 2>/dev/null || true)}"
export PLATFORMIO_BUILD_DIR="${PLATFORMIO_BUILD_DIR:-/tmp/pio_lora_device_build}"

if [[ -z "$PIO_BIN" ]] || [[ ! -x "$PIO_BIN" ]]; then
  echo "PlatformIO not found. Install PlatformIO or set PIO_BIN to a valid CLI path."
  exit 1
fi

echo "Using PlatformIO: $PIO_BIN"
"$PIO_BIN" --version

echo "Running host-side build (no upload, no hardware required)"
echo "Using build directory: $PLATFORMIO_BUILD_DIR"
IFS=' ' read -r -a BUILD_ENVS <<< "${LORA_BUILD_ENVS:-stm32wb55_bringup stm32wb55_hardware_gates}"
for env_name in "${BUILD_ENVS[@]}"; do
  "$PIO_BIN" run -d "$PROJECT_ROOT" -e "$env_name"
done

echo "Host build complete."
