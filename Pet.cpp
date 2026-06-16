#include "Pet.h"
#include "config.h"

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

  if (pet.isNight && !wasNight) {
    Serial.println("Night has fallen...");
  }

  if (pet.isNight && pet.energy < 50 && pet.state != "sleeping" && pet.state != "dead") {
    pet.state = "sleeping";
    pet.stateChangeTime = millis();
    Serial.println("Pet went to sleep (night time, low energy)");
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
}

void updatePet(Pet &pet) {
  if (!pet.isAlive) return;

  // Phase 4: Check evolution animation timeout
  if (pet.isEvolving && millis() - pet.evolutionStartTime > 3000) {
    pet.isEvolving = false;
    Serial.println("Evolution animation complete");
  }

  // Phase 4: Check dying state
  if (pet.isDying) {
    if (millis() - pet.dyingStartTime > 30000) {
      // Dying window expired - pet dies
      pet.isDying = false;
      pet.isAlive = false;
      pet.state = "dead";
      if (pet.soundEnabled) soundDeath();
      Serial.println("Pet has died (dying window expired)");
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

  // Update evolution stage
  updateStage(pet);

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
      Serial.println("Pet woke up automatically after full rest");
    }
    // Wake up if it's daytime and energy >= 80
    else if (!pet.isNight && pet.energy >= 80) {
      pet.state          = "normal";
      pet.stateChangeTime = millis();
      if (pet.soundEnabled) soundWake();
      Serial.println("Pet woke up with the sunrise");
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
}

// ============================================================
// Actions
// ============================================================

void feedPet(Pet &pet) {
  if (!pet.isAlive) return;

  pet.hunger  = clampStat(pet.hunger  + FEED_HUNGER_GAIN);
  pet.energy  = clampStat(pet.energy  - FEED_ENERGY_COST);
  pet.state   = "eating";
  pet.stateChangeTime    = millis();
  pet.lastInteractionTime = millis();

  // Phase 3: track feed count for achievements
  pet.feedCount++;

  if (pet.soundEnabled) soundFeed();

  // If very hungry, improve health too
  if (pet.hunger < HEALTH_DECAY_THRESHOLD) {
    pet.health = clampStat(pet.health + FEED_HEALTH_BONUS);
  }
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
  Serial.print("Pet evolving to stage ");
  Serial.println(pet.stage);
}

void triggerDying(Pet &pet) {
  pet.isDying = true;
  pet.dyingStartTime = millis();
  pet.state = "dying";
  pet.stateChangeTime = millis();
  Serial.println("Pet is dying! 30 second window to heal!");
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
  Serial.println("Pet revived!");
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
    if (pet.weather != oldWeather) {
      Serial.print("Weather changed to: ");
      Serial.println(getWeatherName(pet.weather));
    }
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

  Serial.print("Game started: ");
  Serial.println(gameType);
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
  DynamicJsonDocument doc(512);
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
    playMelody(melodyDying, melodyDyingLen, 200);
  } else if (pet.isEvolving) {
    playMelody(melodyEvolve, melodyEvolveLen, 120);
  } else if (pet.state == "sleeping") {
    playMelody(melodySleep, melodySleepLen, 300);
  } else if (pet.state == "sick") {
    playMelody(melodySick, melodySickLen, 250);
  } else if (pet.state == "normal" && pet.happiness > 70) {
    playMelody(melodyHappy, melodyHappyLen, 150);
  }
}

void stopMusic() {
  noTone(BUZZER_PIN);
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
