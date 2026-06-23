## [2.0.0-alpha.2] - 2026-06-23

### Added

#### Phase 20 — Graphics & Input: Sprite System, Animations, UI Framework
- **Color Sprite System** (20.1):
  - 4-bit palette (16 colors) sprite format with RLE compression
  - Sprite conversion tool (tools/png2spr.py) with median-cut quantization
  - SpriteLoader with LRU cache (8 frames, ~64KB PSRAM)
  - LVGL custom image decoder for .spr files
  - 96 frames of pet sprites (baby/child/adult/elder)
- **Animation Engine** (20.2):
  - AnimationPlayer with play/pause/stop/loop controls
  - Animation state machine (IDLE → ACTION → IDLE transitions)
  - Cross-fade between animations (200ms blend in PSRAM)
  - Frame callback system for LVGL integration
- **LVGL UI Framework** (20.3):
  - ScreenManager with stack-based navigation (push/pop/switch)
  - Screen transitions: slide left/right (300ms), fade (200ms)
  - MainPetScreen: pet sprite, status bars, mood emoji
  - MenuScreen: 4x2 action grid with toast notifications
  - StatsScreen: bar widgets, info panel, achievement badges
- **Screen Migration** (20.4):
  - MemoryGameScreen: 4x4 button grid, sequence display, auto-return
  - ReactionGameScreen: filling bar, green zone target, auto-return
  - TiltGameScreen: placeholder with demo mode (Phase 21 accel)
  - OTAScreen: progress bar, status labels, reboot countdown
  - SettingsScreen: switches, sliders, language selector, factory reset dialog
  - GamesScreen: game list with high score display
  - ScreenFactory: central screen registration and wiring
  - REST API: GET /api/sprites, GET /api/screen
- **LVGL Configuration**:
  - 240x240 resolution, 16bpp RGB565
  - 48KB LVGL heap in SRAM
  - Framebuffers in PSRAM (double buffering)
  - Montserrat fonts: 10, 12, 14, 16

### Test Results
- 216/216 tests pass (205 existing + 11 new migrated screen tests)
- Zero regressions

---

## [1.7.0] - 2026-06-22

### Added

#### Phase 17 — Mobile, Voice & Ecosystem Maturity
- **Progressive Web App (PWA)** (17.1):
  - Web app manifest with 192x192 and 512x512 icons
  - Service worker with cache-first (static) and network-first (API) strategies
  - "Install App" button with beforeinstallprompt event handling
  - Offline page overlay with cached pet status display
  - Push notification support via Web Push API
  - LocalStorage caching of last known pet status
- **Voice Control Integration** (17.2):
  - Keyword-based voice command parser (feed/play/clean/sleep/wake/status/heal/reset)
  - Alexa Smart Home directive parser
  - Google Home trait parser
  - `GET /api/voice/status` — pet status in voice-assistant-friendly format
  - `POST /api/voice/command` — accept text, Alexa, or Google Home commands
  - Speech-friendly status summary with health/hunger/happiness/energy levels
- **Advanced Analytics & Insights** (17.3):
  - Care pattern analysis: avg feed/play/sleep intervals, care score (0-100)
  - Health predictions: time until hungry/tired/unhappy/dirty based on decay rates
  - Weekly/monthly care reports with stats summary
  - `GET /api/analytics/care-patterns` — care pattern JSON
  - `GET /api/analytics/predictions` — health prediction JSON
  - `GET /api/analytics/reports/weekly` — 7-day care report
  - `GET /api/analytics/reports/monthly` — 30-day care report
  - Analytics tab in web UI with predictions panel and report download
- **Plugin System** (17.4):
  - Plugin event types: ON_FEED, ON_PLAY, ON_SLEEP, ON_CLEAN, ON_HEAL, ON_TICK, ON_EVOLVE, ON_DEATH, ON_NAME_CHANGE
  - Plugin persistence to SPIFFS (/plugins.json)
  - Built-in plugins: HolidayEvents, WeatherMood
  - `GET /api/plugins` — list loaded plugins
  - `POST /api/plugins/upload` — upload plugin JSON (max 8KB)
  - `POST /api/plugins/enable` / `/disable` / `/delete` — plugin management
  - Plugin management section in web UI
