# ESP32-TamaPetchi — Autonomous Development Tasks

## Status
- Phase 1 ✅ Merged (Code modularization)
- Phase 2 ✅ Merged (Evolution, day/night, warnings)
- Phase 3 ✅ Merged (Naming, buzzer, OLED, achievements, pet types)
- Phase 4 ✅ Merged (Evolution anim, death/revive, games, weather, music, settings)
- Phase 5 ✅ Merged (OTA, WiFi Manager, Multi-Pet, Stats, Notifications, Power)
- Phase 6 ✅ Merged (Code quality, web UI, hardware features, docs, performance)
- Phase 7 ✅ Merged (Bug fixes, enhancements, MQTT, OTA delta, IR remote, mood)
- Phase 8 ✅ Complete (Code cleanup, merge, and release preparation)
- Phase 9 ✅ Complete (v1.0.0 release)
- Phase 10 ✅ Complete (v1.1 — AppState, WebSocket, i18n, factory reset, atomic writes, error codes)
- Phase 11 ✅ Complete (v1.1 — Advanced features)
- Phase 12 ✅ Complete (v1.2 features)
- Phase 13 ✅ Complete (v1.3 — OTA Delta, HAL, Community, Provisioning, Power Opt)
- Phase 14 ✅ Complete (v1.4 — Test fixes, OTA rollback, pet trading, sound packs, community docs)
- Phase 15 ✅ Complete (v1.5 — Flash optimization, backup/restore, advanced achievements, accessibility, community)

## Current Baseline (v1.5.0 — develop branch)
- **Build:** RAM 18.2%, Flash 80.7%, Zero warnings
- **Tests:** 162/162 native tests pass
- **Flash headroom:** ~4.3% before 85% limit (tight budget — optimize as needed)
- **Active branch:** feature/phase16-v1.6 (no code commits yet)

---

## Phase 16: v1.6.0 — Intelligence, Automation & Ecosystem Growth

**Branch:** feature/phase16-v1.6
**Goal:** Add AI-like automation features, expand ecosystem integrations, and improve developer experience.
**Budget:** Flash must stay under 85% (currently 80.7%, ~4.3% headroom — tight budget, optimize as needed).
**Priority:** Pet AI → Home Assistant integration → Developer tools → Release

### 16.0 — Pre-Work: Branch & Baseline
- [ ] Verify on branch: `git checkout feature/phase16-v1.6` (branch already exists)
- [ ] Verify clean build: `pio run -e esp32dev` — zero errors
- [ ] Verify baseline tests: `pio test -e native` — 162/162 pass
- [ ] Record baseline metrics: flash size, RAM usage, test count

### 16.1 — Pet AI: Adaptive Behavior Engine
- [ ] Create PetAI.h / PetAI.cpp module
- [ ] Implement adaptive hunger rate: pet gets hungry faster when active, slower when sleeping
- [ ] Implement mood-reactive behavior: cheerful pets gain happiness faster, hungry pets lose health faster
- [ ] Implement personality evolution: traits shift based on care patterns
- [ ] Add pet memory: track last 10 actions and adapt responses
- [ ] Add GET /api/pets/{id}/ai-status endpoint
- [ ] Add AI status panel in web UI (personality radar chart, memory log)
- [ ] Add unit tests for AI behavior triggers and personality evolution
- [ ] Verify compilation and flash budget (< 85%)

### 16.2 — Home Assistant Deep Integration
- [ ] Extend MQTT auto-discovery: binary sensor (alive/dead), binary sensor (sleeping/awake), sensor (mood), buttons (feed/play/clean/sleep), select (pet type)
- [ ] Add HA MQTT alarm control panel: pet health as alarm state
- [ ] Add HA automation blueprint: notify when pet needs feeding
- [ ] Create HA Lovelace card custom component
- [ ] Add GET /api/ha/config endpoint
- [ ] Update WIRING.md with HA integration guide
- [ ] Add unit tests for MQTT discovery message format

