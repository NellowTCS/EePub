# EePub
Portable EPUB 3 renderer targeting embedded microcontrollers and desktop platforms alike. The core pipeline handles EPUB container parsing, DOM + CSS preparation, chapter pagination, and renders to both headless/CLI surfaces and future framebuffer/GUI backends.

## Current status (Phase 2 snapshot)
- Deterministic EPUB loader that walks `container.xml`, `package.opf`, manifest, and spine order.
- Memory watchdog surface accessible from every build (used to gate embedded targets later).
- Layout engine that converts XHTML into formatted line runs (headings, lists, paragraph indents) and feeds the CLI fallback renderer.
- Headless-friendly desktop CLI with interactive navigation plus a smoke-test flag for CI.
- Build scripts for Lexbor, desktop, and WASM targets, backed by GitHub Actions coverage.
- Every target (desktop, WASM, ESP32, etc.) links against the full Lexbor HTML parser—no simplified fallbacks—so DOM traversal stays consistent.

## Building

### 1. Build Lexbor
```shell
scripts/build_lexbor.sh
```

### 2. Build EePubDesktop
```shell
scripts/build_desktop.sh    # defaults to Release build under ./build
```

`scripts/build_desktop.sh` configures CMake, rebuilds EePubDesktop, and leaves the binary at `build/EePubDesktop`.

### Optional: build the WASM bindings

```shell
scripts/build_wasm.sh    # produces build-wasm/EePubWasm.js + .wasm (emsdk required)
```

The script assumes `emsdk` is initialized in your shell so `emcmake` is available. It builds Lexbor for Emscripten under `external/lexbor/build-wasm` and then emits a modularized EePub runtime (`EePubModule`) in `build-wasm/`.

### 3. Build the ESP32 Cheap Yellow Display firmware

```shell
scripts/build_esp32_cyd.sh      # runs PlatformIO in platforms/esp32-cyd
```

The helper installs PlatformIO automatically (via `pip`) if `pio` is missing, then calls `pio run` inside `platforms/esp32-cyd`. Pass any extra flags after `--` and ensure an EPUB lives on the SD card when flashing the resulting firmware.

## CLI usage

```shell
./build/EePubDesktop <path/to/book.epub> [--headless] [--render-all] [--chapter N] [--mem-soft-limit BYTES]
```

- `--headless`: Skip the interactive prompt, render the current chapter (or all chapters with `--render-all`) and exit. Used by CI.
- `--chapter N`: Jump to a chapter before rendering/interaction.
- `--mem-soft-limit BYTES`: Configure the memory watchdog. When the decoded text buffer crosses this threshold, the monitor notifies any registered callbacks (desktop build currently prints stats when requested).

Interactive commands once the prompt is active:

| Command | Description |
| --- | --- |
| `list` | Print ordered chapter titles as parsed from the EPUB spine. |
| `next` / `prev` | Step through chapters. |
| `goto <n>` | Jump directly to a chapter index. |
| `render` | Reflow the current chapter using the shared layout engine and display a terminal-friendly preview. |
| `stats` | Show current vs peak memory tracked by the watchdog. |
| `quit` | Exit the CLI. |

## Sample EPUBs

Grab a few public-domain fixtures for testing:

```shell
scripts/fetch_samples.sh
```

Downloads Standard Ebooks' “Alice’s Adventures in Wonderland” into `examples/` (curl or wget required).

## Continuous Integration

`.github/workflows/ci.yml` builds Lexbor, configures EePubDesktop with Ninja, runs the CLI in headless mode against the bundled `Accessible EPUB 3 _ Matt Garrish.epub`, and exercises the WASM toolchain (emsdk) via `scripts/build_wasm.sh`. Extend the workflow with more smoke/regression tests as Phase 2 grows.

## ESP32 Cheap Yellow Display example

The `platforms/esp32-cyd/` folder contains a PlatformIO project that targets the ESP32-2432S028R (Cheap Yellow Display) with the full EePub core. Highlights:

- Custom board definition (`boards/esp32-2432S028Rv3.json`) captures the exact Sunton pinout, SPI buses, SD card lines, RGB LED, and backlight GPIO assignments the user provided.
- The sketch mounts the microSD card via `esp_vfs_fat_sdspi_mount`, searches for the first `.epub` under `/sdcard`, and renders it using the shared layout engine. Tap the BOOT button (GPIO0) to advance chapters.
- Text is drawn with the Adafruit ST7789 driver; EePub's renderer streams wrapped lines to a lightweight `DisplaySink` that clears/redraws the 240×320 panel each frame. Backlight control, heap telemetry, and runtime logs are handled through the new HAL layer.
- Lexbor itself is compiled as an ESP-IDF component inside this project, so the embedded build parses XHTML with the exact same engine as desktop/WASM.

Steps:
1. Copy an EPUB file (for example `Accessible EPUB 3 _ Matt Garrish.epub`) onto a FAT-formatted microSD card and insert it into the CYD slot.
2. From `platforms/esp32-cyd`, run `pio run -t upload` (requires PlatformIO Core) to flash the sketch.
3. Open a serial monitor at 115200 bps to watch EePub logs; press the BOOT button to cycle through chapters.

## Roadmap

1. **Layout & Rendering Core** – CSS subset, pagination, FreeType/Harfbuzz path plus bitmap fallback for MCUs.
2. **Platform HAL & Embedded Targets** – ESP32, STM32, RP2040, Linux/macOS with memory watchdog callbacks wired to platform telemetry.
3. **Desktop GUI + Tooling** – SDL/Qt-based viewer, TOC navigation, theming, developer diagnostics.
4. **Extended Features** – Images, RTL text, annotations, JS bindings (Elk), and comprehensive acceptance tests.

Track progress via issues/PRs; each phase lands behind CI so every target keeps a reproducible baseline.