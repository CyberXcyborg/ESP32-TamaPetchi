// test_impls.cpp - Testable implementations extracted from Pet.cpp
// Strips hardware dependencies (Serial, tone, analogRead, etc.) for native unit testing
#include "Pet.h"
#include "Achievements.h"
#include "Stats.h"
#include "config.h"
#include "ErrorCode.h"
#include <ArduinoJson.h>

// --- Helpers ---
static int clampStat(int value) {
  if (value < STAT_MIN) return STAT_MIN;
  if (value > STAT_MAX) return STAT_MAX;
  return value;
}

// ============================================================
// Randomness
// ============================================================

int randomVariation(int base) {
  if (base == 0) return 0;
  int variation = base * 20 / 100;  // 20%
  if (variation == 0) variation = 1;
  return base + random(-variation, variation + 1);
}

// ============================================================
// Evolution Stages
// ============================================================

void updateStage(Pet &pet) {
  if (pet.age <= BABY_MAX_MINUTES) {
    pet.stage = BABY;
  } else if (pet.age <= CHILD_MAX_MINUTES) {
    pet.stage = CHILD;
  } else if (pet.age <= ADULT_MAX_MINUTES) {
    pet.stage = ADULT;
  } else {
    pet.stage = ELDER;
  }
}

int getStageDecayMultiplier(Pet &pet) {
  switch (pet.stage) {
    case BABY:  return BABY_DECAY_MULT;
    case CHILD: return CHILD_DECAY_MULT;
    case ADULT: return ADULT_DECAY_MULT;
    case ELDER: return ELDER_DECAY_MULT;
    default:    return 100;
  }
}

// ============================================================
// Day/Night Cycle
// ============================================================

void updateDayNightCycle(Pet &pet) {
  pet.virtualMinutes += PET_UPDATE_INTERVAL / 1000;
  if (pet.virtualMinutes >= VIRTUAL_MINUTES_PER_DAY) {
    pet.virtualMinutes = pet.virtualMinutes % VIRTUAL_MINUTES_PER_DAY;
  }
  bool wasNight = pet.isNight;
  pet.isNight = isNightTime(pet.virtualMinutes);
  if (pet.isNight && pet.energy < 50 && pet.state != "sleeping" && pet.state != "dead") {
    pet.state = "sleeping";
    pet.stateChangeTime = millis();
  }
}

bool isNightTime(unsigned long virtualMin) {
  unsigned long hour = (virtualMin / VIRTUAL_MINUTES_PER_HOUR) % 24;
  return (hour >= NIGHT_START_HOUR || hour < DAY_START_HOUR);
}

// ============================================================
// Lifecycle
// ============================================================

void initPet(Pet &pet) {
  pet.hunger          = 50;
  pet.happiness       = 50;
  pet.health          = 80;
  pet.energy          = 100;
  pet.cleanliness     = 80;
  pet.age             = 0;
  pet.isAlive         = true;
  pet.state           = "normal";
  pet.stateChangeTime      = millis();
  pet.lastInteractionTime  = millis();

  pet.stage           = BABY;
  pet.isNight         = false;
  pet.virtualMinutes  = 6 * VIRTUAL_MINUTES_PER_HOUR;

  pet.name            = "Tama";
  pet.soundEnabled    = true;
  pet.type            = BLOB;

  pet.feedCount       = 0;
  pet.playCount       = 0;
  pet.hasBeenNamed    = false;
  pet.elderAchieved   = false;

  pet.isDying           = false;
  pet.dyingStartTime    = 0;
  pet.lastReviveTime    = 0;
  pet.isEvolving        = false;
  pet.previousStage     = BABY;
  pet.evolutionStartTime = 0;
  pet.weather           = 0;
  pet.weatherChangeTime = millis();
  pet.musicEnabled      = true;
  pet.difficulty        = 1;
  pet.gameCooldown      = 0;
  pet.activeGame        = 0;
  pet.gameScore         = 0;
  pet.gameRound         = 0;
  memset(pet.memorySequence, 0, sizeof(pet.memorySequence));
  pet.catchTargetX      = 50;
  pet.catchTargetY      = 50;

  pet.totalPlayTime       = 0;
  pet.totalSleepTime      = 0;
  pet.timesFed            = 0;
  pet.timesPlayed         = 0;
  pet.timesSlept          = 0;
  pet.timesCleaned        = 0;
  pet.timesHealed         = 0;
  pet.highScore           = 0;
  pet.dailyActiveMinutes  = 0;
  pet.weeklyActiveMinutes = 0;
  pet.notificationCount   = 0;
  pet.batteryLevel        = -1;
  pet.lowBatteryWarning   = false;
  pet.lastBatteryCheck    = 0;

  initMoodSystem(pet);
  initScheduledFeed(pet);
  initLineage(pet);
  initAnalytics(pet);
  initAccessibility(pet);
}

// ============================================================
// Actions
// ============================================================

