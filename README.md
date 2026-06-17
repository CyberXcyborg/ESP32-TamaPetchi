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
- 🖥️ **Web Interface**: Control through a beautiful browser interface
- 🔧 **Customizable**: Easily modify behaviors, visuals, and interactions
- 🔄 **State Tracking**: Monitors hunger, happiness, energy, cleanliness, and health


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

### SSE Events
The `/events` endpoint provides Server-Sent Events with pet state updates every 2 seconds. Connect with:
```javascript
const evtSource = new EventSource('/events');
evtSource.onmessage = (e) => {
  const data = JSON.parse(e.data);
  console.log('Pet state:', data);
};
```

## Troubleshooting

- **No IP Address Displayed?** Check your WiFi credentials in `config.h`
- **Compilation Errors?** Make sure all required libraries are installed
- **Pet Not Responding?** Check serial monitor for debugging information
- **Web UI Not Loading?** Re-upload SPIFFS filesystem
- **Pet Dies Quickly?** Lower difficulty to "easy" in settings
- **OLED Not Working?** Add `-DENABLE_OLED` to build flags

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
