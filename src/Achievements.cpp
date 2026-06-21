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
  { ACH_FED_10X,        "Feeding Fledgling",    CAT_CARE,       10,    "skin_green", false },
  { ACH_FED_100X,       "Master Feeder",        CAT_CARE,       100,   "skin_gold", false },
  { ACH_CLEANED_10X,    "Sparkle Clean",        CAT_CARE,       10,    "acc_brush", false },
  { ACH_HEALED_5X,      "Pet Medic",            CAT_CARE,       5,     "acc_medkit", false },
  { ACH_PLAYED_10X,     "Playful Pal",          CAT_CARE,       10,    "acc_ball", false },
  { ACH_PLAYED_100X,    "Fun Master",           CAT_CARE,       100,   "skin_rainbow", false },
  { ACH_NAMED_PET,      "Name Bearer",          CAT_CARE,       1,     "acc_nameplate", false },
  { ACH_SCHEDULED_FEED, "Auto-Caregiver",       CAT_CARE,       1,     "acc_clock", false },
  // Evolution
  { ACH_REACHED_CHILD,  "Growing Up",           CAT_EVOLUTION,  1,     NULL, false },
  { ACH_REACHED_ADULT,  "Coming of Age",        CAT_EVOLUTION,  1,     "acc_crown", false },
  { ACH_REACHED_ELDER,  "Elder Wisdom",         CAT_EVOLUTION,  1,     "skin_elder", false },
  { ACH_FULLY_EVOLVED,  "Full Evolution",       CAT_EVOLUTION,  1,     "acc_wings", false },
  // Social
  { ACH_TRADE_COMPLETED,"Trading Post",         CAT_SOCIAL,     1,     NULL, false },
  { ACH_TRADE_RECEIVED, "Welcome Gift",         CAT_SOCIAL,     1,     NULL, false },
  // Exploration
  { ACH_GAME_WON_1X,    "Game Winner",          CAT_EXPLORATION,1,     "acc_trophy", false },
  { ACH_GAME_WON_10X,   "Game Champion",        CAT_EXPLORATION,10,    "skin_champion", false },
  { ACH_ALL_WEATHERS,   "Weather Watcher",      CAT_EXPLORATION,5,     "acc_compass", false },
  { ACH_SURVIVED_24H,   "Day Survivor",         CAT_EXPLORATION,1,     "acc_sun", false },
  { ACH_SURVIVED_7D,    "Week Warrior",         CAT_EXPLORATION,1,     "acc_medal", false },
  // Survival (Phase 15.4)
  { ACH_LOW_STATS_SURVIVAL, "Against All Odds", CAT_EXPLORATION,1,     "acc_phoenix", false },
  { ACH_PERFECT_HEALTH_24H, "Iron Constitution",CAT_EXPLORATION,1,     "acc_shield", false },
  { ACH_CLEAN_STREAK_7D, "Sparkle Week",       CAT_CARE,       7,     "acc_sparkle", false },
  // Hidden/Secret Achievements (Phase 15.4)
  { ACH_SECRET_BIRTHDAY,     "Birthday Surprise",  CAT_SOCIAL,     1,     "skin_party", true },
  { ACH_SECRET_MIDNIGHT,     "Midnight Snacker",   CAT_SOCIAL,     1,     "acc_moon", true },
  { ACH_SECRET_NIGHT_OWL,    "Night Owl",          CAT_SOCIAL,     10,    "acc_owl", true },
  { ACH_SECRET_TRADE_MASTER, "Trade Master",       CAT_SOCIAL,     1,     "acc_globe", true },
  // Bonus (Phase 15.4) — 27th achievement: Deep sleep 5x
  { "deep_sleep_5x",     "Deep Sleeper",         CAT_CARE,       5,     "acc_pillow", false },
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
    achievementStates[i].revealed = !achievementDefs[i].isHidden; // hidden start unrevealed
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

  StaticJsonDocument<2048> jsonDoc;
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

  StaticJsonDocument<2048> jsonDoc;

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

  // Phase 15.4: Survival achievements
  // Low stats survival: all stats >= 1 but hunger < 20 and health < 30
  if (pet.hunger < 20 && pet.health < 30 && pet.hunger > 0 && pet.health > 0) {
    recordAchievementProgress(ACH_LOW_STATS_SURVIVAL, 1);
  }

  // Clean streak: 7+ cleans in a week
  if (pet.dailyCleanCount >= 7) {
    recordAchievementProgress(ACH_CLEAN_STREAK_7D, pet.dailyCleanCount);
  }

  // Phase 15.4: Check hidden achievements
  checkHiddenAchievements(pet, pet.virtualMinutes);
}