void feedPet(Pet &pet) {
  if (!pet.isAlive) return;

  // Phase 3: track feed count for achievements
  pet.feedCount++;

  // If very hungry, improve health too (check BEFORE feeding)
  if (pet.hunger < HEALTH_DECAY_THRESHOLD) {
    pet.health = clampStat(pet.health + FEED_HEALTH_BONUS);
  }

  pet.hunger  = clampStat(pet.hunger  + FEED_HUNGER_GAIN);
  pet.energy  = clampStat(pet.energy  - FEED_ENERGY_COST);
  pet.state   = "eating";
  pet.stateChangeTime    = millis();
  pet.lastInteractionTime = millis();
}

void playPet(Pet &pet) {
  if (!pet.isAlive) return;
  if (pet.energy < PLAY_ENERGY_MIN) return;
  if (pet.state == "dying") return;
  pet.happiness = clampStat(pet.happiness + PLAY_HAPPINESS_GAIN);
  pet.energy    = clampStat(pet.energy    - PLAY_ENERGY_COST);
  pet.hunger    = clampStat(pet.hunger    - PLAY_HUNGER_COST);
  pet.state     = "playing";
  pet.stateChangeTime     = millis();
  pet.lastInteractionTime = millis();
  pet.playCount++;
}

void cleanPet(Pet &pet) {
  if (!pet.isAlive) return;
  pet.cleanliness = STAT_MAX;
  pet.health      = clampStat(pet.health + CLEAN_HEALTH_BONUS);
  pet.lastInteractionTime = millis();
}

void sleepPet(Pet &pet) {
  if (!pet.isAlive) return;
  if (pet.state == "sleeping") {
    pet.state = "normal";
  } else {
    pet.state          = "sleeping";
    pet.stateChangeTime = millis();
  }
  pet.lastInteractionTime = millis();
}

void healPet(Pet &pet) {
  if (!pet.isAlive) return;
  if (pet.isDying) {
    pet.isDying = false;
    pet.health = 50;
    pet.state  = "normal";
    pet.stateChangeTime = millis();
    pet.lastInteractionTime = millis();
    return;
  }
  pet.health = STAT_MAX;
  pet.state  = "normal";
  pet.lastInteractionTime = millis();
}

void resetPet(Pet &pet) {
  initPet(pet);
}

// ============================================================
// Evolution & Death
// ============================================================

void triggerEvolution(Pet &pet) {
  pet.isEvolving = true;
  pet.previousStage = pet.stage;
  pet.evolutionStartTime = millis();
  pet.state = "evolving";
  pet.stateChangeTime = millis();
}

void triggerDying(Pet &pet) {
  pet.isDying = true;
  pet.dyingStartTime = millis();
  pet.state = "dying";
  pet.stateChangeTime = millis();
}

bool canRevive(const Pet &pet) {
  if (pet.isAlive) return false;
  if (pet.isDying) return false;
  if (millis() - pet.lastReviveTime < 300000 && pet.lastReviveTime != 0) return false;
  return true;
}

void revivePet(Pet &pet) {
  if (!canRevive(pet)) return;
  pet.isAlive = true;
  pet.isDying = false;
  pet.health = 30;
  pet.hunger = 30;
  pet.happiness = 20;
  pet.energy = 50;
  pet.cleanliness = 50;
  pet.state = "normal";
  pet.stateChangeTime = millis();
  pet.lastReviveTime = millis();
  savePetDataForce(pet);
}

// ============================================================
// Weather System
// ============================================================

void updateWeather(Pet &pet) {
  unsigned long weatherInterval = (30 + random(0, 31)) * 60000UL;
  if (millis() - pet.weatherChangeTime > weatherInterval) {
    pet.weather = random(0, 5);
    pet.weatherChangeTime = millis();
  }
}

String getWeatherName(int weather) {
  switch (weather) {
    case 0: return "sunny";
    case 1: return "cloudy";
    case 2: return "rainy";
    case 3: return "stormy";
    case 4: return "snowy";
    default: return "sunny";
  }
}

int getWeatherHappinessMod(int weather) {
  switch (weather) {
    case 0: return 2;
    case 1: return 0;
    case 2: return -3;
    case 3: return -5;
    case 4: return 3;
    default: return 0;
  }
}

int getWeatherEnergyMod(int weather) {
  switch (weather) {
    case 0: return 0;
    case 1: return 0;
    case 2: return -2;
    case 3: return 3;
    case 4: return -3;
    default: return 0;
  }
}

int getWeatherCleanlinessMod(int weather) {
  switch (weather) {
    case 0: return 0;
    case 1: return 0;
    case 2: return 5;
    case 3: return 3;
    case 4: return 0;
    default: return 0;
  }
}

// ============================================================
// Mini-Games
// ============================================================

void startGame(Pet &pet, int gameType) {
  if (pet.gameCooldown > 0) return;
  if (pet.energy < 10) return;
  if (!pet.isAlive || pet.isDying) return;
  pet.activeGame = gameType;
  pet.gameScore = 0;
  pet.gameRound = 0;
  pet.energy = clampStat(pet.energy - 10);
  if (gameType == 1) {
    generateMemorySequence(pet);
  } else if (gameType == 2) {
    updateCatchTarget(pet);
  }
}

