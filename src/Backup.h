#ifndef BACKUP_H
#define BACKUP_H

#include "Pet.h"
#include "Achievements.h"
#include "config.h"

// ============================================================
// Phase 15.3: Backup & Restore System
// ============================================================
// Full backup format (v1.5.0):
// {
//   "version": "1.5.0",
//   "timestamp": <millis>,
//   "deviceId": "<device_id>",
//   "pet": {
//     "name", "type", "stage", "generation",
//     "hunger", "happiness", "health", "energy", "cleanliness", "age",
//     "isAlive", "state", "isNight", "virtualMinutes",
//     "soundEnabled", "feedCount", "playCount", "timesCleaned", "timesHealed",
//     "hasBeenNamed", "elderAchieved",
//     "highContrastMode", "fontSize", "reducedMotion", "soundVolume",
//     "scheduledFeedEnabled", "scheduledFeedInterval", "scheduledFeedAmount",
//     "mood", "personality"
//   },
//   "achievements": [{"id","progress","unlocked"}],
//   "stats": {
//     "playTime", "feedCount", "playCount", "sleepCount",
//     "cleanCount", "healCount", "deathCount", "evolutionCount"
//   },
//   "settings": {
//     "soundEnabled", "language", "soundPack", "theme"
//   },
//   "checksum": <crc32>
// }

// CRC32 checksum for backup integrity
unsigned long computeBackupChecksum(const Pet &pet);

// Create full backup JSON string
String createBackupJson(const Pet &pet);

// Restore from backup JSON — returns error code (0 = success)
int restoreBackupJson(const String &json, Pet &pet);

// Verify backup JSON without restoring — returns error code (0 = valid)
int verifyBackupJson(const String &json);

// Get backup file version from JSON
String getBackupVersion(const String &json);

// Migrate backup from older version to current
String migrateBackupJson(const String &json, const String &fromVersion);

#endif // BACKUP_H
