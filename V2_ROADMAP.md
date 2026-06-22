# ESP32-TamaPetchi v2.0 — Architecture Roadmap

**Date:** 2026-06-22
**Author:** Kael Nexus (Lead Developer)
**Status:** Research & Planning

---

## 1. Executive Summary

v1.x is feature-complete at 83.6% flash / 18.9% RAM on ESP32 Dev Module (single-core, 4MB flash, 520KB SRAM). v2.0 targets **ESP32-S3** with a color TFT display, LVGL graphics, BLE companion app, and expanded interaction modalities. This document captures architecture decisions, hardware analysis, flash budget, and a phased migration plan.

---

## 2. Hardware Analysis

### 2.1 Target SoC: ESP32-S3 vs ESP32-C3

| Feature | ESP32 (current) | ESP32-S3 | ESP32-C3 |
|---------|-----------------|----------|----------|
| Cores | 1 (Xtensa) | 2 (Xtensa LX7) | 1 (RISC-V) |
| Clock | 240 MHz | 240 MHz | 160 MHz |
| SRAM | 520 KB | 512 KB + 8MB PSRAM option | 400 KB |
| Flash | 4 MB (external) | 2–16 MB (external) | 4 MB |
| USB | UART bridge | USB-OTG (native) | USB-Serial |
| BLE | 4.2 | 5.0 | 5.0 |
| WiFi | 802.11 b/g/n | 802.11 b/g/n | 802.11 b/g/n |
| GPIO | 34 | 45 | 22 |
| SPI/I2C/I2S | Yes | Yes + LCD interface | Yes |
| Touch | No | Yes (14 ch) | No |
| Vector instr. | No | Yes (AI accel) | No |
| Price (qty 1) | ~$3 | ~$4 | ~$2 |

**Decision: ESP32-S3** — Dual-core allows rendering on core 0, app logic on core 1. USB-OTG enables native firmware flashing without UART. 8MB PSRAM option gives headroom for LVGL framebuffers, WAV audio, and large sprite sheets. BLE 5.0 for companion app. Built-in touch support eliminates external touch controller.

### 2.2 Display Options

| Display | Resolution | Interface | Colors | Touch | Est. Cost |
|---------|-----------|-----------|--------|-------|-----------|
| ST7789 TFT | 240×240 | SPI | 65K | Optional (XPT2046) | $4–6 |
| ST7789 TFT | 240×320 | SPI | 65K | Optional | $5–8 |
| ILI9341 TFT | 240×320 | SPI | 65K | Optional | $4–7 |
| ESP32-S3-BOX-3 | 240×320 | SPI | 65K | Capacitive (GT911) | $25 (dev board) |

**Decision: 240×240 ST7789 with SPI** — Square format ideal for pet display. 240×240@16bpp = 112.5KB per framebuffer. With 8MB PSRAM, double-buffering (225KB) is trivial. LVGL has native ST7789 driver support. Total display subsystem: ~300KB PSRAM (framebuffers + LVGL draw buffers).

### 2.3 Audio Options

| Option | Quality | Pins | Cost | Notes |
|--------|---------|------|------|-------|
| PWM Buzzer (current) | Low | 1 | $0.10 | Simple tones only |
| I2S DAC (MAX98357A) | Medium | 3 (BCLK, LRC, DIN) | $3 | WAV playback, no amp |
| I2S DAC + Amp (MAX98357A) | High | 3 | $5 | Drives 3W speaker directly |
| PCM5102A | Hi-Fi | 3 | $4 | 24-bit, needs amp |

**Decision: MAX98357A (I2S DAC + 3W amp)** — Plays WAV files from SPIFFS/LittleFS. Drives small speaker directly. 3 pins (BCLK=GPIO4, LRC=GPIO5, DIN=GPIO6). Enables rich sound effects, melodies, and voice prompts.

### 2.4 Accelerometer

| Chip | Interface | Axes | Range | Cost |
|------|-----------|------|-------|------|
| LIS3DH | I2C/SPI | 3 | ±2/4/8/16g | $1.50 |
| MPU6050 | I2C | 6 (accel+gyro) | ±2/4/8/16g | $2 |
| KXTJ3-1057 | I2C | 3 | ±2/4/8/16g | $1 |

**Decision: LIS3DH** — Low power (2μA standby), I2C address 0x18 or 0x19, well-supported Arduino library. Enables shake-to-interact, tilt-based games, and sleep detection.

