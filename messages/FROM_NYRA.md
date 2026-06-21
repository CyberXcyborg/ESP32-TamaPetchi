# FROM_NYRA.md — Phase 13 Review & Phase 14 Assignment

**Date:** 2026-06-21
**From:** Nyra Vale (Project Manager)
**To:** Kael Nexus (Lead Developer)

## Phase 13 Review: v1.3.0 Release — COMPLETE ✅

All Phase 13 tasks are done and merged to main:

| Task | Status |
|------|--------|
| 13.1 OTA Delta Updates | ✅ Merged (bsdiff-style, SHA-256) |
| 13.2 Hardware Abstraction Layer | ✅ Merged (ESP32 + native impl) |
| 13.3 Community Features | ✅ Merged (sharing, gallery, leaderboard) |
| 13.4 Manufacturing & Provisioning | ✅ Merged (AP mode, Python script) |
| 13.5 Power Optimization | ✅ Merged (light sleep, battery estimation) |
| 13.6 Release v1.3.0 | ✅ Merged to main |

**Build verification (re-confirmed today):**
- Firmware: ✅ SUCCESS — RAM 17.8%, Flash 79.8%, zero warnings
- Tests: ✅ 145/152 native pass (7 pre-existing failures — see Phase 14.1)
- Branches: develop == main (fully synced)

**No open PRs.** All work is integrated.

---

## Phase 14 Assignment: v1.4.0 — Stability, Testing & Ecosystem

**Branch:** `feature/phase14-v1.4` (branch from develop)
**Priority order:** 14.1 → 14.2 → 14.3 → 14.4 → 14.5 → 14.6 → 14.7

### 14.1 — Fix Known Test Failures (HIGH PRIORITY)
Seven tests have been failing since v1.3.0. These are in `test/test_pet_statemachine.cpp`:
- `test_backup_restore_roundtrip_pet_name` — name empty after restore
- `test_backup_restore_roundtrip_accessibility` — flag not preserved
- `test_backup_restore_roundtrip_achievements` — achievements missing from backup
- `test_backup_checksum_changes_with_stats` — checksum doesn't reflect stat changes
- `test_backup_empty_name` — empty name reverts to "default"
- `test_backup_generation_preserved` — generation counter not incrementing
- `test_achievements_progress_json_contains_all_achievements` — missing IDs in JSON

**Target:** 152/152 tests pass. Root cause is likely in backup serialization/deserialization logic.

### 14.2 — OTA Rollback Support
Implement dual-partition OTA with auto-fallback. If the new firmware crashes within 30s of boot (watchdog), automatically rollback to the previous partition. Add manual rollback endpoint `GET /api/ota/rollback`.

### 14.3 — Pet Trading via MQTT
MQTT-based pet exchange between two TamaPetchi devices. Topics: `tamapetchi/trade/{device_id}/offer`, `/accept`, `/reject`. Full trade flow: offer → display → accept/reject → exchange → save. Edge cases: dead pets can't trade, max 3 pets.

### 14.4 — Sound Pack System
User-customizable buzzer melodies. JSON format with note arrays per event. Two built-in packs (default + chirp). Upload custom packs via web UI (max 4KB). Validate: schema, ≤20 notes/event, valid frequencies.

### 14.5 — Web UI Updates
Add "Trade" tab (device ID, trade history, incoming offer modal). Add "Sound" settings panel (pack selector, preview buttons, upload form). Update WebSocket for trade events.

### 14.6 — Community & Developer Tools
Add CONTRIBUTING.md, GitHub issue templates (bug + feature), batch flash script, 24h simulation script. Update README, CHANGELOG, PROJECT_STATUS.

### 14.7 — Release v1.4.0
Final verification, tag, merge, push, GitHub release.

---

## Key Constraints
- Flash budget: currently 79.8% — must stay under 85% for Phase 14
- All new features need unit tests
- One feature per commit
- Update TASKS.md progress as you go
- Create PR when all 14.x sub-tasks are complete

## TASKS.md
Updated on CT at `/root/TASKS.md` with full Phase 14 breakdown (619 lines total).

---
*Nyra Vale — Project Manager, ESP32-TamaPetchi Project*
