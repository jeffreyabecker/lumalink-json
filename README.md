# lumalink-json

Design docs from the session decomposition are in [docs/design/README.md](docs/design/README.md).
Usage examples for the shipped API surface are in [docs/usage-guide.md](docs/usage-guide.md).

## PlatformIO Setup

This repository is configured as a PlatformIO library project with a native test environment using Unity.

The library is header-only. Public headers use an Arduino-style single-directory layout under src.

The PlatformIO configuration is split across files under the pio directory:

- pio/common.ini
- pio/env_native_tests.ini

### Native test environment

- Environment: native_tests
- Test framework: Unity
- JSON dependency: bblanchon/ArduinoJson @ ^7.0.0

### Commands

Run tests:

```powershell
pio test -e native_tests
```

Run compile-fail harness checks:

```powershell
powershell -ExecutionPolicy Bypass -File tools/run_compile_fail_checks.ps1
```

## CMake Setup

The repository also supports a CMake-based build for native targets (MSVC on Windows, GCC on Linux). Embedded targets are not covered.

### Prerequisites

- CMake 3.25 or later
- Ninja (recommended; install via `winget install Ninja-build.Ninja` or your package manager)
- **Windows:** run from a Visual Studio Developer PowerShell or after calling `vcvarsall.bat` so that `cl.exe` is on `PATH`
- **Linux:** GCC with C++23 support (`g++ --version` should report 13 or later)

ArduinoJson and Unity are fetched automatically via `FetchContent` on first configure; no manual dependency installation is needed.

### Configure, build, and test

```powershell
# Windows MSVC debug
cmake --preset windows-msvc-debug
cmake --build --preset windows-msvc-debug
ctest --preset windows-msvc-debug
```

```bash
# Linux GCC debug
cmake --preset linux-gcc-debug
cmake --build --preset linux-gcc-debug
ctest --preset linux-gcc-debug
```

Available presets: `windows-msvc-debug`, `windows-msvc-release`, `linux-gcc-debug`, `linux-gcc-release`.

Build output lands in `build/<preset-name>/`.

### Compile-fail harness

The compile-fail cases under `test/compile_fail/` are registered as individual `ctest` tests. They run automatically as part of the full `ctest` invocation above. To run only that subset:

```powershell
ctest --preset windows-msvc-debug -R "compile_fail/"
```

### Install and downstream use

```powershell
cmake --install build/windows-msvc-release --prefix C:/my/prefix
```

A downstream project can then consume the library with:

```cmake
find_package(LumaLinkJson REQUIRED)
target_link_libraries(my_app PRIVATE LumaLinkJson::LumaLinkJson)
```

Or without installing, via `add_subdirectory`:

```cmake
add_subdirectory(path/to/lumalink-json)
target_link_libraries(my_app PRIVATE LumaLinkJson::LumaLinkJson)
```

## Test Naming Conventions

- Native runtime files use `test_<phase-or-area>_<topic>.cpp`.
- Native runtime test functions use `test_<task-id>_<behavior>()`.
- Compile-fail cases use `<task-id>_<description>.cpp` under `test/compile_fail/`.