### 2.5 NFC (Pet Trading)

| Chip | Interface | Range | Cost |
|------|-----------|-------|------|
| PN532 | I2C/SPI/UART | 4cm | $3–5 |
| RC522 | SPI | 3cm | $1–2 |

**Decision: PN532 (I2C mode)** — Supports peer-to-peer mode for device-to-device pet trading. I2C uses only 2 wires. Can also read NFC tags for "pet card" trading.

### 2.6 Bill of Materials (v2.0)

| Component | Qty | Unit Cost | Total |
|-----------|-----|-----------|-------|
| ESP32-S3-DevKitC-1 (8MB PSRAM) | 1 | $8 | $8.00 |
| 240×240 ST7789 TFT + touch | 1 | $6 | $6.00 |
| MAX98357A I2S amp board | 1 | $5 | $5.00 |
| 3W 4Ω speaker (30mm) | 1 | $1 | $1.00 |
| LIS3DH breakout | 1 | $1.50 | $1.50 |
| PN532 NFC module | 1 | $4 | $4.00 |
| Tactile buttons (×4) | 4 | $0.10 | $0.40 |
| LiPo battery (3.7V 1200mAh) | 1 | $5 | $5.00 |
| TP4056 charge module | 1 | $0.50 | $0.50 |
| Custom PCB | 1 | $3 | $3.00 |
| **Total** | | | **$34.40** |

---

## 3. Software Architecture

### 3.1 Framework Decision: ESP-IDF vs Arduino

| Aspect | Arduino (current) | ESP-IDF |
|--------|-------------------|---------|
| OTA | Basic | Native A/B partitions |
| Power mgmt | Limited | Full FreeRTOS tickless |
| BLE | Library-dependent | Native NimBLE |
| LVGL | Possible | First-class support |
| Dual-core | Single-core only | Full FreeRTOS tasks |
| Migration effort | — | ~2–3 weeks |
| Community | Large | Growing |

**Decision: Hybrid approach** — Use ESP-IDF as the base framework but retain Arduino-compatible APIs via the `arduino-esp32` component. This gives us ESP-IDF power features while keeping existing Arduino library compatibility (ArduinoJson, PubSubClient, etc.). PlatformIO supports this natively with `platform = espressif32` and `framework = espidf` + `framework = arduino`.

### 3.2 Core Architecture (v2.0)

```
┌─────────────────────────────────────────────────────────┐
│                    Application Layer                      │
│  ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌──────────┐   │
│  │ Pet Engine│ │  Games   │ │  Social  │ │ Settings │   │
│  └────┬─────┘ └────┬─────┘ └────┬─────┘ └────┬─────┘   │
│       │             │            │             │          │
│  ┌────┴─────────────┴────────────┴─────────────┴────┐   │
│  │              AppState (Singleton)                  │   │
│  └────┬─────────────┬────────────┬─────────────┬────┘   │
│       │             │            │             │          │
│  ┌────┴────┐  ┌─────┴────┐ ┌────┴────┐ ┌─────┴────┐   │
│  │ Storage │  │  Network │ │   HAL   │ │  Audio   │   │
│  │(LittleFS│  │(WiFi/BLE)│ │(GPIO/SPI)│ │ (I2S)   │   │
│  └─────────┘  └──────────┘ └─────────┘ └──────────┘   │
│                                                          │
│  ┌──────────────────────────────────────────────────┐   │
│  │              LVGL Graphics Engine                  │   │
│  │  ┌────────┐ ┌────────┐ ┌────────┐ ┌────────┐    │   │
│  │  │ Sprites│ │  UI    │ │  Anim  │ │  Theme │    │   │
│  │  └────────┘ └────────┘ └────────┘ └────────┘    │   │
│  └──────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────┘
```

### 3.3 Task Distribution (Dual-Core)

| Core 0 (PRO CPU) | Core 1 (APP CPU) |
|-------------------|-------------------|
| LVGL rendering (30 FPS) | Pet stat decay (1 Hz) |
| Touch input polling | Game logic |
| Animation updates | Network (WiFi/BLE/MQTT) |
| Audio playback | Web server |
| Display flush | Storage I/O |

### 3.4 Display Architecture (LVGL)

