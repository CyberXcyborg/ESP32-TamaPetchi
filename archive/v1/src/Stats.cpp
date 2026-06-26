#include "Stats.h"
#include "Storage.h"  // Phase 10.5: for atomicWrite()
#include "config.h"
#include <SPIFFS.h>
#include <ArduinoJson.h>

// STATS_FILE is defined in config.h

void loadStats(GameStats &stats) {
  stats.totalPlayTimeSec = 0;
  stats.totalFeeds = 0;
  stats.totalPlays = 0;
  stats.totalSleeps = 0;
  stats.totalCleans = 0;
  stats.totalHeals = 0;
  stats.highScore = 0;
  stats.deaths = 0;
  stats.evolutions = 0;
  stats.lastSaveTime = 0;

  if (!SPIFFS.exists(STATS_FILE)) return;

  File file = SPIFFS.open(STATS_FILE, "r");
  if (!file) return;

  StaticJsonDocument<512> jsonDoc;
  DeserializationError error = deserializeJson(jsonDoc, file);
  if (!error) {
    stats.totalPlayTimeSec = jsonDoc["totalPlayTimeSec"] | 0UL;
    stats.totalFeeds       = jsonDoc["totalFeeds"] | 0;
    stats.totalPlays       = jsonDoc["totalPlays"] | 0;
    stats.totalSleeps      = jsonDoc["totalSleeps"] | 0;
    stats.totalCleans      = jsonDoc["totalCleans"] | 0;
    stats.totalHeals       = jsonDoc["totalHeals"] | 0;
    stats.highScore        = jsonDoc["highScore"] | 0;
    stats.deaths           = jsonDoc["deaths"] | 0;
    stats.evolutions       = jsonDoc["evolutions"] | 0;
  }
  file.close();
}

void saveStats(const GameStats &stats) {
  StaticJsonDocument<512> jsonDoc;
  jsonDoc["totalPlayTimeSec"] = stats.totalPlayTimeSec;
  jsonDoc["totalFeeds"]       = stats.totalFeeds;
  jsonDoc["totalPlays"]       = stats.totalPlays;
  jsonDoc["totalSleeps"]      = stats.totalSleeps;
  jsonDoc["totalCleans"]      = stats.totalCleans;
  jsonDoc["totalHeals"]       = stats.totalHeals;
  jsonDoc["highScore"]        = stats.highScore;
  jsonDoc["deaths"]           = stats.deaths;
  jsonDoc["evolutions"]       = stats.evolutions;

  String content;
  serializeJson(jsonDoc, content);
  atomicWrite(STATS_FILE, content);
}

void statsOnFeed(GameStats &stats) {
  stats.totalFeeds++;
}

void statsOnPlay(GameStats &stats) {
  stats.totalPlays++;
}

void statsOnSleep(GameStats &stats) {
  stats.totalSleeps++;
}

void statsOnClean(GameStats &stats) {
  stats.totalCleans++;
}

void statsOnHeal(GameStats &stats) {
  stats.totalHeals++;
}

void statsOnDeath(GameStats &stats) {
  stats.deaths++;
}

void statsOnEvolution(GameStats &stats) {
  stats.evolutions++;
}

String getStatsJson(const GameStats &stats) {
  StaticJsonDocument<512> jsonDoc;
  jsonDoc["totalPlayTimeSec"]  = stats.totalPlayTimeSec;
  jsonDoc["totalFeeds"]        = stats.totalFeeds;
  jsonDoc["totalPlays"]        = stats.totalPlays;
  jsonDoc["totalSleeps"]       = stats.totalSleeps;
  jsonDoc["totalCleans"]       = stats.totalCleans;
  jsonDoc["totalHeals"]        = stats.totalHeals;
  jsonDoc["highScore"]         = stats.highScore;
  jsonDoc["deaths"]            = stats.deaths;
  jsonDoc["evolutions"]        = stats.evolutions;

  // Derived
  jsonDoc["totalActions"] = stats.totalFeeds + stats.totalPlays + stats.totalSleeps + stats.totalCleans + stats.totalHeals;

  String result;
  serializeJson(jsonDoc, result);
  return result;
}

void statsTick(GameStats &stats, unsigned long intervalMs) {
  stats.totalPlayTimeSec += intervalMs / 1000;
}
