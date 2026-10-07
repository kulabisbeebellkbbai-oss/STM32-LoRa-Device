#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
DISCOVERY_SCRIPT="${SCRIPT_DIR}/serial-port-discovery.sh"

REQUESTED_PORT=""
OUTPUT=""
DURATION=45
WAIT_SECONDS=30
BAUD=115200

usage() {
  cat <<'EOF'
Usage: capture-boot-log.sh [--port <serial_port>] --output <log_file> [options]

Wait for a USB serial port, then capture a reset boot log.

Options:
  --port <device>       Serial port to capture. If omitted, auto-discover.
  --output <file>       Destination log file.
  --duration <seconds>  Capture duration after the port opens. Default: 45.
  --wait <seconds>      Time to wait for a serial port to appear. Default: 30.
  --baud <baud>         Serial baud rate. Default: 115200.
  -h, --help            Show this help.

Exit:
  0 when the log contains the expected boot banner.
  2 when capture completed but the boot banner was not observed.
  3 when no serial port appeared before the wait timeout.
EOF
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --port)
      REQUESTED_PORT="${2:-}"
      shift
      ;;
    --output)
      OUTPUT="${2:-}"
      shift
      ;;
    --duration)
      DURATION="${2:-}"
      shift
      ;;
    --wait)
      WAIT_SECONDS="${2:-}"
      shift
      ;;
    --baud)
      BAUD="${2:-}"
      shift
      ;;
    -h|--help)
      usage
      exit 0
      ;;
    *)
      echo "Unknown argument: $1" >&2
      usage
      exit 1
      ;;
  esac
  shift
done

if [[ -z "$OUTPUT" ]]; then
  echo "Missing required --output <file>." >&2
  usage
  exit 1
fi

is_positive_integer() {
  local value=$1
  [[ "$value" =~ ^[1-9][0-9]*$ ]]
}

case "$DURATION" in
  ''|*[!0-9]*)
    echo "--duration must be a positive integer." >&2
    exit 1
    ;;
esac
if ! is_positive_integer "$DURATION"; then
  echo "--duration must be a positive integer." >&2
  exit 1
fi

case "$WAIT_SECONDS" in
  ''|*[!0-9]*)
    echo "--wait must be a positive integer." >&2
    exit 1
    ;;
esac
if ! is_positive_integer "$WAIT_SECONDS"; then
  echo "--wait must be a positive integer." >&2
  exit 1
fi

mkdir -p "$(dirname "$OUTPUT")"

discover_port() {
  local port=""
  if [[ -n "$REQUESTED_PORT" ]]; then
    if [[ -e "$REQUESTED_PORT" ]]; then
      echo "$REQUESTED_PORT"
      return 0
    fi
    return 1
  fi
  if [[ -x "$DISCOVERY_SCRIPT" ]]; then
    port="$("$DISCOVERY_SCRIPT" --plain --first 2>/dev/null || true)"
  fi
  if [[ -n "$port" && -e "$port" ]]; then
    echo "$port"
    return 0
  fi
  return 1
}

wait_for_port() {
  local deadline=$1
  local port=""
  while [[ "$SECONDS" -lt "$deadline" ]]; do
    if port="$(discover_port)"; then
      echo "$port"
      return 0
    fi
    sleep 1
  done
  return 1
}

initial_deadline=$((SECONDS + WAIT_SECONDS))
if ! PORT="$(wait_for_port "$initial_deadline")"; then
  {
    echo "No serial port appeared within ${WAIT_SECONDS}s."
    echo "Observed USB devices:"
    lsusb || true
  } | tee "$OUTPUT" >&2
  exit 3
fi

: > "$OUTPUT"
echo "Capturing ${PORT} at ${BAUD} baud for ${DURATION}s."
echo "Press reset or replug the board now if the boot banner has not already started."

capture_deadline=$((SECONDS + DURATION))
while [[ "$SECONDS" -lt "$capture_deadline" ]]; do
  if [[ ! -e "$PORT" ]]; then
    PORT="$(wait_for_port "$capture_deadline" || true)"
    if [[ -z "$PORT" ]]; then
      break
    fi
    echo "Serial port reappeared as ${PORT}."
  fi

  stty -F "$PORT" "$BAUD" raw -echo -crtscts || true
  remaining=$((capture_deadline - SECONDS))
  if [[ "$remaining" -le 0 ]]; then
    break
  fi

  timeout "${remaining}s" cat "$PORT" | tee -a "$OUTPUT" >/dev/null || true
  if grep -q "LoRaWB LoRa Endpoint boot" "$OUTPUT"; then
    break
  fi
  sleep 1
done

if grep -q "LoRaWB LoRa Endpoint boot" "$OUTPUT"; then
  echo "PASS: boot banner captured in ${OUTPUT}."
  exit 0
fi

echo "WARN: capture completed but the boot banner was not observed in ${OUTPUT}." >&2
exit 2
