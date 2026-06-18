# ESP32-TamaPetchi — User Setup Guide

## Quick Start (5 minutes)

### What You Need
- ESP32 development board (any DevKit with USB)
- Micro USB cable
- Computer with Arduino IDE or PlatformIO installed
- WiFi network (2.4GHz)

### Step 1: Install Software

#### Option A: Arduino IDE
1. Install Arduino IDE from https://www.arduino.cc/en/software
2. Add ESP32 board support:
   - File → Preferences → Additional Board Manager URLs
   - Add: `https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json`
   - Tools → Board → Board Manager → Search "ESP32" → Install
3. Install libraries via Library Manager:
   - `ArduinoJson` by Benoit Blanchon (v6.x)
   - `Adafruit SSD1306` (if using OLED)
   - `Adafruit GFX` (required by SSD1306)

#### Option B: PlatformIO (Recommended)
1. Install VS Code + PlatformIO extension
2. Open the project folder in VS Code
3. PlatformIO will auto-install dependencies from `platformio.ini`

### Step 2: Configure WiFi

Edit `config.h`:
```cpp
#define WIFI_SSID     "YourNetworkName"
#define WIFI_PASSWORD "YourPassword"
```

### Step 3: Upload Filesystem (Web UI)

The web interface lives in SPIFFS (flash filesystem).

**Arduino IDE:**
1. Install "ESP32 Sketch Data Upload" tool
2. Copy `index.html` to the `data/` folder next to your `.ino` file
3. Tools → ESP32 Sketch Data Upload

**PlatformIO:**
```pio run --target uploadfs```

### Step 4: Upload Firmware

1. Select board: "ESP32 Dev Module"
2. Select port (usually `/dev/ttyUSB0` or `COM3`)
3. Click Upload

### Step 5: Connect

1. Open Serial Monitor (115200 baud)
2. Note the IP address printed after WiFi connection
3. Open that IP in your browser
4. Your pet is alive! 🎉

---

## OTA Update Procedure

Once your device is running, you can update firmware over WiFi:

1. **From Arduino IDE:**
   - Select the network port (Tools → Port → "ESP32 at [IP]")
   - Upload as normal — it goes over WiFi!

2. **From Web UI:**
   - Navigate to `http://[IP]/update`
   - Select the `.bin` file
   - Click Upload

3. **From PlatformIO:**
   ```pio run --target upload --upload-port [IP]```

**Default OTA password:** `tamapetchi` (change in config.h!)

---

## First-Time Setup

### Initial Pet Care
1. **Name your pet** — Click the name field and type a name (max 16 chars)
2. **Feed** — Click the 🍕 button when hunger drops below 30
3. **Play** — Click the 🎮 button to boost happiness
4. **Clean** — Click the 🧼 button when cleanliness drops
5. **Sleep** — Click the 💤 button to let your pet rest

### Understanding the Stats
| Stat | Decay Rate | Warning Threshold | Critical |
|------|-----------|-------------------|----------|
| Hunger | 5/tick | 20 | 10 |
| Happiness | 3/tick | 30 | 10 |
| Health | When hunger/clean < 20 | 30 | 10 |
| Energy | 2/tick | 30 | 10 |
| Cleanliness | 4/tick | 20 | 10 |

### Evolution Stages
| Stage | Age Range | Decay Multiplier |
|-------|-----------|-----------------|
| Baby | 0-60 min | 80% (slower) |
| Child | 61-480 min | 100% (normal) |
| Adult | 481-1440 min | 110% (faster) |
| Elder | 1440+ min | 130% (fastest) |

### Day/Night Cycle
- **Day:** 06:00 - 22:00 (normal activity)
- **Night:** 22:00 - 06:00 (pet may auto-sleep if energy < 50)

---

## Troubleshooting

| Problem | Solution |
|---------|----------|
| Can't connect to WiFi | Check SSID/password in config.h |
| Web UI not loading | Re-upload SPIFFS filesystem |
| Pet dies quickly | Lower difficulty to "easy" in settings |
| OLED not working | Add `-DENABLE_OLED` to build flags |
| Buzzer too loud/quiet | Adjust BUZZER_PIN or add resistor |
| OTA fails | Ensure device and computer are on same network |

---

## Advanced Configuration

Edit `config.h` to customize:
- Stat decay rates
- Evolution thresholds
- WiFi credentials
- Buzzer pin
- OLED settings

### Compile-Time Feature Flags
Add these to `config.h` (uncomment to disable):
```cpp
#define DISABLE_OTA          // Remove OTA updates
#define DISABLE_OLED         // Remove OLED support
#define DISABLE_BUTTONS      // Remove button support
#define DISABLE_RGB_LED      // Remove RGB LED
#define DISABLE_MUSIC        // Remove buzzer melodies
#define DISABLE_GAMES        // Remove mini-games
#define DISABLE_WEATHER      // Remove weather system
#define DISABLE_ACHIEVEMENTS // Remove achievements
```

Disabling features reduces flash usage and RAM consumption.
