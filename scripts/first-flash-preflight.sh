#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="${FIRST_FLASH_PROJECT_ROOT:-$(cd "${SCRIPT_DIR}/.." && pwd)}"
HOST_BUILD_SMOKE_SCRIPT="${HOST_BUILD_SMOKE_SCRIPT:-${SCRIPT_DIR}/host-build-smoke.sh}"
RENODE_SMOKE_SCRIPT="${RENODE_SMOKE_SCRIPT:-${SCRIPT_DIR}/renode-smoke.sh}"

PIO_BIN="${PIO_BIN:-$(command -v pio 2>/dev/null || true)}"
HOST_BUILD_ENVS="${LORA_BUILD_ENVS:-stm32wb55_bringup stm32wb55_hardware_gates}"
KEY_DOCS="${KEY_DOCS:-docs/flashing-prep.md docs/hardware-baseline.md docs/pin-plan.md docs/wiring-diagrams.md docs/testing-plan.md docs/renode-simulation.md}"

failures=0
optional_skips=""
pio_available=0

add_skip() {
  if [ -z "$optional_skips" ]; then
    optional_skips="$1"
  else
    optional_skips="${optional_skips}; $1"
  fi
}

log() {
  printf '%s\n' "$1"
}

run_host_build_smoke() {
  if [ ! -x "${HOST_BUILD_SMOKE_SCRIPT}" ]; then
    log "SKIP: host build smoke script not found or not executable: ${HOST_BUILD_SMOKE_SCRIPT}"
    add_skip "host-build-smoke.sh"
    return
  fi

  if [ "$pio_available" -eq 0 ]; then
    log "SKIP: host build smoke requires PlatformIO; check failed in preflight."
    add_skip "host-build-smoke (PlatformIO unavailable)"
    return
  fi

  log "Running host build smoke checks..."
  if (cd "$PROJECT_ROOT" && PIO_BIN="$PIO_BIN" LORA_BUILD_ENVS="$HOST_BUILD_ENVS" "$HOST_BUILD_SMOKE_SCRIPT"); then
    log "PASS: host build smoke completed."
  else
    log "FAIL: host build smoke failed."
    failures=$((failures + 1))
  fi
}

run_renode_smoke() {
  if [ ! -x "${RENODE_SMOKE_SCRIPT}" ]; then
    log "SKIP: Renode smoke script not found or not executable: ${RENODE_SMOKE_SCRIPT}"
    add_skip "renode-smoke.sh"
    return
  fi

  if [ "$pio_available" -eq 0 ]; then
    log "SKIP: Renode smoke requires PlatformIO; check failed in preflight."
    add_skip "renode-smoke (PlatformIO unavailable)"
    return
  fi

  if command -v renode-test >/dev/null 2>&1; then
    export RENODE_TEST_BIN="${RENODE_TEST_BIN:-$(command -v renode-test)}"
  fi

  if [ -z "${RENODE_TEST_BIN:-}" ] || [ ! -x "${RENODE_TEST_BIN}" ]; then
    log "SKIP: renode-test not available; install/enable Renode to run this check."
    add_skip "renode-test"
    return
  fi

  if (cd "$PROJECT_ROOT" && RENODE_TEST_BIN="$RENODE_TEST_BIN" "$RENODE_SMOKE_SCRIPT"); then
    log "PASS: Renode smoke completed."
  else
    log "FAIL: Renode smoke failed."
    failures=$((failures + 1))
  fi
}

check_docs() {
  log "Verifying required documentation files..."
  for doc in ${KEY_DOCS}; do
    if [ -f "${PROJECT_ROOT}/${doc}" ]; then
      log "PASS: ${doc}"
    else
      log "FAIL: missing ${doc}"
      failures=$((failures + 1))
    fi
  done
}

check_pio() {
  log "Checking PlatformIO availability..."
  if [ -n "${PIO_BIN}" ] && [ -x "${PIO_BIN}" ]; then
    log "PASS: PlatformIO found at ${PIO_BIN}"
    "${PIO_BIN}" --version
    pio_available=1
    return
  fi

  if command -v pio >/dev/null 2>&1; then
    PIO_BIN="$(command -v pio)"
    log "PASS: PlatformIO found in PATH at ${PIO_BIN}"
    "${PIO_BIN}" --version
    pio_available=1
    return
  fi

  log "FAIL: PlatformIO CLI not found. Set PIO_BIN or install PlatformIO."
  failures=$((failures + 1))
}

print_next_steps() {
  printf '\n'
  log "Next hardware-required commands (run after flash-prep checks):"
  log "1) st-info --probe (or equivalent ST-Link probe check)"
  log "2) STM32_Programmer_CLI -l (if installed)"
  log "3) pio run -t upload"
  log "4) pio device monitor"
  log "5) For hardware-gated stages: pio run -e stm32wb55_hardware_gates -t upload"
  log "6) Continue with docs/physical-bringup-runbook.md before first hardware flash."
}

main() {
  log "First flash preflight (no hardware, no upload):"
  cd "$PROJECT_ROOT"
  check_pio
  check_docs
  run_host_build_smoke
  run_renode_smoke
  print_next_steps

  log ""
  log "Skipped checks: ${optional_skips:-none}"
  if [ "$failures" -eq 0 ]; then
    log "Preflight result: PASS"
    return 0
  fi

  log "Preflight result: FAIL (${failures} failure(s))"
  return 1
}

main "$@"