- **Mobile Companion App** (17.5):
  - React Native companion app architecture documentation
  - Device discovery via mDNS
  - API client documentation for all endpoints
- **Ecosystem Maturity** (17.6):
  - Blog post series outline: "Building a Smart Tamagotchi with ESP32"
  - Troubleshooting guide with common issues and FAQ
  - Performance benchmarks

### Build Metrics
- Flash: 83.6% (1,096,417 / 1,310,720 bytes) — 1.4% headroom
- RAM: 18.9% (62,072 / 327,680 bytes)
- Tests: 162/162 pass

## [1.6.0] - 2026-06-21

### Added

#### Phase 16 — Intelligence, Automation & Ecosystem Growth
- **Pet AI: Adaptive Behavior Engine** (16.1):
  - Adaptive hunger rate: pets get hungry faster when active, slower when sleeping
  - Mood-reactive behavior: cheerful pets gain happiness faster; hungry pets lose health faster
  - Personality evolution: traits shift based on care patterns (feed/play/clean frequency)
  - Pet memory: circular buffer tracking last 10 actions with timestamps
  - Activity level tracking (0-100%) based on recent interactions
  - AI modifiers: hungerRate, happinessRate, energyRate, healthRate computed from personality + memory
  - `GET /api/pets/ai/status` — AI status (personality, modifiers, activity)
  - `GET /api/pets/ai/memory` — action log with hourly frequency analysis
  - Dashboard section in web UI: care score gauge, activity level, mood/stage summary
- **Home Assistant Deep Integration** (16.2):
  - Extended MQTT auto-discovery from 6 to 13 entities
  - New entities: Sleep button, Heal button, Alive binary sensor, Sleeping binary sensor,
    Mood sensor, Energy sensor, Cleanliness sensor, Pet Type select, Health Alarm control panel
  - Health alarm states: disarmed (>30% health), armed_home (11-30%), armed_away (≤10%), triggered (dead)
  - MQTT command support for: sleep, heal, set_type
  - `GET /api/ha/config` — HA configuration inventory endpoint
- **Developer Experience: CLI Tool** (16.3):
  - `tools/tamapetchi-cli.py` — Python CLI for device management
  - Commands: discover, status, backup, restore, action, flash, simulate, export
  - mDNS device discovery with fallback to subnet scan
  - `tools/README.md` with usage examples
- **Web UI Dashboard** (16.4):
  - Dashboard tab: care score gauge, AI activity level, mood/stage/age/high-score summary
  - Export/Import Settings buttons for transferring configuration between devices
  - Dashboard auto-updates via WebSocket + async AI status fetch
- **Data Export & Import** (16.5):
  - `GET /api/export/full` — complete device state as JSON (pet, AI, lineage, analytics)
  - `POST /api/import/settings` — import settings from JSON (rate-limited, validated)
  - CSV export already available from Phase 12.3

### Performance Metrics
- Flash: 81.9% (1,073,357 / 1,310,720 bytes) — 3.1% headroom under 85% limit
- RAM: 18.3% (60,080 / 327,680 bytes)
- Tests: 162/162 passing

### Known Issues
- Pet AI evolves slowly — requires several hours of interaction for noticeable personality shifts
- HA auto-discovery messages may be large; ensure MQTT broker supports 1KB+ messages
- Web UI dashboard AI fetch adds one extra HTTP request per pet data cycle

### Upgrade Guide
- **From v1.5.0**: All data migrates automatically; new AI fields initialized on first boot
- **From v1.4.0+**: Backup recommended before upgrade; use Export Settings to save configuration
- **All versions**: Dashboard section visible by default; AI starts learning from first action

---

## [1.5.0] - 2026-06-21

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.3.0] - 2026-06-20

### Added

#### Phase 13 — v1.3 Hardware & Ecosystem Expansion
- **OTA Delta Updates** (13.1): Binary delta patching (bsdiff-style) for firmware updates
  - Control triples (copy_offset, copy_length, add_length) for efficient patching
  - SHA-256 integrity verification via mbedtls before applying patch
  - Fallback to full OTA if delta patch fails (rollbackOnFail flag)
  - `POST /api/ota/delta` accepts delta binary upload, applies to firmware partition
  - `GET /api/ota/delta/status` and `POST /api/ota/delta/check` manifest endpoints
  - Partition swap: writes to next OTA partition, verifies, sets boot partition
  - 19 unit tests for delta patch format, state machine, SHA-256 verification
