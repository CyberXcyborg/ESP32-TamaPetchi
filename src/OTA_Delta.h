#ifndef OTA_DELTA_H
#define OTA_DELTA_H

#include <Arduino.h>

// WebServer.h is only available in ESP32 builds, not native test builds
#if !defined(UNIT_TEST) && !defined(__linux__) && !defined(__APPLE__)
#include <WebServer.h>
#endif

// ============================================================
// OTA Delta Updates — Binary Delta Patching (bsdiff-style)
// ============================================================
// Reduces OTA bandwidth by applying binary patches instead of
// uploading entire firmware images.
//
// Algorithm: bsdiff-style binary diff
//   - Uses sorted suffix-array matching to find common blocks
//   - Generates a patch of: COPY(src_offset, length) + ADD(bytes)
//   - Patch format: [header][control block][diff block][extra block]
//
// Patch header (40 bytes):
//   magic: "TAMD" (4 bytes)
//   new_size: uint32 — expected output size
//   sha256: 32 bytes — SHA-256 of expected output
//
// Control block (variable, 3 x uint32 per entry):
//   copy_offset, copy_length, add_length
//
// Diff block: bytes to XOR with source during copy
// Extra block: bytes to append after copy
//
// Compile-time flag: DISABLE_OTA_DELTA to exclude
// ============================================================

// Delta patch state
enum DeltaPatchState {
  DELTA_IDLE = 0,
  DELTA_RECEIVING,    // Upload in progress
  DELTA_APPLYING,     // Patch being applied
  DELTA_VERIFYING,    // SHA-256 verification
  DELTA_SUCCESS,      // Patch applied and verified
  DELTA_FAILED        // Error occurred
};

// Delta status structure
struct DeltaStatus {
  DeltaPatchState state;
  String error;           // Error message if FAILED
  size_t patchSize;       // Size of uploaded patch
  size_t newFirmwareSize; // Expected firmware size after patch
  size_t bytesProcessed;  // Bytes processed during application
  bool rollbackOnFail;    // Whether to rollback on failure
};

// Initialize delta patch system
void initOTADelta();

// Register delta web endpoints on the server
// POST /api/ota/delta — Upload and apply a delta patch
// GET  /api/ota/delta/status — Current delta status
// POST /api/ota/delta/check — Check manifest for available delta
#if !defined(UNIT_TEST) && !defined(__linux__) && !defined(__APPLE__)
void registerDeltaRoutes(WebServer &server);
#endif

// Get delta status as JSON string
String getDeltaStatusJson();

// Check if a delta patch is currently being processed
bool isDeltaInProgress();

// Get the current delta status (copy)
DeltaStatus getDeltaStatus();

#endif // OTA_DELTA_H