void endGame(Pet &pet, bool won) {
  if (won) {
    int bonus = pet.gameScore * 2;
    pet.happiness = clampStat(pet.happiness + bonus);
    if (pet.gameScore >= 5) {
      pet.health = clampStat(pet.health + 5);
    }
  }
  pet.gameCooldown = 60;
  pet.activeGame = 0;
  pet.gameScore = 0;
  pet.gameRound = 0;
}

void generateMemorySequence(Pet &pet) {
  for (int i = 0; i < 10; i++) {
    pet.memorySequence[i] = random(0, 4);
  }
}

bool checkMemoryInput(Pet &pet, int input) {
  if (pet.activeGame != 1) return false;
  if (input == pet.memorySequence[pet.gameRound]) {
    pet.gameScore++;
    pet.gameRound++;
    if (pet.gameRound >= 10) {
      endGame(pet, true);
    } else {
      generateMemorySequence(pet);
    }
    return true;
  } else {
    endGame(pet, false);
    return false;
  }
}

void updateCatchTarget(Pet &pet) {
  pet.catchTargetX = random(10, 90);
  pet.catchTargetY = random(10, 90);
  pet.gameRound++;
  if (pet.gameRound > 10) {
    endGame(pet, pet.gameScore >= 5);
  }
}

String getGameStateJSON(const Pet &pet) {
  StaticJsonDocument<512> doc;
  doc["activeGame"] = pet.activeGame;
  doc["score"] = pet.gameScore;
  doc["round"] = pet.gameRound;
  doc["cooldown"] = pet.gameCooldown;
  if (pet.activeGame == 1) {
    JsonArray seq = doc.createNestedArray("sequence");
    for (int i = 0; i <= pet.gameRound && i < 10; i++) {
      seq.add(pet.memorySequence[i]);
    }
  }
  if (pet.activeGame == 2) {
    doc["targetX"] = pet.catchTargetX;
    doc["targetY"] = pet.catchTargetY;
  }
  String result;
  serializeJson(doc, result);
  return result;
}

// ============================================================
// Settings Helpers
// ============================================================

int getDifficultyMultiplier(int difficulty) {
  switch (difficulty) {
    case 0: return 80;
    case 1: return 100;
    case 2: return 120;
    default: return 100;
  }
}

String getDifficultyName(int difficulty) {
  switch (difficulty) {
    case 0: return "easy";
    case 1: return "normal";
    case 2: return "hard";
    default: return "normal";
  }
}

// ============================================================
// Phase 7.5: Pet Mood System
// ============================================================

void initMoodSystem(Pet &pet) {
  pet.mood = 3;
  pet.personalityCheerful = 50 + random(0, 51);
  pet.personalityEnergetic = 50 + random(0, 51);
  pet.personalityHungry = 30 + random(0, 41);
  pet.lastMoodUpdate = millis();
}

void updateMood(Pet &pet) {
  if (!pet.isAlive) return;
  if (millis() - pet.lastMoodUpdate < 300000UL) return;
  pet.lastMoodUpdate = millis();
  int score = 0;
  if (pet.happiness > 80) score += 2;
  else if (pet.happiness > 60) score += 1;
  else if (pet.happiness < 30) score -= 2;
  else if (pet.happiness < 50) score -= 1;
  if (pet.health > 70) score += 1;
  else if (pet.health < 30) score -= 2;
  if (pet.energy > 70) score += 1;
  else if (pet.energy < 20) score -= 1;
  if (pet.hunger < 20) score -= 2;
  else if (pet.hunger > 70) score += 1;
  if (pet.cleanliness < 30) score -= 1;
  else if (pet.cleanliness > 80) score += 1;
  int personalityMod = (pet.personalityCheerful - 50) / 25;
  score += personalityMod;
  if (score >= 4) pet.mood = 0;
  else if (score >= 2) pet.mood = 1;
  else if (score >= 0) pet.mood = 2;
  else if (score >= -1) pet.mood = 3;
  else if (score >= -3) pet.mood = 4;
  else if (score >= -5) pet.mood = 5;
  else pet.mood = 6;
}

String getMoodName(int mood) {
  switch (mood) {
    case 0: return "Ecstatic";
    case 1: return "Happy";
    case 2: return "Content";
    case 3: return "Neutral";
    case 4: return "Sad";
    case 5: return "Upset";
    case 6: return "Miserable";
    default: return "Neutral";
  }
}

String getMoodEmoji(int mood) {
  switch (mood) {
    case 0: return "🤩";
    case 1: return "😊";
    case 2: return "🙂";
    case 3: return "😐";
    case 4: return "😢";
    case 5: return "😠";
    case 6: return "😭";
    default: return "😐";
  }
}

int getMoodHappinessMod(int mood) {
  switch (mood) {
    case 0: return 3;
    case 1: return 2;
    case 2: return 1;
    case 3: return 0;
    case 4: return -1;
    case 5: return -2;
    case 6: return -3;
    default: return 0;
  }
}

// ============================================================
// Phase 7.5: Scheduled Feeding
// ============================================================

void initScheduledFeed(Pet &pet) {
  pet.scheduledFeedEnabled = false;
  pet.scheduledFeedInterval = 4UL * 60UL * 60UL * 1000UL;
  pet.lastScheduledFeed = millis();
  pet.scheduledFeedAmount = 15;
}

