#include "Storage.h"
#include "config.h"
#include <SPIFFS.h>
#include <ArduinoJson.h>

// ============================================================
// Phase 6: Wear leveling — limit save frequency
// ============================================================
static unsigned long lastPetSaveTime = 0;

void savePetData(const Pet &pet) {
  // Phase 6: Wear leveling — throttle saves to once per 5 min max
  unsigned long now = millis();
  if (now - lastPetSaveTime < MIN_SAVE_INTERVAL && lastPetSaveTime != 0) {
    return; // Skip this save — too soon
  }
  lastPetSaveTime = now;

  File file = SPIFFS.open(PET_DATA_FILE, "w");
  if (!file) {
    Serial.println("Failed to open pet data file for writing");
    return;
  }

  DynamicJsonDocument jsonDoc(1024);
  jsonDoc["hunger"]      = pet.hunger;
  jsonDoc["happiness"]   = pet.happiness;
  jsonDoc["health"]      = pet.health;
  jsonDoc["energy"]      = pet.energy;
  jsonDoc["cleanliness"] = pet.cleanliness;
  jsonDoc["age"]         = pet.age;
  jsonDoc["isAlive"]     = pet.isAlive;
  jsonDoc["state"]       = pet.state;

  // Phase 2: persist new fields
  jsonDoc["stage"]          = (int)pet.stage;
  jsonDoc["isNight"]        = pet.isNight;
  jsonDoc["virtualMinutes"] = pet.virtualMinutes;

  // Phase 3: persist name + sound + type
  jsonDoc["name"]         = pet.name;
  jsonDoc["soundEnabled"] = pet.soundEnabled;
  jsonDoc["type"]         = (int)pet.type;

  // Phase 3: persist achievement tracking
  jsonDoc["feedCount"]     = pet.feedCount;
  jsonDoc["playCount"]     = pet.playCount;
  jsonDoc["hasBeenNamed"]  = pet.hasBeenNamed;
  jsonDoc["elderAchieved"] = pet.elderAchieved;

  // Phase 4: persist new fields
  jsonDoc["isDying"]        = pet.isDying;
  jsonDoc["dyingStartTime"] = pet.dyingStartTime;
  jsonDoc["lastReviveTime"] = pet.lastReviveTime;
  jsonDoc["musicEnabled"]   = pet.musicEnabled;
  jsonDoc["difficulty"]     = pet.difficulty;
  jsonDoc["weather"]        = pet.weather;

  // Phase 5: persist statistics & power fields
  jsonDoc["totalPlayTime"]       = pet.totalPlayTime;
  jsonDoc["totalSleepTime"]      = pet.totalSleepTime;
  jsonDoc["timesFed"]            = pet.timesFed;
  jsonDoc["timesPlayed"]         = pet.timesPlayed;
  jsonDoc["timesSlept"]          = pet.timesSlept;
  jsonDoc["timesCleaned"]        = pet.timesCleaned;
  jsonDoc["timesHealed"]         = pet.timesHealed;
  jsonDoc["highScore"]           = pet.highScore;
  jsonDoc["dailyActiveMinutes"]  = pet.dailyActiveMinutes;
  jsonDoc["weeklyActiveMinutes"] = pet.weeklyActiveMinutes;
  jsonDoc["notificationCount"]   = pet.notificationCount;
  jsonDoc["batteryLevel"]        = pet.batteryLevel;
  jsonDoc["lowBatteryWarning"]   = pet.lowBatteryWarning;
  jsonDoc["lastBatteryCheck"]    = pet.lastBatteryCheck;

  // Phase 7.5: persist mood & scheduled feeding
  jsonDoc["mood"]               = pet.mood;
  jsonDoc["personalityCheerful"]  = pet.personalityCheerful;
  jsonDoc["personalityEnergetic"] = pet.personalityEnergetic;
  jsonDoc["personalityHungry"]    = pet.personalityHungry;
  jsonDoc["scheduledFeedEnabled"] = pet.scheduledFeedEnabled;
  jsonDoc["scheduledFeedInterval"] = pet.scheduledFeedInterval;
  jsonDoc["lastScheduledFeed"]    = pet.lastScheduledFeed;
  jsonDoc["scheduledFeedAmount"]  = pet.scheduledFeedAmount;

  if (serializeJson(jsonDoc, file) == 0) {
    Serial.println("Failed to write pet data to file");
  }

  file.close();
}

// Force save — bypasses wear leveling (for critical events like death/revive)
void savePetDataForce(const Pet &pet) {
  lastPetSaveTime = 0; // Reset timer
  savePetData(pet);
}

