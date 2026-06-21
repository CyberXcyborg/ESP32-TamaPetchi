# Pull Request: Phase 14 v1.4.0

**Branch:** feature/phase14-v1.4 → develop

## Summary

Phase 14: Stability, Testing & Ecosystem Expansion — v1.4.0

## Changes

### Test Fixes (Phase 14.1)
- Fixed all 7 pre-existing test failures (152/152 pass, was 145/152)
- Root cause: ArduinoJson 6.18.5 `deserializeJson(const String&)` incompatibility with native test String class
- Fix: Use `.c_str()` for all deserialization calls
- Also: `StaticJsonDocument<2048>` too small for 16 achievement entries → `DynamicJsonDocument(8192)`

### Community Tools (Phase 14.6)
- CONTRIBUTING.md: build instructions, code style, PR process, testing requirements
- GitHub issue templates: bug_report.md, feature_request.md
- scripts/flash-batch.py: batch flash multiple ESP32 devices
- scripts/simulate-24h.py: project health checks and metrics
- Updated README, CHANGELOG, PROJECT_STATUS

### Verified Existing Features
- OTA Rollback (Phase 11.1) ✅
- Pet Trading via MQTT (Phase 11.4) ✅
- Sound Pack System (Phase 11.3) ✅
- Web UI Trade + Sound sections (Phase 11.3-11.4) ✅

## Build Results
- ESP32: ✅ SUCCESS (RAM 17.8%, Flash 79.8%, 0 warnings)
- Tests: ✅ 152/152 native pass

## Commits
- aee703d fix(tests): resolve 7 pre-existing test failures
- 43d218c docs(community): add CONTRIBUTING.md, issue templates, batch flash script
- c0dd58e docs(release): add v1.4.0 release notes

Create PR at: https://github.com/CyberXcyborg/ESP32-TamaPetchi/pull/new/feature/phase14-v1.4
