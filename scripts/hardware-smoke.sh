#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="${HARDWARE_SMOKE_PROJECT_ROOT:-$(cd "${SCRIPT_DIR}/.." && pwd)}"
PORT_DISCOVERY_SCRIPT="${SCRIPT_DIR}/serial-port-discovery.sh"
COMMANDS_FILE="${HARDWARE_SMOKE_COMMANDS_FILE:-${PROJECT_ROOT}/tests/hardware-smoke-commands.tsv}"
TEST_RESULTS_ROOT="${HARDWARE_SMOKE_RESULTS_ROOT:-${PROJECT_ROOT}/tests/results}"
PORT="${HARDWARE_SMOKE_PORT:-}"
BAUD="${HARDWARE_SMOKE_BAUD:-115200}"
COMMAND_TIMEOUT="${HARDWARE_SMOKE_COMMAND_TIMEOUT:-3}"
COMMAND_SETTLE_SECONDS="${HARDWARE_SMOKE_SETTLE_SECONDS:-1}"
DRY_RUN=0
AUTO_DISCOVER=0
LIST_PORTS=0
RUN_TIMESTAMP="$(date -u +'%Y%m%dT%H%M%SZ')"

usage() {
  cat <<'EOF'
Usage: hardware-smoke.sh --port <serial_port> [options]
       hardware-smoke.sh --discover
       hardware-smoke.sh --list
       hardware-smoke.sh --help
       hardware-smoke.sh --dry-run

Run a serial smoke harness against firmware command behavior.

The default path is non-hardware: dry-run, --help, and --list modes can be used
without a connected device.

Options:
  --port <device>          Serial port (for example /dev/ttyACM0).
  --discover               Auto-pick the first discovered serial port.
  --list                   Show discovered ports and exit.
  --dry-run                Validate command plan without serial I/O.
  --commands <file>        Override command corpus TSV (default tests/hardware-smoke-commands.tsv).
  --baud <speed>           Serial baud rate (default 115200).
  --timeout <seconds>      Per-command timeout (default 3).
  --settle <seconds>       Sleep after each command (default 1).
  --results-dir <path>     Override test result root (default tests/results).
  --help                   Show this message.

Results:
  Logs and command records are written under tests/results/hardware-smoke/<timestamp>/.
EOF
}

decode_escapes() {
  printf '%b' "$1"
}

now_ts() {
  date -u +'%Y-%m-%dT%H:%M:%SZ'
}

log() {
  printf '%s %s\n' "$(now_ts)" "$*" | tee -a "$RUN_LOG"
}

cleanup() {
  if [[ -n "${CAPTURE_PID:-}" ]] && kill -0 "$CAPTURE_PID" >/dev/null 2>&1; then
    kill "$CAPTURE_PID" >/dev/null 2>&1 || true
    wait "$CAPTURE_PID" >/dev/null 2>&1 || true
  fi

  if [[ -n "${PORT_TTY_STATE:-}" ]] && [[ -n "${PORT}" ]] && [[ -e "${PORT}" ]]; then
    stty -F "$PORT" "$PORT_TTY_STATE" || true
  fi
}

require_file() {
  if [[ ! -f "$1" ]]; then
    echo "Missing file: $1"
    exit 1
  fi
}

load_ports() {
  if [[ -n "$PORT" || "$AUTO_DISCOVER" -eq 0 ]]; then
    return
  fi

  PORT="$("$PORT_DISCOVERY_SCRIPT" --plain --first || true)"
  if [[ -z "${PORT}" ]]; then
    echo "No serial port discovered. Use --port explicitly."
    exit 1
  fi
}

wait_for_match() {
  local expected="$1"
  local timeout="$2"
  local from_line="$3"
  local deadline
  deadline=$((SECONDS + timeout))

  while (( SECONDS < deadline )); do
    if tail -n +$((from_line + 1)) "$SERIAL_LOG" | grep -Fq -- "$expected"; then
      return 0
    fi
    sleep 0.2
  done
  return 1
}

run_dry() {
  if [[ ! -f "$COMMANDS_FILE" ]]; then
    echo "Command file missing: $COMMANDS_FILE"
    exit 1
  fi

  RESULTS_DIR="${TEST_RESULTS_ROOT}/hardware-smoke/${RUN_TIMESTAMP}"
  mkdir -p "$RESULTS_DIR"
  RUN_LOG="${RESULTS_DIR}/run.log"

  echo "Dry-run mode enabled; no serial I/O will occur." | tee -a "$RUN_LOG"
  echo "Commands file: ${COMMANDS_FILE}" | tee -a "$RUN_LOG"
  if [[ -z "$PORT" && "$AUTO_DISCOVER" -eq 1 ]]; then
    PORT="$("$PORT_DISCOVERY_SCRIPT" --plain --first || true)"
  fi
  echo "Auto-discovered port: ${PORT:-<none>}" | tee -a "$RUN_LOG"
  echo "Results directory: ${RESULTS_DIR}" | tee -a "$RUN_LOG"
  echo "" | tee -a "$RUN_LOG"
  echo "Planned command sequence:" | tee -a "$RUN_LOG"
  while IFS=$'\t' read -r command expected timeout || [[ -n "${command:-}" ]]; do
    if [[ -z "${command// }" ]] || [[ "${command:0:1}" == "#" ]]; then
      continue
    fi
    command="${command//$'\r'/}"
    expected="${expected//$'\r'/}"
    command="$(decode_escapes "$command")"
    expected="$(decode_escapes "${expected:-}")"
    timeout="${timeout:-$COMMAND_TIMEOUT}"
    echo "  - ${command} (expect: ${expected:-<none>} timeout=${timeout}s)" | tee -a "$RUN_LOG"
  done < "$COMMANDS_FILE"
}