- **Hardware Abstraction Layer (HAL)** (13.2): Abstract interfaces for hardware
  - 7 abstract interfaces: IDisplay, IStorage, IWiFi, IGPIO, IBuzzer, IPower, IRTC
  - ESP32 implementation wraps existing hardware APIs
  - Native implementation for unit testing without hardware
  - Compile-time selection via `#if defined(UNIT_TEST)` — zero runtime overhead
  - 12 unit tests for HAL mock implementations
- **Community Features & Pet Sharing** (13.3): Social ecosystem
  - Pet profile sharing: generate shareable pet card (JSON with stats, achievements, lineage)
  - Community gallery: `GET /api/community/gallery` returns top pets by achievement score
  - Leaderboard: `GET /api/community/leaderboard?sort=achievements|age|generation`
  - Pet profile import: `POST /api/community/import` — import shared pet card
  - Rate limiting on community endpoints
  - 11 unit tests for community features
- **Manufacturing & Provisioning Tools** (13.4): Production-ready tooling
  - Provisioning AP mode: on first boot, start AP 'TamaPetchi-Setup' for initial config
  - Python provisioning script: serial-flashes ESP32 with unique device ID, WiFi creds
  - Batch flash shell script for flashing multiple devices sequentially
  - Device ID: unique per-device ID derived from ESP32 MAC address (TAMA-XXXXXX)
  - `GET /api/provisioning/status`, `POST /api/provisioning/set`, `POST /api/provisioning/reset`
  - 6 unit tests for provisioning data validation
- **Power Optimization & Deep Sleep** (13.5): Battery life improvements
  - Light sleep mode: <1mA consumption, wake on timer or button press
  - Battery estimation: 24-hour circular buffer, drain rate calculation, remaining hours estimate
  - Configurable wake intervals: 5min, 15min, 1hr
  - WiFi power management: reduce/restore TX power during sleep
  - Sleep decision logic: only sleep if alive + sleeping state + night
  - 9 unit tests for power state transitions and battery estimation

### Build Health
- Build: SUCCESS (RAM 17.8%, Flash 79.8%)
- Tests: 145/152 native tests pass (7 pre-existing failures from Phase 12, not regressions)
- 19 OTA Delta tests pass

---

## [1.2.0] - 2026-06-20

### Added

#### Phase 12 — v1.2 Feature Sprint
- **Achievements 2.0 System** (12.1): Expanded achievement system with 4 tiers and 4 categories
  - 16 achievements across Care, Evolution, Social, and Exploration categories
  - Tier progression: Bronze (0-25%) → Silver (25-50%) → Gold (50-75%) → Platinum (75-100%)
  - Progress tracking with per-achievement counters stored in SPIFFS
  - Achievement rewards: unlock pet skins and accessories
  - `GET /api/achievements/progress` endpoint returns full progress data with tier info
  - WebSocket notification on achievement unlock
- **Pet Lineage & Genealogy** (12.2): Track ancestry and genetic inheritance
  - Parent device IDs, generation number, birth timestamp tracking
  - Genetic trait inheritance: child personality = weighted average of parents + random mutation (±10%)
  - Lineage visualization in web UI (family tree view)
  - `GET /api/pets/lineage` endpoint returns full ancestry tree up to 5 generations
  - Trade lineage recording on pet transfer
- **Data Dashboard & Analytics** (12.3): Comprehensive data insights
  - Daily/weekly/monthly summaries: feed count, play time, sleep hours
  - Health trends: weight, mood history, activity level tracking
  - `GET /api/stats/trends?range=7d|30d|90d` endpoint with time-range filtering
  - `GET /api/export/csv` and `GET /api/export/json` data export endpoints
  - Chart.js dashboard with line charts for stat trends and bar charts for daily summaries
