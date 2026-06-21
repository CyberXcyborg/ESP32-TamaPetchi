# Release Notes — v1.4.0

**Release Date:** 2026-06-21
**Phase:** 14 — Stability, Testing & Ecosystem Expansion
**Tag:** v1.4.0

## Overview

v1.4.0 focuses on stability improvements, fixing all known test failures, and adding
community/developer tooling. No new hardware features were added — instead, existing
features from Phases 11-13 were verified and documented.

## What's New

### Test Infrastructure Fixes (Phase 14.1)

All 7 pre-existing test failures have been resolved:

- **Root Cause 1 — ArduinoJson String Incompatibility**: The native test `String` class
  (wrapping `std::string`) is not compatible with ArduinoJson 6.18.5's
  `deserializeJson(const String&)` overload. Fixed by using `.c_str()` for all
  deserialization calls.
- **Root Cause 2 — Document Size**: `StaticJsonDocument<2048>` was too small for 16
  achievement entries with all fields. Changed to `DynamicJsonDocument(8192)`.

**Result:** 152/152 native tests now pass (up from 145/152).

### Community & Developer Tools (Phase 14.6)

- **CONTRIBUTING.md**: Complete contributor guide with build instructions, code style,
  PR process, testing requirements, and architecture guidelines
- **GitHub Issue Templates**: Structured bug report and feature request templates
- **Batch Flash Script** (`scripts/flash-batch.py`): Flash multiple ESP32 devices
  sequentially via USB with auto-detection
- **24h Simulation Script** (`scripts/simulate-24h.py`): Project health checks,
  build verification, test execution, and code metrics

### Verified Existing Features

The following features were verified as already implemented from earlier phases:

| Feature | Phase | Status |
|---------|-------|--------|
| OTA Rollback | 11.1 | ✅ Verified working |
| Pet Trading (MQTT) | 11.4 | ✅ Verified working |
| Sound Packs | 11.3 | ✅ Verified working |
| Web UI (Trade + Sound) | 11.3-11.4 | ✅ Verified working |

## Build Results

| Metric | Value | Status |
|--------|-------|--------|
| RAM | 17.8% (58,472 / 327,680 bytes) | ✅ |
| Flash | 79.8% (1,045,645 / 1,310,720 bytes) | ✅ (under 85%) |
| Warnings | 0 | ✅ |
| Native Tests | 152/152 pass | ✅ |

## API Changes

No API changes in this release. All existing endpoints remain compatible.

## Upgrade Guide

- **From v1.3.0**: OTA update preserves all settings and pet data
- **From v1.2.0+**: OTA update preserving all data; OTA rollback available if issues
- **From v1.0.0+**: Standard OTA upgrade path

## Known Issues

- IR_RECEIVER_PIN (GPIO 15) conflicts with OLED CS pin — use one or the other
- MQTT broker must be configured at compile time via config.h
- OTA rollback requires ESP32 partition scheme with at least 2 OTA partitions
- Pet trading requires MQTT broker connection and both devices on same MQTT network

## Upgrade Guide

- **From v1.3.0**: OTA update preserves all settings and pet data; all existing features remain compatible
- **From v1.1.0+**: OTA rollback — if new firmware crashes 3 times, device auto-reverts
- First boot after flash: connect to "TamaPetchi" AP for WiFi configuration

## Contributors

- Kael Nexus (Lead Developer) — autonomous implementation
- Nyra Vale (Project Manager) — task assignment and review
