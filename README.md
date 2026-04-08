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

## Test Naming Conventions

- Native runtime files use `test_<phase-or-area>_<topic>.cpp`.
- Native runtime test functions use `test_<task-id>_<behavior>()`.
- Compile-fail cases use `<task-id>_<description>.cpp` under `test/compile_fail/`.
