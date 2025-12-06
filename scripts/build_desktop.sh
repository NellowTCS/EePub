#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_TYPE=${BUILD_TYPE:-Release}
BUILD_DIR=${BUILD_DIR:-"$ROOT_DIR/build"}

if [[ -z "${BUILD_JOBS:-}" ]]; then
	if command -v nproc >/dev/null 2>&1; then
		BUILD_JOBS=$(nproc)
	else
		BUILD_JOBS=$(sysctl -n hw.ncpu 2>/dev/null || echo 4)
	fi
fi

"$ROOT_DIR/scripts/build_lexbor.sh"

cmake -S "$ROOT_DIR" -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE="$BUILD_TYPE"
cmake --build "$BUILD_DIR" --target EePubDesktop --config "$BUILD_TYPE" -j"$BUILD_JOBS"