void checkScheduledFeed(Pet &pet) {
  if (!pet.scheduledFeedEnabled || !pet.isAlive) return;
  if (pet.hunger >= STAT_MAX) return;
  unsigned long now = millis();
  if (now - pet.lastScheduledFeed >= pet.scheduledFeedInterval) {
    pet.lastScheduledFeed = now;
    pet.hunger = constrain(pet.hunger + pet.scheduledFeedAmount, STAT_MIN, STAT_MAX);
    pet.state = "eating";
    pet.stateChangeTime = now;
    pet.lastInteractionTime = now;
  }
}

void setScheduledFeed(bool enabled, unsigned long intervalMs, int amount) {
  (void)enabled;
  (void)intervalMs;
  (void)amount;
}

String getScheduledFeedJson(Pet &pet) {
  String result = "{";
  result += "\"enabled\":" + String(pet.scheduledFeedEnabled ? "true" : "false") + ",";
  result += "\"intervalMs\":" + String(pet.scheduledFeedInterval) + ",";
  result += "\"intervalHours\":" + String(pet.scheduledFeedInterval / 3600000UL) + ",";
  result += "\"amount\":" + String(pet.scheduledFeedAmount) + ",";
  unsigned long remaining = 0;
  if (pet.scheduledFeedEnabled && pet.isAlive) {
    unsigned long elapsed = millis() - pet.lastScheduledFeed;
    if (elapsed < pet.scheduledFeedInterval) {
      remaining = pet.scheduledFeedInterval - elapsed;
    }
  }
  result += "\"nextFeedMs\":" + String(remaining);
  result += "}";
  return result;
}

// ============================================================
// Music (stubbed for tests)
// ============================================================

#define NOTE_C4  262
#define NOTE_D4  294
#define NOTE_E4  330
#define NOTE_F4  349
#define NOTE_G4  392
#define NOTE_A4  440
#define NOTE_B4  494
#define NOTE_C5  523
#define NOTE_D5  587
#define NOTE_E5  659
#define NOTE_F5  698
#define NOTE_G5  784
#define NOTE_C6  1047
#define NOTE_REST 0

static int melodyHappy[] = {NOTE_C4, NOTE_E4, NOTE_G4, NOTE_C5, NOTE_G4, NOTE_E4, NOTE_C4};
static int melodyHappyLen = 7;
static int melodySleep[] = {NOTE_E4, NOTE_D4, NOTE_C4, NOTE_D4, NOTE_E4, NOTE_E4, NOTE_E4};
static int melodySleepLen = 7;
static int melodySick[] = {NOTE_A4, NOTE_REST, NOTE_A4, NOTE_REST, NOTE_G4, NOTE_REST, NOTE_G4};
static int melodySickLen = 7;
static int melodyDying[] = {NOTE_C5, NOTE_B4, NOTE_A4, NOTE_G4, NOTE_F4, NOTE_E4, NOTE_D4, NOTE_C4};
static int melodyDyingLen = 8;
static int melodyEvolve[] = {NOTE_C4, NOTE_E4, NOTE_G4, NOTE_C5, NOTE_E5, NOTE_G5};
static int melodyEvolveLen = 6;

static int melodyFeed[] = {NOTE_C4, NOTE_E4, NOTE_G4, NOTE_C5};
static int melodyFeedLen = 4;
static int melodyPlay[] = {NOTE_E4, NOTE_G4, NOTE_C5, NOTE_E5, NOTE_C5, NOTE_G4};
static int melodyPlayLen = 6;
static int melodyDeath[] = {NOTE_C5, NOTE_B4, NOTE_A4, NOTE_G4, NOTE_F4, NOTE_E4, NOTE_D4, NOTE_C4};
static int melodyDeathLen = 8;
static int melodyEvolve2[] = {NOTE_C4, NOTE_D4, NOTE_E4, NOTE_F4, NOTE_G4, NOTE_A4, NOTE_B4, NOTE_C5};
static int melodyEvolveLen2 = 8;
static int melodyBirth[] = {NOTE_C4, NOTE_E4, NOTE_G4, NOTE_C5, NOTE_E5, NOTE_G5, NOTE_C6};
static int melodyBirthLen = 7;
static int melodyAlert[] = {NOTE_A4, NOTE_REST, NOTE_A4, NOTE_REST, NOTE_A4, NOTE_REST, NOTE_A4};
static int melodyAlertLen = 7;
static int melodyLullaby[] = {NOTE_E4, NOTE_D4, NOTE_C4, NOTE_D4, NOTE_E4, NOTE_E4, NOTE_E4, NOTE_D4, NOTE_D4, NOTE_D4, NOTE_E4, NOTE_G4, NOTE_G4};
static int melodyLullabyLen = 13;