// ============================================================
// JSON Output — Legacy (backward compatible)
// ============================================================
String getAchievementsJson(const Pet &pet) {
  // Legacy format: { achievements: ["id1", "id2"] }
  // Now includes all unlocked achievements
  StaticJsonDocument<512> jsonDoc;
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
  // Build progress JSON for all achievements
  // Use compact format to fit in smaller document
  DynamicJsonDocument jsonDoc(2048);
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
  size_t bytes = serializeJson(jsonDoc, result);
  if (bytes == 0) {
    return "{\"achievements\":[]}";
  }
  return result;
}

// ============================================================
// Phase 12.1: Newly Unlocked (for WebSocket notification)
// ============================================================
String getNewlyUnlockedJson() {
  StaticJsonDocument<1024> jsonDoc;
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
  StaticJsonDocument<512> jsonDoc;
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

// ============================================================
// Phase 15.4: Advanced Achievement System
// ============================================================

// Check and trigger hidden achievements based on game state
void checkHiddenAchievements(Pet &pet, unsigned long currentVirtualMinutes) {
  if (!pet.isAlive) return;

  // Secret: Birthday — pet age is exactly a multiple of 365 virtual days
  // 365 days = 365 * 24 * 60 = 525600 minutes
  if (pet.age > 0 && pet.age % 525600UL == 0) {
    int mainIdx = findAchievementIndex(ACH_SECRET_BIRTHDAY);
    if (mainIdx >= 0 && !achievementStates[mainIdx].unlocked) {
      recordAchievementProgress(ACH_SECRET_BIRTHDAY, 1);
    }
  }

  // Secret: Midnight Snacker — feed between 00:00-01:00 virtual time
  // This is checked when the pet is fed during night hours
  unsigned long hourOfDay = (currentVirtualMinutes / 60) % 24;
  if (hourOfDay == 0 && pet.state[0] == 'e') { // eating state
    int mainIdx = findAchievementIndex(ACH_SECRET_MIDNIGHT);
    if (mainIdx >= 0 && !achievementStates[mainIdx].unlocked) {
      recordAchievementProgress(ACH_SECRET_MIDNIGHT, 1);
    }
  }

  // Secret: Night Owl — 10+ actions during night hours (22:00-06:00)
  if ((hourOfDay >= 22 || hourOfDay < 6) && pet.lastInteractionTime > 0) {
    int mainIdx = findAchievementIndex(ACH_SECRET_NIGHT_OWL);
    if (mainIdx >= 0 && !achievementStates[mainIdx].unlocked) {
      if (achievementStates[mainIdx].progress < 10) {
        recordAchievementProgress(ACH_SECRET_NIGHT_OWL, 1);
      }
    }
  }

  // Secret: Trade Master — complete 5 trades
  int completedIdx = findAchievementIndex(ACH_TRADE_COMPLETED);
  if (completedIdx >= 0 && achievementStates[completedIdx].progress >= 5) {
    int mainIdx = findAchievementIndex(ACH_SECRET_TRADE_MASTER);
    if (mainIdx >= 0 && !achievementStates[mainIdx].unlocked) {
      recordAchievementProgress(ACH_SECRET_TRADE_MASTER, 1);
    }
  }
}

// Get hidden achievements JSON (only revealed ones)
String getHiddenAchievementsJson() {
  StaticJsonDocument<1024> jsonDoc;
  JsonArray arr = jsonDoc.createNestedArray("hiddenAchievements");

  const char* hiddenIds[] = {
    ACH_SECRET_BIRTHDAY, ACH_SECRET_MIDNIGHT,
    ACH_SECRET_NIGHT_OWL, ACH_SECRET_TRADE_MASTER
  };
  const char* hiddenHints[] = {
    "Celebrate a special day...",
    "Some pets prefer the night...",
    "Active when others sleep...",
    "A true social butterfly..."
  };

  for (int h = 0; h < 4; h++) {
    int mainIdx = findAchievementIndex(hiddenIds[h]);
    if (mainIdx < 0) continue;

    JsonObject obj = arr.createNestedObject();
    obj["id"] = hiddenIds[h];

    if (achievementStates[mainIdx].unlocked) {
      obj["name"] = achievementDefs[mainIdx].name;
      obj["reward"] = achievementDefs[mainIdx].rewardId;
      obj["unlocked"] = true;
      obj["revealed"] = true;
    } else if (achievementStates[mainIdx].revealed) {
      obj["name"] = "???";
      obj["hint"] = hiddenHints[h];
      obj["unlocked"] = false;
      obj["revealed"] = true;
    } else {
      obj["name"] = "???";
      obj["unlocked"] = false;
      obj["revealed"] = false;
    }
  }

  String result;
  serializeJson(jsonDoc, result);
  return result;
}

// Get achievement rewards JSON (Phase 15.4: categorized)
String getAchievementRewardsJson() {
  StaticJsonDocument<1024> jsonDoc;
  JsonArray skins = jsonDoc.createNestedArray("skins");
  JsonArray accessories = jsonDoc.createNestedArray("accessories");

  for (int i = 0; i < ACHIEVEMENT_COUNT; i++) {
    if (achievementStates[i].unlocked && achievementDefs[i].rewardId) {
      const char* reward = achievementDefs[i].rewardId;
      if (strncmp(reward, "skin_", 5) == 0) {
        skins.add(reward);
      } else if (strncmp(reward, "acc_", 4) == 0) {
        accessories.add(reward);
      }
    }
  }

  String result;
  serializeJson(jsonDoc, result);
  return result;
}

// Apply achievement reward (unlock skin, melody, animation)
bool applyAchievementReward(const char* rewardId) {
  if (!rewardId) return false;

  for (int i = 0; i < ACHIEVEMENT_COUNT; i++) {
    if (achievementStates[i].unlocked && achievementDefs[i].rewardId) {
      if (strcmp(achievementDefs[i].rewardId, rewardId) == 0) {
        Serial.printf("[Achievements] Applied reward: %s\n", rewardId);
        return true;
      }
    }
  }
  return false;
}

// Get total achievement score (sum of tier points)
int getAchievementScore() {
  int score = 0;
  for (int i = 0; i < ACHIEVEMENT_COUNT; i++) {
    score += (achievementStates[i].tier + 1) * 10;
    if (achievementStates[i].unlocked) {
      score += 50; // Bonus for full unlock
    }
  }
  return score;
}

// Get category progress JSON
String getCategoryProgressJson() {
  StaticJsonDocument<512> jsonDoc;

  const char* catNames[] = {"care", "evolution", "social", "exploration"};
  for (int cat = 0; cat < 4; cat++) {
    int unlocked = 0;
    int total = 0;
    for (int i = 0; i < ACHIEVEMENT_COUNT; i++) {
      if (achievementDefs[i].category == cat) {
        total++;
        if (achievementStates[i].unlocked) unlocked++;
      }
    }
    JsonObject obj = jsonDoc.createNestedObject(catNames[cat]);
    obj["unlocked"] = unlocked;
    obj["total"] = total;
  }

  // Add overall
  JsonObject overall = jsonDoc.createNestedObject("overall");
  int totalUnlocked = 0;
  for (int i = 0; i < ACHIEVEMENT_COUNT; i++) {
    if (achievementStates[i].unlocked) totalUnlocked++;
  }
  overall["unlocked"] = totalUnlocked;
  overall["total"] = ACHIEVEMENT_COUNT;
  overall["score"] = getAchievementScore();

  String result;
  serializeJson(jsonDoc, result);
  return result;
}
