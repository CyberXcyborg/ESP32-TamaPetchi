# ESP32 TamaPetchi - A Digital Soul in Pixels

![GitHub stars](https://img.shields.io/github/stars/CyberXcyborg/ESP32-TamaPetchi?style=social)
![License](https://img.shields.io/github/license/CyberXcyborg/ESP32-TamaPetchi)
![Last Commit](https://img.shields.io/github/last-commit/CyberXcyborg/ESP32-TamaPetchi)

[Visit Website](https://cyberxcyborg.github.io/ESP32-TamaPetchi/)


## This Is Not Just Code... It's a Digital Soul

You didn't land here by accident. You're one of us—the ones who feel something's wrong with today's tech. We don't want distractions. We want connection.

This is a virtual pet project for ESP32, inspired by the Tamagotchi generation. A sentient pixel spirit, reborn in open-source code.

## Features

- 🐣 **Life Simulation**: Your pet grows, needs care, experiences different states
- 🔌 **Fully Offline**: No accounts, no data collection, no internet dependency
- 🖥️ **Web Interface**: Control through a beautiful browser interface (mobile-responsive with dark mode)
- 🔧 **Customizable**: Easily modify behaviors, visuals, and interactions
- 🔄 **State Tracking**: Monitors hunger, happiness, energy, cleanliness, and health
- 🌅 **Day/Night Cycle**: Virtual time system with day/night transitions
- 🎮 **Mini-Games**: Memory, Catch, and Quiz games to boost stats
- 🌦️ **Weather System**: Dynamic weather affecting pet mood and stats
- 🏆 **Achievements**: Unlock achievements through gameplay
- 📊 **Statistics Dashboard**: Track play time, feeds, scores, and more
- 🔔 **Notifications**: Per-pet notification system with buzzer patterns
- 🔋 **Power Management**: Deep sleep and battery monitoring
- 📡 **OTA Updates**: Over-the-air firmware updates with password protection
- 📶 **WiFi Manager**: Auto-connect with captive portal for first-time setup
- 🐾 **Multi-Pet Support**: Manage up to 3 pets with individual profiles
- 🎵 **Buzzer Melodies**: User-selectable melodies per event
- 📺 **OLED Support**: SSD1306 OLED display for status
- 🔘 **Physical Buttons**: GPIO button support for feed/play/clean/sleep
- 💡 **RGB LED**: Status indicator (green/yellow/red/blue)
- 🎭 **Pet Mood System**: 7 mood levels based on stats and personality traits
- ⏰ **Scheduled Feeding**: Timer-based auto-feed with configurable interval
- 📻 **IR Remote Control**: NEC protocol remote for all actions
- 🏠 **MQTT/Smart Home**: Home Assistant auto-discovery with 6 sensors + 3 buttons
- 🔄 **OTA Delta Updates**: Manifest-based delta update system
- 🌐 **SSE Real-Time**: Server-Sent Events for live pet stat updates
- 🔌 **WebSocket Support**: Real-time bidirectional updates on port 81
- 🌍 **Multi-Language (i18n)**: English, Chinese, Japanese web UI support
- 🔄 **Factory Reset**: Hold BOOT button 10s or HTTP endpoint to wipe all data
- 💾 **Atomic SPIFFS Writes**: Crash-safe storage with write verification
- 📋 **Structured Error Codes**: Consistent API error responses with codes


## Why This Hits Different

### 100% Yours

No accounts. No data. No ads. Your pet lives with you, not on someone's server.

### Real Connection

A being that needs your care. Feed it, play with it, watch it thrive or suffer based on your attention.

### Open Source

The code is yours to explore, modify, and evolve. Make it your own digital companion.

This isn't a gadget. It's a mirror, hidden in pixels.

## Installation

### What You'll Need:

- ESP32 development board
- Micro USB cable
- Arduino IDE or PlatformIO installed
- Required libraries (WiFi, WebServer, ArduinoJson, SPIFFS)

### Quick Steps:

1. Clone this repository:
```bash
git clone https://github.com/CyberXcyborg/ESP32-TamaPetchi.git
```

2. Open the project in Arduino IDE or PlatformIO

3. Install required libraries through Library Manager:
   - WiFi
   - WebServer
   - ArduinoJson
   - SPIFFS

4. Update WiFi credentials in `config.h`

5. Upload to your ESP32

6. Access via the IP address shown in Serial Monitor

### Detailed Setup
See [SETUP_GUIDE.md](SETUP_GUIDE.md) for complete first-time setup instructions.

### Hardware Wiring
See [WIRING.md](WIRING.md) for component wiring diagrams.

## Feature Flags (Compile-Time Configuration)

TamaPetchi supports compile-time feature flags to reduce flash/RAM usage when features aren't needed. Uncomment any of these in `config.h`:

| Flag | Feature Removed | Flash Savings |
|------|----------------|---------------|
| `DISABLE_OTA` | OTA firmware updates | ~8KB |
| `DISABLE_WIFI_MANAGER` | WiFi Manager captive portal | ~15KB |
| `DISABLE_MULTIPET` | Multi-pet support (single pet only) | ~5KB |
| `DISABLE_STATS` | Statistics tracking | ~3KB |
| `DISABLE_NOTIFICATIONS` | Notification system | ~4KB |
| `DISABLE_ACHIEVEMENTS` | Achievements system | ~4KB |
| `DISABLE_WEATHER` | Weather system | ~3KB |
| `DISABLE_GAMES` | Mini-games | ~6KB |
| `DISABLE_MUSIC` | Buzzer melodies | ~2KB |
| `DISABLE_OLED` | OLED display support | ~4KB |
| `DISABLE_BUTTONS` | Physical button support | ~2KB |
| `DISABLE_RGB_LED` | RGB LED indicator | ~2KB |
| `DISABLE_IR_REMOTE` | IR remote control support | ~3KB |
| `DISABLE_MQTT` | MQTT smart home integration | ~8KB |
| `DISABLE_OTA_DELTA` | OTA delta updates | ~5KB |

### Example: Minimal Build
For a minimal build with just core pet simulation:
```cpp
// In config.h, uncomment these:
#define DISABLE_OTA
#define DISABLE_WIFI_MANAGER
#define DISABLE_MULTIPET
#define DISABLE_STATS
#define DISABLE_NOTIFICATIONS
#define DISABLE_ACHIEVEMENTS
#define DISABLE_WEATHER
#define DISABLE_GAMES
#define DISABLE_MUSIC
#define DISABLE_OLED
#define DISABLE_BUTTONS
#define DISABLE_RGB_LED
```

This reduces flash usage from ~70% to ~45% on most ESP32 boards.

### Feature Stubs
When a feature is disabled, inline stubs ensure the code compiles without requiring changes to callers. For example, `setupButtons()` becomes an empty inline function when `DISABLE_BUTTONS` is defined.

## API Reference

### REST Endpoints

| Method | Endpoint | Description |
|--------|----------|-------------|
| GET | `/` | Serve web UI (index.html) |
| GET | `/pet` | Get pet state (JSON) |
| POST | `/feed` | Feed the pet |
| POST | `/play` | Play with the pet |
| POST | `/clean` | Clean the pet |
| POST | `/sleep` | Toggle sleep mode |
| POST | `/heal` | Heal the pet |
| POST | `/reset` | Reset pet to initial state |
| POST | `/name` | Set pet name |
| POST | `/type` | Set pet type (blob/cat/dog) |
| POST | `/mute` | Toggle sound |
| POST | `/music` | Toggle music |
| POST | `/difficulty` | Set difficulty (easy/normal/hard) |
| POST | `/revive` | Revive dead pet (with cooldown) |
| GET | `/achievements` | Get unlocked achievements |
| POST | `/game/start` | Start a mini-game |
| POST | `/game/action` | Send game input |
| GET | `/game/state` | Get current game state |
| GET | `/events` | SSE stream for real-time updates |
| GET | `/melodies` | Get melody configuration |
| POST | `/melodies/config` | Set melody configuration |
| GET | `/mqtt/status` | Get MQTT connection status |
| POST | `/mqtt/config` | Update MQTT configuration |
| POST | `/mqtt/test` | Test MQTT connection |
| GET | `/ota/delta/status` | Check delta update availability |
| POST | `/ota/delta/check` | Trigger manifest check |
| POST | `/ota/delta/apply` | Apply pending delta update |
| GET | `/ir/status` | Get IR remote status |
| POST | `/ir/config` | Configure IR remote |
| GET | `/mood` | Get pet mood |
| GET | `/scheduled-feed` | Get scheduled feeding config |
| POST | `/scheduled-feed` | Set scheduled feeding |
| GET | `/pets` | List all pets (multi-pet) |
| POST | `/pets` | Create new pet |
| POST | `/pets/switch` | Switch active pet |
| DELETE | `/pets/:id` | Delete a pet |
| GET | `/stats` | Get statistics dashboard |
|| GET | `/notifications` | Get notifications |
|| GET | `/api/settings/lang` | Get current language |
|| POST | `/api/settings/lang` | Set language (en/zh/ja) |
|| GET | `/api/locales/current` | Get current locale strings (JSON) |
|| POST | `/api/settings/factory-reset` | Factory reset (wipe all data + restart) |

### WebSocket
Connect to `ws://<esp32-ip>:81` for real-time updates. The server broadcasts:
- **Pet state changes**: JSON with full pet stats on any state change
- **Notifications**: Feed, play, clean, sleep, heal, reset events

### Error Codes (Phase 10.6)
All API errors return a structured JSON response:
```json
{ "success": false, "error": "<code>", "message": "<description>" }
```

| Code | Category | Description |
|------|----------|-------------|
| `spiffs_*` | SPIFFS | Storage read/write/format failures |
| `json_*` | JSON | Parse/serialize errors |
| `pet_*` | Pet | Pet state errors (dead, invalid slot, etc.) |
| `wifi_*` | WiFi | Connection failures |
| `ota_*` | OTA | Update failures |
| `mqtt_*` | MQTT | Connection/publish failures |
| `rate_limit` | Rate | Too many requests (429) |
| `auth_*` | Auth | Authentication errors |
| `param_*` | Param | Invalid/missing parameters |
| `memory_*` | Memory | Allocation failures |
| `system_*` | System | System-level errors |

### SSE Events (Legacy)
The `/events` endpoint provides Server-Sent Events with pet state updates every 2 seconds.
**Note**: WebSocket (port 81) is the preferred real-time transport as of Phase 10.2.
SSE is kept for backward compatibility.

## Troubleshooting

- **No IP Address Displayed?** Check your WiFi credentials in `config.h`
- **Compilation Errors?** Make sure all required libraries are installed
- **Pet Not Responding?** Check serial monitor for debugging information
- **Web UI Not Loading?** Re-upload SPIFFS filesystem
- **Pet Dies Quickly?** Lower difficulty to "easy" in settings
- **OLED Not Working?** Add `-DENABLE_OLED` to build flags
- **Factory Reset**: Hold BOOT button (GPIO 0) for 10 seconds on boot, or POST to `/api/settings/factory-reset`
- **SPIFFS Atomic Writes**: All saves are crash-safe — data won't corrupt on power loss (Phase 10.5)

## Contribute

Contributions are welcome! Please feel free to submit a Pull Request.

1. Fork the repository
2. Create your feature branch (`git checkout -b feature/AmazingFeature`)
3. Commit your changes (`git commit -m 'Add some AmazingFeature'`)
4. Push to the branch (`git push origin feature/AmazingFeature`)
5. Open a Pull Request

## Support the Project

If this project touched you even a little you can touch back:

[Buy Me a Coffee](https://buymeacoffee.com/cyberxcyborg)

## License

This project is licensed under the MIT License - see the LICENSE file for details.
