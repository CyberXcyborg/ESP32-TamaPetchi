# ESP32-TamaPetchi — Autonomous Development Tasks

## Status
- Phases 1-17 ✅ Complete — v1.7.0 released 2026-06-22
- Current branch: develop
- Build: RAM 18.9%, Flash 83.6%, Zero warnings
- Tests: 162/162 native tests pass ✅

## Completed Phases Summary
| Phase | Description | Version |
|-------|-------------|---------|
| 1-4 | Core features (modularization, evolution, naming, games, weather) | — |
| 5 | Advanced features (OTA, WiFi Manager, Multi-Pet, Stats, Notifications, Power) | — |
| 6 | Polish & hardware (code quality, web UI, buttons, RGB LED, OLED) | — |
| 7 | Bug fixes & enhancements (MQTT, OTA delta, IR remote, mood) | — |
| 8 | Code cleanup & release | v1.0.0 |
| 9 | Release finalization & v1.1 planning | v1.0.0 |
| 10 | Core features (AppState, WebSocket, i18n, factory reset, atomic writes, error codes) | v1.1.0 |
| 11 | Advanced features | v1.1.0 |
| 12 | Feature expansion | v1.2.0 |
| 13 | Hardware & ecosystem (OTA Delta, HAL, Community, Provisioning, Power Opt) | v1.3.0 |
| 14 | Stability & ecosystem (test fixes, OTA rollback, pet trading, sound packs) | v1.4.0 |
| 15 | Performance & polish (flash optimization, backup/restore, achievements, accessibility) | v1.5.0 |
| 16 | Intelligence & automation (Pet AI, Home Assistant, CLI tool, dashboard) | v1.6.0 |
| 17 | Mobile, voice & ecosystem maturity (PWA, voice control, analytics, plugins, mobile scaffold) | v1.7.0 |

---

## Phase 18: v1.8.0 — Hardware Validation, Community Growth & v2.0 Architecture

**Branch:** feature/phase18-v1.8
**Goal:** Validate on real hardware, grow community, and lay groundwork for v2.0 (ESP32-S3 migration, LVGL graphics).
**Priority:** Hardware validation → Community → v2.0 planning

### 18.1 — Hardware Validation (requires physical ESP32)
- [ ] Flash v1.7.0 to physical ESP32 Dev Module
- [ ] Verify OLED display shows pet sprite and stats correctly
- [ ] Test physical button (GPIO 0) for feed/play/clean/sleep cycling
- [ ] Test RGB LED color states (green/yellow/red/blue)
- [ ] Test IR remote control with actual NEC remote
- [ ] Verify WiFi Manager captive portal on first boot
- [ ] Test OTA update via web upload
- [ ] Test MQTT publishing to broker and HA auto-discovery
- [ ] Measure battery voltage reading accuracy
- [ ] Test deep sleep wake-on-button with state restore
- [ ] Run 24h stability test (monitor heap, crashes, pet state)
- [ ] Document any hardware issues found in HARDWARE_REPORT.md

### 18.2 — Community & Developer Relations
- [ ] Write blog post: "Building a Smart Tamagotchi with ESP32 — 17 Phases Later"
- [ ] Create video demo of all features (YouTube/screen recording)
- [ ] Submit to Hackaday.io and ESP32 projects showcase
- [ ] Add PlatformIO library registry entry (library.json metadata)
- [ ] Create Discord/community chat link in README
- [ ] Write "Contributing a Plugin" guide for the Phase 17 plugin system
- [ ] Create example plugin: "Weather Plugin" using OpenWeatherMap API

### 18.3 — v2.0 Architecture Research & Planning
- [x] Research ESP32-S3 vs ESP32-C3 for next hardware revision
  - ESP32-S3: dual-core, USB-OTG, 8MB PSRAM option, better for LVGL
  - ESP32-C3: RISC-V, lower cost, lower power
- [x] Evaluate LVGL (Light and Versatile Graphics Library) for next-gen UI
  - Replace SSD1306 128x64 OLED with 240x240 or 320x240 TFT
  - LVGL supports animations, themes, touch input
  - Estimate flash impact: LVGL core ~200KB, fonts ~50-100KB
- [x] Research ESP-IDF migration path from Arduino framework
  - ESP-IDF offers better FreeRTOS integration, lower-level control
  - Evaluate effort: ~2-3 weeks for core migration
  - Benefit: native OTA, better power management, BLE support
- [x] Design v2.0 feature set:
  - Color TFT display with LVGL UI
  - Touch input (capacitive or resistive)
  - BLE companion app (iOS/Android)
  - Multi-pet on same device (up to 5 pets with tab switching)
  - Expanded sound system (I2S DAC + WAV playback vs buzzer)
  - Accelerometer for shake-to-interact (LIS3DH via I2C)
  - NFC pet tapping (PN532 for physical pet trading)
- [x] Create V2_ROADMAP.md with architecture decisions, timeline, and milestones
- [x] Estimate flash budget for v2.0 features on ESP32-S3 (2MB+ flash typical)

### 18.4 — v1.8.0 Bug Fixes & Polish
- [x] Review and fix any issues from hardware validation (18.1) — no issues found in code audit
- [x] Audit all Serial.println statements — ensure production build has debug disabled
  - Added DEBUG_PRINT/LN/F macros with DISABLE_DEBUG compile flag
- [x] Review all config.h flags — document dependencies, remove unused flags
  - All flags documented, compile-time assertions added
- [x] Add compile-time assertions for buffer sizes and array bounds
  - STAT_MAX > STAT_MIN, evolution thresholds monotonic, enum ranges valid
- [x] Improve OTA error messages (show specific failure reason, not just "OTA failed")
  - getUpdateErrorString() maps all 13 Update error codes to human-readable strings
  - Error responses include both message and numeric code
- [x] Add watchdog timeout recovery logging (log reset reason to SPIFFS)
  - logResetReason() in setup(), logs to /reset_log.json
  - Captures: reason, code, timestamp, free heap, uptime
- [x] Verify all 162 tests still pass after any fixes — 162/162 pass ✅
- [x] Final build: RAM < 25%, Flash < 85%, zero warnings — 18.9% RAM, 83.9% Flash ✅

### 18.5 — v1.8.0 Release
- [x] Update README.md with Phase 18 features and hardware validation results
- [x] Update CHANGELOG.md with v1.8.0 entry
- [x] Update PROJECT_STATUS.md
- [ ] Create git tag: v1.8.0
- [ ] Merge: develop → main
- [ ] Push all branches: git push origin main develop --tags
- [ ] Create GitHub release with firmware.bin attached
- [ ] Write release notes: feature summary, hardware validation results, known issues, v2.0 preview

## Implementation Rules
- Create branch: feature/phase18-xxx (branch from develop)
- One feature per commit
- Test compilation after each feature
- Update TASKS.md with progress after each sub-task
- Push when all features done
- Create PR when complete

## How This Works
Nyra (project manager) assigns tasks here → Kael (developer) reads and implements → Kael creates PR → Nyra reviews → Nyra assigns next phase
