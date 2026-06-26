#include "Backup.h"
#include "Stats.h"
#include "AppState.h"
#include "ErrorCode.h"
#include <ArduinoJson.h>

// ============================================================
// CRC32 Implementation (for backup integrity)
// ============================================================
static unsigned long crc32_table[256];
static bool crc32_table_initialized = false;

static void init_crc32_table() {
  if (crc32_table_initialized) return;
  for (unsigned long i = 0; i < 256; i++) {
    unsigned long c = i;
    for (int j = 0; j < 8; j++) {
      if (c & 1)
        c = 0xEDB88320UL ^ (c >> 1);
      else
        c >>= 1;
    }
    crc32_table[i] = c;
  }
  crc32_table_initialized = true;
}

static unsigned long crc32(const char *data, size_t len) {
  init_crc32_table();
  unsigned long crc = 0xFFFFFFFFUL;
  for (size_t i = 0; i < len; i++) {
    crc = crc32_table[(crc ^ (unsigned char)data[i]) & 0xFF] ^ (crc >> 8);
  }
  return crc ^ 0xFFFFFFFFUL;
}

// ============================================================
// Checksum
// ============================================================
unsigned long computeBackupChecksum(const Pet &pet) {
  // Build a deterministic string from pet stats for checksum
  char buf[256];
  snprintf(buf, sizeof(buf), "%s:%d:%d:%d:%d:%d:%d:%d:%d:%d:%d",
    pet.name.c_str(),
    pet.hunger, pet.happiness, pet.health, pet.energy, pet.cleanliness,
    pet.age, pet.feedCount, pet.playCount, pet.generation, pet.stage);
  return crc32(buf, strlen(buf));
}

// ============================================================
// Create Backup JSON
// ============================================================
String createBackupJson(const Pet &pet) {
  AppState &state = APP_STATE;
  DynamicJsonDocument jsonDoc(8192);

  jsonDoc["version"] = BACKUP_VERSION;
  jsonDoc["timestamp"] = millis();

  // Pet data — comprehensive
  JsonObject petObj = jsonDoc.createNestedObject("pet");
  petObj["name"] = pet.name.c_str();
  petObj["type"] = (int)pet.type;
  petObj["stage"] = (int)pet.stage;
  petObj["generation"] = pet.generation;
  petObj["birthDeviceId"] = pet.birthDeviceId.c_str();
  petObj["hunger"] = pet.hunger;
  petObj["happiness"] = pet.happiness;
  petObj["health"] = pet.health;
  petObj["energy"] = pet.energy;
  petObj["cleanliness"] = pet.cleanliness;
  petObj["age"] = pet.age;
  petObj["isAlive"] = pet.isAlive;
  petObj["state"] = pet.state.c_str();
  petObj["isNight"] = pet.isNight;
  petObj["virtualMinutes"] = pet.virtualMinutes;
  petObj["soundEnabled"] = pet.soundEnabled;
  petObj["feedCount"] = pet.feedCount;
  petObj["playCount"] = pet.playCount;
  petObj["timesCleaned"] = pet.timesCleaned;
  petObj["timesHealed"] = pet.timesHealed;
  petObj["hasBeenNamed"] = pet.hasBeenNamed;
  petObj["elderAchieved"] = pet.elderAchieved;
  petObj["highContrastMode"] = pet.highContrastMode;
  petObj["fontSize"] = pet.fontSize;
  petObj["reducedMotion"] = pet.reducedMotion;
  petObj["soundVolume"] = pet.soundVolume;
  petObj["scheduledFeedEnabled"] = pet.scheduledFeedEnabled;
  petObj["scheduledFeedInterval"] = pet.scheduledFeedInterval;
  petObj["scheduledFeedAmount"] = pet.scheduledFeedAmount;
  petObj["mood"] = pet.mood;
  petObj["personalityCheerful"] = pet.personalityCheerful;
  petObj["personalityEnergetic"] = pet.personalityEnergetic;
  petObj["personalityHungry"] = pet.personalityHungry;

  // Achievements
  JsonArray achArr = jsonDoc.createNestedArray("achievements");
  for (int i = 0; i < ACHIEVEMENT_COUNT; i++) {
    if (achievementStates[i].progress > 0 || achievementStates[i].unlocked) {
      JsonObject obj = achArr.createNestedObject();
      obj["id"] = achievementDefs[i].id;
      obj["progress"] = achievementStates[i].progress;
      obj["unlocked"] = achievementStates[i].unlocked;
    }
  }

  // Statistics
  JsonObject statsObj = jsonDoc.createNestedObject("stats");
  statsObj["playTime"] = state.stats.totalPlayTimeSec;
  statsObj["feedCount"] = state.stats.totalFeeds;
  statsObj["playCount"] = state.stats.totalPlays;
  statsObj["sleepCount"] = state.stats.totalSleeps;
  statsObj["cleanCount"] = state.stats.totalCleans;
  statsObj["healCount"] = state.stats.totalHeals;
  statsObj["deathCount"] = state.stats.deaths;
  statsObj["evolutionCount"] = state.stats.evolutions;

  // Settings
  JsonObject settingsObj = jsonDoc.createNestedObject("settings");
  settingsObj["soundEnabled"] = pet.soundEnabled;
  settingsObj["language"] = "en";  // TODO: read from i18n module

  // Checksum
  jsonDoc["checksum"] = computeBackupChecksum(pet);

  String result;
  serializeJson(jsonDoc, result);
  return result;
}

