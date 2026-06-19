#include "Achievements.h"
#include "Storage.h"
#include "config.h"
#include <SPIFFS.h>
#include <ArduinoJson.h>

// ============================================================
// Achievement Definitions Table
// ============================================================
const AchievementDef achievementDefs[ACHIEVEMENT_COUNT] = {
  // Care
  { ACH_FED_10X,        "Feeding Fledgling",    CAT_CARE,       10,    "skin_green" },
  { ACH_FED_100X,       "Master Feeder",        CAT_CARE,       100,   "skin_gold" },
  { ACH_CLEANED_10X,    "Sparkle Clean",        CAT_CARE,       10,    "acc_brush" },
  { ACH_HEALED_5X,      "Pet Medic",            CAT_CARE,       5,     "acc_medkit" },
  { ACH_PLAYED_10X,     "Playful Pal",          CAT_CARE,       10,    "acc_ball" },
  { ACH_PLAYED_100X,    "Fun Master",           CAT_CARE,       100,   "skin_rainbow" },
  { ACH_NAMED_PET,      "Name Bearer",          CAT_CARE,       1,     "acc_nameplate" },
  { ACH_SCHEDULED_FEED, "Auto-Caregiver",       CAT_CARE,       1,     "acc_clock" },
  // Evolution
  { ACH_REACHED_CHILD,  "Growing Up",           CAT_EVOLUTION,  1,     NULL },
  { ACH_REACHED_ADULT,  "Coming of Age",        CAT_EVOLUTION,  1,     "acc_crown" },
  { ACH_REACHED_ELDER,  "Elder Wisdom",         CAT_EVOLUTION,  1,     "skin_elder" },
  { ACH_FULLY_EVOLVED,  "Full Evolution",       CAT_EVOLUTION,  1,     "acc_wings" },
  // Social
  { ACH_TRADE_COMPLETED,"Trading Post",         CAT_SOCIAL,     1,     NULL },
  { ACH_TRADE_RECEIVED, "Welcome Gift",         CAT_SOCIAL,     1,     NULL },
  // Exploration
  { ACH_GAME_WON_1X,    "Game Winner",          CAT_EXPLORATION,1,     "acc_trophy" },
  { ACH_GAME_WON_10X,   "Game Champion",        CAT_EXPLORATION,10,    "skin_champion" },
};

// ============================================================
// Runtime State
// ============================================================
AchievementState achievementStates[ACHIEVEMENT_COUNT];

// ============================================================
// Helper: Find achievement index by ID
// ============================================================
int findAchievementIndex(const char* id) {
  for (int i = 0; i < ACHIEVEMENT_COUNT; i++) {
    if (strcmp(achievementDefs[i].id, id) == 0) return i;
  }
  return -1;
}

// ============================================================
// Load/Save
// ============================================================
void loadAchievements(Pet &pet) {
  // Initialize runtime state
  for (int i = 0; i < ACHIEVEMENT_COUNT; i++) {
    achievementStates[i].progress = 0;
    achievementStates[i].tier = TIER_NONE;
    achievementStates[i].unlocked = false;
    achievementStates[i].notified = false;
  }

  if (!SPIFFS.exists(ACHIEVEMENTS_FILE)) {
    Serial.println("No achievements file found, starting fresh");
    // Load legacy data into new system
    achievementStates[findAchievementIndex(ACH_FED_10X)].progress = pet.feedCount;
    achievementStates[findAchievementIndex(ACH_PLAYED_10X)].progress = pet.playCount;
    achievementStates[findAchievementIndex(ACH_NAMED_PET)].progress = pet.hasBeenNamed ? 1 : 0;
    achievementStates[findAchievementIndex(ACH_REACHED_ELDER)].progress = pet.elderAchieved ? 1 : 0;
    // Recalculate tiers
    for (int i = 0; i < ACHIEVEMENT_COUNT; i++) {
      achievementStates[i].tier = calculateTier(achievementStates[i].progress, achievementDefs[i].target);
      achievementStates[i].unlocked = (achievementStates[i].tier == TIER_PLATINUM);
    }
    return;
  }

  File file = SPIFFS.open(ACHIEVEMENTS_FILE, "r");
  if (!file) {
    Serial.println("Failed to open achievements file");
    return;
  }

  DynamicJsonDocument jsonDoc(2048);
  DeserializationError error = deserializeJson(jsonDoc, file);
  if (error) {
    Serial.println("Failed to parse achievements file");
  } else {
    // Load legacy fields for backward compatibility
    pet.feedCount     = constrain((int)jsonDoc["feedCount"]     | 0, 0, 99999);
    pet.playCount     = constrain((int)jsonDoc["playCount"]     | 0, 0, 99999);
    pet.hasBeenNamed  = jsonDoc["hasBeenNamed"]  | false;
    pet.elderAchieved = jsonDoc["elderAchieved"] | false;

    // Load Phase 12.1 progress array
    JsonArray arr = jsonDoc["progress"];
    if (arr) {
      for (JsonObject obj : arr) {
        const char* id = obj["id"];
        int idx = findAchievementIndex(id);
        if (idx >= 0) {
          achievementStates[idx].progress = obj["p"] | 0;
          achievementStates[idx].unlocked = obj["u"] | false;
          achievementStates[idx].tier = calculateTier(achievementStates[idx].progress, achievementDefs[idx].target);
        }
      }
    } else {
      // First load from legacy: migrate data
      int idx;
      idx = findAchievementIndex(ACH_FED_10X);
      if (idx >= 0) achievementStates[idx].progress = pet.feedCount;
      idx = findAchievementIndex(ACH_PLAYED_10X);
      if (idx >= 0) achievementStates[idx].progress = pet.playCount;
      idx = findAchievementIndex(ACH_NAMED_PET);
      if (idx >= 0) achievementStates[idx].progress = pet.hasBeenNamed ? 1 : 0;
      idx = findAchievementIndex(ACH_REACHED_ELDER);
      if (idx >= 0) achievementStates[idx].progress = pet.elderAchieved ? 1 : 0;
      for (int i = 0; i < ACHIEVEMENT_COUNT; i++) {
        achievementStates[i].tier = calculateTier(achievementStates[i].progress, achievementDefs[i].target);
        achievementStates[i].unlocked = (achievementStates[i].tier == TIER_PLATINUM);
      }
    }
  }

  file.close();
}

