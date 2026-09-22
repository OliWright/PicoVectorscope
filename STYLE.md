# PicoVectorscope Style Guide

This guide describes the conventions used across the PicoVectorscope codebase.
Where a rule and `.clang-format` conflict, the rule here takes precedence and
`.clang-format` should be updated to match.

## Files

- C++17 (framework); `pico_sdk` and C-SDK headers where applicable.
- Headers use `#pragma once` — never include guards (`#ifndef` / `#define`), except
  when the file is consumed by the PIC toolchain.
- Each `.h` has a matching `.cpp` in `src/`. Free helpers and small inline
  templates may live in the header alone.
- Master include for applications is `picovectorscope.h`; it pulls in all public
  types. Individual public headers can be included directly and are fine to
  combine.

## License header

Every source and header file begins with the GPL-3.0+ block comment:

```
// <one-line description of the file>.
//
// Copyright (C) 2022 Oli Wright
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// A copy of the GNU General Public License can be found in the file
// LICENSE.txt in the root of this project.
// If not, see <https://www.gnu.org/licenses/>.
//
// oli.wright.github@gmail.com
```

The first line is a short description of the file's purpose (e.g.
`// Master include for applications using the picovectorscope framework.`).

## Formatting

- Indentation: 4 spaces. No tabs.
- Line limit: 100 columns.
- Braces: Allman (opening brace on its own line), for functions, classes,
  `struct`s, `enum`s, and control statements (`if`, `else`, `for`, `while`,
  `switch`).
- `if` statements with a single-statement body still use braces.
  Multi-line "short" `if`s (one statement spanning multiple lines) are allowed
  on a single line only when the body fits on the same line as the `if` in
  the SpaceTanks project, and are discouraged in PicoVectorscope.
  Prefer braces in PicoVectorscope for consistency.
- Pointer and reference alignment: left (`type* ptr`, not `type *ptr`).
- Initializer lists in constructors: aligned in a single column with the
  first member's opening parenthesis; one member per line.

```cpp
DisplayList::DisplayList(uint32_t maxNumItems)
    : m_pDisplayListVectors((Vector*)malloc(...)),
      m_numDisplayListVectors(1),
      m_maxDisplayListVectors(maxNumItems)
{
}
```

- Aligned declarations: when a block of local variables is declared together,
  pad with spaces so the `=` (or type) column lines up:

```cpp
static DisplayListVector2 s_calibrationScale(0.875f, 0.875f);
static DisplayListVector2 s_calibrationBias(0.0625f, 0.0625f);
```

```cpp
Vector&       vector                   = m_pDisplayListVectors[m_numDisplayListVectors++];
vector.x                               = (x * s_calibrationScale.x) + s_calibrationBias.x;
vector.numSteps = 1;
```

- `switch` cases are indented one level from the `switch` keyword (see
  `displaylist.cpp::OutputToDACs`), and each case with a body is in braces.
- `for` loop declarations: init in parens, condition in parens, increment in
  the last parens. No space inside or after the parens.
- Multiple statements on one line are allowed for very compact,
  tightly-related initializers (e.g. `DisplayListScalar x, y;` for a pair).
  Otherwise one statement per line.

## Includes

- Order: the file's own header (in `.cpp` files), then project-local headers,
  then C library / standard headers (`<cstdint>`, `<cstdlib>` etc).
- Group project-local includes together; there is no strict alphabetical sort
  required, but keep related headers adjacent (e.g. `displaylist.h` immediately
  after `displaylist.cpp`).
- Use `#include "<file>.h"` for project headers, `#include <file.h>` for
  system and pico-SDK headers.
- Prefer `extern "C"` wrappers or forward declarations over pulling in large
  SDK headers in small public headers. Public headers should only include the
  minimum they need (`types.h`, `fixedpoint.h`).

## Naming

| Entity                          | Convention                    | Example                              |
|---------------------------------|-------------------------------|--------------------------------------|
| Types (class/struct/enum)       | `PascalCase`                  | `DisplayList`, `FixedPoint`, `Camera`|
| Type aliases / typedefs         | `PascalCase`                  | `DisplayListScalar`, `BurnLength`    |
| Free functions                  | `PascalCase`                  | `PushShapeToDisplayList`, `FragmentShape` |
| Member functions                | `PascalCase` (public), `camelCase` (private helpers) | `PushVector`, `terminateVectors` |
| Public getters                    | `Get<PascalCase>`              | `GetId()`, `GetPosition()`           |
| Static / file-scope constants   | `kCamelCase`                  | `kPi`, `kBurnFadeLength`, `kMaxSteps` |
| File-scope `static` variables   | `s_camelCase`                 | `s_calibrationScale`, `s_pixelToHold` |
| Class data members              | `m_camelCase`                 | `m_pDisplayListVectors`, `m_intensity` |
| Local variables / parameters    | `camelCase`                   | `displayList`, `numPoints`, `point`  |
| Fixed-point types               | `k` + descriptive name, `typedef` to a descriptive name | `DisplayListScalar` |
| Boolean parameters/variables    | `camelCase` with a descriptive verb/adjective | `closed`, `initialEnabled` |
| Pointers                        | `p` prefix on the name        | `pOutput`, `pPoint`                  |
| Loop counters                   | `i`, `idx` (when unambiguous) | `i`, `scanlineIdx`, `bitIdx`         |
| Enum members                    | `e` prefix + `PascalCase`     | `e1Bit`, `e4BitLinear`, `e8BitGamma` |