// ============================================================
// Verify Backup JSON
// ============================================================
int verifyBackupJson(const String &json) {
  DynamicJsonDocument jsonDoc(8192);
  DeserializationError err = deserializeJson(jsonDoc, json.c_str());
  if (err) return ERR_INT_JSON_PARSE_FAIL;

  // Check version exists
  const char *ver = jsonDoc["version"];
  if (!ver) return ERR_BACKUP_VERSION_MISSING;

  // Check pet object exists
  if (!jsonDoc["pet"]) return ERR_BACKUP_NO_PET;

  // Verify checksum
  unsigned long storedChecksum = jsonDoc["checksum"] | 0UL;
  if (storedChecksum == 0) return ERR_BACKUP_NO_CHECKSUM;

  // Recompute checksum from pet data
  Pet tempPet;
  initPet(tempPet);
  JsonObject petObj = jsonDoc["pet"];
  tempPet.name = (const char*)(petObj["name"] | "");
  tempPet.hunger = (int)(petObj["hunger"] | 50);
  tempPet.happiness = (int)(petObj["happiness"] | 50);
  tempPet.health = (int)(petObj["health"] | 50);
  tempPet.energy = (int)(petObj["energy"] | 50);
  tempPet.cleanliness = (int)(petObj["cleanliness"] | 50);
  tempPet.age = (int)(petObj["age"] | 0);
  tempPet.feedCount = (int)(petObj["feedCount"] | 0);
  tempPet.playCount = (int)(petObj["playCount"] | 0);
  tempPet.generation = (int)(petObj["generation"] | 1);
  tempPet.stage = (PetStage)(int)(petObj["stage"] | 0);

  unsigned long computedChecksum = computeBackupChecksum(tempPet);
  if (computedChecksum != storedChecksum) return ERR_BACKUP_CHECKSUM_MISMATCH;

  return ERR_OK;
}

// ============================================================
// Get Backup Version
// ============================================================
String getBackupVersion(const String &json) {
  DynamicJsonDocument jsonDoc(8192);
  DeserializationError err = deserializeJson(jsonDoc, json.c_str());
  if (err) return "";
  const char *ver = jsonDoc["version"];
  return ver ? String(ver) : "";
}

