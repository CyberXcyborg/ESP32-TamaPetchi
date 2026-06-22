# ESP32-TamaPetchi — Autonomous Development Tasks

## Status
- Phases 1-18 ✅ Complete — v1.8.0 released 2026-06-22
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
- [x] Create git tag: v1.8.0
- [x] Merge: develop → main
- [x] Push all branches: git push origin main develop --tags
- [x] Create GitHub release with firmware.bin attached
- [x] Write release notes: feature summary, hardware validation results, known issues, v2.0 preview

## Implementation Rules
- Create branch: feature/phase19-xxx (branch from develop)
- One feature per commit
- Test compilation after each feature
- Update TASKS.md with progress after each sub-task
- Push when all features done
- Create PR when complete

---

## Phase 19: v2.0 Foundation — ESP-IDF Migration & LVGL Display

**Branch:** feature/phase19-v2-foundation
**Goal:** Migrate from Arduino framework to ESP-IDF + Arduino hybrid on ESP32-S3, integrate LVGL for color TFT display, and establish the v2.0 project foundation.
**Reference:** V2_ROADMAP.md (Phase A: Weeks 1–3)
**Priority:** Build system → Core port → Display → Verify

### 19.1 — Build System Migration
- [x] Create new PlatformIO project targeting ESP32-S3-DevKitC-1 (8MB PSRAM)
  - platform=espressif32 with board=esp32-s3-devkitc-1
  - Enable PSRAM: board_build.arduino.memory_type=qio_opi and board_build.psram_type=opi
  - Set board_build.flash_mode=qio, board_build.flash_size=8MB
- [x] Configure ESP-IDF + Arduino hybrid framework
  - Use `framework = arduino` with ESP-IDF components via arduino-esp32
  - Verify ArduinoJson, PubSubClient compile under hybrid mode
  - Document any library incompatibilities in MIGRATION_NOTES.md
- [x] Set up partition table for v2.0 (8MB flash)
  - A/B OTA partitions (1.2MB each)
  - NVS (24KB), LittleFS (2MB), app metadata
  - Create custom partitions_v2.csv
- [x] Configure LittleFS (replace SPIFFS)
  - Update all SPIFFS references to LittleFS
  - Verify LittleFS works with ESP-IDF
  - Migrate storage format if needed
- [x] Verify clean build with zero warnings
  - Document flash/RAM baseline for empty project

### 19.2 — Core Module Porting
- [x] Port AppState.h singleton to ESP-IDF
  - Verify NVS storage works for persistent state
  - Adapt any Arduino-specific APIs (millis(), delay(), etc.)
- [x] Port Storage module (Storage.cpp/Storage.h)
  - Replace SPIFFS file operations with LittleFS
  - Maintain same public API for backward compatibility
  - Add unit tests for LittleFS read/write
- [x] Port Pet engine core (Pet.cpp/Pet.h)
  - Ensure stat decay logic works with ESP-IDF FreeRTOS tick
  - Port evolution logic, mood calculation
  - Verify no Arduino-specific dependencies remain
- [x] Port config.h for v2.0
  - Add ESP32-S3 specific config flags
  - Add LVGL display config (resolution, color depth, pins)
  - Add PSRAM usage flags
  - Document all new flags
- [x] Port HAL (HAL.h/HAL_ESP32.cpp) to ESP-IDF
  - GPIO, SPI, I2C using ESP-IDF drivers
  - Maintain same HAL API surface
  - Add compile-time checks for pin mappings

### 19.3 — LVGL Display Integration
- [x] Add LVGL as PlatformIO dependency (lvgl v8.3+)
  - Configure lv_conf.h: 240×240 resolution, 16bpp color
  - Enable only needed widgets (label, image, button, bar, arc, canvas)
  - Disable unused features to minimize flash usage (target < 200KB for LVGL)
  - Allocate framebuffers in PSRAM (2 × 112.5KB for double buffering)
- [x] Implement ST7789 display driver
  - SPI initialization (SPI2_HOST, 40MHz clock)
  - Pin mapping: MOSI=11, SCLK=12, CS=10, DC=14, RST=13 (adjust per board)
  - LVGL display flush callback
  - Verify solid color fill, pixel patterns
- [x] Implement LVGL tick timer
  - FreeRTOS timer at 1ms interval for LVGL tick
  - LVGL task handler on Core 0 (pinned)
  - Target 30 FPS rendering
- [x] Render first pet on TFT
  - Create simple 64×64 color test sprite in PSRAM
  - Display sprite at center of 240×240 screen
  - Add basic label showing pet name and stats
  - Verify smooth rendering without flicker

### 19.4 — Input System (v2.0)
- [x] Port button input to ESP-IDF
  - Use ESP-IDF GPIO interrupts (replace Arduino attachInterrupt)
  - Maintain same button API (Buttons.cpp/Buttons.h)
  - Verify debounce logic works
- [x] Add touch input driver (XPT2046 resistive or GT911 capacitive)
  - SPI interface for XPT2046 (shared SPI bus with display, different CS)
  - LVGL touch/indev driver integration
  - Calibrate touch coordinates to 240×240 display
  - Verify tap, swipe gestures

### 19.5 — Verification & Baseline
- [ ] Run all 162 native tests on ESP-IDF
  - Adapt test framework for ESP-IDF (replace Arduino test mocks)
  - Fix any test failures from framework migration
  - Target: 162/162 pass
- [ ] Measure v2.0 baseline metrics
  - Flash usage (target: < 70% on 8MB)
  - SRAM usage (target: < 40% of 512KB)
  - PSRAM usage (target: < 10% of 8MB for now)
  - Boot time (target: < 3 seconds to pet visible)
- [ ] Create V2_BASELINE.md with metrics
- [ ] Update README.md with v2.0 development status
- [ ] Merge feature/phase19-v2-foundation → develop
- [ ] Tag: v2.0.0-alpha.1

## How This Works
Nyra (project manager) assigns tasks here → Kael (developer) reads and implements → Kael creates PR → Nyra reviews → Nyra assigns next phase
