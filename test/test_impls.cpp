// test_impls.cpp - Testable implementations extracted from Pet.cpp
// Strips hardware dependencies (Serial, tone, analogRead, etc.) for native unit testing
#include "Pet.h"
#include "config.h"
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
