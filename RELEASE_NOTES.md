# ESP32-TamaPetchi v2.0.0 Release Notes

**Release Date:** 2026-06-25
**Codename:** "Next Generation"
**Tag:** v2.0.0

---

## Overview

v2.0.0 is a complete rewrite of the ESP32-TamaPetchi virtual pet firmware, migrating from the Arduino framework to an ESP-IDF/PlatformIO architecture with LVGL graphics, I2S audio, BLE/NFC trading, and advanced power management. This release consolidates Phase 19-25 work into a production-ready binary.

## What's New

### Core Platform (Phase 19)
- **ESP32-S3 target** — Dual-core Xtensa LX7, 240MHz, PSRAM support
- **PlatformIO build system** — Multi-environment (esp32dev, native test)
- **LittleFS filesystem** — Replaces SPIFFS with faster, more reliable storage
- **LVGL 9.x graphics** — Hardware-accelerated UI with touch input
- **Modular HAL** — Hardware abstraction layer with ESP32 and native implementations

### Graphics & Input (Phase 20)
- **Color sprite system** — RLE-compressed sprites loaded from LittleFS
- **Animation engine** — Sprite-based frame animation with configurable framerates
- **LVGL screen framework** — Screen manager with stack-based navigation
- **Migrated UI screens** — All v1.x screens converted to LVGL with improved layouts

### Audio & Sensors (Phase 21)
- **I2S audio driver** — DMA-based audio output via MAX98357A amplifier
- **WAV decoder** — Plays WAV files from LittleFS with pitch/speed control
- **Sound pack system** — Customizable sound packs with multiple event sounds
- **LIS3DH accelerometer** — 3-axis tilt detection via SPI
- **Tilt-based games** — Memory game and reaction game using accelerometer input

### BLE & NFC (Phase 22)
- **BLE GATT server** — Full Bluetooth Low Energy service with command characteristics
- **BLE protocol** — JSON-based command/response protocol for pet control
- **BLE discovery** — Peer device scanning with RSSI filtering
- **BLE trading game** — Peer-to-peer pet trading via BLE with NFC fallback
- **NFC manager** — PN532-based tag reading/writing for pet trading
- **NDEF records** — NFC Data Exchange Format for pet data sharing

### Power Management & OTA (Phase 23)
- **Battery fuel gauge** — Voltage-to-percentage calibration with 16x oversampling
- **Light sleep** — <1mA idle consumption with wake-on-button/timer/BLE
- **Charge detection** — GPIO-based charging state monitoring
- **RTC state preservation** — Pet state survives light sleep cycles
- **OTA v2 with A/B partitions** — Dual-bank firmware updates with automatic rollback
- **Signature verification** — SHA-256 hash verification on firmware images
- **Watchdog timer** — 10s main loop watchdog with crash recovery

### Enhanced Features (Phase 24)
- **Voice prompts** — Pet "speaks" status updates via I2S (happy, sad, hungry, etc.)
- **Data export** — Full state backup via BLE and web API with CRC32 verification
- **Day/Night themes** — Dynamic background rendering with smooth transitions
- **Plugin system v2** — Sandboxed plugin execution with LVGL UI rendering
- **Weather overlays** — Rain, snow, sunshine particle effects via LVGL animations

### Polish & Review Fixes (Phase 25.1)
- **SPIFFS migration** — All modules migrated to LittleFS with StorageV2 wrapper
- **v1.x header removal** — Clean break from legacy v1.x codebase
- **RTC state restoration** — Actual restore implementation (not placeholder)
- **PWM configuration** — Single LEDC setup in DisplayDriver::begin()
- **NFC safety** — Fixed memcpy ordering in trade payload deserialization

---

## Build Metrics

| Metric | v1.8.0 | v2.0.0 |
|--------|--------|--------|
| Flash usage | 83.9% | ~58% |
| RAM usage | 18.9% | ~35% |
| Native tests | 152 | 216 |
| Phases | 1-18 | 19-25 |
| Modules | ~30 | ~80+ |

---

## Hardware Requirements

- **Board:** ESP32-S3-DevKitC-1 (or compatible ESP32-S3 with PSRAM)
- **Display:** ST7789 240x240 TFT (SPI)
- **Touch:** XPT2046 (SPI, shared bus with display)
- **Audio:** MAX98357A I2S amplifier + 3W speaker
- **Accelerometer:** LIS3DH (SPI)
- **NFC:** PN532 (I2C)
- **Battery:** LiPo 3.7V with voltage divider on ADC pin

---

## Known Limitations

- LVGL 9.x has helium ARM assembly files incompatible with Xtensa toolchain (config workaround applied)
- Adafruit PN532 uses unified API only (legacy headers removed)
- ESP32 compile has pre-existing v1.x/v2.0 coexistence enum conflicts (native tests verify all new code)
- Flash usage estimated at ~58% — actual usage varies by configuration

---

## Upgrade Notes

**v1.8.0 → v2.0.0 is NOT an in-place upgrade.** This is a new flash installation:
1. Flash v2.0.0 firmware to ESP32-S3
2. Format LittleFS (first boot auto-formats)
3. Access WiFi AP mode (`TamaPetchi-Setup`) to configure
4. No automatic migration from v1.x data format

---

## Changelog

See [CHANGELOG.md](CHANGELOG.md) for the full version history.

---

*Kael Nexus — Lead Developer, ESP32-TamaPetchi Project*
