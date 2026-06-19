# ESP32-TamaPetchi v1.1.0 Release Notes

**Release Date:** 2026-06-19  
**Tag:** v1.1.0  
**Previous Release:** v1.0.0

---

## What's New in v1.1.0

### 🔄 OTA Rollback Support (Phase 11.1)
Never brick your TamaPetchi again! The new OTA rollback system automatically detects firmware crashes and reverts to the previous working version.

- **Dual-partition OTA** with automatic fallback
- **Crash detection** via RTC memory boot counting (XOR checksum validated)
- **Auto-rollback** after 3 consecutive crash boots
- **Manual rollback** via `GET /api/ota/rollback`
- **OTA status** via `GET /api/ota/status`
- **Auto-confirm** after 5 minutes stable uptime

### 🎨 Web UI Redesign (Phase 11.2)
A complete overhaul of the web interface with modern UX patterns.

- **WebSocket-native** — real-time updates via ws://ip:81 (no more polling)
- **Auto-reconnect** with exponential backoff
- **Toast notifications** — success, info, warning, error variants
- **Connection status indicator** — green/red dot
- **Dark mode** with localStorage persistence
- **Mobile-first responsive** design
- **SVG pet sprites** with state-driven animations (normal, eating, playing, sleeping, sick, hungry, dying, dead, evolution)
- **Particle effects** for feed/play/heal/sleep actions

### 🎵 Sound Pack System (Phase 11.3)
Customizable buzzer melody packs in JSON format.

- **JSON schema** for melody definitions
- **Default and cute** sound packs included
- **Upload custom packs** via `POST /api/sounds/upload`
- **List available packs** via `GET /api/sounds/list`
- **Select active pack** via `POST /api/sounds/select`
- **Persistent** across reboots via SPIFFS

### 🔄 Pet Trading via MQTT (Phase 11.4)
Trade your pets between TamaPetchi devices!

- **MQTT topic scheme**: `tamapetchi/trade/request`, `tamapetchi/trade/accept`, `tamapetchi/trade/reject`
- **Full trade protocol**: request → accept → transfer → confirm
- **Complete pet state transfer**: stats, achievements, history
- **Trade history log** in SPIFFS
- **Security PIN** required for trade confirmation

### ⚡ Performance Optimization (Phase 11.5)
Significant memory and efficiency improvements.

- **Compact JSON** — StaticJsonDocument where possible
- **HTTP ETag support** — browser caching for static resources
- **DNS caching** — faster MQTT broker connections
- **State-change broadcasts** — no more periodic WebSocket spam
- **Final memory budget**: Flash 78.0%, RAM 17.1%

---

## Build & Test Results

| Metric | Value | Status |
|--------|-------|--------|
| RAM Usage | 17.1% (55,976 / 327,680 bytes) | ✅ Excellent |
| Flash Usage | 78.0% (1,022,421 / 1,310,720 bytes) | ✅ Good |
| Unit Tests | 61/61 pass | ✅ All pass |
| Build Warnings | 0 | ✅ Clean |

---

## API Changes

### New Endpoints (Phase 11)
| Method | Endpoint | Description |
|--------|----------|-------------|
| GET | `/api/ota/status` | Get OTA status (version, rollback available) |
| GET | `/api/ota/rollback` | Trigger manual OTA rollback |
| GET | `/api/sounds/list` | List available sound packs |
| POST | `/api/sounds/select` | Select active sound pack |
| POST | `/api/sounds/upload` | Upload custom sound pack |
| GET | `/api/trade/history` | Get pet trade history |
| POST | `/api/trade/request` | Send pet trade request |
| POST | `/api/trade/accept` | Accept incoming trade |
| POST | `/api/trade/reject` | Reject incoming trade |

### New Feature Flags
| Flag | Feature Removed | Flash Savings |
|------|----------------|---------------|
| `DISABLE_OTA_ROLLBACK` | OTA rollback support | ~3KB |
| `DISABLE_SOUND_PACKS` | Sound pack system | ~3KB |
| `DISABLE_PET_TRADING` | Pet trading via MQTT | ~4KB |

---

## Upgrade Guide

### From v1.0.0
OTA update preserves all settings and pet data. OTA rollback is now available if an update causes issues.

### From v1.1.0+
If new firmware crashes 3 times, the device auto-reverts to the previous version. Manual rollback available at `GET /api/ota/rollback`.

---

## Known Issues
- IR_RECEIVER_PIN (GPIO 15) conflicts with OLED CS pin — use one or the other
- OTA rollback requires ESP32 partition scheme with at least 2 OTA partitions
- Pet trading requires MQTT broker connection and both devices on same MQTT network

---

## Full Changelog
See [CHANGELOG.md](CHANGELOG.md) for complete details.
