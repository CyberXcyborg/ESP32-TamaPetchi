# ESP32-TamaPetchi — Autonomous Development Tasks

## Status
- Phase 1 ✅ Merged (Code modularization)
- Phase 2 ✅ Merged (Evolution, day/night, warnings)
- Phase 3 ✅ Merged (Naming, buzzer, OLED, achievements, pet types)
- Phase 4 ✅ Merged (Evolution anim, death/revive, games, weather, music, settings)
- Phase 5 ✅ Merged (OTA, WiFi Manager, Multi-Pet, Stats, Notifications, Power)

## Phase 5: Advanced Features — COMPLETE ✅

### 1. OTA (Over-The-Air) Updates ✅
- ArduinoOTA support with password protection (default: tamapetchi)
- Web UI status endpoint at GET /ota/status
- Auto-reboot after successful update

### 2. WiFi Manager ✅
- Auto-connect to stored WiFi credentials
- Fallback AP mode: "TamaPetchi-Setup" with captive portal
- Credentials stored in SPIFFS (/wifi_config.json)
- POST /wifi/reset to clear credentials and restart

### 3. Multi-Pet Support ✅
- Up to 3 pets stored in SPIFFS (/multi_pet.json)
- GET /pets — list all pets
- POST /pets/switch — switch active pet
- POST /pets/create — create new pet
- POST /pets/delete — delete a pet

### 4. Statistics Dashboard ✅
- Track: total play time, times fed, times played, times slept, high score
- GET /stats endpoint with full statistics
- Daily/weekly active minutes tracking
- Persistent storage in SPIFFS (/stats.json)

### 5. Notification System ✅
- Distinct buzzer patterns per event type (low health, evolution, achievement, death, etc.)
- Visual notification badge support
- GET /notifications endpoint
- POST /notifications/clear
- Persistent storage in SPIFFS (/notifications.json)

### 6. Power Management ✅
- Battery level monitoring via ADC pin (configurable)
- GET /battery endpoint
- Low battery warning notification
- Deep sleep when pet sleeping + energy full (wake on BOOT button)
- WiFi power reduction when idle

## Phase 6: Future — Awaiting Nyra's Assignment

## Implementation Rules
- Create branch: feature/phase6-xxx
- One feature per commit
- Test compilation after each feature
- Update TASKS.md with progress
- Push when all features done
- Create PR when complete

## How This Works
Nyra (project manager) assigns tasks here → Kael (developer) reads and implements → Kael creates PR → Nyra reviews → Nyra assigns next phase
