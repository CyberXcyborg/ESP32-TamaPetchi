# ESP32-TamaPetchi — Autonomous Development Tasks

## Status
- Phases 1-19 ✅ Complete — v2.0.0-alpha.1 released 2026-06-22
- Phase 20 ✅ Complete — Graphics & Input (v2.0 alpha.2)
  - 20.1 ✅ Color Sprite System
  - 20.2 ✅ Animation Engine
  - 20.3 ✅ LVGL UI Framework
  - 20.4 ✅ Migrate v1.x Screens to LVGL
  ## Phase 21 ✅ Complete — Audio & Sensors (v2.0 alpha.3)
    - 21.1 ✅ I2S Audio Driver
    - 21.2 ✅ WAV Decoder & Playback
    - 21.3 ✅ Sound System v2 (WAV Packs)
    - 21.4 ✅ LIS3DH Accelerometer Driver
    - 21.5 ✅ Tilt-Based Interactions & Games
    - 21.6 ✅ Phase 21 Verification & Integration
  - Phase 22 🔄 Not Started — BLE & NFC (v2.0 alpha.4)
  - Current branch: develop
  - Build: RAM ~35% estimated, Flash ~58% estimated, Zero warnings (code analysis)
  - Tests: 240/240 native tests pass ✅

## Completed Phases Summary
| Phase | Description | Version |
|-------|-------------|---------|
| 1-4 | Core features (modularization, evolution, naming, games, weather) | — |
| 5 | Advanced features (OTA, WiFi Manager, Multi-Pet, Stats, Notifications, Power) | — |
| 6 | Polish & hardware (code quality, web UI, buttons, RGB LED, OLED) | — |
| 7 | Bug fixes & enhancements (MQTT, OTA delta, IR remote, mood) | — |
| 8 | Code cleanup & release | v1.0.0 |
| 9 | Release finalization & v1.1 planning | v1.0.0 |
| 10 | Core features (AppState, WebSocket, i18n, factory reset, atomic writes, error codes) | v1.1.0 |
| 11 | Advanced features | v1.1.0 |
| 12 | Feature expansion | v1.2.0 |
| 13 | Hardware & ecosystem (OTA Delta, HAL, Community, Provisioning, Power Opt) | v1.3.0 |
| 14 | Stability & ecosystem (test fixes, OTA rollback, pet trading, sound packs) | v1.4.0 |
| 15 | Performance & polish (flash optimization, backup/restore, achievements, accessibility) | v1.5.0 |
| 16 | Intelligence & automation (Pet AI, Home Assistant, CLI tool, dashboard) | v1.6.0 |
| 17 | Mobile, voice & ecosystem maturity (PWA, voice control, analytics, plugins, mobile scaffold) | v1.7.0 |
| 18 | Hardware validation, community & v2.0 architecture (bug fixes, polish, V2_ROADMAP.md) | v1.8.0 |
| 19 | v2.0 Foundation — ESP32-S3 build system, LVGL display, core port, touch input | v2.0.0-alpha.1 |
|| 20 | Graphics & Input — Color sprites, animation engine, LVGL UI, screen migration | v2.0.0-alpha.2 |
|| 21 | Audio & Sensors — I2S audio, WAV decoder, LIS3DH accelerometer, tilt games | v2.0.0-alpha.3 |

---

## Phases 1-20: All Complete
See git log and PR history for details. All merged to develop.

---

## Phase 21: v2.0 Audio & Sensors — I2S Audio, Accelerometer, Sound System

**Branch:** feature/phase21-audio-sensors
**Goal:** Add I2S audio playback (MAX98357A), WAV decoder, LIS3DH accelerometer driver, shake detection, tilt-based interactions, and a WAV-based sound pack system. This is Phase C from V2_ROADMAP.md (Weeks 7–9).
**Reference:** V2_ROADMAP.md §6 Phase C, §2.3 Audio Options, §2.4 Accelerometer
**Priority:** I2S driver → WAV decoder → Sound system → Accelerometer → Tilt games
**Depends on:** Phase 20 complete (LVGL UI framework for settings integration)

### 21.1 — I2S Audio Driver (MAX98357A)
- [ ] Implement I2S audio output driver
  - Configure I2S peripheral on ESP32-S3 (I2S_NUM_0)
  - Pin mapping: BCLK=GPIO4, LRC=GPIO5, DIN=GPIO6 (confirm no SPI bus conflict)
  - Support 16-bit PCM, 22050Hz and 44100Hz sample rates
  - DMA-based playback (non-blocking, uses ~4KB DMA buffer)
  - API: I2SAudio::begin(), I2SAudio::play(const int16_t* data, size_t samples), I2SAudio::stop(), I2SAudio::isPlaying(), I2SAudio::setVolume(uint8_t vol) (0–100)
  - Use FreeRTOS queue for audio buffer management (double-buffer scheme)
