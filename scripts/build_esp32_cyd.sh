#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

if ! command -v pio >/dev/null 2>&1; then
    echo "[EePub] PlatformIO CLI not found. Installing via pip..." >&2
    if command -v python3 >/dev/null 2>&1; then
        python3 -m pip install --user --upgrade platformio
    else
        python -m pip install --user --upgrade platformio
    fi
    export PATH="$HOME/.local/bin:$PATH"
fi

pushd "$ROOT_DIR/platforms/esp32-cyd" >/dev/null
pio pkg install
pio run "$@"
popd >/dev/null
