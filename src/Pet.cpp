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

  // Phase 2 init
  pet.stage           = BABY;
  pet.isNight         = false;
  pet.virtualMinutes  = 6 * VIRTUAL_MINUTES_PER_HOUR; // Start at 6:00 (morning)

  // Phase 3 init
  pet.name            = "Tama";
  pet.soundEnabled    = true;
  pet.type            = BLOB;

  // Achievement tracking init
  pet.feedCount       = 0;
  pet.playCount       = 0;
  pet.hasBeenNamed    = false;
  pet.elderAchieved   = false;

  // Phase 4 init
  pet.isDying           = false;
  pet.dyingStartTime    = 0;
  pet.lastReviveTime    = 0;
  pet.isEvolving        = false;
  pet.previousStage     = BABY;
  pet.evolutionStartTime = 0;
  pet.weather           = 0; // sunny
  pet.weatherChangeTime = millis();
  pet.musicEnabled      = true;
  pet.difficulty        = 1; // normal
  pet.gameCooldown      = 0;
  pet.activeGame        = 0;
  pet.gameScore         = 0;
  pet.gameRound         = 0;
  memset(pet.memorySequence, 0, sizeof(pet.memorySequence));
  pet.catchTargetX      = 50;
  pet.catchTargetY      = 50;

  // Phase 5 init
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

  // Phase 7.5 init
  initMoodSystem(pet);
  initScheduledFeed(pet);
}

