#!/usr/bin/env bash
set -euo pipefail

if ! command -v emcmake >/dev/null 2>&1; then
  echo "emcmake not found. Please activate emsdk before running this script." >&2
  exit 1
fi

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_TYPE=${BUILD_TYPE:-Release}
BUILD_DIR=${BUILD_DIR:-"$ROOT_DIR/build-wasm"}
LEXBOR_BUILD_DIR=${LEXBOR_BUILD_DIR:-"$ROOT_DIR/external/lexbor/build-wasm"}

if [[ -z "${BUILD_JOBS:-}" ]]; then
  if command -v nproc >/dev/null 2>&1; then
    BUILD_JOBS=$(nproc)
  else
    BUILD_JOBS=$(sysctl -n hw.ncpu 2>/dev/null || echo 4)
  fi
fi

TOOLCHAIN=emscripten BUILD_DIR="$LEXBOR_BUILD_DIR" BUILD_JOBS="$BUILD_JOBS" \
  "$ROOT_DIR/scripts/build_lexbor.sh"

LEXBOR_LIB_PATH="$LEXBOR_BUILD_DIR/liblexbor_static.a"
if [[ ! -f "$LEXBOR_LIB_PATH" ]]; then
  echo "Lexbor static library not found at $LEXBOR_LIB_PATH" >&2
  exit 1
fi

emcmake cmake -S "$ROOT_DIR" -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
  -DEEPUB_ENABLE_WASM=ON -DLEXBOR_LIB_PATH="$LEXBOR_LIB_PATH"

cmake --build "$BUILD_DIR" --target EePubWasm --config "$BUILD_TYPE" -j"$BUILD_JOBS"

cp "$ROOT_DIR/frameworks/Wasm/index.html" "$BUILD_DIR/index.html"
echo "WASM artifacts available in $BUILD_DIR (open index.html via a static file server)."