### Constants

- Compile-time values: `static constexpr` or `constexpr` with `k...` naming.
  ```cpp
  static constexpr uint kBurnFadeLength = 8;
  constexpr float kPi = 3.14159265358979323846f;
  ```
- Values that could plausibly be reconfigured: `#define` with a `K`-free
  `UPPER_SNAKE_CASE` name, or `static const`.
  ```cpp
  #define SPEED_CONSTANT 2048
  ```
  (Prefer `static constexpr` for new code; `#define` is kept for legacy
  compatibility.)

### Fixed-point types

- The `FixedPoint<W, F, T, IT, sign>` template is instantiated once per
  semantic use, then `typedef`ed to a descriptive type name.
- All fixed-point instantiations have a clear comment explaining the precision
  choice.
- Do not use raw `FixedPoint<...>` in function signatures — use the `typedef`.

```cpp
typedef FixedPoint<1, 14, int16_t, int32_t, false> DisplayListScalar;
typedef FixedPoint<1, 28, int32_t, int32_t, false> DisplayListIntermediate;
```

## Comments

- File header: see License header above.
- Public API: every public function, class, and struct has a `//` comment
  describing what it does and any non-obvious constraints. Comments are in
  complete sentences.
- `// Why?` / `// Note:` / `// Note -` style inline annotations are used to
  give context, especially in `displaylist.h`.
- TODOs: prefixed with `// TODO:`.
  ```cpp
  // TODO: Make these configurable
  ```
- Disabled but kept-for-reference code: commented out with `// **** Disabled for
  now` as a banner.
- Doxygen-style `/** ... */` is not used; use `//` line comments.

## Error handling and assertions

- `assert()` is used for internal invariants (debug builds); pico-sdk's
  `pico/assert.h`.
- No exceptions are used in the codebase.
- For "should not happen" situations in hot paths, use `panic()` from
  pico-sdk rather than silently returning.

## Logging

- Use the `LogChannel` + `LOG_INFO` / `LOG_WARN` / `LOG_ERROR` macros
  (`include/log.h`).
- Declare channels as file-scope `static LogChannel` instances, with an
  inline comment if their on/off state is non-obvious:

```cpp
static LogChannel DisplayListSynchronisation(false);
```

- Format strings use printf-style specifiers. For fixed-point values, cast to
  `float` for display: `(float)vector.x`.
- Channels default to disabled (`false`) unless they are core diagnostic.

## Memory

- Heap allocation is done with `malloc` / `free` in C-compatible code, and
  `new` / `delete` in C++-only code. Prefer C++ style where both are possible.
- Large buffers (display lists, DAC buffers) are allocated once in the
  constructor and freed in the destructor. No per-frame heap allocations.
- Pointers are not shared between threads without explicit synchronisation.

## Concurrency

- The framework is built around Pico PIO state machines; the main thread does
  updates/draws, and PIO handles DAC output.
- Shared flags between the main thread and the PIO/ISR context are declared
  `volatile` (e.g. `static volatile bool s_dacOutputRunning`), and updated as
  single word-size values so they are effectively atomic on Cortex-M0+.
  For anything more complex, prefer `hardware/sync.h` spinlocks.
- The DMA chain control register is accessed through a `volatile` pointer to
  guarantee the compiler does not reorder the chaining writes.
- Never assume atomicity of multi-word values when sharing with PIO or ISR
  context.

## Templates

- Free functions and operator overloads that are generic live in headers.
- Template parameters use `typename` / `class` + `PascalCase` type name:
  `<typename TA, typename TB>`.
- Non-type template parameters are `constexpr` with `k...` naming when
  they are semantic (e.g. bit counts).

## Misc

- No `using namespace std;` in headers.
- No single-argument constructors without `explicit`.
- Prefer `const` reference parameters for function inputs by value; by-value
  is fine for small trivially-copyable types (PODs).
- Return `const` references from getters:
  `const FixedTransform3D& GetCameraToWorld() const { return m_cameraToWorld; }`
- Avoid macro-ification of simple functions; use `inline` or header placement.