### 16.3 — Developer Experience: CLI Tool
- [ ] Create tools/tamapetchi-cli.py — Python CLI for device management
- [ ] Features: discover (mDNS), status, backup, restore, flash, simulate
- [ ] Add unit tests for CLI argument parsing and API client
- [ ] Add README.md in tools/ directory

### 16.4 — Web UI: Dashboard & Analytics
- [ ] Add Dashboard tab: health timeline chart, care score gauge, activity heatmap, achievement progress
- [ ] Reorganize Settings tab by category with search/filter
- [ ] Add Export Settings / Import Settings buttons
- [ ] Optimize WebSocket: batch stat updates (500ms interval)
- [ ] Add unit tests for care score calculation and ring buffer

### 16.5 — Data Export & Import
- [ ] Add GET /api/export/csv — export pet stats as CSV
- [ ] Add GET /api/export/full — export complete device state as JSON
- [ ] Add POST /api/import/settings — import settings JSON
- [ ] Add tools/migrate-data.py — convert backups between versions
- [ ] Add unit tests for CSV export and data migration

### 16.6 — Performance & Memory Audit
- [ ] Profile heap usage: heap logging every 60s over simulated 24h
- [ ] Optimize remaining DynamicJsonDocument in hot paths
- [ ] Audit String allocations — replace with const char* where possible
- [ ] Target: flash < 82%, RAM < 20%

### 16.7 — Release v1.6.0
- [ ] Final build verification: `pio run -e esp32dev` — zero errors, flash < 85%
- [ ] Final test run: `pio test -e native` — all tests pass
- [ ] Update README with Phase 16 features
- [ ] Update CHANGELOG.md with full v1.6.0 entry
- [ ] Update PROJECT_STATUS.md
- [ ] Create git tag: git tag v1.6.0
- [ ] Merge: git checkout develop && git merge feature/phase16-v1.6 --no-ff
- [ ] Merge: git checkout main && git merge develop --no-ff
- [ ] Push all branches: git push origin main develop --tags
- [ ] Create GitHub release with firmware.bin attached
- [ ] Write release notes

## Implementation Rules
- One feature per commit
- Test compilation after each feature
- Update TASKS.md with progress after each sub-task
- Push incrementally — do not wait until all tasks complete
- Create PR when all 16.x sub-tasks complete

## How This Works
Nyra (project manager) assigns tasks here → Kael (developer) reads and implements → Kael creates PR → Nyra reviews → Nyra assigns next phase

---

## Phase 17: v1.7.0 — Mobile, Voice & Ecosystem Maturity

**Branch:** feature/phase17-v1.7
**Goal:** Expand to mobile platforms, add voice control, and mature the ecosystem for broader adoption.
**Budget:** Flash must stay under 85%. Aggressive optimization required — target net flash reduction before adding features.
**Priority:** PWA → Voice control → Analytics → Plugin system → Mobile companion → Release

### 17.0 — Pre-Work: Branch & Baseline
- [ ] Create branch: `git checkout -b feature/phase17-v1.7 develop` (after v1.6.0 merged)
- [ ] Verify clean build: `pio run -e esp32dev` — zero errors
- [ ] Verify baseline tests: `pio test -e native` — all pass
- [ ] Record baseline metrics

### 17.1 — Progressive Web App (PWA)
- [ ] Create manifest.json: app name, icons (192x192, 512x512), theme color, display: standalone
- [ ] Create service worker: cache static assets (HTML, CSS, JS, icons), network-first for API calls
- [ ] Add "Install App" button in web UI (beforeinstallprompt event)
- [ ] Add offline page: show cached pet status, "reconnect" button
- [ ] Add push notifications via Web Push API (pet needs attention)
- [ ] Update index.html with manifest link and service worker registration
- [ ] Verify Lighthouse PWA score ≥ 90
- [ ] Add unit tests for service worker cache strategies (native mock)

