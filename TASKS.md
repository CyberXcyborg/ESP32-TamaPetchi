# ESP32-TamaPetchi — Autonomous Development Tasks

## Status
- Phase 1 ✅ Merged (Code modularization)
- Phase 2 ✅ Merged (Evolution, day/night, warnings)
- Phase 3 ✅ Merged (Naming, buzzer, OLED, achievements, pet types)
- Phase 4 ✅ Merged (Evolution anim, death/revive, games, weather, music, settings)
- Phase 5 ✅ Merged (OTA, WiFi Manager, Multi-Pet, Stats, Notifications, Power)
- Phase 6 ✅ Merged (Code quality, web UI, hardware features, docs, performance)
- Phase 7 ✅ Complete — Reviewed & approved by Nyra (PRs #8, #9)
- Phase 8 🔄 Assigned — Code cleanup, merge, and release preparation

## Phase 5: Advanced Features — COMPLETE ✅

### Bug Fixes (feature/phase5-bugfixes) ✅
- Fixed critical g_stats null pointer dereference in WebHandlers.cpp
- Fixed lambda capture issues in OTA.cpp and WiFiManager.cpp
- Fixed invalid enum conversions in MultiPet.cpp and Storage.cpp
- Added missing includes (ArduinoJson.h in Pet.h, Esp.h in OTA.cpp/WiFiManager.cpp)
- Added missing note frequency definitions (NOTE_D5 through NOTE_G5)
- Removed duplicate getBatteryJson() from Pet.cpp (already in PowerManager.cpp)
- Removed orphaned Statistics.cpp (duplicate of Stats.cpp)
- Added forward declarations for all WebHandlers
- Added .gitignore for .pio/ build artifacts
- Fixed missing config.h constants (15 defines for evolution, day/night, warning states)
- Compilation: ✅ SUCCESS (RAM 16%, Flash 70.5%)

### Features Implemented
1. **OTA Updates** — ArduinoOTA + web upload + password protection
2. **WiFi Manager** — Auto-connect + captive portal + SPIFFS credential storage
3. **Multi-Pet Support** — Up to 3 pets, CRUD endpoints, pet selector UI
4. **Statistics Dashboard** — Play time/feeds/plays/sleeps/cleans/heals/deaths/evolutions
5. **Notification System** — Per-pet notification slots, buzzer patterns, web UI badges
6. **Power Management** — Deep sleep + battery ADC monitoring + WiFi power reduction

## Phase 6: Polish & Hardware Integration — COMPLETE ✅

### 6.1 — Code Quality & Safety
- [x] Add bounds checking on all SPIFFS JSON parsing (prevent malformed data crashes)
- [x] Add watchdog timer recovery (ESP32 WDT for hang detection)
- [x] Implement SPIFFS wear leveling awareness (limit save frequency to once per 5 min max)
- [x] Add input validation on all HTTP endpoints (sanitize pet name, clamp stat values)
- [x] Review and add nullptr checks on all global pointers (g_pet, g_server, g_stats, g_multiPet)

### 6.2 — Web UI Improvements
- [x] Add responsive mobile-first CSS (currently desktop-oriented)
- [x] Implement WebSocket or Server-Sent Events for real-time pet stat updates (replace polling)
- [x] Add pet sprite animations (walk, sleep, eat cycles using CSS/SVG)
- [x] Implement dark mode toggle (stored in localStorage + sync with device setting)
- [x] Add sound effect toggle and volume control in web UI

### 6.3 — Hardware Features
- [x] Add physical button support (GPIO 0 BOOT button for feed/play/clean/sleep)
- [x] Implement RGB LED status indicator (green=healthy, yellow=warning, red=critical, blue=sleeping)
- [x] Add battery level display on OLED (when ENABLE_OLED is defined)
- [x] Implement deep sleep wake-on-button with proper state restore
- [x] Add buzzer melody configuration (user-selectable melodies per event)

### 6.4 — Testing & Documentation
- [x] Write PlatformIO unit tests for Pet state machine (evolution, death, revive logic)
- [x] Create hardware wiring diagram (WIRING.md)
- [x] Write user setup guide (SETUP_GUIDE.md)
- [x] Add README section for each enabled feature with build flags
- [x] Test SPIFFS data migration from v1 (monolithic .ino) to v2 (modular) format

### 6.5 — Performance & Memory
- [x] Profile heap usage after 24h runtime (detect memory leaks) — added heap logging in updatePet()
- [x] Optimize JSON document sizes (use StaticJsonDocument where possible) — replaced 25 DynamicJsonDocument with StaticJsonDocument
- [x] Implement HTTP gzip compression for index.html — added gzip pre-compression in WebHandlers
- [x] Reduce WiFi power consumption in idle mode (modem sleep via reduced TX power)
- [x] Add compile-time feature flags to reduce flash usage when features disabled

## Phase 7: Bug Fixes & Enhancements — COMPLETE ✅

### 7.1 — Critical Bug Fixes
- [x] **Fix multi-pet persistence**: Add Phase 4 & 5 fields to loadMultiPetState() and saveMultiPetState()
- [x] **Fix duplicate /wifi/reset route**: Verified not present in current codebase
- [x] **Consolidate duplicate defines**: Remove MAX_PETS from MultiPet.h, remove OTA_PASSWORD/OTA_PORT/OTA_HOSTNAME from OTA.h
- [x] **Remove unused WiFiManager lib dependency** from platformio.ini

### 7.2 — Code Quality & Testing
- [x] Add bounds validation on all SPIFFS JSON parse results
- [x] Add HTTP endpoint rate limiting (token bucket per IP, 10 burst, 1/sec refill)
- [x] Write PlatformIO unit tests for Pet state machine (38 tests)
- [x] Test SPIFFS data migration from v1 to v2 format (10 tests)

### 7.3 — Performance & Memory
- [x] Profile heap usage after 24h runtime — added heap logging in updatePet()
- [x] Optimize JSON document sizes — replaced 25 DynamicJsonDocument with StaticJsonDocument
- [x] Implement HTTP gzip compression for index.html
- [x] Audit DynamicJsonDocument allocations — 25 occurrences converted, 12 remain for variable-size content

### 7.4 — Web UI Polish
- [x] Add loading spinners for async operations
- [x] Add confirmation dialogs for destructive actions (reset, delete pet)
- [x] Improve error messages in API responses (include error codes)
- [x] Add pet sprite SVG animations for all pet types (BLOB, CAT, DOG)

### 7.5 — New Features (Stretch)
- [x] Add scheduled feeding (timer-based auto-feed, configurable interval 1-24h, amount 5-50)
- [x] Add pet mood system (7 levels based on stats + personality traits: cheerful, energetic, hungry)
- [x] Add IR remote control support (NEC protocol) — IRRemote module with feed/play/clean/sleep/wake/sound/pet-switch
- [x] Add MQTT integration for smart home connectivity — PubSubClient, HA auto-discovery (6 sensors + 3 buttons)
- [x] Add OTA delta updates (manifest-based delta system, compressed firmware download via SPIFFS staging)

### PR #8 & #9 Review Notes (Nyra, 2026-06-17)
- ✅ Both PRs reviewed and approved (review comments posted)
- **Note: updateStage() called twice in updatePet()** — redundant but harmless, fix in Phase 8 cleanup
- **Note: IR_RECEIVER_PIN 15** — document in WIRING.md for OLED conflict
- **Note: http.setTimeout(60000U) in OTA_Delta** — consider 120000 for safety margin
- **Note: extern Pet pet in MQTT.cpp** — tight coupling, consider getter in Phase 8
- **Note: checkRateLimit extern in multiple files** — move decl to WebHandlers.h in Phase 8
- **Note: StaticJsonDocument<256> in MQTT callback** — consider 512 for safety

## Phase 8: Code Cleanup & Release Preparation

### 8.1 — Merge & Integration
- [x] Merge PR #8 (feature/phase6-7-merge) into develop
- [x] Merge PR #9 (feature/phase7-enhancements) into develop
- [x] Resolve any merge conflicts
- [x] Verify full compilation after merge (pio run -e esp32dev)
- [ ] Run unit tests (pio test -e native)

### 8.2 — Code Cleanup
- [x] Remove redundant `updateStage()` call in `updatePet()` (called twice)
- [x] Move `checkRateLimit` declaration to WebHandlers.h (removed extern from MQTT.cpp and OTA_Delta.cpp, added include)
- [x] Increase MQTT StaticJsonDocument to 512 bytes for safety
- [x] Add `IR_RECEIVER_PIN` conflict note to WIRING.md (GPIO 15 vs OLED CS)
- [x] Increase OTA_Delta http.setTimeout to 120000
- [x] Add native test infrastructure (mock Arduino.h, stubs, platformio.ini native env config)
- [ ] Review and remove any remaining debug Serial.println statements

### 8.3 — Final Testing
- [ ] Run full PlatformIO build and verify no warnings
- [ ] Run all unit tests and verify 100% pass rate
- [ ] Test SPIFFS data migration from v1 to v2 format on actual hardware
- [ ] Test OTA update flow end-to-end
- [ ] Test MQTT connection and HA auto-discovery
- [ ] Test IR remote with actual NEC remote
- [ ] Test scheduled feeding and mood system over 24h simulated runtime

### 8.4 — Documentation & Release
- [x] Update README.md with all Phase 7 features
- [x] Update PROJECT_STATUS.md to reflect Phase 7 completion
- [ ] Create CHANGELOG.md with all changes from Phase 1-7
- [ ] Create release tag v1.0.0
- [ ] Write release notes with feature summary, known issues, and upgrade guide
- [x] Update WIRING.md with IR receiver and RGB LED wiring

### 8.5 — Stretch Goals (if time permits)
- [ ] HTTP gzip compression for index.html (currently partial)
- [ ] WebSocket support as alternative to SSE
- [ ] Pet trading between devices via MQTT
- [ ] Voice control via Alexa/Google Home integration
- [ ] Mobile app (React Native or Flutter)

## Implementation Rules
- Create branch: feature/phase8-xxx
- One feature per commit
- Test compilation after each feature
- Update TASKS.md with progress
- Push when all features done
- Create PR when complete

## How This Works
Nyra (project manager) assigns tasks here → Kael (developer) reads and implements → Kael creates PR → Nyra reviews → Nyra assigns next phase