// ============================================================
// Restore Backup JSON
// ============================================================
int restoreBackupJson(const String &json, Pet &pet) {
  AppState &state = APP_STATE;

  // Verify first
  int verifyResult = verifyBackupJson(json);
  if (verifyResult != ERR_OK) return verifyResult;

  DynamicJsonDocument jsonDoc(8192);
  DeserializationError err = deserializeJson(jsonDoc, json.c_str());
  if (err) return ERR_INT_JSON_PARSE_FAIL;

  JsonObject petObj = jsonDoc["pet"];
  if (petObj) {
    // Restore pet stats with bounds checking
    String name = (const char*)(petObj["name"] | "");
    if (name.length() > 0 && name.length() <= 16) {
      pet.name = name;
      pet.hasBeenNamed = true;
    }
    pet.type = (PetType)(int)(petObj["type"] | 0);
    pet.stage = (PetStage)(int)(petObj["stage"] | 0);
    pet.generation = (int)(petObj["generation"] | 1);
    pet.birthDeviceId = (const char*)(petObj["birthDeviceId"] | "");

    // Restore core stats with clamping
    pet.hunger = constrain((int)(petObj["hunger"] | 50), STAT_MIN, STAT_MAX);
    pet.happiness = constrain((int)(petObj["happiness"] | 50), STAT_MIN, STAT_MAX);
    pet.health = constrain((int)(petObj["health"] | 50), STAT_MIN, STAT_MAX);
    pet.energy = constrain((int)(petObj["energy"] | 50), STAT_MIN, STAT_MAX);
    pet.cleanliness = constrain((int)(petObj["cleanliness"] | 50), STAT_MIN, STAT_MAX);
    pet.age = (int)(petObj["age"] | 0);
    pet.isAlive = (bool)(petObj["isAlive"] | true);
    pet.isNight = (bool)(petObj["isNight"] | false);
    pet.virtualMinutes = (int)(petObj["virtualMinutes"] | 0);
    pet.soundEnabled = (bool)(petObj["soundEnabled"] | true);
    pet.feedCount = constrain((int)(petObj["feedCount"] | 0), 0, 99999);
    pet.playCount = constrain((int)(petObj["playCount"] | 0), 0, 99999);
    pet.timesCleaned = (int)(petObj["timesCleaned"] | 0);
    pet.timesHealed = (int)(petObj["timesHealed"] | 0);
    pet.hasBeenNamed = (bool)(petObj["hasBeenNamed"] | false);
    pet.elderAchieved = (bool)(petObj["elderAchieved"] | false);

    // Restore accessibility settings
    pet.highContrastMode = (bool)(petObj["highContrastMode"] | false);
    pet.fontSize = constrain((int)(petObj["fontSize"] | 1), 0, 2);
    pet.reducedMotion = (bool)(petObj["reducedMotion"] | false);
    pet.soundVolume = constrain((int)(petObj["soundVolume"] | 80), 0, 100);

    // Restore scheduled feed settings
    pet.scheduledFeedEnabled = (bool)(petObj["scheduledFeedEnabled"] | false);
    pet.scheduledFeedInterval = constrain((int)(petObj["scheduledFeedInterval"] | 4), 1, 24);
    pet.scheduledFeedAmount = constrain((int)(petObj["scheduledFeedAmount"] | 10), 5, 50);

    // Restore mood/personality
    pet.mood = constrain((int)(petObj["mood"] | 3), 0, 6);
    pet.personalityCheerful = constrain((int)(petObj["personalityCheerful"] | 50), 0, 100);
    pet.personalityEnergetic = constrain((int)(petObj["personalityEnergetic"] | 50), 0, 100);
    pet.personalityHungry = constrain((int)(petObj["personalityHungry"] | 50), 0, 100);
  }

  // Restore achievements
  JsonArray achArr = jsonDoc["achievements"];
  if (achArr) {
    for (JsonObject obj : achArr) {
      const char *id = obj["id"];
      int idx = findAchievementIndex(id);
      if (idx >= 0) {
        achievementStates[idx].progress = constrain((int)(obj["progress"] | 0), 0, 99999);
        achievementStates[idx].unlocked = (bool)(obj["unlocked"] | false);
        achievementStates[idx].tier = calculateTier(achievementStates[idx].progress, achievementDefs[idx].target);
      }
    }
  }

  return ERR_OK;
}

// ============================================================
// Migrate Backup from Older Version
// ============================================================
String migrateBackupJson(const String &json, const String &fromVersion) {
  // For now, just update the version string
  // In the future, handle schema migrations here
  DynamicJsonDocument jsonDoc(8192);
  DeserializationError err = deserializeJson(jsonDoc, json.c_str());
  if (err) return "";

  jsonDoc["version"] = BACKUP_VERSION;
  String result;
  serializeJson(jsonDoc, result);
  return result;
}
