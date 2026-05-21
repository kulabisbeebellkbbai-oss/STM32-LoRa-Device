#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="${RENODE_PROJECT_ROOT:-$(cd "${SCRIPT_DIR}/.." && pwd)}"
PIO_BIN="${PIO_BIN:-$(command -v pio 2>/dev/null || true)}"
RENODE_TEST_BIN="${RENODE_TEST_BIN:-$(command -v renode-test 2>/dev/null || true)}"
RENODE_BIN_DIR="${RENODE_BIN_DIR:-}"
RENODE_CSS="${RENODE_CSS:-}"
RENODE_BIN_NAME="${RENODE_BIN_NAME:-renode}"

export RENODE_PROJECT_NAME="${RENODE_PROJECT_NAME:-lora_device}"
RENODE_PROJECT_SLUG="${RENODE_PROJECT_SLUG:-${RENODE_PROJECT_NAME//[^A-Za-z0-9_.-]/_}}"
export RENODE_PROJECT_SLUG
export PLATFORMIO_BUILD_DIR="${PLATFORMIO_BUILD_DIR:-/tmp/pio_${RENODE_PROJECT_SLUG}_build}"
export PLATFORMIO_RENODE_ENV="${PLATFORMIO_RENODE_ENV:-stm32wb55_renode}"
export RENODE_REPL_SOURCE="${RENODE_REPL_SOURCE:-${PROJECT_ROOT}/renode/stm32wb55cg_stub.repl}"
export RENODE_REPL_DIR="${RENODE_REPL_DIR:-/tmp/${RENODE_PROJECT_SLUG}_renode}"
export RENODE_REPL_COPY_MODE="${RENODE_REPL_COPY_MODE:-dir}"
export RENODE_ROBOT_FILE="${RENODE_ROBOT_FILE:-${PROJECT_ROOT}/renode/lora_device_boot.robot}"
export RENODE_WORKDIR="${RENODE_WORKDIR:-${PROJECT_ROOT}}"
export RENODE_ELF_VARIABLE_NAME="${RENODE_ELF_VARIABLE_NAME:-RENODE_ELF}"
export RENODE_PLATFORM_VARIABLE_NAME="${RENODE_PLATFORM_VARIABLE_NAME:-RENODE_PLATFORM}"

