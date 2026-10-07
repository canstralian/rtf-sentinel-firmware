#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${ROOT}/.ci-build/sentinel"
mkdir -p "${BUILD_DIR}"

COMMON=(
  -std=c++17
  -I"${ROOT}/include"
  -Wall
  -Wextra
  -Wpedantic
  -Werror
  -Wconversion
  -Wsign-conversion
  "${ROOT}/src/sentinel/governance.cpp"
  "${ROOT}/src/sentinel/bounded_job.cpp"
  "${ROOT}/tests/sentinel/test_sentinel.cpp"
)

g++ "${COMMON[@]}" -O2 -o "${BUILD_DIR}/sentinel-tests"
"${BUILD_DIR}/sentinel-tests"

g++ "${COMMON[@]}" -O1 -g -fno-omit-frame-pointer -fsanitize=address,undefined \
  -o "${BUILD_DIR}/sentinel-tests-sanitized"
ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 \
UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1 \
  "${BUILD_DIR}/sentinel-tests-sanitized"