const int *melodyLibrary[] = {
  melodyHappy, melodySleep, melodySick, melodyDying, melodyEvolve,
  melodyFeed, melodyPlay, melodyDeath, melodyEvolve2, melodyBirth, melodyAlert, melodyLullaby
};
const int melodyLengths[] = {
  melodyHappyLen, melodySleepLen, melodySickLen, melodyDyingLen,
  melodyEvolveLen, melodyFeedLen, melodyPlayLen, melodyDeathLen,
  melodyEvolveLen2, melodyBirthLen, melodyAlertLen, melodyLullabyLen
};
const char *melodyNames[] = {
  "Happy", "Sleep", "Sick", "Dying", "Evolve", "Feed", "Play", "Death",
  "Ascending", "Birth", "Alert", "Lullaby"
};
const int melodyCount = 12;

int melodyConfig[MELODY_COUNT] = {0,1,2,3,4,5,6,7};

void playMelody(const int *melody, int length, int tempo) { (void)melody; (void)length; (void)tempo; }
void playStateMelody(const Pet &pet) { (void)pet; }
void stopMusic() {}
void playMelodyById(int melodyId) { (void)melodyId; }
void setMelodyConfig(int event, int melodyIndex) {
  if (event < 0 || event >= MELODY_COUNT) return;
  if (melodyIndex < 0 || melodyIndex >= melodyCount) return;
  melodyConfig[event] = melodyIndex;
}
int getMelodyConfig(int event) {
  if (event < 0 || event >= MELODY_COUNT) return 0;
  return melodyConfig[event];
}
String getMelodyConfigJson() { return "{}"; }
void setMelodyConfigFromJson(const String &json) { (void)json; }

// ============================================================
// Battery / Power Management (stubbed for tests)
// ============================================================

void updateBatteryLevel(Pet &pet) { (void)pet; }

// ============================================================
// Phase 12.2: Pet Lineage & Genealogy
// ============================================================

void initLineage(Pet &pet) {
  pet.parentIds[0] = "";
  pet.parentIds[1] = "";
  pet.generation = 0;
  pet.birthTimestamp = millis();
  pet.birthDeviceId = "local";
}

void inheritTraits(Pet &pet, const Pet &parent1, const Pet &parent2) {
  // Weighted average of parents' personality traits + random mutation (±10%)
  pet.personalityCheerful = (parent1.personalityCheerful + parent2.personalityCheerful) / 2;
  pet.personalityEnergetic = (parent1.personalityEnergetic + parent2.personalityEnergetic) / 2;
  pet.personalityHungry = (parent1.personalityHungry + parent2.personalityHungry) / 2;

  // Apply mutation: ±10%
  int mutCheer = pet.personalityCheerful / 10;
  int mutEnerg = pet.personalityEnergetic / 10;
  int mutHung = pet.personalityHungry / 10;
  pet.personalityCheerful = constrain(pet.personalityCheerful + random(-mutCheer, mutCheer + 1), 0, 100);
  pet.personalityEnergetic = constrain(pet.personalityEnergetic + random(-mutEnerg, mutEnerg + 1), 0, 100);
  pet.personalityHungry = constrain(pet.personalityHungry + random(-mutHung, mutHung + 1), 0, 100);

  // Set lineage
  pet.parentIds[0] = parent1.birthDeviceId;
  pet.parentIds[1] = parent2.birthDeviceId;
  pet.generation = max(parent1.generation, parent2.generation) + 1;
  pet.birthTimestamp = millis();
  pet.birthDeviceId = "local";
}

String getLineageJson(const Pet &pet) {
  StaticJsonDocument<512> jsonDoc;
  jsonDoc["generation"] = pet.generation;
  jsonDoc["birthTimestamp"] = pet.birthTimestamp;
  jsonDoc["birthDeviceId"] = pet.birthDeviceId.c_str();

  JsonArray parents = jsonDoc.createNestedArray("parents");
  if (pet.parentIds[0].length() > 0) parents.add(pet.parentIds[0].c_str());
  if (pet.parentIds[1].length() > 0) parents.add(pet.parentIds[1].c_str());

  // Include personality traits (genetic)
  JsonObject traits = jsonDoc.createNestedObject("traits");
  traits["cheerful"] = pet.personalityCheerful;
  traits["energetic"] = pet.personalityEnergetic;
  traits["hungry"] = pet.personalityHungry;

  String result;
  serializeJson(jsonDoc, result);
  return result;
}

void recordTradeLineage(Pet &pet, const String &partnerDeviceId, bool isOutgoing) {
  if (isOutgoing) {
    // Record that this pet was traded out
    pet.parentIds[0] = pet.birthDeviceId;
    pet.parentIds[1] = partnerDeviceId;
  } else {
    // Received pet: mark as new generation
    pet.parentIds[0] = partnerDeviceId;
    pet.parentIds[1] = "";
    pet.generation = pet.generation; // Keep existing generation
  }
  pet.birthTimestamp = millis();
}

// ============================================================
// Phase 12.3: Analytics
// ============================================================

void initAnalytics(Pet &pet) {
  pet.dailyFeedCount = 0;
  pet.dailyPlayCount = 0;
  pet.dailySleepCount = 0;
  pet.dailyPlayTimeSec = 0;
  pet.weeklyPlayTimeSec = 0;
  pet.dailyCleanCount = 0;
  pet.dailyHealCount = 0;
  pet.lastDayReset = millis();
  pet.lastWeekReset = millis();
}