void updatePet(Pet &pet) {
  if (!pet.isAlive) return;

  // Phase 7.3: Periodic heap logging (every 10 ticks = 10 minutes)
  static uint8_t heapLogCounter = 0;
  if (++heapLogCounter >= 10) {
    heapLogCounter = 0;
    Serial.printf("[MEM] Free heap: %u bytes, min free: %u bytes\n",
                  ESP.getFreeHeap(), ESP.getMinFreeHeap());
  }

  // Phase 4: Check evolution animation timeout
  if (pet.isEvolving && millis() - pet.evolutionStartTime > 3000) {
    pet.isEvolving = false;
  }

  // Phase 4: Check dying state
  if (pet.isDying) {
    if (millis() - pet.dyingStartTime > 30000) {
      // Dying window expired - pet dies
      pet.isDying = false;
      pet.isAlive = false;
      pet.state = "dead";
      if (pet.soundEnabled) soundDeath();
      return;
    }
    // While dying, don't do other state updates
    return;
  }

  // Phase 4: Update weather
  updateWeather(pet);

  // Phase 4: Update game cooldown
  if (pet.gameCooldown > 0) {
    pet.gameCooldown--;
  }

  // Phase 4: Check stage evolution
  PetStage oldStage = pet.stage;
  updateStage(pet);
  if (pet.stage != oldStage && !pet.isEvolving) {
    triggerEvolution(pet);
  }

  // Update day/night cycle
  updateDayNightCycle(pet);

  // Get stage multiplier for decay rates
  int mult = getStageDecayMultiplier(pet);

  if (pet.state == "sleeping") {
    int hungerDecay = randomVariation((HUNGER_DECAY_SLEEP * mult) / 100);
    int happyDecay  = randomVariation(HAPPINESS_DECAY_SLEEP);
    int energyRegen = randomVariation(ENERGY_REGEN_SLEEP);
    int cleanDecay  = randomVariation(CLEANLINESS_DECAY_SLEEP);

    pet.hunger      = clampStat(pet.hunger      - hungerDecay);
    pet.happiness   = clampStat(pet.happiness   - happyDecay);
    pet.energy      = clampStat(pet.energy      + energyRegen);
    pet.cleanliness = clampStat(pet.cleanliness - cleanDecay);

    // Auto wake-up when energy is full
    if (pet.energy >= STAT_MAX) {
      pet.state          = "normal";
      pet.stateChangeTime = millis();
      if (pet.soundEnabled) soundWake();
    }
    // Wake up if it's daytime and energy >= 80
    else if (!pet.isNight && pet.energy >= 80) {
      pet.state          = "normal";
      pet.stateChangeTime = millis();
      if (pet.soundEnabled) soundWake();
    }
  } else {
    int diffMult = getDifficultyMultiplier(pet.difficulty);
    int hungerDecay = randomVariation((HUNGER_DECAY_NORMAL * mult * diffMult) / 10000);
    int happyDecay  = randomVariation((HAPPINESS_DECAY_NORMAL * mult * diffMult) / 10000);
    int energyDecay = randomVariation((ENERGY_DECAY_NORMAL * mult * diffMult) / 10000);
    int cleanDecay  = randomVariation((CLEANLINESS_DECAY_NORMAL * mult * diffMult) / 10000);

    pet.hunger      = clampStat(pet.hunger      - hungerDecay);
    pet.happiness   = clampStat(pet.happiness   - happyDecay);
    pet.energy      = clampStat(pet.energy      - energyDecay);
    pet.cleanliness = clampStat(pet.cleanliness - cleanDecay);
  }

  // Health decreases if hunger or cleanliness too low
  if (pet.hunger < HEALTH_DECAY_THRESHOLD || pet.cleanliness < HEALTH_DECAY_THRESHOLD) {
    pet.health = clampStat(pet.health - HEALTH_DECAY_AMOUNT);
  }

  // Phase 4: Weather effects on stats
  if (pet.isAlive && !pet.isDying && !pet.isEvolving) {
    static unsigned long lastWeatherEffect = 0;
    if (millis() - lastWeatherEffect > 60000) { // Every 60 seconds
      lastWeatherEffect = millis();
      pet.happiness = clampStat(pet.happiness + getWeatherHappinessMod(pet.weather));
      pet.energy    = clampStat(pet.energy    + getWeatherEnergyMod(pet.weather));
      pet.cleanliness = clampStat(pet.cleanliness + getWeatherCleanlinessMod(pet.weather));
    }
  }

  // Warning States & State Transitions
  if (pet.health <= 0) {
    pet.health = 0;
    pet.isAlive = false;
    pet.state   = "dead";
    if (pet.soundEnabled) soundDeath();
    return;
  }

  if (pet.health <= DYING_HEALTH_MAX && pet.health >= DYING_HEALTH_MIN && pet.isAlive) {
    if (pet.state != "sleeping") {
      pet.state = "dying";
      pet.stateChangeTime = millis();
    }
  }
  else if (pet.health <= CRITICAL_HEALTH_MAX && pet.health >= CRITICAL_HEALTH_MIN) {
    if (pet.state != "sick" && pet.state != "sleeping") {
      pet.state = "critical";
      pet.stateChangeTime = millis();
    }
  }
  else if (pet.health < SICK_THRESHOLD && pet.state != "sleeping") {
    pet.state = "sick";
    pet.stateChangeTime = millis();
  }
  // Phase 4: Dying state (health 1-10)
  else if (pet.health <= 10 && pet.health > 0 && !pet.isDying && pet.state != "sleeping") {
    triggerDying(pet);
    return;
  }
  else if (pet.hunger < HEALTH_DECAY_THRESHOLD && pet.state != "sick" &&
           pet.state != "sleeping") {
    pet.state = "hungry";
    pet.stateChangeTime = millis();
  }
  else if ((pet.state == "eating" || pet.state == "playing") &&
           millis() - pet.stateChangeTime > PET_STATE_TIMEOUT) {
    pet.state = "normal";
  }
  else if ((pet.state == "hungry" || pet.state == "critical") &&
           pet.hunger >= HEALTH_DECAY_THRESHOLD && pet.health > CRITICAL_HEALTH_MAX) {
    pet.state = "normal";
  }
  else if (pet.state == "sick" && pet.health >= SICK_THRESHOLD) {
    pet.state = "normal";
  }

  pet.age++;

  // Phase 7.5: Update mood and check scheduled feeding
  updateMood(pet);
  checkScheduledFeed(pet);
}

// ============================================================
// Actions
// ============================================================

