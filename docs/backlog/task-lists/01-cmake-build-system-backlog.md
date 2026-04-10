# CMake Build System Migration Backlog

Purpose: define and sequence the work needed to replace PlatformIO with a CMake-based build and test system for native targets, supporting MSVC on Windows and GCC on Linux, and exposing the library as a proper CMake package so downstream applications can consume it via `find_package` or `add_subdirectory`.

Embedded targets are explicitly out of scope for this backlog.

Status legend:
- `todo`: not started
- `doing`: active
- `done`: completed
- `blocked`: waiting on dependency
- `deferred`: intentionally postponed

## Implementation Status

Current status: phases 1–5 implemented.

The project currently builds and tests exclusively through PlatformIO (`platformio.ini`, `pio/common.ini`, `pio/env_native_tests.ini`). No CMake files exist yet. The library headers reside under `src/` with `src/libs/pfr/` as a vendored dependency. Tests live under `test/test_native/` and use the Unity test framework.

## Design Intent

- Replace the PlatformIO native test pipeline with a CMake-native equivalent that works with `ctest`.
- Use MSVC (`/std:c++23`) on Windows and GCC (`-std=gnu++23`) on Linux with equivalent warning surfaces.
- Expose the project as an `INTERFACE` CMake library target so consumer projects can link against it with a single `target_link_libraries` call.
- Vendor ArduinoJson through CMake's `FetchContent` rather than PlatformIO's `lib_deps`.
- Vendor the Unity test framework through `FetchContent` for test builds only.
- Keep PlatformIO files in place and untouched until the CMake pipeline is fully verified; do not delete them as part of this backlog.

## Scope

- CMake project definition for the `lumalink-json` interface library
- Compiler flag parity with the existing PlatformIO configuration
- Native test executable wired to `ctest`
- `FetchContent` integration for ArduinoJson and Unity
- CMake package installation rules so downstream projects can use `find_package(LumaLinkJson)`
- Compile-fail test integration under CMake

## Non-Goals

- Do not add embedded or cross-compile toolchain files
- Do not remove or modify PlatformIO configuration files
- Do not change source or test code as part of the CMake migration
- Do not introduce a CI pipeline; that is a separate concern
- Do not add new library features or tests as part of this work

## Architectural Rules

- The library target must be `INTERFACE` only; there are no compiled sources.
- Compiler flags must be applied per-target, not via `CMAKE_CXX_FLAGS` globals.
- `FetchContent` declarations must be isolated so they do not pollute consumer builds when the project is used as a subdirectory.
- Installation rules must produce a relocatable package with a proper `LumaLinkJsonConfig.cmake` file.
- The `cmake/` directory already exists and should hold all helper modules and config templates.

## Work Phases

## Phase 1 - Root CMake Project And Interface Library Target

| ID | Status | Task | Depends On | Definition of Done |
|---|---|---|---|---|
| CMAKE-01 | done | Author `CMakeLists.txt` at the repo root defining the project, minimum CMake version, and C++23 standard requirement | none | `cmake --preset` or a manual configure step completes without error on both Windows (MSVC) and Linux (GCC) |
| CMAKE-02 | done | Define the `lumalink_json` `INTERFACE` library target with `src/` and `src/libs/pfr/` on its include path | CMAKE-01 | A minimal consumer `add_subdirectory` or `find_package` can `#include "LumaLinkJson.h"` without additional include directories |
| CMAKE-03 | done | Apply compiler flags equivalent to the PlatformIO `common.ini` settings to the interface target | CMAKE-02 | MSVC receives `/W4 /WX- /EHs-` (no-exceptions) and `/std:c++latest`; GCC receives `-std=gnu++23 -Wall -Wextra -Wno-unused-parameter -fno-exceptions` |
| CMAKE-04 | done | Integrate ArduinoJson via `FetchContent` and link it to the interface target | CMAKE-02 | The interface target transitively pulls ArduinoJson for any consumer without manual `include_directories` |

## Phase 2 - Native Test Executable

