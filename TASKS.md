# ESP32-TamaPetchi — Autonomous Development Tasks

## Status
- Phase 1 ✅ Merged (Code modularization)
- Phase 2 ✅ Merged (Evolution, day/night, warnings)
- Phase 3 ✅ Merged (Naming, buzzer, OLED, achievements, pet types)
- Phase 4 ✅ Merged (Evolution anim, death/revive, games, weather, music, settings)
- Phase 5 ✅ Merged (OTA, WiFi Manager, Multi-Pet, Stats, Notifications, Power)
- Phase 6 ✅ Merged (Code quality, web UI, hardware features, docs, performance)
- Phase 7 ✅ Merged (Bug fixes, enhancements, MQTT, OTA delta, IR remote, mood)
- Phase 8 ✅ Complete — Code cleanup, merge, and release preparation
- Phase 9 ✅ Complete — v1.0.0 released, v1.1 features in development
- Phase 10 🔄 In Progress — 10.1 ✅, 10.2 ✅, 10.3 ✅, PR #11 approved (awaiting merge), 10.4-10.7 pending

## Phase 10: v1.1 Core Features Sprint

**Branch:** feature/phase10-v1.1-core
**Goal:** Implement the highest-impact v1.1 features that don't require hardware.
**Priority:** Architecture cleanup → WebSocket → i18n → Factory reset

### 10.1 — Architecture: Singleton AppState Refactor ✅ MERGED
- [x] Create AppState.h / AppState.cpp singleton class
- [x] Migrate all globals into AppState
- [x] Add AppState::getInstance() accessor
- [x] Update all modules to use AppState::getInstance() instead of extern globals
- [x] Verify compilation: pio run -e esp32dev — zero errors
- [x] Run unit tests: pio test -e native — all 61 tests pass

### 10.2 — WebSocket Real-Time Updates ✅ COMPLETE (PR #11 approved, awaiting merge)
- [x] Add WebSockets library to platformio.ini (links2004/WebSockets@2.4.1)
- [x] Create WebSocket.h / WebSocket.cpp module
- [x] Implement WebSocket server on port 81
- [x] Add real-time pet stat push (JSON broadcast on stat change)
- [x] Add real-time notification push (feed, play, clean, sleep, heal, reset)
- [x] Update web UI (data/index.html) to use WebSocket instead of SSE
- [x] Remove duplicate function declarations from WebHandlers.h
- [x] Replace g_server with APP_STATE.server in WebHandlers.cpp
- [x] Update WebHandlers to call WebSocket broadcasts
- [x] Verify compilation and memory usage (RAM 17.0%, Flash 76.2%)
- [x] Create PR #11 and get approval
- [ ] Add unit tests for WebSocket message serialization

### 10.3 — i18n Multi-Language Support ✅ COMPLETE (in PR #11)
- [x] Create i18n.h / i18n.cpp module
- [x] Create data/locales/en.json, data/locales/zh.json, data/locales/ja.json
- [x] Translate all web UI strings: nav labels, button text, status messages, error messages
- [x] Add language selector in web UI (stored in localStorage)
- [x] Add Accept-Language header detection for auto-select (with quality value parsing)
- [x] Add GET /api/settings/lang and POST /api/settings/lang endpoints
- [x] Add GET /api/locales/current endpoint
- [x] Verify SPIFFS data directory updated and buildfs succeeds
- [ ] Add unit tests for i18n string lookup and fallback

### 10.4 — Factory Reset
- [ ] Implement factory reset: hold BOOT button (GPIO 0) for 10 seconds
- [ ] Add Buttons::isFactoryResetPressed() — check button held > 10s on boot
- [ ] On trigger: wipe SPIFFS (SPIFFS.format()), reset WiFi credentials, restart
- [ ] Add visual feedback: RGB LED flashes red 3 times, OLED shows "Factory Reset"
- [ ] Add confirmation: require button hold during reset (not just on boot)
- [ ] Add POST /api/settings/factory-reset HTTP endpoint (software trigger)
- [ ] Verify compilation and test reset flow in native test environment

### 10.5 — SPIFFS Atomic Writes
- [ ] Create Storage::atomicWrite(filename, content) — write to .tmp, then rename
- [ ] Update all save functions to use atomicWrite:
  - [ ] savePetState() / loadPetState()
  - [ ] saveMultiPetState() / loadMultiPetState()
  - [ ] saveSettings() / loadSettings()
  - [ ] saveStats() / loadStats()
  - [ ] saveAchievements() / loadAchievements()