parse_args() {
  while [[ $# -gt 0 ]]; do
    case "$1" in
      --help|-h)
        usage
        exit 0
        ;;
      --list)
        LIST_PORTS=1
        ;;
      --port)
        PORT="${2:?--port requires a value}"
        shift
        ;;
      --discover)
        AUTO_DISCOVER=1
        ;;
      --dry-run)
        DRY_RUN=1
        ;;
      --commands)
        COMMANDS_FILE="${2:?--commands requires a value}"
        shift
        ;;
      --baud)
        BAUD="${2:?--baud requires a value}"
        shift
        ;;
      --timeout)
        COMMAND_TIMEOUT="${2:?--timeout requires a value}"
        shift
        ;;
      --settle)
        COMMAND_SETTLE_SECONDS="${2:?--settle requires a value}"
        shift
        ;;
      --results-dir)
        TEST_RESULTS_ROOT="${2:?--results-dir requires a value}"
        shift
        ;;
      *)
        echo "Unknown argument: $1"
        usage
        exit 2
        ;;
    esac
    shift
  done
}

parse_args "$@"

RESULTS_DIR="${TEST_RESULTS_ROOT}/hardware-smoke/${RUN_TIMESTAMP}"
mkdir -p "$RESULTS_DIR"
RUN_LOG="${RESULTS_DIR}/run.log"
SERIAL_LOG="${RESULTS_DIR}/serial.log"
COMMAND_RESULTS="${RESULTS_DIR}/commands.tsv"
SUMMARY_TSV="${RESULTS_DIR}/summary.tsv"
trap cleanup EXIT

if [[ "$LIST_PORTS" -eq 1 ]]; then
  "$PORT_DISCOVERY_SCRIPT"
  exit $?
fi

if [[ "$DRY_RUN" -eq 1 ]]; then
  run_dry
  exit 0
fi

require_file "$COMMANDS_FILE"
load_ports

if [[ -z "$PORT" ]]; then
  echo "A serial port is required for non-dry-run mode."
  echo "Use --port /dev/ttyACM0, --discover, or --list."
  exit 1
fi

if [[ ! -e "$PORT" ]]; then
  echo "Serial port does not exist: $PORT"
  exit 1
fi

if [[ ! -c "$PORT" ]]; then
  echo "Serial path is not a character device: $PORT"
  exit 1
fi

touch "$COMMAND_RESULTS"
touch "$SUMMARY_TSV"
touch "$SERIAL_LOG"
printf 'run_timestamp=%s\nstart_utc=%s\nport=%s\nbaud=%s\ncommands_file=%s\ncommand_timeout=%s\nsettle_seconds=%s\n' \
  "$RUN_TIMESTAMP" "$(now_ts)" "$PORT" "$BAUD" "$COMMANDS_FILE" "$COMMAND_TIMEOUT" "$COMMAND_SETTLE_SECONDS" > "$SUMMARY_TSV"
printf 'outcome\tcommand\texpected\n' >> "$SUMMARY_TSV"

PORT_TTY_STATE="$(stty -F "$PORT" -g)"
log "Using serial port: ${PORT}"
log "Command set: ${COMMANDS_FILE}"
log "Run timeout(default): ${COMMAND_TIMEOUT}s"
log "Results directory: ${RESULTS_DIR}"

stty -F "$PORT" "$BAUD" cs8 -cstopb -parenb -ixon -ixoff -crtscts -icanon -echo
cat "$PORT" | while IFS= read -r line; do
  printf '%s serial_rx %s\n' "$(now_ts)" "$line" >> "$SERIAL_LOG"
done &
CAPTURE_PID=$!

overall_status=0
command_index=0
while IFS=$'\t' read -r command expected timeout || [[ -n "${command:-}" ]]; do
  if [[ -z "${command// }" ]] || [[ "${command:0:1}" == "#" ]]; then
    continue
  fi
  command="${command//$'\r'/}"
  expected="${expected//$'\r'/}"
  command="$(decode_escapes "${command}")"
  expected="$(decode_escapes "${expected:-}")"
  timeout="${timeout:-$COMMAND_TIMEOUT}"

  if [[ -z "$expected" ]]; then
    log "Skipping '${command}' because no expected substring is defined."
    continue
  fi

  command_index=$((command_index + 1))
  snapshot_lines="$(wc -l < "$SERIAL_LOG")"
  printf '%s\t%s\t%s\t%s\n' "$command_index" "$command" "$expected" "$timeout" >> "$COMMAND_RESULTS"
  log "[$command_index] send: $command"
  printf '%s\r\n' "$command" > "$PORT"

  if wait_for_match "$expected" "$timeout" "$snapshot_lines"; then
    log "[$command_index] PASS expected substring found: $expected"
    printf 'pass\t%s\t%s\n' "$command" "$expected" >> "$SUMMARY_TSV"
  else
    log "[$command_index] FAIL expected substring not seen in ${timeout}s: $expected"
    printf 'fail\t%s\t%s\n' "$command" "$expected" >> "$SUMMARY_TSV"
    overall_status=1
  fi

  sleep "$COMMAND_SETTLE_SECONDS"
done < "$COMMANDS_FILE"

if [[ "$overall_status" -eq 0 ]]; then
  echo "result=PASS" >> "$SUMMARY_TSV"
  log "Smoke result: PASS"
else
  echo "result=FAIL" >> "$SUMMARY_TSV"
  log "Smoke result: FAIL"
fi

exit "$overall_status"