| ID | Status | Task | Depends On | Definition of Done |
|---|---|---|---|---|
| CMAKE-05 | done | Integrate the Unity test framework via `FetchContent` guarded by a `LUMALINK_JSON_BUILD_TESTS` option | CMAKE-01 | Unity is fetched only when tests are enabled and is not propagated to consumers |
| CMAKE-06 | done | Define a `lumalink_json_tests` executable target that compiles all files under `test/test_native/` | CMAKE-02, CMAKE-05 | The test executable builds without error on MSVC and GCC |
| CMAKE-07 | done | Register the test executable with `ctest` so `ctest --output-on-failure` runs the full suite | CMAKE-06 | `ctest` exits zero when all Unity tests pass |
| CMAKE-08 | done | Add `UNIT_TEST` and `NATIVE_TESTS` compile definitions to the test target to match PlatformIO behavior | CMAKE-06 | Preprocessor guards that gate test-only code compile correctly under CMake |

## Phase 3 - Compile-Fail Test Integration

| ID | Status | Task | Depends On | Definition of Done |
|---|---|---|---|---|
| CMAKE-09 | done | Audit `test/compile_fail/` and `tools/run_compile_fail_checks.ps1` to understand the current detection mechanism | none | A written note or inline comment describes what the script does and what CMake hook is needed |
| CMAKE-10 | done | Add a CMake custom target or `ctest` fixture that invokes each compile-fail case and asserts non-zero exit | CMAKE-07, CMAKE-09 | `ctest -R compile_fail` runs all cases and fails the suite if any case unexpectedly compiles |

## Phase 4 - CMake Package Installation

| ID | Status | Task | Depends On | Definition of Done |
|---|---|---|---|---|
| CMAKE-11 | done | Author installation rules for the interface target, headers, and generated CMake config files | CMAKE-02, CMAKE-04 | `cmake --install` produces a prefix tree that a consumer project can point `CMAKE_PREFIX_PATH` at |
| CMAKE-12 | done | Author `cmake/LumaLinkJsonConfig.cmake.in` and wire it through `configure_package_config_file` | CMAKE-11 | A consumer project with no source access to this repo can call `find_package(LumaLinkJson REQUIRED)` and get the interface target |
| CMAKE-13 | done | Author a `LumaLinkJsonVersion.cmake` via `write_basic_package_version_file` | CMAKE-12 | `find_package(LumaLinkJson 0.1 REQUIRED)` resolves correctly against the installed package |

## Phase 5 - CMake Presets And Developer Workflow

| ID | Status | Task | Depends On | Definition of Done |
|---|---|---|---|---|
| CMAKE-14 | done | Author a `CMakePresets.json` with configure presets for `windows-msvc-debug`, `windows-msvc-release`, `linux-gcc-debug`, and `linux-gcc-release` | CMAKE-03 | Developers can run `cmake --preset windows-msvc-debug` without specifying generator or flags manually |
| CMAKE-15 | done | Add a build preset and a test preset that chain from each configure preset | CMAKE-14 | `cmake --build --preset windows-msvc-debug` and `ctest --preset windows-msvc-debug` both work end-to-end |
| CMAKE-16 | done | Update `README.md` with CMake build, test, and install instructions alongside the existing PlatformIO instructions | CMAKE-07, CMAKE-11, CMAKE-15 | A new contributor can build and run tests using only the README as a guide |

## Phase 6 - Verification And PlatformIO Parity Check

| ID | Status | Task | Depends On | Definition of Done |
|---|---|---|---|---|
| CMAKE-17 | todo | Run the full test suite under both MSVC and GCC and record any failures or flag discrepancies | CMAKE-07, CMAKE-08 | All tests that pass under PlatformIO also pass under CMake; any divergence is documented |
| CMAKE-18 | todo | Confirm that a minimal external consumer project can include the library via `add_subdirectory` | CMAKE-02, CMAKE-04 | A throw-away consumer `CMakeLists.txt` outside this repo builds against the library with no extra configuration |
| CMAKE-19 | todo | Confirm that a minimal external consumer project can include the library via `find_package` after install | CMAKE-11, CMAKE-12, CMAKE-13 | The same throw-away consumer builds using the installed package rather than the source tree |
| CMAKE-20 | todo | Decide whether PlatformIO files should be kept, archived, or removed and record the decision | CMAKE-17 | A note in the backlog or a design doc captures the disposition of the PlatformIO configuration going forward |

## Sequencing Notes

- Complete the interface library target definition before touching tests; tests depend on the library target.
- Keep ArduinoJson and Unity fetched through separate `FetchContent` declarations so Unity never leaks into consumer builds.
- Do not apply compiler flags globally; always use `target_compile_options` with `INTERFACE` or `PRIVATE` scope.
- Author the install and package config rules before verifying consumer use; verification depends on them being in place.
- Do not delete PlatformIO files until CMAKE-20 is resolved and the team has made an explicit decision.
