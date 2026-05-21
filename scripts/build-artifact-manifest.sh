#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="${BUILD_MANIFEST_PROJECT_ROOT:-$(cd "${SCRIPT_DIR}/.." && pwd)}"
PIO_BIN="${PIO_BIN:-$(command -v pio 2>/dev/null || true)}"
PLATFORMIO_BUILD_DIR="${PLATFORMIO_BUILD_DIR:-${PROJECT_ROOT}/.pio/build}"
LORA_BUILD_ENVS="${LORA_BUILD_ENVS:-stm32wb55_bringup stm32wb55_hardware_gates}"
ARTIFACT_DIR="${BUILD_MANIFEST_DIR:-${PROJECT_ROOT}/artifacts}"
OUTPUT_JSON="${BUILD_MANIFEST_OUTPUT:-${ARTIFACT_DIR}/build-artifact-manifest.json}"
RUN_BUILD=1
DRY_RUN=0
export PLATFORMIO_BUILD_DIR

read -r -a BUILD_ENVS <<< "${LORA_BUILD_ENVS}"

usage() {
  cat <<'EOF'
Usage: build-artifact-manifest.sh [options]

Collects build artifact metadata without flashing and writes build-artifact-manifest.json.

Options:
  --build            Build all target environments before collection (default)
  --no-build         Skip builds and only collect existing artifacts
  --dry-run          Alias for --no-build
  --output PATH      Override output JSON path
  --help             Show this help text
  --envs "a b"       Override environments (space-separated list in one argument)
EOF
}

