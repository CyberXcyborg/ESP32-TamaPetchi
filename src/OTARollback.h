#ifndef OTA_ROLLBACK_H
#define OTA_ROLLBACK_H

#include <Arduino.h>

// ============================================================
// OTA Rollback Support (Phase 11.1)
//
// Implements dual-partition OTA with automatic fallback.
// Uses RTC memory to track boot count for crash detection.
// If new firmware crashes 3 times consecutively, reverts to
// the previous partition.
// ============================================================

// RTC memory layout for rollback tracking
// Stored in RTC slow memory (survives deep sleep, resets on power cycle)
typedef struct {
  uint32_t bootCount;        // Consecutive boots without clean shutdown
  uint32_t rollbackCount;    // Number of rollbacks performed
  uint32_t magic;            // Validation marker (0xDEADBEEF)
  uint32_t checksum;         // Simple checksum for integrity
} RTCRollbackData;

#define RTC_ROLLBACK_MAGIC    0xDEADBEEF
#define RTC_CRASH_THRESHOLD   3   // Crashes before auto-rollback

// Initialize rollback system — call early in setup()
void initRollbackSystem();

// Call when firmware is confirmed working (e.g., after 5 min uptime)
void confirmFirmwareStable();

// Check if rollback is available
bool isRollbackAvailable();

// Trigger manual rollback — returns true if rollback initiated
bool triggerRollback();

// Get rollback status as JSON
String getRollbackStatusJson();

// Get current boot count
uint32_t getBootCount();

// Check if firmware has been confirmed stable (for auto-confirm logic)
bool isFirmwareConfirmed();

// Get stable start time (for auto-confirm logic)
unsigned long getStableStartTime();

// Register rollback API routes
void registerRollbackRoutes();

#endif // OTA_ROLLBACK_H