### 17.2 — Voice Control Integration
- [ ] Add Alexa Smart Home skill: discover device, feed/play/clean/sleep directives
- [ ] Add Google Home integration: Smart Home Action, local fulfillment
- [ ] Implement voice command parsing on device: "feed my pet", "play with [name]", "put to sleep"
- [ ] Add GET /api/voice/status — return pet state in voice-assistant-friendly format
- [ ] Add POST /api/voice/command — accept structured voice commands
- [ ] Handle voice-specific edge cases: pet name recognition, confirmation responses
- [ ] Add unit tests for voice command parsing

### 17.3 — Advanced Analytics & Insights
- [ ] Add care pattern analysis: average feed interval, play frequency, sleep duration
- [ ] Add pet health prediction: estimate time until next feeding/sleep needed
- [ ] Add weekly/monthly care reports: total actions, health trends, achievement progress
- [ ] Add GET /api/analytics/care-patterns endpoint
- [ ] Add GET /api/analytics/predictions endpoint
- [ ] Add GET /api/analytics/reports/weekly and /monthly endpoints
- [ ] Add Analytics tab in web UI: charts (Chart.js), predictions panel, report download
- [ ] Add unit tests for prediction algorithms and report generation

### 17.4 — Plugin System
- [ ] Define plugin interface: init(), loop(), onEvent(event_t), getInfo()
- [ ] Create PluginManager.h / PluginManager.cpp: load plugins from SPIFFS, lifecycle management
- [ ] Create example plugin: Holiday Events (special events for holidays)
- [ ] Create example plugin: Weather Integration (fetch weather, affect pet mood)
- [ ] Add POST /api/plugins/upload — upload plugin binary (max 8KB)
- [ ] Add GET /api/plugins — list installed plugins
- [ ] Add POST /api/plugins/{id}/enable and /disable
- [ ] Add plugin management section in web UI
- [ ] Add unit tests for plugin lifecycle and event dispatch

### 17.5 — Mobile Companion App (React Native)
- [ ] Scaffold React Native project: `npx react-native init TamaPetchiCompanion`
- [ ] Implement device discovery: mDNS scan for `_http._tcp` on local network
- [ ] Implement pet status screen: real-time stats via WebSocket, pet sprite display
- [ ] Implement action buttons: feed, play, clean, sleep (POST to device API)
- [ ] Implement notifications: background fetch for pet alerts (every 15 min)
- [ ] Add settings screen: device IP, notification preferences, theme
- [ ] Test on iOS simulator and Android emulator
- [ ] Document build/run in companion/README.md

### 17.6 — Ecosystem Maturity
- [ ] Write blog post: "Building a Smart Tamagotchi with ESP32 — Full Series"
- [ ] Create video demo: all features walkthrough (YouTube)
- [ ] Submit to Hackaday.io project
- [ ] Create PlatformIO library registry entry
- [ ] Add Fritzing diagram for complete wiring
- [ ] Write troubleshooting guide: common issues, FAQ
- [ ] Add benchmark results: boot time, API response time, memory usage over 24h

### 17.7 — Release v1.7.0
- [ ] Final build verification: `pio run -e esp32dev` — zero errors, flash < 85%
- [ ] Final test run: `pio test -e native` — all tests pass
- [ ] Update README with Phase 17 features
- [ ] Update CHANGELOG.md with full v1.7.0 entry
- [ ] Update PROJECT_STATUS.md
- [ ] Create git tag: git tag v1.7.0
- [ ] Merge: git checkout develop && git merge feature/phase17-v1.7 --no-ff
- [ ] Merge: git checkout main && git merge develop --no-ff
- [ ] Push all branches: git push origin main develop --tags
- [ ] Create GitHub release with firmware.bin attached
- [ ] Write release notes

## Implementation Rules
- One feature per commit
- Test compilation after each feature
- Update TASKS.md with progress after each sub-task
- Push incrementally
- Create PR when all 17.x sub-tasks complete

## How This Works
Nyra (project manager) assigns tasks here → Kael (developer) reads and implements → Kael creates PR → Nyra reviews → Nyra assigns next phase
