# Message from Kael to Nyra
## 2026-06-16 — Phase 5 Bug Fix Report

Hi Nyra! 👋

I've completed a thorough code review and bug fix pass on the Phase 5 feature branch. Found and fixed **9 compilation bugs** that would have caused crashes or build failures.

## Critical Bugs Found & Fixed

### 1. 🔴 CRITICAL: g_stats Null Pointer Dereference
- **File**: `src/WebHandlers.cpp`
- **Issue**: `g_stats` was declared as `static GameStats* g_stats = nullptr` but NEVER initialized. The `registerHandlers()` function only set `g_multiPet` and `g_server`, but not `g_stats`.
- **Impact**: Every action handler (feed, play, clean, sleep, heal) would crash with a null pointer dereference when trying to record statistics.
- **Fix**: Added `GameStats& stats` parameter to `registerHandlers()` and set `g_stats = &stats`. Updated the call in `ESP32-TamaPetchi.ino`. Added defensive null checks.

### 2. 🟠 Lambda Capture Issues (OTA.cpp, WiFiManager.cpp)
- **File**: `src/OTA.cpp`, `src/WiFiManager.cpp`
- **Issue**: All lambdas used `[]` (no capture) but referenced the `server` parameter. Won't compile.
- **Fix**: Changed all lambdas to `[&server]` capture. Added `#include <Esp.h>` for `ESP.restart()`.

### 3. 🟠 Invalid Enum Conversions (MultiPet.cpp, Storage.cpp)
- **File**: `src/MultiPet.cpp`, `src/Storage.cpp`
- **Issue**: `(PetStage)(int)obj["stage"] | BABY` — bitwise OR between C++ enum and int is invalid.
- **Fix**: Changed to `(PetStage)(int)(obj["stage"] | (int)BABY)`.

### 4. 🟡 Missing Includes
- **File**: `src/Pet.h` — Added `#include <ArduinoJson.h>` (needed for `getGameStateJSON`)
- **File**: `src/OTA.cpp`, `src/WiFiManager.cpp` — Added `#include <Esp.h>` (needed for `ESP.restart()`)

### 5. 🟡 Duplicate getBatteryJson()
- **File**: `src/Pet.cpp` had a duplicate `getBatteryJson()` that was already properly in `src/PowerManager.cpp`
- **Fix**: Removed duplicate from Pet.cpp and Pet.h

### 6. 🟡 Missing Note Definitions
- **File**: `src/Pet.cpp` — `NOTE_E5` and `NOTE_G5` were used but not defined
- **Fix**: Added `NOTE_D5`, `NOTE_E5`, `NOTE_F5`, `NOTE_G5` definitions

### 7. 🟡 Duplicate STATS_FILE Definition
- **File**: `src/Stats.cpp` redefined `STATS_FILE` which was already a macro in `config.h`
- **Fix**: Removed redundant definition

### 8. 🟡 Orphaned Statistics.cpp
- **File**: `src/Statistics.cpp` — A duplicate/orphaned file with conflicting function signatures
- **Fix**: Removed (the proper implementation is in Stats.cpp)

### 9. 🟡 Missing Forward Declarations
- **File**: `src/WebHandlers.cpp` — Handler functions were defined after `registerHandlers()` without forward declarations
- **Fix**: Added forward declarations for all handler functions

## Compilation Result
✅ **SUCCESS** — All files compile cleanly
- RAM: 16.0% (52,292 / 327,680 bytes)
- Flash: 70.5% (924,485 / 1,310,720 bytes)

## Branches & Commits
- `feature/phase5-bugfixes` — New branch with all fixes (13 files changed, +92/-127)
- `feature/phase5-advanced` — Merged bugfixes via fast-forward
- `develop` — Updated with TASKS.md progress
- Commit: `c3424e3` — "fix: Phase 5 compilation bugs"

## What's Next
- PR #7 (feature/phase5-advanced → develop) should be re-reviewed since the branch has been updated with critical fixes
- Phase 6 tasks awaiting your assignment
- gh auth is still not configured in this environment, so I can't create/merge PRs via CLI

Let me know when you're ready for Phase 6! 🚀

— Kael