void resetDailyCounters(Pet &pet) {
  pet.dailyFeedCount = 0;
  pet.dailyPlayCount = 0;
  pet.dailySleepCount = 0;
  pet.dailyPlayTimeSec = 0;
  pet.dailyCleanCount = 0;
  pet.dailyHealCount = 0;
  pet.lastDayReset = millis();
}

void resetWeeklyCounters(Pet &pet) {
  pet.weeklyPlayTimeSec = 0;
  pet.lastWeekReset = millis();
}

void analyticsOnFeed(Pet &pet) {
  pet.dailyFeedCount++;
}

void analyticsOnPlay(Pet &pet) {
  pet.dailyPlayCount++;
}

void analyticsOnSleep(Pet &pet) {
  pet.dailySleepCount++;
}

void analyticsOnClean(Pet &pet) {
  pet.dailyCleanCount++;
}

void analyticsOnHeal(Pet &pet) {
  pet.dailyHealCount++;
}

void analyticsTick(Pet &pet, unsigned long intervalMs) {
  // Check if day has passed (simplified: 86400000 ms = 24h)
  unsigned long now = millis();
  if (now - pet.lastDayReset >= 86400000UL) {
    resetDailyCounters(pet);
  }
  if (now - pet.lastWeekReset >= 604800000UL) {
    resetWeeklyCounters(pet);
  }
}

String getAnalyticsTrendsJson(const Pet &pet, const String &range) {
  StaticJsonDocument<512> jsonDoc;
  jsonDoc["range"] = range.c_str();

  JsonObject today = jsonDoc.createNestedObject("today");
  today["feeds"] = pet.dailyFeedCount;
  today["plays"] = pet.dailyPlayCount;
  today["sleeps"] = pet.dailySleepCount;
  today["playTimeSec"] = pet.dailyPlayTimeSec;
  today["cleans"] = pet.dailyCleanCount;
  today["heals"] = pet.dailyHealCount;

  JsonObject week = jsonDoc.createNestedObject("week");
  week["playTimeSec"] = pet.weeklyPlayTimeSec;

  // Cumulative stats
  JsonObject cumulative = jsonDoc.createNestedObject("cumulative");
  cumulative["totalFeeds"] = pet.timesFed;
  cumulative["totalPlays"] = pet.timesPlayed;
  cumulative["totalSleeps"] = pet.timesSlept;
  cumulative["totalCleans"] = pet.timesCleaned;
  cumulative["totalHeals"] = pet.timesHealed;
  cumulative["totalPlayTimeSec"] = pet.totalPlayTime;

  String result;
  serializeJson(jsonDoc, result);
  return result;
}

String getAnalyticsCsv(const Pet &pet, const String &range) {
  String csv = "metric,value\n";
  csv += "dailyFeeds,";
  csv += String(pet.dailyFeedCount);
  csv += "\n";
  csv += "dailyPlays,";
  csv += String(pet.dailyPlayCount);
  csv += "\n";
  csv += "totalFeeds,";
  csv += String(pet.timesFed);
  csv += "\n";
  csv += "totalPlays,";
  csv += String(pet.timesPlayed);
  csv += "\n";
  csv += "totalPlayTimeSec,";
  csv += String(pet.totalPlayTime);
  csv += "\n";
  return csv;
}

String getAnalyticsJson(const Pet &pet, const String &range) {
  return getAnalyticsTrendsJson(pet, range);
}

// ============================================================
// Phase 12.4: Accessibility
// ============================================================

void initAccessibility(Pet &pet) {
  pet.highContrastMode = false;
  pet.fontSize = 1;       // medium
  pet.reducedMotion = false;
  pet.soundVolume = 80;   // 80%
}

String getAccessibilityJson(const Pet &pet) {
  StaticJsonDocument<256> jsonDoc;
  jsonDoc["highContrastMode"] = pet.highContrastMode;
  jsonDoc["fontSize"] = pet.fontSize;
  jsonDoc["reducedMotion"] = pet.reducedMotion;
  jsonDoc["soundVolume"] = pet.soundVolume;
  String result;
  serializeJson(jsonDoc, result);
  return result;
}

void setAccessibilityFromJson(Pet &pet, const String &json) {
  StaticJsonDocument<256> jsonDoc;
  DeserializationError err = deserializeJson(jsonDoc, json.c_str());
  if (err) return;

  if (jsonDoc["highContrastMode"].is<bool>()) {
    pet.highContrastMode = jsonDoc["highContrastMode"].as<bool>();
  }
  if (jsonDoc["fontSize"].is<int>()) {
    pet.fontSize = constrain(jsonDoc["fontSize"].as<int>(), 0, 2);
  }
  if (jsonDoc["reducedMotion"].is<bool>()) {
    pet.reducedMotion = jsonDoc["reducedMotion"].as<bool>();
  }
  if (jsonDoc["soundVolume"].is<int>()) {
    pet.soundVolume = constrain(jsonDoc["soundVolume"].as<int>(), 0, 100);
  }
}

// ============================================================
// Phase 12.1: Achievements 2.0 — Test-safe implementations
// These mirror Achievements.cpp but without SPIFFS dependencies
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

