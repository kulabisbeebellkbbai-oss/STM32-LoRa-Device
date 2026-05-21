#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="${HOST_UNIT_TEST_ROOT:-$(cd "${SCRIPT_DIR}/.." && pwd)}"

CXX_BIN="${CXX_BIN:-g++}"
BUILD_DIR="${HOST_UNIT_TEST_BUILD_DIR:-${PROJECT_ROOT}/tests/build}"
TEST_CASES_TSV="${USB_COMMAND_CASES_TSV:-${PROJECT_ROOT}/tests/usb-command-cases.tsv}"
TEST_BINARY="${HOST_UNIT_TEST_BINARY:-${BUILD_DIR}/host-unit-tests}"

mkdir -p "$BUILD_DIR"

if ! command -v "$CXX_BIN" >/dev/null 2>&1; then
  echo "Compiler not found: $CXX_BIN"
  exit 1
fi

if [ ! -f "$TEST_CASES_TSV" ]; then
  echo "USB command expectation file not found: $TEST_CASES_TSV"
  exit 1
fi

SOURCE_FILES=(
  "${PROJECT_ROOT}/tests/host_unit_tests.cpp"
  "${PROJECT_ROOT}/src/node_identity.cpp"
  "${PROJECT_ROOT}/src/native_packet.cpp"
  "${PROJECT_ROOT}/src/telemetry_packet_encoder.cpp"
  "${PROJECT_ROOT}/src/message_store.cpp"
  "${PROJECT_ROOT}/src/safety_audit_log.cpp"
  "${PROJECT_ROOT}/src/radio_receive_inbox.cpp"
  "${PROJECT_ROOT}/src/radio_receive_classifier.cpp"
  "${PROJECT_ROOT}/src/radio_tx_gate.cpp"
  "${PROJECT_ROOT}/src/usb_command_parser.cpp"
  "${PROJECT_ROOT}/src/config_service.cpp"
  "${PROJECT_ROOT}/src/app_modes.cpp"
  "${PROJECT_ROOT}/src/app_profile_registry.cpp"
)

echo "Building host unit tests..."
"$CXX_BIN" \
  -std=c++17 \
  -Wall -Wextra -Wpedantic \
  -I"${PROJECT_ROOT}/tests/stubs" \
  -I"${PROJECT_ROOT}/include" \
  -I"${PROJECT_ROOT}/src" \
  "${SOURCE_FILES[@]}" \
  -o "$TEST_BINARY"

echo "Running host unit tests..."
"$TEST_BINARY" "$TEST_CASES_TSV"

echo "Host unit tests complete."
