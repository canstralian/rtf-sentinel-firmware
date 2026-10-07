#!/usr/bin/env bash
set -euo pipefail

if [[ $# -ne 1 || -z "${1}" ]]; then
    echo "usage: $0 PLATFORMIO_ENV" >&2
    exit 2
fi

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
ENV_NAME="$1"
LOG_DIR="${ROOT}/.ci-build/platformio"
mkdir -p "${LOG_DIR}"
cd "${ROOT}"

export PLATFORMIO_SETTING_ENABLE_TELEMETRY=No

platformio --version
printf 'Building PlatformIO environment: %s\n' "${ENV_NAME}"
set -o pipefail
platformio run -e "${ENV_NAME}" 2>&1 | tee "${LOG_DIR}/${ENV_NAME}.log"
