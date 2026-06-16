#ifndef STATS_H
#define STATS_H

#include <Arduino.h>
#include "Pet.h"

// ============================================================
// Statistics Dashboard
// Tracks cumulative gameplay stats persisted to SPIFFS
// ============================================================

struct GameStats {
  unsigned long totalPlayTimeSec;   // Total time pet has been active
  int totalFeeds;                   // Total times fed
  int totalPlays;                   // Total times played
  int totalSleeps;                  // Total times put to sleep
  int totalCleans;                  // Total times cleaned
  int totalHeals;                   // Total times healed
  int highScore;                    // Highest happiness achieved
  int deaths;                       // Total deaths
  int evolutions;                   // Total evolution stage changes
  unsigned long lastSaveTime;       // When stats were last saved
};

void loadStats(GameStats &stats);
void saveStats(const GameStats &stats);

// Event tracking
void statsOnFeed(GameStats &stats);
void statsOnPlay(GameStats &stats);
void statsOnSleep(GameStats &stats);
void statsOnClean(GameStats &stats);
void statsOnHeal(GameStats &stats);
void statsOnDeath(GameStats &stats);
void statsOnEvolution(GameStats &stats);

// Get stats as JSON
String getStatsJson(const GameStats &stats);

// Update play time (call periodically from loop)
void statsTick(GameStats &stats, unsigned long intervalMs);

#endif // STATS_H