```
PSRAM Layout:
┌──────────────────────────────┐ 0x3F800000
│ Framebuffer 1 (240×240×2)    │ 112.5 KB
├──────────────────────────────┤
│ Framebuffer 2 (240×240×2)    │ 112.5 KB
├──────────────────────────────┤
│ LVGL Draw Buffer             │  48 KB
├──────────────────────────────┤
│ Sprite Sheet Cache           │ 128 KB
├──────────────────────────────┤
│ Font Data (3 sizes)          │  64 KB
├──────────────────────────────┤
│ WAV Audio Buffer             │  32 KB
├──────────────────────────────┤
│ Free PSRAM                   │ ~7.6 MB
└──────────────────────────────┘
```

### 3.5 Sprite System

Replace monochrome bitmaps with color sprite sheets:

- **Pet sprites:** 64×64 pixels, 16-color palette, 8 frames per animation
- **Stages:** Baby (4 animations), Child (6), Adult (8), Elder (6)
- **Animations:** idle, eat, play, sleep, sick, happy, sad, evolve
- **Storage:** PNG → custom 4-bit palette format → SPIFFS/LittleFS
- **Memory:** ~8KB per animation set, ~256KB total for all pet types
- **Rendering:** LVGL img decoder with custom palette mapping

### 3.6 BLE Companion App Protocol

```
Service UUID: 0x180F (Battery) + custom 128-bit UUID for TamaPetchi

Characteristics:
┌──────────────────────┬──────────┬─────────────────────────┐
│ UUID                 │ Props    │ Description             │
├──────────────────────┼──────────┼─────────────────────────┤
│ 0x2A19               │ R        │ Battery level           │
│ TAMA_CHAR_PET_STATE  │ R/N      │ Full pet state (JSON)   │
│ TAMA_CHAR_COMMAND    │ W        │ Send command (feed/etc) │
│ TAMA_CHAR_HISTORY    │ R/N      │ Activity history        │
│ TAMA_CHAR_TRADE      │ R/W/N    │ Pet trading data        │
└──────────────────────┴──────────┴─────────────────────────┘
```

---

## 4. Flash Budget Estimate

### 4.1 ESP32-S3 with 8MB Flash

| Component | Flash Size | Notes |
|-----------|-----------|-------|
| Bootloader | 32 KB | ESP-IDF |
| Partition table | 4 KB | |
| NVS (settings) | 24 KB | |
| App firmware | ~1.2 MB | Core + features |
| LVGL library | 200 KB | Core + widgets |
| Font data | 100 KB | 3 sizes, Latin + symbols |
| Sprite sheets | 300 KB | 3 pet types, all stages |
| WAV audio | 500 KB | Sound effects + melodies |
| LittleFS | 2 MB | Pet data, settings, plugins |
| OTA partition | 1.2 MB | A/B update |
| **Total used** | **~5.5 MB** | **69% of 8MB** |
| **Headroom** | **2.5 MB** | **31% for future** |

### 4.2 RAM Budget (512KB SRAM + 8MB PSRAM)

| Component | SRAM | PSRAM |
|-----------|------|-------|
| App code + heap | 80 KB | — |
| LVGL internal | 32 KB | — |
| Framebuffers (×2) | — | 225 KB |
| Draw buffer | — | 48 KB |
| Sprite cache | — | 128 KB |
| Font cache | — | 64 KB |
| Audio buffer | — | 32 KB |
| Network buffers | 24 KB | — |
| FreeRTOS tasks (×6) | 48 KB | — |
| **Total** | **184 KB (36%)** | **497 KB (6% of 8MB)** |

---

## 5. v2.0 Feature Set

### 5.1 Core Features (Must Have)

1. **Color TFT Display** — 240×240 ST7789 with LVGL, 30 FPS rendering
2. **Animated Pet Sprites** — Color sprites per stage/state, smooth animations
3. **Touch Input** — Tap to interact, swipe for menus, pinch for zoom
4. **I2S Audio** — WAV playback via MAX98357A, rich sound effects
5. **BLE Companion** — iOS/Android app for remote pet management
6. **Multi-Pet** — Up to 5 pets on same device with tab switching
7. **Accelerometer** — Shake-to-interact, tilt games, sleep detection
8. **NFC Trading** — Tap two devices to trade pets (PN532)
9. **Improved Power** — LiPo battery, USB-C charging, 8+ hours active use
10. **ESP-IDF Base** — A/B OTA, BLE native, dual-core utilization

### 5.2 Enhanced Features (Should Have)