void feedPet(Pet &pet) {
  if (!pet.isAlive) return;

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

  if (pet.soundEnabled) soundFeed();
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

  // Phase 3: track play count for achievements
  pet.playCount++;

  if (pet.soundEnabled) soundPlay();
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

  // Phase 4: Can heal during dying window to save pet
  if (pet.isDying) {
    pet.isDying = false;
    pet.health = 50;
    pet.state  = "normal";
    pet.stateChangeTime = millis();
    pet.lastInteractionTime = millis();
    Serial.println("Pet saved from dying!");
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
// Sound Effects (Buzzer)
// ============================================================

void soundFeed() {
  tone(BUZZER_PIN, 1000, 200);
}

void soundPlay() {
  tone(BUZZER_PIN, 800, 100);
  delay(150);
  tone(BUZZER_PIN, 1200, 100);
}

void soundDeath() {
  for (int freq = 1000; freq >= 200; freq -= 100) {
    tone(BUZZER_PIN, freq, 50);
    delay(60);
  }
  noTone(BUZZER_PIN);
}

void soundWake() {
  for (int freq = 200; freq <= 1000; freq += 200) {
    tone(BUZZER_PIN, freq, 60);
    delay(80);
  }
  noTone(BUZZER_PIN);
}


// ============================================================
// Phase 4: Evolution & Death
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
  if (pet.isDying) return false; // Can't revive while dying - must heal
  // 5 minute cooldown between revives
  if (millis() - pet.lastReviveTime < 300000 && pet.lastReviveTime != 0) return false;
  return true;
}

void revivePet(Pet &pet) {
  if (!canRevive(pet)) return;
  pet.isAlive = true;
  pet.isDying = false;
  pet.health = 30;
  pet.hunger = 30;
  pet.happiness = 20; // Happiness penalty
  pet.energy = 50;
  pet.cleanliness = 50;
  pet.state = "normal";
  pet.stateChangeTime = millis();
  pet.lastReviveTime = millis();
  if (pet.soundEnabled) soundWake();
  // Phase 6: Force save on critical event
  savePetDataForce(pet);
}

// ============================================================
// Phase 4: Weather System
// ============================================================

void updateWeather(Pet &pet) {
  // Change weather every 30-60 minutes (random)
  unsigned long weatherInterval = (30 + random(0, 31)) * 60000UL;
  if (millis() - pet.weatherChangeTime > weatherInterval) {
    int oldWeather = pet.weather;
    pet.weather = random(0, 5); // 0-4
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
    case 0: return 2;   // sunny: happy
    case 1: return 0;   // cloudy: neutral
    case 2: return -3;  // rainy: sad
    case 3: return -5;  // stormy: scared
    case 4: return 3;   // snowy: playful
    default: return 0;
  }
}

int getWeatherEnergyMod(int weather) {
  switch (weather) {
    case 0: return 0;
    case 1: return 0;
    case 2: return -2;
    case 3: return 3;   // storm: excited
    case 4: return -3;  // snow: cold
    default: return 0;
  }
}

int getWeatherCleanlinessMod(int weather) {
  switch (weather) {
    case 0: return 0;
    case 1: return 0;
    case 2: return 5;   // rain: natural bath
    case 3: return 3;
    case 4: return 0;
    default: return 0;
  }
}

// ============================================================
// Phase 4: Mini-Games
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
    // Award happiness based on score
    int bonus = pet.gameScore * 2;
    pet.happiness = clampStat(pet.happiness + bonus);
    if (pet.gameScore >= 5) {
      pet.health = clampStat(pet.health + 5);
    }
  }
  pet.gameCooldown = 60; // 60 second cooldown
  pet.activeGame = 0;
  pet.gameScore = 0;
  pet.gameRound = 0;
}

