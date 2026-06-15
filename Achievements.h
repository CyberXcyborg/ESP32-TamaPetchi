#ifndef ACHIEVEMENTS_H
#define ACHIEVEMENTS_H

#include <Arduino.h>
#include "Pet.h"

// ============================================================
// Achievements System
// Tracks milestones and persists to SPIFFS /achievements.json
// ============================================================

// Achievement IDs
#define ACH_SURVIVED_1H     "survived_1h"
#define ACH_SURVIVED_24H    "survived_24h"
#define ACH_FED_10X         "fed_10x"
#define ACH_PLAYED_10X      "played_10x"
#define ACH_NAMED_PET       "named_pet"
#define ACH_REACHED_ELDER   "reached_elder"

#define ACHIEVEMENTS_FILE   "/achievements.json"

// Load/save achievements from SPIFFS
void loadAchievements(Pet &pet);
void saveAchievements(const Pet &pet);

// Check and unlock achievements (call after updatePet)
void checkAchievements(Pet &pet);

// Get achievements as JSON string
String getAchievementsJson(const Pet &pet);

#endif // ACHIEVEMENTS_H