11. **Expanded Games** — 5+ mini-games using touch + accelerometer
12. **Day/Night Visual** — Dynamic backgrounds, weather effects
13. **Achievement Expansion** — 50+ achievements with visual rewards
14. **Plugin System v2** — LVGL-based plugin UI, sandboxed execution
15. **Voice Prompts** — Pet "speaks" via I2S audio clips
16. **Data Export** — Full state export via BLE or web

### 5.3 Nice-to-Have Features

17. **E-ink Secondary Display** — Always-on pet status
18. **GPS Module** — Location-based events (outdoor play bonus)
19. **Temperature Sensor** — Ambient temp affects pet mood
20. **IR Blaster** — Control room lights as "pet environment"

---

## 6. Migration Plan

### Phase A: Foundation (Weeks 1–3)
- Set up ESP-IDF + Arduino hybrid project on PlatformIO
- Port core modules (Pet, Storage, AppState) to ESP-IDF
- Implement LVGL display driver for ST7789
- Basic pet rendering with LVGL (static image)
- Verify build, flash, and basic functionality

### Phase B: Graphics & Input (Weeks 4–6)
- Sprite system: load from LittleFS, render with LVGL
- Animation engine: frame-based with timing
- Touch input driver (XPT2046 or capacitive)
- UI framework: menus, buttons, dialogs
- Migrate all v1.x screens to LVGL

### Phase C: Audio & Sensors (Weeks 7–9)
- I2S audio driver (MAX98357A)
- WAV decoder and playback
- LIS3DH accelerometer driver
- Shake detection, tilt games
- Sound pack system v2 (WAV-based)

### Phase D: Connectivity (Weeks 10–12)
- BLE GATT server implementation
- Companion app protocol
- NFC pet trading (PN532)
- WiFi + web server (ported from v1.x)
- MQTT (ported from v1.x)

### Phase E: Polish & Release (Weeks 13–15)
- Power optimization (light sleep between frames)
- Battery management (LiPo + TP4056)
- Full test suite (adapt v1.x native tests)
- Documentation update
- v2.0 release

---

## 7. Risk Assessment

| Risk | Impact | Mitigation |
|------|--------|------------|
| LVGL flash budget exceeds 85% | High | Use LVGL custom config, disable unused widgets, compress fonts |
| PSRAM not available on cheap S3 modules | High | Target 8MB PSRAM variant specifically; fallback to 240×240×8bpp (56KB buffers) |
| ESP-IDF breaks Arduino library compat | Medium | Test each library early; use arduino-esp32 as component |
| I2S audio conflicts with SPI display | Medium | Use separate SPI buses (SPI2 for display, I2S for audio) |
| BLE + WiFi coexistence issues | Medium | Use ESP-IDF coexistence APIs; test early |
| Touch controller adds complexity | Low | Start with buttons, add touch in Phase B |
| Migration takes longer than 3 weeks | Medium | Parallelize: keep v1.x on Arduino, build v2.x alongside |

---

## 8. Open Questions

1. **Display size:** 240×240 (square, pet-focused) vs 240×320 (rectangular, more UI space)?
   - Leaning 240×240 for pet-centric design and lower memory usage.

2. **Touch type:** Resistive (cheaper, works with glove) vs Capacitive (better UX)?
   - Capacitive for premium feel; resistive as fallback.

3. **Companion app:** Native (Swift/Kotlin) vs Flutter (cross-platform)?
   - Flutter for faster development; native if performance critical.

4. **Storage:** SPIFFS (familiar) vs LittleFS (better, ESP-IDF native)?
   - LittleFS — better wear leveling, crash-safe, ESP-IDF native.

5. **OTA strategy:** A/B partitions (ESP-IDF native) vs custom rollback?
   - A/B partitions — simpler, proven, automatic fallback.

---

## 9. Success Criteria

- [ ] Pet renders as animated color sprite at 30 FPS
- [ ] All v1.x features functional on v2.0 hardware
- [ ] BLE companion app connects and controls pet
- [ ] Battery lasts 8+ hours active use
- [ ] Flash usage < 80% on 8MB module
- [ ] SRAM usage < 50% (PSRAM for graphics)
- [ ] All 162+ tests pass (adapted for v2.0)
- [ ] Pet trading via NFC works between two devices

---

*Kael Nexus — Lead Developer, ESP32-TamaPetchi Project*
*Next review: After Phase A completion*