void generateMemorySequence(Pet &pet) {
  for (int i = 0; i < 10; i++) {
    pet.memorySequence[i] = random(0, 4); // 4 buttons: 0-3
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
      generateMemorySequence(pet); // Extend sequence
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
// Phase 4: Music (Melodies)
// ============================================================

// Note frequencies
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

void playMelody(const int *melody, int length, int tempo) {
  if (!BUZZER_PIN) return;
  for (int i = 0; i < length; i++) {
    if (melody[i] == NOTE_REST) {
      noTone(BUZZER_PIN);
    } else {
      tone(BUZZER_PIN, melody[i], tempo);
    }
    delay(tempo + 50);
  }
  noTone(BUZZER_PIN);
}

void playStateMelody(const Pet &pet) {
  if (!pet.soundEnabled || !pet.musicEnabled) return;

  static unsigned long lastMelody = 0;
  if (millis() - lastMelody < 10000) return; // Max every 10 seconds
  lastMelody = millis();

  if (pet.isDying) {
    playMelodyById(melodyConfig[MELODY_DYING]);
  } else if (pet.isEvolving) {
    playMelodyById(melodyConfig[MELODY_EVOLVE]);
  } else if (pet.state == "sleeping") {
    playMelodyById(melodyConfig[MELODY_SLEEP]);
  } else if (pet.state == "sick") {
    playMelodyById(melodyConfig[MELODY_SICK]);
  } else if (pet.state == "normal" && pet.happiness > 70) {
    playMelodyById(melodyConfig[MELODY_HAPPY]);
  }
}

void stopMusic() {
  noTone(BUZZER_PIN);
}

// ============================================================
// Phase 6.3: Buzzer Melody Configuration
// User-selectable melodies per event, persisted to SPIFFS
// ============================================================

// Extended melody library (8 melodies)
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

// Melody library table
const int *melodyLibrary[] = {
  melodyHappy,    // 0: Happy
  melodySleep,    // 1: Sleep
  melodySick,     // 2: Sick
  melodyDying,    // 3: Dying
  melodyEvolve,   // 4: Evolve
  melodyFeed,     // 5: Feed
  melodyPlay,     // 6: Play
  melodyDeath,    // 7: Death
  melodyEvolve2,  // 8: Ascending
  melodyBirth,    // 9: Birth/Celebration
  melodyAlert,    // 10: Alert
  melodyLullaby,  // 11: Lullaby
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

// Default melody configuration (event -> melody index)
int melodyConfig[MELODY_COUNT] = {
  0,  // HAPPY -> Happy
  1,  // SLEEP -> Sleep
  2,  // SICK -> Sick
  3,  // DYING -> Dying
  4,  // EVOLVE -> Evolve
  5,  // FEED -> Feed
  6,  // PLAY -> Play
  7,  // DEATH -> Death
};

void playMelodyById(int melodyId) {
  if (melodyId < 0 || melodyId >= melodyCount) return;
  playMelody(melodyLibrary[melodyId], melodyLengths[melodyId], 150);
}

void setMelodyConfig(int event, int melodyIndex) {
  if (event < 0 || event >= MELODY_COUNT) return;
  if (melodyIndex < 0 || melodyIndex >= melodyCount) return;
  melodyConfig[event] = melodyIndex;
}

int getMelodyConfig(int event) {
  if (event < 0 || event >= MELODY_COUNT) return 0;
  return melodyConfig[event];
}

String getMelodyConfigJson() {
  StaticJsonDocument<512> doc;

  // Available melodies
  JsonArray lib = doc.createNestedArray("library");
  for (int i = 0; i < melodyCount; i++) {
    JsonObject m = lib.createNestedObject();
    m["id"] = i;
    m["name"] = melodyNames[i];
  }

  // Current config
  JsonArray cfg = doc.createNestedArray("config");
  const char *eventNames[] = {"happy", "sleep", "sick", "dying", "evolve", "feed", "play", "death"};
  for (int i = 0; i < MELODY_COUNT; i++) {
    JsonObject c = cfg.createNestedObject();
    c["event"] = eventNames[i];
    c["melodyId"] = melodyConfig[i];
    c["melodyName"] = melodyNames[melodyConfig[i]];
  }

  String result;
  serializeJson(doc, result);
  return result;
}

void setMelodyConfigFromJson(const String &json) {
  StaticJsonDocument<512> doc;
  if (deserializeJson(doc, json)) return;

  const char *eventNames[] = {"happy", "sleep", "sick", "dying", "evolve", "feed", "play", "death"};
  for (int i = 0; i < MELODY_COUNT; i++) {
    if (doc[eventNames[i]].is<int>()) {
      int melodyId = doc[eventNames[i]].as<int>();
      if (melodyId >= 0 && melodyId < melodyCount) {
        melodyConfig[i] = melodyId;
      }
    }
  }
}

// ============================================================
// Phase 4: Settings Helpers
// ============================================================

int getDifficultyMultiplier(int difficulty) {
  switch (difficulty) {
    case 0: return 80;   // Easy: 80% decay
    case 1: return 100;  // Normal: 100% decay
    case 2: return 120;  // Hard: 120% decay
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
// Phase 5: Battery / Power Management
// ============================================================

void updateBatteryLevel(Pet &pet) {
  // Only check every SLEEP_CHECK_INTERVAL
  if (millis() - pet.lastBatteryCheck < SLEEP_CHECK_INTERVAL && pet.lastBatteryCheck != 0) {
    return;
  }
  pet.lastBatteryCheck = millis();

  // Read ADC and convert to percentage
  // Assumes voltage divider on BATTERY_ADC_PIN (GPIO 34)
  int raw = analogRead(BATTERY_ADC_PIN);
  // Map 0-4095 to 0-100 (approximate, adjust for your voltage divider)
  int pct = map(raw, 0, 4095, 0, 100);
  pct = constrain(pct, 0, 100);
  pet.batteryLevel = pct;

  if (pct < LOW_BATTERY_THRESHOLD && !pet.lowBatteryWarning) {
    pet.lowBatteryWarning = true;
    Serial.println("LOW BATTERY WARNING!");
  } else if (pct >= LOW_BATTERY_THRESHOLD + 5) {
    pet.lowBatteryWarning = false; // Hysteresis
  }
}

// ============================================================
// Phase 7.5: Pet Mood System
// Personality traits affect stat decay and mood evolution
// ============================================================

void initMoodSystem(Pet &pet) {
  pet.mood = 3; // neutral
  pet.personalityCheerful = 50 + random(0, 51);   // 50-100
  pet.personalityEnergetic = 50 + random(0, 51);  // 50-100
  pet.personalityHungry = 30 + random(0, 41);     // 30-70
  pet.lastMoodUpdate = millis();
}

void updateMood(Pet &pet) {
  if (!pet.isAlive) return;

  // Update mood every 5 minutes
  if (millis() - pet.lastMoodUpdate < 300000UL) return;
  pet.lastMoodUpdate = millis();

  // Calculate mood score based on stats and personality
  int score = 0;

  // Happiness contribution (weighted by personality)
  if (pet.happiness > 80) score += 2;
  else if (pet.happiness > 60) score += 1;
  else if (pet.happiness < 30) score -= 2;
  else if (pet.happiness < 50) score -= 1;

  // Health contribution
  if (pet.health > 70) score += 1;
  else if (pet.health < 30) score -= 2;

  // Energy contribution
  if (pet.energy > 70) score += 1;
  else if (pet.energy < 20) score -= 1;

  // Hunger contribution
  if (pet.hunger < 20) score -= 2;
  else if (pet.hunger > 70) score += 1;

  // Cleanliness contribution
  if (pet.cleanliness < 30) score -= 1;
  else if (pet.cleanliness > 80) score += 1;

  // Personality modifier: cheerful pets tend happier
  int personalityMod = (pet.personalityCheerful - 50) / 25; // -2 to +2
  score += personalityMod;

  // Map score to mood
  if (score >= 4) pet.mood = 0;       // ecstatic
  else if (score >= 2) pet.mood = 1;  // happy
  else if (score >= 0) pet.mood = 2;  // content
  else if (score >= -1) pet.mood = 3; // neutral
  else if (score >= -3) pet.mood = 4; // sad
  else if (score >= -5) pet.mood = 5; // upset
  else pet.mood = 6;                   // miserable
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
// Timer-based auto-feeding at configurable intervals
// ============================================================

void initScheduledFeed(Pet &pet) {
  pet.scheduledFeedEnabled = false;
  pet.scheduledFeedInterval = 4UL * 60UL * 60UL * 1000UL; // 4 hours default
  pet.lastScheduledFeed = millis();
  pet.scheduledFeedAmount = 15;
}

void checkScheduledFeed(Pet &pet) {
  if (!pet.scheduledFeedEnabled || !pet.isAlive) return;
  if (pet.hunger >= STAT_MAX) return; // Already full

  unsigned long now = millis();
  if (now - pet.lastScheduledFeed >= pet.scheduledFeedInterval) {
    pet.lastScheduledFeed = now;
    pet.hunger = constrain(pet.hunger + pet.scheduledFeedAmount, STAT_MIN, STAT_MAX);
    pet.state = "eating";
    pet.stateChangeTime = now;
    pet.lastInteractionTime = now;
    if (pet.soundEnabled) soundFeed();
  }
}

void setScheduledFeed(bool enabled, unsigned long intervalMs, int amount) {
  // Note: these are stored in the global pet instance
  // The actual values are applied via the web handler
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
