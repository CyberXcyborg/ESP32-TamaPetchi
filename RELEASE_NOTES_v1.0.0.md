# TamaPetchi v1.0.0 Release Notes

**Release Date:** 2026-06-18
**Tag:** v1.0.0
**Commit:** ed155ba

## What's New

### Bug Fixes
- **feedPet health bonus**: Fixed health bonus check to evaluate hunger threshold BEFORE feeding (was checking after, making the bonus unreachable when hungry)
- **Native test infrastructure**: Fixed PlatformIO native test build configuration — all 61 unit tests now pass

### Test Coverage
- 61 unit tests covering:
  - Pet state machine (evolution, actions, death/revive)
  - Day/night cycle logic
  - Mini-games (memory, catch)
  - Stat clamping and bounds checking
  - SPIFFS data migration (v1→v5 format compatibility)
  - Enum validation and corrupted data handling

## Features (Complete)

### Core
- 🐣 Pet lifecycle: Baby → Child → Adult → Elder evolution
- 💀 Death system with 30-second dying window and revive mechanic
- 🐾 Multi-pet support (up to 3 pets with individual profiles)
- 🎭 Pet mood system (7 levels based on stats + personality traits)
- ⏰ Scheduled feeding (configurable 1-24h interval)

### Web Interface
- 🌐 Mobile-responsive web UI with dark mode
- 📊 Real-time stat updates via Server-Sent Events (SSE)
- 🏆 Achievements system with SVG sprite animations
- 🎮 Mini-games: Memory sequence, Catch target, Quiz
- 🌦️ Dynamic weather system (sunny, cloudy, rainy, stormy, snowy)

### Connectivity
- 📡 OTA firmware updates (full + delta) with password protection
- 📶 WiFi Manager with captive portal for first-time setup
- 🏠 MQTT smart home integration (Home Assistant auto-discovery: 6 sensors + 3 buttons)
- 📻 IR remote control (NEC protocol)

### Hardware
- 📺 SSD1306 OLED display support
- 🔘 Physical button support (GPIO 0 BOOT button)
- 💡 RGB LED status indicator (green/yellow/red/blue)
- 🔋 Battery monitoring with deep sleep support
- 🔊 Buzzer melodies (user-selectable per event)

### Quality
- ✅ Bounds checking on all SPIFFS JSON parsing
- ✅ Watchdog timer for hang detection
- ✅ HTTP endpoint rate limiting (token bucket)
- ✅ SPIFFS wear leveling awareness
- ✅ Input validation on all HTTP endpoints
- ✅ StaticJsonDocument optimization
- ✅ HTTP gzip compression

## Build Metrics
- **RAM:** 16.6% (54,556 / 327,680 bytes)
- **Flash:** 74.6% (977,861 / 1,310,720 bytes)
- **Unit Tests:** 61 passed ✅

## File Structure
```
src/
├── ESP32-TamaPetchi.ino    # Main entry point
├── config.h                 # All configuration constants
├── Pet.h / Pet.cpp          # Pet state, evolution, actions, games, music
├── Storage.h / Storage.cpp  # SPIFFS persistence
├── WebHandlers.h/.cpp       # HTTP API routes
├── Achievements.h/.cpp      # Achievement tracking
├── OLED.h / OLED.cpp        # Display driver
├── MultiPet.h/.cpp          # Multi-pet management
├── Stats.h / Stats.cpp      # Statistics tracking
├── Notifications.h/.cpp     # Notification system
├── PowerManager.h/.cpp      # Power management & battery
├── WiFiManager.h/.cpp       # WiFi connection management
├── OTA.h / OTA.cpp          # Over-the-air updates
├── OTA_Delta.h/.cpp         # Delta update system
├── MQTT.h / MQTT.cpp        # MQTT smart home integration
├── IRRemote.h / IRRemote.cpp # IR remote control
├── Buttons.h / Buttons.cpp  # Physical button support
├── RGB_LED.h / RGB_LED.cpp  # RGB LED indicator
└── data/
    └── index.html           # Web UI (SPIFFS)
```

## Known Issues
- IR_RECEIVER_PIN (GPIO 15) conflicts with OLED CS pin — use one or the other
- MQTT broker must be configured at compile time via config.h

## Upgrade Guide
- **From v1 (monolithic .ino):** Flash new firmware, SPIFFS will auto-migrate save data
- **From v2-v7:** OTA update preserves all settings and pet data
- First boot after flash: connect to "TamaPetchi" AP for WiFi configuration

## Testing Matrix
| Test | Status |
|------|--------|
| PlatformIO build (esp32dev) | ✅ SUCCESS |
| Unit tests (native, 61 tests) | ✅ PASSED |
| ESP32 hardware validation | ⬜ Pending physical device |
| OTA end-to-end | ⬜ Pending physical device |
| MQTT/HA integration | ⬜ Pending physical device |
| IR remote | ⬜ Pending physical device |
| 24h stability | ⬜ Pending physical device |

## Next Steps (Phase 9)
- Physical hardware validation
- v1.1 feature development (WebSocket, pet trading, sound packs, i18n)
- Community outreach (blog post, video demo, Hackaday submission)