while (($# > 0)); do
  case "$1" in
    --build)
      RUN_BUILD=1
      DRY_RUN=0
      shift
      ;;
    --no-build|--dry-run)
      RUN_BUILD=0
      DRY_RUN=1
      shift
      ;;
    --output)
      if (($# < 2)); then
        echo "Missing argument to --output"
        exit 1
      fi
      OUTPUT_JSON="$2"
      shift 2
      ;;
    --envs)
      if (($# < 2)); then
        echo "Missing argument to --envs"
        exit 1
      fi
      read -r -a BUILD_ENVS <<< "$2"
      shift 2
      ;;
    --help)
      usage
      exit 0
      ;;
    *)
      echo "Unknown option: $1"
      usage
      exit 1
      ;;
  esac
done

if [ ${#BUILD_ENVS[@]} -eq 0 ]; then
  echo "No build environments configured."
  exit 1
fi

if [ -z "$PIO_BIN" ] || [ ! -x "$PIO_BIN" ]; then
  echo "PlatformIO not found. Install PlatformIO or set PIO_BIN to a valid CLI path."
  exit 1
fi

if ! command -v git >/dev/null 2>&1; then
  echo "git is required to record branch/commit metadata."
  exit 1
fi

timestamp_utc="$(date -u +'%Y-%m-%dT%H:%M:%SZ')"
pio_version="$( "$PIO_BIN" --version 2>&1 | head -n 1 )"
repo_branch="$(git -C "$PROJECT_ROOT" rev-parse --abbrev-ref HEAD 2>/dev/null || echo 'unknown')"
repo_commit="$(git -C "$PROJECT_ROOT" rev-parse HEAD 2>/dev/null || echo 'unknown')"
repo_dirty="false"
if ! git -C "$PROJECT_ROOT" diff --quiet; then
  repo_dirty="true"
fi

if [ -z "$repo_commit" ] || [ "$repo_commit" = "unknown" ]; then
  repo_dirty="true"
fi

json_escape() {
  local value="${1:-}"
  value="${value//\\/\\\\}"
  value="${value//\"/\\\"}"
  value="${value//$'\n'/\\n}"
  value="${value//$'\r'/\\r}"
  value="${value//$'\t'/\\t}"
  printf '%s' "$value"
}

artifact_bytes() {
  local artifact=$1
  if [ -f "$artifact" ]; then
    stat -c "%s" "$artifact"
  else
    echo -1
  fi
}

artifact_sha256() {
  local artifact=$1
  if [ -f "$artifact" ] && command -v sha256sum >/dev/null 2>&1; then
    sha256sum "$artifact" | awk '{print $1}'
  else
    echo ""
  fi
}

mkdir -p "$ARTIFACT_DIR"
tmp_manifest="$(mktemp)"
trap 'rm -f "$tmp_manifest"' EXIT
append_json() {
  printf '%s\n' "$1" >> "$tmp_manifest"
}

append_json "{"
append_json "  \"generated_utc\": \"$(json_escape "$timestamp_utc")\","
append_json "  \"dry_run\": $((DRY_RUN)),"
append_json "  \"project_path\": \"$(json_escape "$PROJECT_ROOT")\","
append_json "  \"platformio\": {"
append_json "    \"cli\": \"$(json_escape "$PIO_BIN")\","
append_json "    \"version\": \"$(json_escape "$pio_version")\","
append_json "    \"build_dir\": \"$(json_escape "$PLATFORMIO_BUILD_DIR")\""
append_json "  },"
append_json "  \"git\": {"
append_json "    \"branch\": \"$(json_escape "$repo_branch")\","
append_json "    \"commit\": \"$(json_escape "$repo_commit")\","
append_json "    \"dirty\": $repo_dirty"
append_json "  },"
append_json "  \"environments\": ["

  first_env="true"
  for env_name in "${BUILD_ENVS[@]}"; do
    env_build_start="$(date -u +'%Y-%m-%dT%H:%M:%SZ')"
    env_build_status="skipped"
    if [ "$RUN_BUILD" -eq 1 ]; then
      env_build_status="running"
      if "$PIO_BIN" run -d "$PROJECT_ROOT" -e "$env_name"; then
        env_build_status="built"
      else
        env_build_status="build_failed"
      fi
    fi
    env_build_end="$(date -u +'%Y-%m-%dT%H:%M:%SZ')"

    env_dir="${PLATFORMIO_BUILD_DIR%/}/${env_name}"
    env_elf="${env_dir}/firmware.elf"
    env_bin="${env_dir}/firmware.bin"
    env_elf_size="$(artifact_bytes "$env_elf")"
    env_bin_size="$(artifact_bytes "$env_bin")"
    env_elf_sha="$(artifact_sha256 "$env_elf")"
    env_bin_sha="$(artifact_sha256 "$env_bin")"
    env_status="missing_artifacts"
    if [ "$env_elf_size" -ge 0 ] || [ "$env_bin_size" -ge 0 ]; then
      env_status="artifacts_present"
    fi
    if [ "$env_build_status" = "build_failed" ]; then
      env_status="build_failed"
    fi

    if [ "$first_env" = "true" ]; then
      first_env="false"
    else
      append_json ","
    fi

    append_json "    {"
    append_json "      \"environment\": \"$(json_escape "$env_name")\","
    append_json "      \"build_status\": \"$(json_escape "$env_build_status")\","
    append_json "      \"build_start_utc\": \"$(json_escape "$env_build_start")\","
    append_json "      \"build_end_utc\": \"$(json_escape "$env_build_end")\","
    append_json "      \"status\": \"$(json_escape "$env_status")\","
    append_json "      \"elf\": {"
    append_json "        \"path\": \"$(json_escape "$env_elf")\","
    append_json "        \"size_bytes\": $env_elf_size,"
    append_json "        \"sha256\": \"$(json_escape "$env_elf_sha")\""
    append_json "      },"
    append_json "      \"bin\": {"
    append_json "        \"path\": \"$(json_escape "$env_bin")\","
    append_json "        \"size_bytes\": $env_bin_size,"
    append_json "        \"sha256\": \"$(json_escape "$env_bin_sha")\""
    append_json "      }"
    append_json "    }"
  done

append_json ""
append_json "  ]"
append_json "}"

mv "$tmp_manifest" "$OUTPUT_JSON"
chmod 0644 "$OUTPUT_JSON"

if [ "$DRY_RUN" -eq 1 ]; then
  echo "Manifest generated in dry-run mode: $OUTPUT_JSON"
else
  echo "Manifest generated: $OUTPUT_JSON"
fi
echo "Generated at: $timestamp_utc"
echo "Project: $PROJECT_ROOT"
echo "Branch/commit: $repo_branch / $repo_commit"