void saveAchievements(const Pet &pet) {
  // Phase 6: Wear leveling — throttle achievement saves
  static unsigned long lastAchSaveTime = 0;
  unsigned long now = millis();
  if (now - lastAchSaveTime < MIN_SAVE_INTERVAL && lastAchSaveTime != 0) {
    return;
  }
  lastAchSaveTime = now;

  DynamicJsonDocument jsonDoc(2048);

  // Legacy fields for backward compatibility
  jsonDoc["feedCount"]     = pet.feedCount;
  jsonDoc["playCount"]     = pet.playCount;
  jsonDoc["hasBeenNamed"]  = pet.hasBeenNamed;
  jsonDoc["elderAchieved"] = pet.elderAchieved;

  // Phase 12.1: Progress array (compact format)
  JsonArray arr = jsonDoc.createNestedArray("progress");
  for (int i = 0; i < ACHIEVEMENT_COUNT; i++) {
    if (achievementStates[i].progress > 0 || achievementStates[i].unlocked) {
      JsonObject obj = arr.createNestedObject();
      obj["id"] = achievementDefs[i].id;
      obj["p"]  = achievementStates[i].progress;
      obj["u"]  = achievementStates[i].unlocked;
    }
  }

  String content;
  if (serializeJson(jsonDoc, content) == 0) {
    Serial.println("Failed to serialize achievements");
    return;
  }

  atomicWrite(ACHIEVEMENTS_FILE, content);
}

// ============================================================
// Record Achievement Progress
// ============================================================
void recordAchievementProgress(const char* achId, int amount) {
  int idx = findAchievementIndex(achId);
  if (idx < 0) return;
  if (achievementStates[idx].unlocked) return; // Already fully unlocked

  achievementStates[idx].progress += amount;
  if (achievementStates[idx].progress > achievementDefs[idx].target) {
    achievementStates[idx].progress = achievementDefs[idx].target;
  }

  AchievementTier newTier = calculateTier(achievementStates[idx].progress, achievementDefs[idx].target);
  if (newTier != achievementStates[idx].tier) {
    achievementStates[idx].tier = newTier;
    achievementStates[idx].notified = false; // New tier = needs notification
  }
  if (newTier == TIER_PLATINUM) {
    achievementStates[idx].unlocked = true;
  }
}

