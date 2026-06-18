# Changelog

All notable changes to ESP32-TamaPetchi are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.0.0] - 2026-06-18

### Added

#### Phase 1 — Code Modularization
- Refactored monolithic 1437-line `.ino` file into modular architecture
- Created separate modules: `Pet`, `Storage`, `WebHandlers`, `config.h`
- PlatformIO project structure with proper build configuration

#### Phase 2 — Evolution, Day/Night, Warnings
- Pet evolution system (Baby → Child → Adult → Elder) based on age
- Virtual day/night cycle with automatic sleep at night
- Low stat warning indicators
- Game improvements and stat decay balancing
- Bug fixes: feed health check, sick recovery, hungry recovery, sleep state timing, stat clamping

#### Phase 3 — Naming, Buzzer, OLED, Achievements, Pet Types
- Pet naming via web UI (`POST /name`)
- Buzzer sound effects for feed, play, death, wake events (`POST /mute` toggle)
- SSD1306 OLED display support (optional, `-DENABLE_OLED`)
- Achievements system: survived_1h/24h, fed_10x, played_10x, named_pet, reached_elder
- Multiple pet types: BLOB, CAT, DOG with SVG sprites per type
- Type selector UI and persistence

#### Phase 4 — Evolution Anim, Death/Revive, Games, Weather, Music, Settings
- Evolution animation with 3-second transition
- Pet death system with 30-second dying window and revive mechanic
- Mini-games: Memory sequence, Catch target, Quiz
- Dynamic weather system (sunny, cloudy, rainy, stormy, snowy) affecting mood
- Music/melody system with configurable per-event melodies
- Dark mode toggle in web UI (localStorage + device sync)
- Sound effect toggle and volume control
- Settings page with difficulty selection

#### Phase 5 — OTA, WiFi Manager, Multi-Pet, Stats, Notifications, Power
- OTA firmware updates via ArduinoOTA + web upload with password protection
- WiFi Manager with auto-connect and captive portal for first-time setup
- Multi-pet support: up to 3 pets with CRUD endpoints and pet selector UI
- Statistics dashboard: play time, feeds, plays, sleeps, cleans, heals, deaths, evolutions
- Per-pet notification slots with buzzer patterns and web UI badges
- Power management: deep sleep, battery ADC monitoring, WiFi power reduction

#### Phase 6 — Code Quality, Web UI, Hardware, Docs, Performance
- Bounds checking on all SPIFFS JSON parsing
- Watchdog timer (WDT) for hang detection and recovery
- SPIFFS wear leveling awareness (max save frequency: once per 5 min)
- Input validation on all HTTP endpoints (sanitize names, clamp values)
- Nullptr checks on all global pointers
- Responsive mobile-first CSS for web UI
- Server-Sent Events (SSE) for real-time pet stat updates
- Pet sprite SVG animations (walk, sleep, eat cycles)
- Physical button support (GPIO 0 BOOT button: feed/play/clean/sleep)
- RGB LED status indicator (green=healthy, yellow=warning, red=critical, blue=sleeping)
- Battery level display on OLED
- Deep sleep wake-on-button with RTC state restore
- User-selectable buzzer melodies per event
- PlatformIO unit tests for Pet state machine (38 tests)
- SPIFFS data migration tests (10 tests)
- Hardware wiring diagram (`WIRING.md`)
- User setup guide (`SETUP_GUIDE.md`)
- Heap usage logging for memory leak detection
- StaticJsonDocument optimization (25 DynamicJsonDocument → StaticJsonDocument)
- HTTP gzip compression for index.html
- WiFi power reduction in idle mode
- Compile-time feature flags for flash usage optimization

#### Phase 7 — Bug Fixes, Enhancements, MQTT, OTA Delta, IR, Mood
- Fixed multi-pet persistence (Phase 4/5 fields in load/save)
- Consolidated duplicate defines (MAX_PETS, OTA_PASSWORD, etc.)
- Removed unused WiFiManager lib dependency
- Bounds validation on all SPIFFS JSON parse results
- HTTP endpoint rate limiting (token bucket: 10 burst, 1/sec refill)
- Loading spinners for async web operations
- Confirmation dialogs for destructive actions (reset, delete pet)
- Improved API error responses with error codes
- SVG sprite animations for all pet types
- Scheduled feeding (configurable interval 1-24h, amount 5-50)
- Pet mood system (7 levels based on stats + personality traits: cheerful, energetic, hungry)
- IR remote control (NEC protocol): feed, play, clean, sleep, wake, sound toggle, pet switch
- MQTT smart home integration (Home Assistant auto-discovery: 6 sensors + 3 buttons)
- OTA delta updates (manifest-based, compressed firmware via SPIFFS staging)

#### Phase 8 — Code Cleanup & Release Preparation
- Removed redundant `updateStage()` call in `updatePet()`
- Consolidated `checkRateLimit` declaration to `WebHandlers.h`
- Increased MQTT StaticJsonDocument to 512 bytes for safety
- Added IR receiver pin conflict note to WIRING.md (GPIO 15 vs OLED CS)
- Increased OTA_Delta http.setTimeout to 120000ms
- Added native test infrastructure (mock Arduino.h, stubs, platformio.ini native env)
- Removed 30+ debug Serial.println statements from production code
- Full PlatformIO build verified: RAM 16.6%, Flash 74.6%, zero warnings

### Security
- OTA password protection
- HTTP input validation and sanitization
- Rate limiting on API endpoints
- SPIFFS data validation with bounds checking

### Documentation
- Comprehensive README with all features and API endpoints
- WIRING.md with complete hardware diagram including IR receiver and RGB LED
- SETUP_GUIDE.md for first-time configuration
- PROJECT_STATUS.md tracking all phases
- This CHANGELOG.md

### Known Issues
- IR_RECEIVER_PIN (GPIO 15) conflicts with OLED CS pin — use one or the other
- Unit tests require ESP32 toolchain for full hardware abstraction (native tests need additional stubs)
- MQTT broker must be configured at compile time via config.h

### Upgrade Guide
- **From v1 (monolithic .ino)**: Flash new firmware, SPIFFS will auto-migrate save data
- **From v2-v7**: OTA update preserves all settings and pet data
- First boot after flash: connect to "TamaPetchi" AP for WiFi configuration
