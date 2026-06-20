# ESP32-TamaPetchi v1.2.0 Release Notes

**Release Date:** 2026-06-20
**Tag:** v1.2.0
**Previous Release:** v1.1.0

---

## What's New in v1.2.0

### 🏆 Achievements 2.0 System (Phase 12.1)
A complete overhaul of the achievement system with tiered progression and rewards.

- **16 achievements** across 4 categories: Care, Evolution, Social, Exploration
- **4 tiers**: Bronze (0-25%) → Silver (25-50%) → Gold (50-75%) → Platinum (75-100%)
- **Progress tracking** with per-achievement counters persisted in SPIFFS
- **Rewards system**: unlock pet skins and accessories by completing achievements
- **WebSocket notification** when achievements are unlocked
- **API**: `GET /api/achievements/progress` returns full progress data with tier info

### 🌳 Pet Lineage & Genealogy (Phase 12.2)
Track your pet's ancestry and watch traits pass down through generations.

- **Parent tracking**: parent device IDs, generation number, birth timestamp
- **Genetic inheritance**: child personality = weighted average of parents + random mutation (±10%)
- **Family tree visualization** in web UI with clickable nodes
- **Trade lineage recording**: traded pets get new parent IDs and incremented generation
- **API**: `GET /api/pets/lineage` returns full ancestry tree up to 5 generations

### 📊 Data Dashboard & Analytics (Phase 12.3)
Deep insights into your pet's life patterns and health trends.

- **Daily/weekly/monthly summaries**: feed count, play time, sleep hours
- **Health trends**: weight, mood history, activity level over time
- **Chart.js dashboard**: line charts for stat trends, bar charts for daily summaries
- **Data export**: CSV and JSON formats with date range filtering
- **APIs**: `GET /api/stats/trends?range=7d|30d|90d`, `GET /api/export/csv`, `GET /api/export/json`

### ♿ Accessibility & UX Improvements (Phase 12.4)
Making TamaPetchi accessible to everyone.

- **Keyboard navigation**: Tab/Enter/Space for all interactive elements
- **ARIA labels** on all buttons, status indicators, and dynamic content
- **High-contrast mode** toggle (CSS class switch)
- **Font size adjustment**: small/medium/large (CSS variable)
- **Reduced-motion mode**: disables all CSS animations and transitions
- **Sound volume control**: slider for sound effect volume (0-100%)
- **APIs**: `GET /api/settings/accessibility`, `POST /api/settings/accessibility`

### 💾 Backup & Restore (Phase 12.5)
Never lose your pet data again.

- **Full SPIFFS backup**: `GET /api/backup` returns tar of all config files
- **Integrity verification**: SHA-256 checksum on all backups
- **Restore**: `POST /api/restore` accepts tar upload, verifies checksum, extracts and applies
- **Scheduled auto-backup**: daily/weekly option
- **Round-trip tests**: verified backup/restore integrity

---

## Build & Test Results

| Metric | Value | Status |
|--------|-------|--------|
| RAM Usage | 17.3% (56,600 / 327,680 bytes) | ✅ Excellent |
| Flash Usage | 79.0% (1,035,241 / 1,310,720 bytes) | ✅ Good |
| Unit Tests | 152/152 pass | ✅ All pass |
| Build Warnings | 0 | ✅ Clean |

---

## API Changes

### New Endpoints (Phase 12)
| Method | Endpoint | Description |
|--------|----------|-------------|
| GET | `/api/achievements/progress` | Get achievement progress with tiers |
| GET | `/api/pets/lineage` | Get pet lineage/family tree |
| GET | `/api/stats/trends` | Get stat trends (7d/30d/90d) |
| GET | `/api/export/csv` | Export data as CSV |
| GET | `/api/export/json` | Export data as JSON |
| GET | `/api/settings/accessibility` | Get accessibility settings |
| POST | `/api/settings/accessibility` | Set accessibility settings |
| GET | `/api/backup` | Download full SPIFFS backup (tar) |
| POST | `/api/restore` | Restore from backup tar |

### New Feature Flags
| Flag | Feature Removed | Flash Savings |
|------|----------------|---------------|
| `DISABLE_ACHIEVEMENTS` | Achievements 2.0 system | ~6KB |
| `DISABLE_LINEAGE` | Pet lineage & genealogy | ~3KB |
| `DISABLE_ANALYTICS` | Data dashboard & analytics | ~5KB |
| `DISABLE_ACCESSIBILITY` | Accessibility features | ~2KB |
| `DISABLE_BACKUP` | Backup & restore | ~3KB |

---

## Upgrade Guide

### From v1.1.0
OTA update preserves all settings, pet data, achievements, lineage, and analytics. Backup & restore available via API for manual migration.

### From v1.0.0
Update to v1.1.0 first for OTA rollback support, then to v1.2.0. All data will be preserved.

---

## Known Issues
- IR_RECEIVER_PIN (GPIO 15) conflicts with OLED CS pin — use one or the other
- OTA rollback requires ESP32 partition scheme with at least 2 OTA partitions
- Pet trading requires MQTT broker connection and both devices on same MQTT network
- Flash usage at 79.0% — minimal builds recommended for boards with <1MB flash

---

## Full Changelog
See [CHANGELOG.md](CHANGELOG.md) for complete details.