resolve_abs_path() {
  local path=$1
  local base=${2:-${PROJECT_ROOT}}
  if [[ "$path" = /* ]]; then
    echo "$path"
  else
    echo "${base}/${path}"
  fi
}

set_env_value() {
  local var_name=$1
  local var_value=$2
  if [[ ! "$var_name" =~ ^[A-Za-z_][A-Za-z0-9_]*$ ]]; then
    echo "Invalid environment variable name: $var_name"
    exit 1
  fi
  export "$var_name=$var_value"
}

if [[ -z "${RENODE_REPL_DIR}" ]]; then
  echo "RENODE_REPL_DIR cannot be empty."
  exit 1
fi

if [[ -z "$PIO_BIN" ]] || [[ ! -x "$PIO_BIN" ]]; then
  echo "PlatformIO not found. Install PlatformIO or set PIO_BIN to a valid CLI path."
  exit 1
fi

if [[ -z "$RENODE_TEST_BIN" ]] || [[ ! -x "$RENODE_TEST_BIN" ]]; then
  echo "renode-test not found. Install Renode or set RENODE_TEST_BIN to a valid path."
  exit 1
fi

if [[ -z "$RENODE_BIN_DIR" ]]; then
  RENODE_TEST_REALPATH="$(readlink -f "$RENODE_TEST_BIN" 2>/dev/null || echo "$RENODE_TEST_BIN")"
  RENODE_BIN_DIR="$(cd "$(dirname "$RENODE_TEST_REALPATH")" && pwd)"
fi

if [[ -z "$RENODE_CSS" ]] && [[ -f "${RENODE_BIN_DIR}/tests/robot.css" ]]; then
  RENODE_CSS="${RENODE_BIN_DIR}/tests/robot.css"
fi

RENODE_ARGS=()
if [[ -n "$RENODE_CSS" ]]; then
  if [[ ! -f "$RENODE_CSS" ]]; then
    echo "Renode Robot CSS file not found at $RENODE_CSS"
    echo "Set RENODE_CSS to match the Renode installation, or leave it unset."
    exit 1
  fi
  RENODE_ARGS+=(--css-file "$RENODE_CSS")
fi

if [[ -n "$RENODE_BIN_DIR" ]]; then
  RENODE_ARGS+=(--robot-framework-remote-server-full-directory "$RENODE_BIN_DIR")
fi

RENODE_REPL_SOURCE="$(resolve_abs_path "$RENODE_REPL_SOURCE" "$PROJECT_ROOT")"
RENODE_ROBOT_FILE="$(resolve_abs_path "$RENODE_ROBOT_FILE" "$PROJECT_ROOT")"

if [[ ! -f "$RENODE_ROBOT_FILE" ]]; then
  echo "Robot test file not found at $RENODE_ROBOT_FILE"
  exit 1
fi

if [[ ! -f "$RENODE_REPL_SOURCE" ]]; then
  echo "Renode board platform file not found at $RENODE_REPL_SOURCE"
  exit 1
fi

case "$RENODE_REPL_COPY_MODE" in
  dir)
    mkdir -p "$RENODE_REPL_DIR"
    RENODE_REPL_SOURCE_DIR="$(cd "$(dirname "$RENODE_REPL_SOURCE")" && pwd)"
    RENODE_REPL_COPY_DIR="${RENODE_REPL_DIR}/$(basename "$RENODE_REPL_SOURCE_DIR")"
    rm -rf "$RENODE_REPL_COPY_DIR"
    cp -a "$RENODE_REPL_SOURCE_DIR" "$RENODE_REPL_DIR/"
    RENODE_REPL_FILE="${RENODE_REPL_COPY_DIR}/$(basename "$RENODE_REPL_SOURCE")"
    ;;
  file)
    mkdir -p "$RENODE_REPL_DIR"
    RENODE_REPL_FILE="${RENODE_REPL_DIR}/$(basename "$RENODE_REPL_SOURCE")"
    cp "$RENODE_REPL_SOURCE" "$RENODE_REPL_FILE"
    ;;
  none)
    RENODE_REPL_FILE="$RENODE_REPL_SOURCE"
    ;;
  *)
    echo "RENODE_REPL_COPY_MODE must be one of: dir, file, none."
    exit 1
    ;;
esac

RENODE_ELF_PATH="${PLATFORMIO_BUILD_DIR}/${PLATFORMIO_RENODE_ENV}/firmware.elf"

echo "Building Renode simulation firmware with PlatformIO env: ${PLATFORMIO_RENODE_ENV}"
cd "$RENODE_WORKDIR"
"$PIO_BIN" run --project-dir "$PROJECT_ROOT" -e "$PLATFORMIO_RENODE_ENV"

if [[ ! -f "$RENODE_ELF_PATH" ]]; then
  echo "Renode ELF was not produced at $RENODE_ELF_PATH"
  exit 1
fi

set_env_value "$RENODE_ELF_VARIABLE_NAME" "$RENODE_ELF_PATH"
set_env_value "$RENODE_PLATFORM_VARIABLE_NAME" "$RENODE_REPL_FILE"
export RENODE_ELF="$RENODE_ELF_PATH"
export RENODE_PLATFORM="$RENODE_REPL_FILE"
export LORA_RENODE_ELF="$RENODE_ELF_PATH"
export LORA_RENODE_PLATFORM="$RENODE_REPL_FILE"

echo "Running Renode UART smoke test with ${RENODE_ELF_VARIABLE_NAME}=${!RENODE_ELF_VARIABLE_NAME}"
"$RENODE_TEST_BIN" \
  "${RENODE_ARGS[@]}" \
  --robot-framework-remote-server-name "$RENODE_BIN_NAME" \
  --robot-framework-remote-server-port 0 \
  --runner portable \
  --stop-on-error \
  "$RENODE_ROBOT_FILE"
