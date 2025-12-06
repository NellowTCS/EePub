#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR=${BUILD_DIR:-"$ROOT_DIR/external/lexbor/build"}

if [[ -z "${BUILD_JOBS:-}" ]]; then
	if command -v nproc >/dev/null 2>&1; then
		BUILD_JOBS=$(nproc)
	else
		BUILD_JOBS=$(sysctl -n hw.ncpu 2>/dev/null || echo 4)
	fi
fi

mkdir -p "$BUILD_DIR"

CONFIGURE_CMD=(cmake)
if [[ "${TOOLCHAIN:-native}" == "emscripten" ]]; then
	CONFIGURE_CMD=(emcmake cmake)
fi

"${CONFIGURE_CMD[@]}" -S "$ROOT_DIR/external/lexbor" -B "$BUILD_DIR" \
	-DCMAKE_BUILD_TYPE=${BUILD_TYPE:-Release} -DBUILD_SHARED_LIBS=OFF

cmake --build "$BUILD_DIR" --config ${BUILD_TYPE:-Release} -j"$BUILD_JOBS"
