# lumalink-json

Design docs from the session decomposition are in [docs/design/README.md](docs/design/README.md).

## PlatformIO Setup

This repository is configured as a PlatformIO library project with a native test environment using Unity.

Library source and public headers use an Arduino-style single-directory layout under src.

The PlatformIO configuration is split across files under the pio directory:

- pio/common.ini
- pio/env_native_lib.ini
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

Build the native library environment:

```powershell
pio run -e native_lib
```