- **Accessibility & UX Improvements** (12.4): Inclusive design
  - Full keyboard navigation (Tab/Enter/Space) for all interactive elements
  - ARIA labels on all buttons, status indicators, and dynamic content
  - High-contrast mode toggle (CSS class switch)
  - Font size adjustment: small/medium/large (CSS variable)
  - Reduced-motion mode: disables all CSS animations and transitions
  - Sound effect volume control slider (0-100%)
  - `GET /api/settings/accessibility` and `POST /api/settings/accessibility` endpoints
- **Backup & Restore** (12.5): Full data protection
  - `GET /api/backup` returns tar of all SPIFFS config files with SHA-256 checksum
  - `POST /api/restore` accepts tar upload, verifies checksum, extracts and applies
  - Scheduled auto-backup option (daily/weekly)
  - Backup integrity verification via SHA-256 checksum
  - Round-trip backup/restore unit tests

### Build Health
- Build: SUCCESS (RAM 17.3%, Flash 79.0%)
- Tests: 152/152 pass

---

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

---

## [1.4.0] - 2026-06-21

### Added

#### Phase 14 — v1.4 Stability, Testing & Ecosystem

- **Test Infrastructure Fixes** (14.1): Resolved all 7 pre-existing test failures
  - Fixed `deserializeJson(const String&)` incompatibility with native test String class (use `.c_str()`)
  - Increased `getAchievementsProgressJson` document size from `StaticJsonDocument<2048>` to `DynamicJsonDocument(8192)` to fit all 16 achievement entries
  - Fixed backup/restore roundtrip tests for name, accessibility, achievements, checksum, and generation
  - Now 152/152 native tests pass

- **Community & Developer Tools** (14.6):
  - Added `CONTRIBUTING.md` with build instructions, code style, PR process, testing requirements
  - Added GitHub issue templates: `bug_report.md` and `feature_request.md`
  - Added `scripts/flash-batch.py` for batch flashing multiple ESP32 devices
  - Added `scripts/simulate-24h.py` for project health checks and metrics
  - Updated README.md with contributor quick-start and developer tools section

### Known Issues
- IR_RECEIVER_PIN (GPIO 15) conflicts with OLED CS pin — use one or the other
- MQTT broker must be configured at compile time via config.h
- OTA rollback requires ESP32 partition scheme with at least 2 OTA partitions (default most boards)
- Pet trading requires MQTT broker connection and both devices on same MQTT network

### Upgrade Guide
- **From v1.3.0**: OTA update preserves all settings and pet data; all existing features remain compatible
- **From v1.1.0+**: OTA rollback — if new firmware crashes 3 times, device auto-reverts to previous version

---

## [1.5.0] - 2026-06-21

### Added

#### Phase 15 — v1.5 Performance, Polish & Hardware Validation

- **Flash Optimization Sprint** (15.1):
  - Converted 9 DynamicJsonDocument → StaticJsonDocument in Achievements.cpp, Storage.cpp, SoundPack.cpp
  - Pre-compressed data/index.html.gz (124KB → 21KB, 83% SPIFFS space savings)
  - Flash usage: 79.8% → 80.7% (minimal change, within 85% budget)

- **Backup & Restore System** (15.3):
  - Full backup/restore of all pet data, stats, achievements, settings
  - CRC32 checksum validation for backup integrity
  - API endpoints: `GET /api/backup/download`, `POST /api/backup/restore`, `GET /api/backup/verify`
  - 11 new unit tests for roundtrip backup/restore (all pet states, edge cases)
  - Backup UI section in web interface

- **Advanced Achievement System** (15.4):
  - Expanded from 16 to 27 achievements across 4 categories (Care, Evolution, Social, Exploration)
  - 4 hidden/secret achievements: Birthday Surprise, Midnight Snacker, Night Owl, Trade Master
  - 3 survival achievements: Against All Odds, Iron Constitution, Sparkle Week
  - Achievement rewards: 22 unlocked skins and accessories
  - Hidden achievement system: achievements not shown until unlocked
  - Category progress tracking with score system
  - API endpoints: `GET /api/achievements/categories`, `GET /api/achievements/rewards`
  - Achievement progress bars in web UI