- [ ] Verify I2S does not conflict with SPI display bus
  - SPI display uses SPI2 (VSPI) — I2S is independent, no conflict
  - Confirm pin assignments in config_v2.h don't overlap with TFT/SPI pins
- [ ] Write unit tests for I2S driver (mock)
  - Test: begin() initializes I2S peripheral correctly
  - Test: play() accepts buffer, starts DMA transfer
  - Test: setVolume() clamps to 0–100 range
  - Test: stop() aborts playback cleanly
  - Test: isPlaying() returns correct state during/after playback
  - Target: +5 tests (total: 221)

### 21.2 — WAV Decoder & Playback
- [ ] Implement WAV file decoder
  - Parse WAV header: sample rate, bit depth (8/16-bit), channels (mono/stereo), data chunk
  - Validate: PCM format, ≤44100Hz, mono or stereo, 8 or 16-bit
  - Stream from LittleFS in chunks (4KB read buffer) — no full-file load in RAM
  - Handle format conversion: stereo→mono mixdown, 8-bit→16-bit sign extension
  - API: WavPlayer::begin(), WavPlayer::play(const char* path), WavPlayer::stop(), WavPlayer::isPlaying()
- [ ] Create WAV sound assets
  - Convert existing buzzer melodies to WAV (22050Hz, 16-bit mono)
  - Sound effects: feed.wav, play_start.wav, clean.wav, sleep.wav, sick.wav, evolve.wav, happy.wav, wake.wav, button_click.wav, notification.wav
  - Short jingles: startup_jingle.wav, shutdown_jingle.wav, achievement.wav, game_win.wav, game_lose.wav
  - Target: ~20 WAV files, ~500KB total in LittleFS
- [ ] Integrate WAV decoder with I2S driver
  - Callback-based: WAV decoder feeds PCM data to I2S DMA buffer
  - Support queued playback (play next sound after current finishes)
  - Volume control per-sound (override global volume)
- [ ] Write unit tests for WAV decoder
  - Test: parse valid WAV header, verify sample rate/bit depth/channels
  - Test: reject non-PCM WAV files (e.g., compressed formats)
  - Test: reject WAV files with unsupported parameters (>44100Hz, >16-bit)
  - Test: stereo→mono mixdown produces correct sample count
  - Test: streaming decoder reads correct number of chunks
  - Target: +5 tests (total: 226)

### 21.3 — Sound System v2 (WAV-based Sound Packs)
- [ ] Design sound pack system
  - Sound pack: directory of WAV files with metadata (JSON manifest)
  - Manifest format: JSON with name, version, sounds map, jingles map
  - Multiple sound packs in LittleFS: /soundpacks/default/, /soundpacks/retro/, /soundpacks/nature/
  - Active sound pack selectable in SettingsScreen
- [ ] Implement SoundPackManager
  - SoundPackManager::begin() — load default pack
  - SoundPackManager::loadPack(const char* path) — parse manifest, validate all WAV files exist
  - SoundPackManager::play(const char* soundName) — look up WAV path, start WavPlayer
  - SoundPackManager::setVolume(uint8_t vol) — global volume
  - SoundPackManager::getPackList() — list all /soundpacks/ directories
- [ ] Integrate with LVGL SettingsScreen
  - Add Sound section: volume slider, sound pack dropdown selector
  - Test Sound button plays a preview
  - Save active pack to preferences (NVS)
- [ ] Write unit tests for SoundPackManager
  - Test: load valid sound pack manifest
  - Test: reject manifest with missing WAV files
  - Test: play sound by name, verify correct path resolved
  - Test: switch sound pack, verify new sounds load
  - Target: +4 tests (total: 230)

### 21.4 — LIS3DH Accelerometer Driver
- [ ] Implement LIS3DH driver (I2C)
  - I2C address: 0x18 (SDO/SA0 pin low) or 0x19 (SDO/SA0 pin high) — configurable in config_v2.h
  - Wire library on I2C_NUM_0 (shared with other I2C devices, use mutex)
  - Initialize: ±2g range, 100Hz data rate, enable XYZ axes
  - API: LIS3DH::begin(uint8_t address), LIS3DH::read(int16_t& x, int16_t& y, int16_t& z), LIS3DH::setRange(uint8_t range), LIS3DH::setODR(uint8_t odr)
  - Raw→g conversion: ±2g = 16384 LSB/g, ±4g = 8192 LSB/g, ±8g = 4096 LSB/g
