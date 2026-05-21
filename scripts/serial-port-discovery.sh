#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="${SERIAL_DISCOVERY_PROJECT_ROOT:-$(cd "${SCRIPT_DIR}/.." && pwd)}"
PIO_BIN="${PIO_BIN:-$(command -v pio 2>/dev/null || true)}"

PLAIN=0
FIRST=0
INCLUDE_SYSTEM_UART=0

usage() {
  cat <<'EOF'
Usage: serial-port-discovery.sh [--plain] [--first] [--include-system-uart]

Print candidate host serial ports for quick harness discovery.

Options:
  --plain                Print only newline-separated device paths.
  --first                Print only the first discovered port.
  --include-system-uart  Include /dev/ttyS* host UARTs. Off by default.
  -h, --help             Show this help.

Exit:
  0 when at least one port is found, 1 when no ports are found.
EOF
}

add_candidate() {
  local candidate=$1
  if [[ ! -e "$candidate" ]]; then
    return
  fi
  if [[ -L "$candidate" ]]; then
    candidate="$(readlink -f "$candidate")"
  fi
  if [[ "$INCLUDE_SYSTEM_UART" -ne 1 ]] && [[ "$candidate" == /dev/ttyS* ]]; then
    return
  fi
  if [[ "$candidate" == /dev/* ]] && [[ -c "$candidate" ]]; then
    PORTS["$candidate"]=1
  fi
}

collect_from_pio() {
  if [[ -z "${PIO_BIN}" ]]; then
    return
  fi
  if ! command -v "$PIO_BIN" >/dev/null 2>&1; then
    return
  fi
  while IFS= read -r candidate; do
    if [[ "$candidate" == /dev/* ]]; then
      add_candidate "$candidate"
    fi
  done < <("$PIO_BIN" device list 2>/dev/null | grep -Eo '/dev/[A-Za-z0-9./_-]+')
}

collect_glob_ports() {
  local pattern
  local candidate
  shopt -s nullglob
  for pattern in '/dev/ttyACM*' '/dev/ttyUSB*' '/dev/cu.*'; do
    for candidate in $pattern; do
      add_candidate "$candidate"
    done
  done
  if [[ "$INCLUDE_SYSTEM_UART" -eq 1 ]]; then
    for candidate in /dev/ttyS*; do
      add_candidate "$candidate"
    done
  fi
  shopt -u nullglob

  if [[ -d '/dev/serial/by-id' ]]; then
    for candidate in /dev/serial/by-id/*; do
      add_candidate "$candidate"
    done
  fi
}

print_ports() {
  local idx=1
  if [[ "${#PORT_LIST[@]}" -eq 0 ]]; then
    if [[ "$PLAIN" -eq 1 ]]; then
      return 1
    fi
    echo "No serial ports found under common USB/ACM/serial-by-id patterns."
    echo "Attach hardware and retry discovery."
    return 1
  fi

  if [[ "$PLAIN" -eq 1 ]]; then
    for port in "${PORT_LIST[@]}"; do
      echo "$port"
    done
    return 0
  fi

  echo "Serial port discovery from ${PROJECT_ROOT}:"
  for port in "${PORT_LIST[@]}"; do
    if [[ "$port" == /dev/serial/by-id/* ]]; then
      echo "  ${idx}) ${port}"
    else
      echo "  ${idx}) ${port}"
    fi
    idx=$((idx + 1))
  done
  return 0
}

declare -A PORTS

while [[ $# -gt 0 ]]; do
  case "$1" in
    --plain)
      PLAIN=1
      ;;
    --first)
      FIRST=1
      ;;
    --include-system-uart)
      INCLUDE_SYSTEM_UART=1
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

collect_glob_ports
collect_from_pio

PORT_LIST=()
set +u
PORT_COUNT="${#PORTS[@]}"
set -u
if [[ "$PORT_COUNT" -gt 0 ]]; then
  mapfile -t PORT_LIST < <(printf '%s\n' "${!PORTS[@]}" | sort)
fi

if [[ "$FIRST" -eq 1 ]]; then
  if [[ "${#PORT_LIST[@]}" -eq 0 ]]; then
    if [[ "$PLAIN" -eq 1 ]]; then
      exit 1
    fi
    echo "No serial ports discovered."
    exit 1
  fi
  if [[ "$PLAIN" -eq 1 ]]; then
    echo "${PORT_LIST[0]}"
  else
    echo "Discovered first port: ${PORT_LIST[0]}"
  fi
  exit 0
fi

print_ports
exit $?
