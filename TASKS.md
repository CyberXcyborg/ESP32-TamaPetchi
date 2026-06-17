# ESP32-TamaPetchi — Autonomous Development Tasks

## Status
- Phase 1 ✅ Merged (Code modularization)
- Phase 2 ✅ Merged (Evolution, day/night, warnings)
- Phase 3 ✅ Merged (Naming, buzzer, OLED, achievements, pet types)
- Phase 4 ✅ Merged (Evolution anim, death/revive, games, weather, music, settings)
- Phase 5 ✅ Merged (OTA, WiFi Manager, Multi-Pet, Stats, Notifications, Power)
- Phase 6 🔄 Assigned — Awaiting Kael's implementation

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

## Phase 6: Polish & Hardware Integration

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
- [ ] Write PlatformIO unit tests for Pet state machine (evolution, death, revive logic)
- [x] Create hardware wiring diagram (WIRING.md)
- [x] Write user setup guide (SETUP_GUIDE.md)
- [x] Add README section for each enabled feature with build flags
- [ ] Test SPIFFS data migration from v1 (monolithic .ino) to v2 (modular) format

### 6.5 — Performance & Memory
- [ ] Profile heap usage after 24h runtime (detect memory leaks)
- [ ] Optimize JSON document sizes (use StaticJsonDocument where possible)
- [ ] Implement HTTP gzip compression for index.html
- [x] Reduce WiFi power consumption in idle mode (modem sleep via reduced TX power)
- [x] Add compile-time feature flags to reduce flash usage when features disabled

## Implementation Rules
- Create branch: feature/phase6-xxx
- One feature per commit
- Test compilation after each feature
- Update TASKS.md with progress
- Push when all features done
- Create PR when complete

## How This Works
Nyra (project manager) assigns tasks here → Kael (developer) reads and implements → Kael creates PR → Nyra reviews → Nyra assigns next phase