// ============================================================
// Check & Unlock Achievements
// ============================================================
void checkAchievements(Pet &pet) {
  if (!pet.isAlive) return;

  // Care achievements
  recordAchievementProgress(ACH_FED_10X, 0); // recalc only
  if (pet.feedCount >= 10)  recordAchievementProgress(ACH_FED_10X, 0);
  if (pet.feedCount >= 100) recordAchievementProgress(ACH_FED_100X, 0);
  if (pet.playCount >= 10)  recordAchievementProgress(ACH_PLAYED_10X, 0);
  if (pet.playCount >= 100) recordAchievementProgress(ACH_PLAYED_100X, 0);
  if (pet.hasBeenNamed)    recordAchievementProgress(ACH_NAMED_PET, 1);
  if (pet.scheduledFeedEnabled) recordAchievementProgress(ACH_SCHEDULED_FEED, 1);

  // Clean/Heal tracking via pet fields
  if (pet.timesCleaned >= 10) recordAchievementProgress(ACH_CLEANED_10X, 0);
  if (pet.timesHealed >= 5)   recordAchievementProgress(ACH_HEALED_5X, 0);

  // Evolution achievements
  if (pet.stage >= CHILD)  recordAchievementProgress(ACH_REACHED_CHILD, 1);
  if (pet.stage >= ADULT)  recordAchievementProgress(ACH_REACHED_ADULT, 1);
  if (pet.stage == ELDER && !pet.elderAchieved) {
    pet.elderAchieved = true;
    recordAchievementProgress(ACH_REACHED_ELDER, 1);
    recordAchievementProgress(ACH_FULLY_EVOLVED, 1);
    Serial.println("Achievement: Reached Elder!");
    saveAchievements(pet);
  }

  // Exploration: survival
  if (pet.age >= 1440) recordAchievementProgress(ACH_SURVIVED_24H, 1);
  if (pet.age >= 10080) recordAchievementProgress(ACH_SURVIVED_7D, 1);
}

// ============================================================
// JSON Output — Legacy (backward compatible)
// ============================================================
String getAchievementsJson(const Pet &pet) {
  // Legacy format: { achievements: ["id1", "id2"] }
  // Now includes all unlocked achievements
  DynamicJsonDocument jsonDoc(1024);
  JsonArray arr = jsonDoc.createNestedArray("achievements");

  for (int i = 0; i < ACHIEVEMENT_COUNT; i++) {
    if (achievementStates[i].unlocked) {
      arr.add(achievementDefs[i].id);
    }
  }

  // Legacy IDs for backward compatibility
  if (pet.age >= 60) arr.add("survived_1h");
  if (pet.age >= 1440) arr.add("survived_24h");
  if (pet.feedCount >= 10) arr.add("fed_10x");
  if (pet.playCount >= 10) arr.add("played_10x");
  if (pet.hasBeenNamed) arr.add("named_pet");
  if (pet.elderAchieved || pet.stage == ELDER) arr.add("reached_elder");

  String result;
  serializeJson(jsonDoc, result);
  return result;
}

// ============================================================
// Phase 12.1: Full Progress JSON
// ============================================================
String getAchievementsProgressJson(const Pet &pet) {
  DynamicJsonDocument jsonDoc(8192);
  JsonArray arr = jsonDoc.createNestedArray("achievements");

  for (int i = 0; i < ACHIEVEMENT_COUNT; i++) {
    JsonObject obj = arr.createNestedObject();
    obj["id"]       = achievementDefs[i].id;
    obj["name"]     = achievementDefs[i].name;
    obj["tier"]     = tierToString(achievementStates[i].tier);
    obj["category"] = categoryToString(achievementDefs[i].category);
    obj["progress"] = achievementStates[i].progress;
    obj["target"]   = achievementDefs[i].target;
    obj["unlocked"] = achievementStates[i].unlocked;
    if (achievementDefs[i].rewardId) {
      obj["reward"] = achievementDefs[i].rewardId;
    }
  }

  String result;
  serializeJson(jsonDoc, result);
  return result;
}

// ============================================================
// Phase 12.1: Newly Unlocked (for WebSocket notification)
// ============================================================
String getNewlyUnlockedJson() {
  DynamicJsonDocument jsonDoc(2048);
  JsonArray arr = jsonDoc.createNestedArray("newAchievements");
  bool any = false;

  for (int i = 0; i < ACHIEVEMENT_COUNT; i++) {
    if (!achievementStates[i].notified && achievementStates[i].tier >= TIER_BRONZE) {
      JsonObject obj = arr.createNestedObject();
      obj["id"]   = achievementDefs[i].id;
      obj["name"] = achievementDefs[i].name;
      obj["tier"] = tierToString(achievementStates[i].tier);
      achievementStates[i].notified = true;
      any = true;
    }
  }

  if (!any) {
    return "{\"newAchievements\":[]}";
  }

  String result;
  serializeJson(jsonDoc, result);
  return result;
}

// ============================================================
// Phase 12.1: Unlocked Rewards
// ============================================================
String getUnlockedRewardsJson() {
  DynamicJsonDocument jsonDoc(1024);
  JsonArray arr = jsonDoc.createNestedArray("rewards");

  for (int i = 0; i < ACHIEVEMENT_COUNT; i++) {
    if (achievementStates[i].unlocked && achievementDefs[i].rewardId) {
      arr.add(achievementDefs[i].rewardId);
    }
  }

  String result;
  serializeJson(jsonDoc, result);
  return result;
}