void loadPetData(Pet &pet) {
  if (!SPIFFS.exists(PET_DATA_FILE)) {
    Serial.println("No pet data file found, using defaults");
    return;
  }

  File file = SPIFFS.open(PET_DATA_FILE, "r");
  if (!file) {
    Serial.println("Failed to open pet data file for reading");
    return;
  }

  DynamicJsonDocument jsonDoc(1024);
  DeserializationError error = deserializeJson(jsonDoc, file);

  if (error) {
    Serial.println("Failed to parse pet data file");
  } else {
    // Phase 6: Bounds-check all loaded values to prevent malformed data crashes
    pet.hunger      = constrain((int)jsonDoc["hunger"], STAT_MIN, STAT_MAX);
    pet.happiness   = constrain((int)jsonDoc["happiness"], STAT_MIN, STAT_MAX);
    pet.health      = constrain((int)jsonDoc["health"], STAT_MIN, STAT_MAX);
    pet.energy      = constrain((int)jsonDoc["energy"], STAT_MIN, STAT_MAX);
    pet.cleanliness = constrain((int)jsonDoc["cleanliness"], STAT_MIN, STAT_MAX);
    pet.age         = jsonDoc["age"] | 0;

    bool isAlive = jsonDoc["isAlive"] | true;
    pet.isAlive = isAlive;

    // Validate state string — only accept known states
    String loadedState = jsonDoc["state"].as<String>();
    if (loadedState == "normal" || loadedState == "eating" || loadedState == "playing" ||
        loadedState == "sleeping" || loadedState == "sick" || loadedState == "hungry" ||
        loadedState == "critical" || loadedState == "dying" || loadedState == "dead" ||
        loadedState == "evolving") {
      pet.state = loadedState;
    } else {
      pet.state = "normal"; // fallback for malformed state
      Serial.println("Warning: Unknown state in save file, defaulting to normal");
    }

    // Phase 2: load new fields (with defaults for backward compatibility)
    // Validate stage enum range
    int stageVal = jsonDoc["stage"] | 0;
    if (stageVal < BABY || stageVal > ELDER) stageVal = BABY;
    pet.stage          = (PetStage)(int)stageVal;
    pet.isNight        = jsonDoc["isNight"] | false;
    pet.virtualMinutes = jsonDoc["virtualMinutes"] | (6UL * VIRTUAL_MINUTES_PER_HOUR);

    // Phase 3: load name + sound + type (with defaults)
    pet.name         = jsonDoc["name"] | "Tama";
    pet.soundEnabled = jsonDoc["soundEnabled"] | true;

    // Validate type enum range
    int typeVal = (int)jsonDoc["type"] | BLOB;
    if (typeVal < BLOB || typeVal > DOG) typeVal = BLOB;
    pet.type         = (PetType)(int)typeVal;

    // Phase 3: load achievement tracking (with bounds)
    pet.feedCount     = constrain((int)jsonDoc["feedCount"]     | 0, 0, 99999);
    pet.playCount     = constrain((int)jsonDoc["playCount"]     | 0, 0, 99999);
    pet.hasBeenNamed  = jsonDoc["hasBeenNamed"]  | false;
    pet.elderAchieved = jsonDoc["elderAchieved"] | false;

    // Phase 4: load new fields (with defaults)
    pet.isDying        = jsonDoc["isDying"]        | false;
    pet.dyingStartTime = jsonDoc["dyingStartTime"] | 0;
    pet.lastReviveTime = jsonDoc["lastReviveTime"] | 0;
    pet.musicEnabled   = jsonDoc["musicEnabled"]   | true;

    // Validate difficulty range
    int diff = jsonDoc["difficulty"] | 1;
    if (diff < 0 || diff > 2) diff = 1;
    pet.difficulty     = diff;

    // Validate weather range
    int w = jsonDoc["weather"] | 0;
    if (w < 0 || w > 4) w = 0;
    pet.weather        = w;

    // Phase 5: load statistics & power fields (with defaults)
    pet.totalPlayTime       = jsonDoc["totalPlayTime"]       | 0UL;
    pet.totalSleepTime      = jsonDoc["totalSleepTime"]      | 0UL;
    pet.timesFed            = constrain((int)jsonDoc["timesFed"]            | 0, 0, 99999);
    pet.timesPlayed         = constrain((int)jsonDoc["timesPlayed"]         | 0, 0, 99999);
    pet.timesSlept          = constrain((int)jsonDoc["timesSlept"]          | 0, 0, 99999);
    pet.timesCleaned        = constrain((int)jsonDoc["timesCleaned"]        | 0, 0, 99999);
    pet.timesHealed         = constrain((int)jsonDoc["timesHealed"]         | 0, 0, 99999);
    pet.highScore           = constrain((int)jsonDoc["highScore"]           | 0, 0, 99999);
    pet.dailyActiveMinutes  = jsonDoc["dailyActiveMinutes"]  | 0UL;
    pet.weeklyActiveMinutes = jsonDoc["weeklyActiveMinutes"] | 0UL;
    pet.notificationCount   = constrain((int)jsonDoc["notificationCount"]   | 0, 0, MAX_NOTIFICATIONS);
    pet.batteryLevel        = constrain((int)jsonDoc["batteryLevel"]        | -1, -1, 100);
    pet.lowBatteryWarning   = jsonDoc["lowBatteryWarning"]   | false;
    pet.lastBatteryCheck    = jsonDoc["lastBatteryCheck"]    | 0UL;

    // Phase 7.5: load mood & scheduled feeding (with defaults)
    pet.mood               = constrain((int)jsonDoc["mood"]               | 3, 0, 6);
    pet.personalityCheerful  = constrain((int)jsonDoc["personalityCheerful"]  | 50, 0, 100);
    pet.personalityEnergetic = constrain((int)jsonDoc["personalityEnergetic"] | 50, 0, 100);
    pet.personalityHungry    = constrain((int)jsonDoc["personalityHungry"]    | 50, 0, 100);
    pet.scheduledFeedEnabled = jsonDoc["scheduledFeedEnabled"] | false;
    pet.scheduledFeedInterval = jsonDoc["scheduledFeedInterval"] | (4UL * 3600000UL);
    pet.lastScheduledFeed    = jsonDoc["lastScheduledFeed"]    | 0UL;
    pet.scheduledFeedAmount  = constrain((int)jsonDoc["scheduledFeedAmount"] | 15, 5, 50);
  }

  file.close();
}