AchievementState achievementStates[ACHIEVEMENT_COUNT];

int findAchievementIndex(const char* id) {
  for (int i = 0; i < ACHIEVEMENT_COUNT; i++) {
    if (strcmp(achievementDefs[i].id, id) == 0) return i;
  }
  return -1;
}

void recordAchievementProgress(const char* achId, int amount) {
  int idx = findAchievementIndex(achId);
  if (idx < 0) return;
  if (achievementStates[idx].unlocked) return;
  achievementStates[idx].progress += amount;
  if (achievementStates[idx].progress > achievementDefs[idx].target) {
    achievementStates[idx].progress = achievementDefs[idx].target;
  }
  AchievementTier newTier = calculateTier(achievementStates[idx].progress, achievementDefs[idx].target);
  if (newTier != achievementStates[idx].tier) {
    achievementStates[idx].tier = newTier;
    achievementStates[idx].notified = false;
  }
  if (newTier == TIER_PLATINUM) {
    achievementStates[idx].unlocked = true;
  }
}

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
  if (!any) return "{\"newAchievements\":[]}";
  String result;
  serializeJson(jsonDoc, result);
  return result;
}

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

String getAchievementsJson(const Pet &pet) {
  StaticJsonDocument<512> jsonDoc;
  JsonArray arr = jsonDoc.createNestedArray("achievements");
  for (int i = 0; i < ACHIEVEMENT_COUNT; i++) {
    if (achievementStates[i].unlocked) {
      arr.add(achievementDefs[i].id);
    }
  }
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

void loadAchievements(Pet &pet) {
  for (int i = 0; i < ACHIEVEMENT_COUNT; i++) {
    achievementStates[i].progress = 0;
    achievementStates[i].tier = TIER_NONE;
    achievementStates[i].unlocked = false;
    achievementStates[i].notified = false;
  }
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

void saveAchievements(const Pet &pet) {
  // No-op in test environment (no SPIFFS)
}

void checkAchievements(Pet &pet) {
  if (!pet.isAlive) return;
  if (pet.feedCount >= 10)  recordAchievementProgress(ACH_FED_10X, 0);
  if (pet.feedCount >= 100) recordAchievementProgress(ACH_FED_100X, 0);
  if (pet.playCount >= 10)  recordAchievementProgress(ACH_PLAYED_10X, 0);
  if (pet.playCount >= 100) recordAchievementProgress(ACH_PLAYED_100X, 0);
  if (pet.hasBeenNamed)    recordAchievementProgress(ACH_NAMED_PET, 1);
  if (pet.scheduledFeedEnabled) recordAchievementProgress(ACH_SCHEDULED_FEED, 1);
  if (pet.timesCleaned >= 10) recordAchievementProgress(ACH_CLEANED_10X, 0);
  if (pet.timesHealed >= 5)   recordAchievementProgress(ACH_HEALED_5X, 0);
  if (pet.stage >= CHILD)  recordAchievementProgress(ACH_REACHED_CHILD, 1);
  if (pet.stage >= ADULT)  recordAchievementProgress(ACH_REACHED_ADULT, 1);
  if (pet.stage == ELDER && !pet.elderAchieved) {
    pet.elderAchieved = true;
    recordAchievementProgress(ACH_REACHED_ELDER, 1);
    recordAchievementProgress(ACH_FULLY_EVOLVED, 1);
  }
  if (pet.age >= 1440) recordAchievementProgress(ACH_SURVIVED_24H, 1);
  if (pet.age >= 10080) recordAchievementProgress(ACH_SURVIVED_7D, 1);
}

// ============================================================
// Phase 15.3: Backup & Restore — Test-safe implementations
// ============================================================
// These mirror the production Backup.cpp logic without SPIFFS dependency

static unsigned long crc32_table_bak[256];
static bool crc32_table_bak_initialized = false;

static void init_crc32_table_bak() {
  if (crc32_table_bak_initialized) return;
  for (unsigned long i = 0; i < 256; i++) {
    unsigned long c = i;
    for (int j = 0; j < 8; j++) {
      if (c & 1)
        c = 0xEDB88320UL ^ (c >> 1);
      else
        c >>= 1;
    }
    crc32_table_bak[i] = c;
  }
  crc32_table_bak_initialized = true;
}

static unsigned long crc32_bak(const char *data, size_t len) {
  init_crc32_table_bak();
  unsigned long crc = 0xFFFFFFFFUL;
  for (size_t i = 0; i < len; i++) {
    crc = crc32_table_bak[(crc ^ (unsigned char)data[i]) & 0xFF] ^ (crc >> 8);
  }
  return crc ^ 0xFFFFFFFFUL;
}

// Test-global stats (simulates APP_STATE.stats)
static GameStats testStats = {0,0,0,0,0,0,0,0,0,0};

unsigned long computeBackupChecksum(const Pet &pet) {
  char buf[256];
  snprintf(buf, sizeof(buf), "%s:%d:%d:%d:%d:%d:%d:%d:%d:%d:%d",
    pet.name.c_str(),
    pet.hunger, pet.happiness, pet.health, pet.energy, pet.cleanliness,
    pet.age, pet.feedCount, pet.playCount, pet.generation, pet.stage);
  return crc32_bak(buf, strlen(buf));
}

String createBackupJson(const Pet &pet) {
  DynamicJsonDocument jsonDoc(8192);
  jsonDoc["version"] = BACKUP_VERSION;
  jsonDoc["timestamp"] = millis();

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

  JsonArray achArr = jsonDoc.createNestedArray("achievements");
  for (int i = 0; i < ACHIEVEMENT_COUNT; i++) {
    if (achievementStates[i].progress > 0 || achievementStates[i].unlocked) {
      JsonObject obj = achArr.createNestedObject();
      obj["id"] = achievementDefs[i].id;
      obj["progress"] = achievementStates[i].progress;
      obj["unlocked"] = achievementStates[i].unlocked;
    }
  }

  JsonObject statsObj = jsonDoc.createNestedObject("stats");
  statsObj["playTime"] = testStats.totalPlayTimeSec;
  statsObj["feedCount"] = testStats.totalFeeds;
  statsObj["playCount"] = testStats.totalPlays;
  statsObj["sleepCount"] = testStats.totalSleeps;
  statsObj["cleanCount"] = testStats.totalCleans;
  statsObj["healCount"] = testStats.totalHeals;
  statsObj["deathCount"] = testStats.deaths;
  statsObj["evolutionCount"] = testStats.evolutions;

  JsonObject settingsObj = jsonDoc.createNestedObject("settings");
  settingsObj["soundEnabled"] = pet.soundEnabled;
  settingsObj["language"] = "en";

  jsonDoc["checksum"] = computeBackupChecksum(pet);

  String result;
  serializeJson(jsonDoc, result);
  return result;
}

int verifyBackupJson(const String &json) {
  DynamicJsonDocument jsonDoc(8192);
  DeserializationError err = deserializeJson(jsonDoc, json.c_str());
  if (err) return ERR_INT_JSON_PARSE_FAIL;

  const char *ver = jsonDoc["version"];
  if (!ver) return ERR_BACKUP_VERSION_MISSING;

  if (!jsonDoc["pet"]) return ERR_BACKUP_NO_PET;

  unsigned long storedChecksum = jsonDoc["checksum"] | 0UL;
  if (storedChecksum == 0) return ERR_BACKUP_NO_CHECKSUM;

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

int restoreBackupJson(const String &json, Pet &pet) {
  int verifyResult = verifyBackupJson(json);
  if (verifyResult != ERR_OK) return verifyResult;

  // Verify already parsed, skip re-parsing for restore
  DynamicJsonDocument jsonDoc(8192);
  DeserializationError err = deserializeJson(jsonDoc, json.c_str());
  if (err) return ERR_INT_JSON_PARSE_FAIL;

  JsonObject petObj = jsonDoc["pet"];
  if (petObj) {
    String name = (const char*)(petObj["name"] | "");
    if (name.length() > 0 && name.length() <= 16) {
      pet.name = name;
      pet.hasBeenNamed = true;
    }
    pet.type = (PetType)(int)(petObj["type"] | 0);
    pet.stage = (PetStage)(int)(petObj["stage"] | 0);
    pet.generation = (int)(petObj["generation"] | 1);
    pet.birthDeviceId = (const char*)(petObj["birthDeviceId"] | "");
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
    pet.highContrastMode = (bool)(petObj["highContrastMode"] | false);
    pet.fontSize = constrain((int)(petObj["fontSize"] | 1), 0, 2);
    pet.reducedMotion = (bool)(petObj["reducedMotion"] | false);
    pet.soundVolume = constrain((int)(petObj["soundVolume"] | 80), 0, 100);
    pet.scheduledFeedEnabled = (bool)(petObj["scheduledFeedEnabled"] | false);
    pet.scheduledFeedInterval = constrain((int)(petObj["scheduledFeedInterval"] | 4), 1, 24);
    pet.scheduledFeedAmount = constrain((int)(petObj["scheduledFeedAmount"] | 10), 5, 50);
    pet.mood = constrain((int)(petObj["mood"] | 3), 0, 6);
    pet.personalityCheerful = constrain((int)(petObj["personalityCheerful"] | 50), 0, 100);
    pet.personalityEnergetic = constrain((int)(petObj["personalityEnergetic"] | 50), 0, 100);
    pet.personalityHungry = constrain((int)(petObj["personalityHungry"] | 50), 0, 100);
  }

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

String getBackupVersion(const String &json) {
  DynamicJsonDocument jsonDoc(8192);
  DeserializationError err = deserializeJson(jsonDoc, json.c_str());
  if (err) return "";
  const char *ver = jsonDoc["version"];
  return ver ? String(ver) : "";
}

String migrateBackupJson(const String &json, const String &fromVersion) {
  DynamicJsonDocument jsonDoc(8192);
  DeserializationError err = deserializeJson(jsonDoc, json.c_str());
  if (err) return "";
  jsonDoc["version"] = BACKUP_VERSION;
  String result;
  serializeJson(jsonDoc, result);
  return result;
}
