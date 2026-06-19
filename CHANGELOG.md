# Changelog

All notable changes to ESP32-TamaPetchi are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.1.0] - 2026-06-19

### Added

#### Phase 11 — v1.1 Advanced Features
- **OTA Rollback Support** (11.1): Dual-partition OTA with crash detection and fallback
  - Boot count tracking in RTC memory with XOR checksum validation
  - Auto-rollback after 3 consecutive crash boots
  - Manual rollback via `GET /api/ota/rollback` endpoint
  - OTA status endpoint: `GET /api/ota/status` (current version, rollback available)
  - Auto-confirm firmware after 5 minutes stable uptime
- **Web UI Redesign** (11.2): WebSocket-native modern interface
  - Full WebSocket integration (ws://ip:81) replacing SSE/polling
  - Auto-reconnect with exponential backoff in JS client
  - Toast notification system (success/info/warning/error variants)
  - Connection status indicator (green/red dot)
  - Dark mode with localStorage persistence
  - Mobile-first responsive design
  - SVG pet sprites with state-driven animations (normal, eating, playing, sleeping, sick, hungry, dying, dead, evolution)
  - Particle effects for feed/play/heal/sleep actions
- **Sound Pack System** (11.3): JSON-based buzzer melody packs
  - `data/sounds/` directory with JSON schema for melody definitions
  - Default and cute sound packs included
  - `POST /api/sounds/upload` — upload custom sound packs
  - `GET /api/sounds/list` — list available sound packs
  - `POST /api/sounds/select` — choose active sound pack
  - Active pack persistence in SPIFFS
- **Pet Trading via MQTT** (11.4): Trade pets between devices
  - MQTT topic scheme: `tamapetchi/trade/request`, `tamapetchi/trade/accept`, `tamapetchi/trade/reject`
  - Full trade protocol: request → accept → transfer pet data → confirm
  - Serialized pet state transfer (stats, achievements, history)
  - Trade history log in SPIFFS
  - Trade PIN/security confirmation required
- **Performance Optimization** (11.5): Memory and efficiency improvements
  - Compact JSON documents (StaticJsonDocument where possible)
  - HTTP ETag support for static resource caching
  - DNS caching for MQTT broker connection
  - State-change-only WebSocket broadcasts (no periodic spam)
  - Final memory budget: Flash 78.0%, RAM 17.1%

#### Phase 10 — v1.1 Core Features
- **WebSocket Real-Time Updates** (10.2): WebSocket server on port 81 replacing SSE for lower-latency updates
  - Real-time pet stat push (JSON broadcast on stat change)
  - Real-time notification push (feed, play, clean, sleep, heal, reset)
  - Web UI updated to use WebSocket instead of SSE
  - Auto-reconnect with exponential backoff in JS client
- **i18n Multi-Language Support** (10.3): English, Chinese, Japanese web UI
  - `GET /api/settings/lang` / `POST /api/settings/lang` endpoints
  - `GET /api/locales/current` endpoint
  - Accept-Language header detection with quality value parsing
  - Language selector in web UI (stored in localStorage)
- **Factory Reset** (10.4): Wipe all data and restart
  - Hold BOOT button (GPIO 0) for 10 seconds on boot
  - `POST /api/settings/factory-reset` HTTP endpoint (software trigger)
  - Visual feedback: RGB LED flashes red 3 times, OLED shows "Factory Reset"
  - Wipes SPIFFS, resets WiFi credentials, restarts device
- **SPIFFS Atomic Writes** (10.5): Crash-safe storage
  - Write to `.tmp` file, verify, then rename to final filename
  - 3 retry attempts on failure with read-back verification
  - All save functions updated: pet data, multi-pet, stats, achievements
- **Structured Error Code System** (10.6): Consistent API error responses
  - 40+ error codes across 11 categories (SPIFFS, JSON, Pet, WiFi, OTA, MQTT, Rate, Auth, Param, Memory, System)
  - All API errors return `{ "success": false, "error": "<code>", "message": "<description>" }`
  - Rate limit errors use `ERR_RATE_LIMIT` code

### Changed
- Web UI now uses WebSocket (port 81) as primary real-time transport (SSE kept for backward compatibility)
- All SPIFFS writes now use atomic write pattern (crash-safe on power loss)
- API error responses now include structured error codes

### Security
- Factory reset requires physical button hold (10s) or authenticated HTTP POST
- Atomic SPIFFS writes prevent data corruption on unexpected power loss

---

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
- OTA rollback requires ESP32 partition scheme with at least 2 OTA partitions (default most boards)
- Pet trading requires MQTT broker connection and both devices on same MQTT network

### Upgrade Guide
- **From v1 (monolithic .ino)**: Flash new firmware, SPIFFS will auto-migrate save data
- **From v2-v7**: OTA update preserves all settings and pet data
- **From v1.0.0**: OTA update preserves all settings and pet data; OTA rollback available if update causes issues
- **From v1.1.0+**: OTA rollback — if new firmware crashes 3 times, device auto-reverts to previous version
- First boot after flash: connect to "TamaPetchi" AP for WiFi configuration