- [ ] Add write verification: read back after write, retry on mismatch
- [ ] Add unit tests for atomic write (mock SPIFFS rename failure)

### 10.6 — Error Code System
- [ ] Create ErrorCode.h enum with all error codes
- [ ] Update all API responses to include structured error objects
- [ ] Add error code documentation in README
- [ ] Add unit tests for error code propagation

### 10.7 — Documentation & Polish
- [ ] Update README with Phase 10 features (WebSocket, i18n, factory reset, atomic writes)
- [ ] Update CHANGELOG.md with Phase 10 changes
- [ ] Update TASKS.md with progress after each sub-task
- [ ] Create PR when all 10.x sub-tasks complete

---

## Phase 11: v1.1 Advanced Features Sprint

**Branch:** feature/phase11-advanced
**Goal:** Implement advanced v1.1 features: OTA rollback, web UI redesign, sound packs, pet trading.
**Priority:** OTA rollback → Web UI redesign → Sound packs → Pet trading

### 11.1 — OTA Rollback Support
- [ ] Implement dual-partition OTA with fallback (ESP32 OTA partition scheme)
- [ ] Add rollback detection: if new firmware crashes 3 times, revert to previous
- [ ] Store boot count in RTC memory for crash detection
- [ ] Add GET /api/ota/rollback endpoint to trigger manual rollback
- [ ] Add OTA status endpoint: GET /api/ota/status (current version, rollback available)
- [ ] Verify compilation and test rollback flow in native environment
- [ ] Add unit tests for rollback logic

### 11.2 — Web UI Redesign (WebSocket-Native)
- [ ] Redesign data/index.html with modern component-based UI
- [ ] Replace all SSE/polling code with WebSocket (ws://ip:81)
- [ ] Add WebSocket connection status indicator in UI (green dot = connected)
- [ ] Implement auto-reconnect with exponential backoff in JS client
- [ ] Add notification toast system for WebSocket notification events
- [ ] Add pet sprite animation using CSS/SVG with WebSocket-driven state updates
- [ ] Ensure mobile-first responsive design
- [ ] Test in Chrome, Firefox, Safari, mobile browsers

### 11.3 — Sound Pack System
- [ ] Create data/sounds/ directory for buzzer melody packs
- [ ] Define JSON schema for sound pack (name, melodies array, default flag)
- [ ] Create default sound pack: data/sounds/default.json
- [ ] Create cute sound pack: data/sounds/cute.json
- [ ] Add POST /api/sounds/upload endpoint for custom sound pack upload
- [ ] Add GET /api/sounds/list endpoint
- [ ] Add POST /api/sounds/select endpoint to choose active pack
- [ ] Update Buzzer module to load melodies from selected sound pack
- [ ] Add unit tests for sound pack parsing and selection

### 11.4 — Pet Trading via MQTT
- [ ] Define MQTT topic scheme for pet trading: tamapetchi/trade/request, tamapetchi/trade/accept, tamapetchi/trade/reject
- [ ] Add trade request UI in web UI (select pet, enter target device IP/topic)
- [ ] Implement trade protocol: request → accept → transfer pet data → confirm
- [ ] Serialize full pet state (stats, achievements, history) for transfer
- [ ] Add trade history log in SPIFFS
- [ ] Add security: trade PIN or confirmation required
- [ ] Add unit tests for trade protocol serialization/deserialization

### 11.5 — Performance Optimization
- [ ] Profile heap usage with WebSocket + i18n + all features enabled
- [ ] Optimize WebSocket JSON document sizes (use StaticJsonDocument where possible)
- [ ] Add HTTP ETag support for static resources (cache validation)
- [ ] Implement DNS caching for MQTT broker connection
- [ ] Reduce WebSocket broadcast overhead (only send on actual state change, not every 1s)
- [ ] Verify final memory budget: Flash < 85%, RAM < 50%

### 11.6 — Final Testing & Release Preparation
- [ ] Run full PlatformIO build with all features enabled
- [ ] Run all unit tests
- [ ] Create comprehensive integration test checklist
- [ ] Update README with all Phase 11 features
- [ ] Update CHANGELOG.md
- [ ] Create v1.1.0 release tag
- [ ] Write v1.1.0 release notes
- [ ] Merge develop → main for v1.1.0 release

## Implementation Rules
- Create branch: feature/phase11-xxx
- One feature per commit
- Test compilation after each feature
- Update TASKS.md with progress
- Push when all features done
- Create PR when complete

## How This Works
Nyra (project manager) assigns tasks here → Kael (developer) reads and implements → Kael creates PR → Nyra reviews → Nyra assigns next phase
