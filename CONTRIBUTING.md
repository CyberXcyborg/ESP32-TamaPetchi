# Contributing to ESP32-TamaPetchi

Thank you for your interest in contributing to the ESP32-TamaPetchi project!
This document outlines how to contribute, build, test, and submit changes.

## Table of Contents

- [Getting Started](#getting-started)
- [Build Instructions](#build-instructions)
- [Code Style](#code-style)
- [Pull Request Process](#pull-request-process)
- [Testing Requirements](#testing-requirements)
- [Project Structure](#project-structure)
- [Adding New Features](#adding-new-features)

## Getting Started

### Prerequisites

- [PlatformIO](https://platformio.org/) (VS Code extension or CLI)
- [ESP32 Dev Module](https://www.espressif.com/en/products/devkits/esp32-devkitc) (for hardware testing)
- USB-C or Micro-USB cable (depending on your board)
- Arduino ESP32 Core (installed automatically by PlatformIO)

### Clone and Build

```bash
# Clone the repository
git clone https://github.com/CyberXcyborg/ESP32-TamaPetchi.git
cd ESP32-TamaPetchi

# Build for ESP32
pio run -e esp32dev

# Run native unit tests
pio test -e native

# Upload to ESP32
pio run -e esp32dev --target upload

# Upload SPIFFS filesystem
pio run -e esp32dev --target uploadfs
```

## Build Instructions

### Build Environments

| Environment | Purpose | Command |
|-------------|---------|---------|
| `esp32dev` | ESP32 firmware build | `pio run -e esp32dev` |
| `native` | PC unit tests | `pio test -e native` |

### Feature Flags

Edit `src/config.h` to enable/disable features:

| Flag | Default | Description |
|------|---------|-------------|
| `ENABLE_OLED` | undefined | SSD1306 OLED display support |
| `ENABLE_RGB_LED` | defined | RGB LED status indicator |
| `ENABLE_BUZZER` | defined | Buzzer melody support |
| `ENABLE_IR_REMOTE` | defined | IR remote control (NEC protocol) |
| `ENABLE_MQTT` | defined | MQTT smart home integration |

Example — disable OLED:
```cpp
#define ENABLE_OLED
```

### Partition Scheme

The project uses the default `default.csv` partition scheme with OTA support.
Two OTA partitions (`ota_0`, `ota_1`) plus `otadata` for dual-partition rollback.

## Code Style

### C++ Conventions

- **Indentation**: 2 spaces (no tabs)
- **Braces**: K&R style (opening brace on same line)
- **Naming**:
  - Classes/Structs: `PascalCase`
  - Functions: `camelCase()`
  - Variables: `camelCase`
  - Constants/Macros: `UPPER_SNAKE_CASE`
  - Member variables: no prefix (use `this->` if needed)
- **Headers**: Use include guards (`#ifndef HEADER_H` / `#define HEADER_H` / `#endif`)
- **Comments**: Use `//` for single-line, `/* */` for multi-line

### File Organization

```
src/
  ESP32-TamaPetchi.ino   # Main entry point
  config.h                # All configuration constants
  Pet.h / Pet.cpp         # Core pet state machine
  Storage.h / Storage.cpp # SPIFFS persistence
  WebHandlers.h / WebHandlers.cpp  # HTTP API routes
  OTA.h / OTA.cpp         # OTA update support
  ...

data/
  index.html              # Web UI (SPIFFS)
  locales/                # i18n JSON files
  sounds/                 # Sound pack JSON files
```

### Module Pattern

Each feature module follows this pattern:
1. `Module.h` — public API declarations only
2. `Module.cpp` — implementation
3. Register routes in `Module.cpp` or call from `WebHandlers.cpp`
4. Add `initModule()` call in `setup()` of main `.ino`
5. Add `handleModule()` call in `loop()` if needed

## Pull Request Process

1. **Branch from `develop`**: `git checkout -b feature/my-feature develop`
2. **One feature per commit**: Keep commits atomic and focused
3. **Clear commit messages**: Use conventional commit prefixes:
   - `feat:` — new feature
   - `fix:` — bug fix
   - `docs:` — documentation only
   - `test:` — test additions/changes
   - `chore:` — build/CI/config changes
4. **Test before pushing**:
   ```bash
   pio run -e esp32dev  # Must compile without errors
   pio test -e native   # All 152 tests must pass
   ```
5. **Update TASKS.md**: Mark completed tasks with `[x]`
6. **Create PR to `develop`**: Don't push directly to `main`
7. **PR description**: Include what changed, why, and test results

## Testing Requirements

### Unit Tests

All new features must include unit tests in `test/`.
The project uses [Unity](https://github.com/ThrowTheSwitch/Unity) test framework.

Test file naming: `test_<module>.cpp`

Adding a test:
1. Create `test/test_mymodule.cpp`
2. Add test functions and `RUN_TEST()` calls
3. Add forward declaration in the master test file
4. Add `run_mymodule_tests()` call in `test/stubs.cpp`
5. Increment `main()` to call your new `run_*` function

### Test Coverage Targets

- Pet state machine: 100% (evolution, death, revive, actions)
- Web API handlers: serialization roundtrips
- SPIFFS operations: save/load/atomic writes
- Backup/restore: full roundtrip verification
- Achievements: tier calculation, progress tracking

### Hardware Testing

When possible, test on real hardware:
1. Flash firmware: `pio run -e esp32dev --target upload`
2. Upload filesystem: `pio run -e esp32dev --target uploadfs`
3. Monitor serial: `pio device monitor --baud 115200`
4. Verify web UI at `http://<esp32-ip>/`

## Project Structure

```
ESP32-TamaPetchi/
  src/              # C++ source files (PlatformIO)
  data/             # SPIFFS files (web UI, locales, sounds)
  test/             # Unit test files
  docs/             # Documentation
  scripts/          # Utility scripts
  platformio.ini    # PlatformIO configuration
  TASKS.md          # Development task tracking
  CHANGELOG.md      # Version history
  README.md         # Project overview
```

## Adding New Features

### Checklist for New Features

- [ ] Feature flag in `config.h` (compile-time toggle)
- [ ] Module header/source (`src/Module.h`, `src/Module.cpp`)
- [ ] Unit tests (`test/test_module.cpp`)
- [ ] Web API endpoints (if applicable)
- [ ] Web UI integration (if applicable)
- [ ] Updated README.md
- [ ] Updated CHANGELOG.md
- [ ] Updated TASKS.md
- [ ] All tests pass (`pio test -e native`)
- [ ] ESP32 build succeeds (`pio run -e esp32dev`)
- [ ] Flash usage < 85%

### Architecture Guidelines

1. **No global state outside AppState**: All shared state goes through `AppState::getInstance()`
2. **Hardware abstraction**: Use stubs for hardware-dependent code in tests
3. **SPIFFS atomic writes**: Use `Storage::atomicWrite()` for all file operations
4. **Error handling**: Return error codes, don't crash on invalid input
5. **Rate limiting**: Apply rate limits to all HTTP mutation endpoints
6. **Memory awareness**: ESP32 has 320KB RAM — prefer `StaticJsonDocument` where possible

### MQTT Integration

MQTT topics follow the pattern: `tamapetchi/<feature>/<action>`

Existing topics:
- `tamapetchi/state` — full pet state broadcast
- `tamapetchi/notification` — event notifications
- `tamapetchi/trade/+` — pet trading protocol

### i18n

Locale files in `data/locales/` (JSON format):
- `en.json` — English (default)
- `zh.json` — Chinese
- `ja.json` — Japanese

The web UI auto-detects language from `Accept-Language` header.

## License

This project is open source. See LICENSE file for details.