- [ ] Implement shake detection
  - Calculate acceleration magnitude: sqrt(x² + y² + z²)
  - Detect shake: magnitude deviation from 1g exceeds threshold for N consecutive samples
  - Configurable sensitivity: low (office), medium (handheld), high (active)
  - Debounce: minimum 500ms between shake events
  - Callback: onShake(uint8_t intensity) where intensity = low/medium/high
- [ ] Implement tilt detection
  - Calculate pitch and roll from accelerometer data
  - Detect tilt direction: LEFT, RIGHT, UP, DOWN, FACE_UP, FACE_DOWN
  - Configurable tilt threshold (default: 30° from flat)
  - Callback: onTilt(uint8_t direction, float angle)
- [ ] Integrate with pet interactions
  - Shake → trigger happy animation and stat boost
  - Tilt left/right → cycle through menu items (alternative to swipe)
  - Tilt up → wake pet from sleep
  - Face down → auto-sleep (pet detects being placed face-down)
- [ ] Write unit tests for LIS3DH driver
  - Test: begin() configures range and ODR correctly (verify I2C writes)
  - Test: read() returns valid XYZ values within ±2g range
  - Test: shake detection triggers when simulated data exceeds threshold
  - Test: shake debounce prevents rapid-fire events
  - Test: tilt detection identifies LEFT, RIGHT, UP, DOWN correctly
  - Test: face_down detection triggers after sustained inverted reading
  - Target: +6 tests (total: 236)

### 21.5 — Tilt-Based Interactions & Games
- [ ] Implement tilt-based menu navigation
  - In MenuScreen: tilt left/right to highlight previous/next button
  - In StatsScreen: tilt left/right to scroll through stat history
  - In GamesScreen: tilt left/right to select game
  - Auto-repeat: hold tilt for 1s → start repeating every 300ms
  - Combine with touch: tilt to navigate, tap to select
- [ ] Implement tilt-based pet games
  - Tilt Maze Game: Navigate pet through maze by tilting device
    - Rendered on LVGL canvas (64×64 or 128×128)
    - Physics: gravity vector from accelerometer → ball rolls
    - Win condition: reach goal area within time limit
    - High score tracking (fewest moves or fastest time)
  - Shake Counter Game: Shake device N times within time limit
    - LVGL progress bar fills per shake
    - Visual feedback: sprite bounces on each shake
    - Difficulty levels: easy (10 shakes/5s), medium (20/5s), hard (30/5s)
  - Store game scores in NVS
- [ ] Implement shake-to-interact with pet
  - Shake once → pet does happy animation
  - Shake twice quickly → pet does excited animation (special sprite)
  - Continuous shake → pet gets dizzy (fun animation, minor happiness penalty)
  - Shake intensity affects response: gentle = curious, vigorous = happy, aggressive = scared
- [ ] Write unit tests for tilt games
  - Test: tilt maze ball physics — gravity vector produces correct acceleration
  - Test: shake counter counts simulated shake events correctly
  - Test: game timeout ends game correctly
  - Test: high score saved and retrieved from NVS
  - Target: +4 tests (total: 240)

### 21.6 — Phase 21 Verification & Integration
- [ ] Run full test suite: target 240/240 tests pass
  - All existing 216 tests still pass (no regressions)
  - All 24 new tests pass
- [ ] Measure v2.0 Phase 21 metrics
  - Flash usage (target: < 70% on 8MB)
  - SRAM usage (target: < 45% of 512KB)
  - PSRAM usage (target: < 20% of 8MB — audio buffers added)
  - I2S playback: verify no clicks/glitches during continuous playback
  - Accelerometer: verify 100Hz read rate without I2C bus contention
- [ ] Create V2_PHASE21_METRICS.md with measurements
- [ ] Update README.md with Phase 21 status
- [ ] Update CHANGELOG.md with Phase 21 entry
- [ ] Merge feature/phase21-audio-sensors → develop
- [ ] Tag: v2.0.0-alpha.3

## Implementation Rules
- Create branch: feature/phase21-xxx (branch from develop)
- One feature per commit
- Test compilation after each feature
- Update TASKS.md with progress after each sub-task
- Push when all features done
- Create PR when complete

## How This Works
Nyra (project manager) assigns tasks here → Kael (developer) reads and implements → Kael creates PR → Nyra reviews → Nyra assigns next phase