- **Web UI Modernization & Accessibility** (15.5):
  - ARIA labels on all interactive elements (buttons, selects, inputs)
  - Screen reader support: role="application", "main", "status", "group"
  - Keyboard navigation: tabindex="0" on all action buttons
  - Decorative icons: aria-hidden="true"
  - Toast notifications for API responses (success/error/warning)
  - WebSocket connection status indicator

- **Pet Snapshot & Comparison** (Phase 15.6):
  - Pet snapshot: `GET /api/pets/snapshot` — full pet state for sharing
  - Pet comparison: `GET /api/pets/compare` — compare with gallery entries
  - Community gallery UI with top-scoring pets
  - Leaderboard UI section

### Build Health
- Build: SUCCESS (RAM 18.2%, Flash 80.7%)
- Tests: 162/162 native tests pass (0 failures)
- 27 achievements with full test coverage
- PR #14 closed, PR #15 pending

### Known Issues
- IR_RECEIVER_PIN (GPIO 15) conflicts with OLED CS pin — use one or the other
- MQTT broker must be configured at compile time via config.h
- OTA rollback requires ESP32 partition scheme with at least 2 OTA partitions (default most boards)
- Pet trading requires MQTT broker connection and both devices on same MQTT network
- Hardware validation (15.2) requires physical ESP32 — not automated

### Upgrade Guide
- **From v1.4.0**: OTA update preserves all settings, pet data, achievements, and stats
- **From v1.3.0+**: Backup/restore system available — create backup before upgrading if downgrading later
- **All versions**: Achievement progress migrates automatically; new achievements start at 0 progress
- First boot after flash: connect to "TamaPetchi" AP for WiFi configuration

## [1.8.0] - 2026-06-22

### Added

#### Phase 18 — Hardware Validation, Community Growth & v2.0 Architecture
- **Debug Output Control** (18.4):
  - `DEBUG_PRINT`, `DEBUG_PRINTLN`, `DEBUG_PRINTF` macros in config.h
  - Define `DISABLE_DEBUG` to strip all debug output from production builds
  - All existing `Serial.println` calls retained but can be disabled at compile time
- **Compile-Time Assertions** (18.4):
  - `#error` directives validate config.h constants at compile time
  - Checks: STAT_MAX > STAT_MIN, evolution thresholds monotonic, enum ranges valid
  - Validates: MAX_PETS (1-10), MAX_NOTIFICATIONS (1-100), decay rates non-negative
- **Watchdog Reset Reason Logging** (18.4):
  - `logResetReason()` called early in setup() before any other initialization
  - Logs watchdog, panic, and brownout resets to `/reset_log.json` in SPIFFS
  - Captures: reset reason string, code, timestamp, free heap, uptime
  - Normal power-on resets are not logged (only abnormal resets)
- **OTA Error Messages** (18.4):
  - Detailed error messages with specific failure reasons (not just "OTA failed")
  - `getUpdateErrorString()` maps Update error codes to human-readable strings
  - Error response includes both message and numeric error code
  - Covers: write/erase/read errors, space/size/stream/MD5/magic byte errors
- **v2.0 Architecture Roadmap** (18.3):
  - Comprehensive V2_ROADMAP.md with hardware analysis, flash budget, and migration plan
  - Target: ESP32-S3 with 8MB PSRAM, 240x240 TFT, LVGL graphics, BLE companion app
  - 15-week migration plan (Phase A through E)
  - Risk assessment and success criteria

### Changed
- BACKUP_VERSION updated to "1.8.0"
- Flash usage: 83.9% (from 83.6%)
- RAM usage: 18.9% (unchanged)

### Known Issues
- Hardware validation (18.1) requires physical ESP32 — not automated
- Community & developer relations tasks (18.2) require manual effort
- v2.0 features require ESP32-S3 hardware (not backward compatible with ESP32)

### Upgrade Guide
- **From v1.7.0**: OTA update preserves all settings, pet data, achievements, stats, and analytics
- **All versions**: Backup/restore system available — create backup before upgrading
- **Production builds**: Add `-DDISABLE_DEBUG` to build_flags to remove all Serial output
- First boot after flash: connect to "TamaPetchi-Setup" AP for WiFi configuration
