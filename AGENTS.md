# AGENTS.md

Guidance for AI coding agents working in this repository.

## Project overview

PicoVectorscope is a C++17 framework for the Raspberry Pi Pico (RP2040) that
drives a pair of 12-bit parallel DACs (AD767JN) connected to an XY
oscilloscope. It renders vector graphics, points, and limited raster graphics
at high refresh rates. It is a work-in-progress.

This repo is a **framework, not a standalone application**. There is no
top-level CMakeLists.txt here; it is consumed by application projects
(e.g. PicoVectorscopeExamples) which include the `pico_sdk` and the two `.cmake`
include files. Do not add a build system, `main()`, or tests to this repo.

## Repository layout

```
include/        Public API headers. picovectorscope.h is the master include.
src/            Framework implementation (paired with the private headers in
                src/: dacout.h, dacoutputsm.h, serial.h, dacoutpioconfig.h).
                src/*.pio are PIO programs, compiled by pico_generate_pio_header.
extras/
    include/extras/  Optional add-ons: bitmap, bitmapfont (font8x8 submodule
                      in extras/src/font8x8), tilemap, shapes3d (camera).
    extras.cmake
vectorscope.cmake   CMake include file: PIO generation, sources, link libs.
STYLE.md      Code style guide (authoritative — see below).
LICENSE.txt   GPL-3.0+.
```

## How the framework works

- **Demo registration**: an application defines a class deriving from `Demo`
  (`include/demo.h`) and instantiates it; the constructor self-registers.
  `Init()` → `Start()` → `UpdateAndRender(DisplayList&, float dt)` per frame →
  `End()`. Multiple demos cycle via holding Left+Right and pressing Fire
  (`include/buttons.h`, hard-coded GPIOs 19-22, active-low with pull-ups).
- **Rendering**: `Demo::UpdateAndRender` draws into a `DisplayList`
  (`PushVector`, `PushPoint`, `PushRasterDisplay`; see `include/displaylist.h`).
  Coordinates are fixed-point (`DisplayListScalar`, 1.14) in a normalized
  space. Intensity 0 = cursor move.
- **Output pipeline**: `main()` (`src/main.cpp`) double-buffers display lists
  protected by a mutex. On core 1 (`DAC_OUTPUT_CORE`), one frame is
  `OutputToDACs()` into circular entry buffers (`src/dacout.h`) which are sent
  over chained DMA to 4 PIO programs (`idle`, `vector`, `points`, `raster` in
  `src/*.pio`, switched per data type) driving GP2-GP13 (12 DAC bits) with
  X/Y/Z latch pins on GP16-GP18 (`src/dacoutpioconfig.h`).
- **Concurrency**: main thread updates/draws; core 1 does DAC output. Shared
  state is word-sized `volatile` values (atomic on Cortex-M0+). Keep it that way.

## Conventions (summary)

Full detail is in **STYLE.md — read it before making changes. It takes
precedence over `.clang-format`; keep the two consistent.** Highlights:

- GPL-3.0+ license header required at the top of every source/header file
  (template in STYLE.md), first line is a one-sentence file description.
- Allman braces, 4-space indent, 100-col limit, left-point binding
  (`type* p`), `#pragma once` (no include guards), no exceptions (use
  `assert`/`panic`), no Doxygen.
- Naming: `m_camelCase` members, `s_camelCase` file-scope statics,
  `kCamelCase` constants, `PascalCase` functions/types, `p` pointer prefix,
  `e` enum-member prefix.
- Logging via `LogChannel` + `LOG_INFO/WARN/ERROR` macros (`include/log.h`);
  channels default to *disabled*.
- No per-frame heap allocation; buffer sizes are fixed.
- Fixed-point: instantiate `FixedPoint<W,F,T,IT,sign>` once, `typedef` to a
  descriptive name; never put raw `FixedPoint<...>` in public signatures.

## Verification

There is no test suite and no CI in this repo; code targets RP2040 firmware.
Verify changes by reasoning about the pipeline above, checking that any new
`src/*.cpp` file is listed in `vectorscope.cmake` (and new `extras` sources in
`extras/extras.cmake`), and that new public headers are declared with
minimum-SDK includes. When in doubt about formatting, follow STYLE.md exactly.
