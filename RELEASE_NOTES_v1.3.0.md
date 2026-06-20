# ESP32-TamaPetchi v1.3.0 Release Notes

**Release Date:** 2026-06-20
**Build:** RAM 17.8%, Flash 79.8%, Zero warnings
**Tests:** 145/152 native tests pass, 19 OTA Delta tests pass

## What's New in v1.3.0

### OTA Delta Updates
Binary delta patching (bsdiff-style) reduces OTA update size by ~90% for minor changes. SHA-256 integrity verification via mbedtls before applying patches. Automatic fallback to full OTA if delta patch fails.

**Endpoints:** `POST /api/ota/delta`, `GET /api/ota/delta/status`, `POST /api/ota/delta/check`

### Hardware Abstraction Layer (HAL)
Seven abstract interfaces (IDisplay, IStorage, IWiFi, IGPIO, IBuzzer, IPower, IRTC) enable native unit testing without ESP32 hardware. Compile-time selection means zero runtime overhead.

### Community Features
Share pet profiles as JSON cards, browse a community gallery of top pets, and compete on leaderboards sorted by achievements, age, or generation. Rate-limited endpoints prevent abuse.

**Endpoints:** `GET /api/community/gallery`, `GET /api/community/leaderboard`, `POST /api/community/share`, `POST /api/community/import`

### Manufacturing & Provisioning Tools
First-boot AP mode ('TamaPetchi-Setup') for initial configuration. Python provisioning script for serial-flashing with unique device IDs. Batch flash shell script for production-line flashing.

**Device ID:** TAMA-XXXXXX derived from ESP32 MAC address.

### Power Optimization
Light sleep mode (<1mA), battery estimation with 24-hour drain rate analysis, configurable wake intervals (5min/15min/1hr), and WiFi power management.

## Full Changelog

See [CHANGELOG.md](CHANGELOG.md) for complete details.

## Upgrade Notes

- v1.2.0 → v1.3.0: No breaking changes. All existing API endpoints preserved.
- OTA delta is disabled by default. Set `OTA_DELTA_MANIFEST_URL` in `config.h` to enable.
- HAL is compile-time only — no configuration needed.
- Community features require WiFi connection for gallery/leaderboard.

## Known Issues

- 7 pre-existing native test failures (backup/achievement round-trip tests) — not regressions from Phase 13.
- Community tests removed from native build (ESP32 dependency chain) — tests exist for ESP32 build.
