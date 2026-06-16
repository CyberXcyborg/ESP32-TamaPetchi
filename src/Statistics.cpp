#include "Pet.h"
#include "config.h"
#include <SPIFFS.h>
#include <ArduinoJson.h>

// ============================================================
// Statistics Tracking
// ============================================================

void updateStatistics(Pet &pet) {
  // Track active time (called every update interval)
  pet.dailyActiveMinutes++;
  pet.weeklyActiveMinutes++;

  // Reset daily counter every 24 hours of virtual time
  if (pet.virtualMinutes < (6UL * VIRTUAL_MINUTES_PER_HOUR)) {
    // New day started (time wrapped past midnight)
    pet.dailyActiveMinutes = 0;
  }

  // Save periodically (every 10 ticks)
  static int saveCounter = 0;
  saveCounter++;
  if (saveCounter >= 10) {
    saveCounter = 0;
    saveStatistics(pet);
  }
}

void loadStatistics(Pet &pet) {
  if (!SPIFFS.exists(STATS_FILE)) return;

  File file = SPIFFS.open(STATS_FILE, "r");
  if (!file) return;

  DynamicJsonDocument jsonDoc(1024);
  DeserializationError error = deserializeJson(jsonDoc, file);
  if (!error) {
    pet.totalPlayTime    = jsonDoc["totalPlayTime"]    | 0UL;
    pet.totalSleepTime   = jsonDoc["totalSleepTime"]   | 0UL;
    pet.timesFed         = jsonDoc["timesFed"]         | 0;
    pet.timesPlayed      = jsonDoc["timesPlayed"]      | 0;
    pet.timesSlept       = jsonDoc["timesSlept"]       | 0;
    pet.timesCleaned     = jsonDoc["timesCleaned"]     | 0;
    pet.timesHealed      = jsonDoc["timesHealed"]      | 0;
    pet.highScore        = jsonDoc["highScore"]        | 0;
    pet.dailyActiveMinutes  = jsonDoc["dailyActiveMinutes"]  | 0UL;
    pet.weeklyActiveMinutes = jsonDoc["weeklyActiveMinutes"] | 0UL;
  }
  file.close();
}

void saveStatistics(const Pet &pet) {
  File file = SPIFFS.open(STATS_FILE, "w");
  if (!file) return;

  DynamicJsonDocument jsonDoc(1024);
  jsonDoc["totalPlayTime"]    = pet.totalPlayTime;
  jsonDoc["totalSleepTime"]   = pet.totalSleepTime;
  jsonDoc["timesFed"]         = pet.timesFed;
  jsonDoc["timesPlayed"]      = pet.timesPlayed;
  jsonDoc["timesSlept"]       = pet.timesSlept;
  jsonDoc["timesCleaned"]     = pet.timesCleaned;
  jsonDoc["timesHealed"]      = pet.timesHealed;
  jsonDoc["highScore"]        = pet.highScore;
  jsonDoc["dailyActiveMinutes"]  = pet.dailyActiveMinutes;
  jsonDoc["weeklyActiveMinutes"] = pet.weeklyActiveMinutes;

  serializeJson(jsonDoc, file);
  file.close();
}

String getStatsJson(const Pet &pet) {
  DynamicJsonDocument jsonDoc(1024);
  jsonDoc["totalPlayTime"]    = pet.totalPlayTime;
  jsonDoc["totalSleepTime"]   = pet.totalSleepTime;
  jsonDoc["timesFed"]         = pet.timesFed;
  jsonDoc["timesPlayed"]      = pet.timesPlayed;
  jsonDoc["timesSlept"]       = pet.timesSlept;
  jsonDoc["timesCleaned"]     = pet.timesCleaned;
  jsonDoc["timesHealed"]      = pet.timesHealed;
  jsonDoc["highScore"]        = pet.highScore;
  jsonDoc["dailyActiveMinutes"]  = pet.dailyActiveMinutes;
  jsonDoc["weeklyActiveMinutes"] = pet.weeklyActiveMinutes;

  // Derived stats
  jsonDoc["avgHappiness"] = pet.age > 0 ? (int)(pet.happiness * 100 / pet.age) : 0;
  jsonDoc["totalActions"] = pet.timesFed + pet.timesPlayed + pet.timesSlept + pet.timesCleaned + pet.timesHealed;

  String result;
  serializeJson(jsonDoc, result);
  return result;
}
