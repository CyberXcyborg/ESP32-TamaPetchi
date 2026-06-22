# ESP32-TamaPetchi v2.0 — Migration Notes

## Build System Changes

### PlatformIO Configuration
- **New file**: `platformio_v2.ini` — v2.0 build configuration
- **Target board**: `esp32-s3-devkitc-1` (8MB PSRAM)
- **Framework**: Arduino (with ESP-IDF components via arduino-esp32)
- **Partition table**: `partitions_v2.csv` — 8MB layout with A/B OTA + LittleFS
- **Filesystem**: LittleFS (replaces SPIFFS)

### New Dependencies
- `lvgl/lvgl@8.3.11` — Light and Versatile Graphics Library
- `bodmer/TFT_eSPI@2.5.43` — TFT display driver with touch support
- `bblanchon/ArduinoJson@6.18.5` — JSON (same version as v1.x)

### Partition Layout (8MB Flash)
| Partition | Size | Purpose |
|-----------|------|---------|
| Bootloader | 32 KB | ESP-IDF boot |
| Partition table | 4 KB | Partition map |
| NVS | 24 KB | Settings storage |
| App0 (OTA A) | 1.2 MB | Firmware slot A |
| App1 (OTA B) | 1.2 MB | Firmware slot B |
| LittleFS | 2.0 MB | Pet data, sprites, audio |
| Core dump | 1.0 MB | Crash dumps |
| Free | ~3.4 MB | Future expansion |

## Code Changes

### New Files (v2.0)
- `src/config_v2.h` — v2.0 configuration with display/touch/audio pins
- `src/main_v2.cpp` — New main entry point with LVGL + touch
- `src/Pet_v2.h/.cpp` — Pet engine ported to v2.0
- `src/Storage_v2.h/.cpp` — LittleFS storage (replaces SPIFFS)
- `src/AppState_v2.h/.cpp` — Global state singleton
- `src/DisplayDriver.h/.cpp` — LVGL display driver for ST7789
- `src/TouchDriver.h/.cpp` — LVGL touch driver for XPT2046
- `src/HAL_v2.h/.cpp` — Hardware abstraction layer
- `src/TFT_eSPI_V2.h` — TFT_eSPI wrapper
- `src/User_Setup.h` — TFT_eSPI pin configuration

### Key API Changes
1. **SPIFFS → LittleFS**: All `SPIFFS.` calls replaced with `LittleFS.`
2. **Display**: Replaced SSD1306 OLED with ST7789 TFT via LVGL
3. **Input**: Added touch input alongside physical buttons
4. **PSRAM**: Framebuffers allocated in PSRAM (8MB on ESP32-S3)
5. **Dual-core**: LVGL rendering on Core 0, app logic on Core 1

### Disabled Features (Not Yet Ported)
All v1.x advanced features are disabled via compile flags:
- OTA, WiFi Manager, Multi-Pet, Stats, Notifications
- Achievements, Weather, Games, Music
- OLED, IR Remote, MQTT, Community
- Plugins, Voice Control, Analytics
- Pet Trading, Sound Packs, Pet AI
- Backup, Lineage, Accessibility, i18n
- OTA Delta, OTA Rollback, Power Management

These will be re-enabled incrementally in later phases.

## Build Instructions

```bash
# Build v2.0
pio run -c platformio_v2.ini -e esp32s3

# Upload
pio run -c platformio_v2.ini -e esp32s3 --target upload

# Monitor
pio device monitor -b 115200
```

## Pin Mapping (ESP32-S3-DevKitC-1)

### TFT (ST7789 via SPI2)
| Signal | GPIO |
|--------|------|
| MOSI | 11 |
| SCLK | 12 |
| CS | 10 |
| DC | 14 |
| RST | 13 |
| BL | 9 |

### Touch (XPT2046, shares SPI2)
| Signal | GPIO |
|--------|------|
| CS | 15 |
| IRQ | -1 |

### I2C (Accelerometer, NFC)
| Signal | GPIO |
|--------|------|
| SDA | 21 |
| SCL | 22 |

### Battery ADC
| Signal | GPIO |
|--------|------|
| ADC | 1 |

### I2S Audio (MAX98357A)
| Signal | GPIO |
|--------|------|
| BCLK | 4 |
| LRC | 5 |
| DIN | 6 |

## Known Issues
- TFT_eSPI library requires `User_Setup.h` in the library folder or use `-DUSER_SETUP_LOADED` flag
- LVGL buffer size set to 1/10 screen (5760 pixels) — balances memory vs performance
- Touch calibration values are defaults — needs calibration on real hardware
- PSRAM allocation falls back to regular heap if PSRAM not available
