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
  pet.isDying              = false;
  pet.dyingStartTime       = 0;
  pet.lastReviveTime       = 0;
  pet.isEvolving           = false;
  pet.previousStage        = BABY;
  pet.evolutionStartTime   = 0;
  pet.weather              = 0;
  pet.weatherChangeTime    = 0;
  pet.musicEnabled         = true;
  pet.difficulty           = 1;
  pet.gameCooldown         = 0;
  pet.activeGame           = 0;
  pet.gameScore            = 0;
  pet.gameRound            = 0;
  memset(pet.memorySequence, 0, sizeof(pet.memorySequence));
  pet.catchTargetX         = 50;
  pet.catchTargetY         = 50;

  // Phase 5 init
  pet.totalPlayTime        = 0;
  pet.totalSleepTime       = 0;
  pet.timesFed             = 0;
  pet.timesPlayed          = 0;
  pet.timesSlept           = 0;
  pet.timesCleaned         = 0;
  pet.timesHealed          = 0;
  pet.highScore            = 0;
  pet.dailyActiveMinutes   = 0;
  pet.weeklyActiveMinutes  = 0;
  pet.notificationCount    = 0;
  pet.batteryLevel         = -1;
  pet.lowBatteryWarning    = false;
  pet.lastBatteryCheck     = 0;
}

void updatePet(Pet &pet) {
  if (!pet.isAlive) return;

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
    int hungerDecay = randomVariation((HUNGER_DECAY_NORMAL * mult) / 100);
    int happyDecay  = randomVariation((HAPPINESS_DECAY_NORMAL * mult) / 100);
    int energyDecay = randomVariation((ENERGY_DECAY_NORMAL * mult) / 100);
    int cleanDecay  = randomVariation((CLEANLINESS_DECAY_NORMAL * mult) / 100);

    pet.hunger      = clampStat(pet.hunger      - hungerDecay);
    pet.happiness   = clampStat(pet.happiness   - happyDecay);
    pet.energy      = clampStat(pet.energy      - energyDecay);
    pet.cleanliness = clampStat(pet.cleanliness - cleanDecay);
  }

  // Health decreases if hunger or cleanliness too low
  if (pet.hunger < HEALTH_DECAY_THRESHOLD || pet.cleanliness < HEALTH_DECAY_THRESHOLD) {
    pet.health = clampStat(pet.health - HEALTH_DECAY_AMOUNT);
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

  // Phase 5: statistics
  pet.timesFed++;

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

  // Phase 5: statistics
  pet.timesPlayed++;
  pet.totalPlayTime += PET_UPDATE_INTERVAL / 1000;

  if (pet.soundEnabled) soundPlay();
}

void cleanPet(Pet &pet) {
  if (!pet.isAlive) return;

  pet.cleanliness = STAT_MAX;
  pet.health      = clampStat(pet.health + CLEAN_HEALTH_BONUS);
  pet.lastInteractionTime = millis();
  pet.timesCleaned++;
}

void sleepPet(Pet &pet) {
  if (!pet.isAlive) return;

  if (pet.state == "sleeping") {
    pet.state = "normal";
  } else {
    pet.state          = "sleeping";
    pet.stateChangeTime = millis();
    pet.timesSlept++;
  }
  pet.lastInteractionTime = millis();
}

void healPet(Pet &pet) {
  if (!pet.isAlive) return;

  pet.health = STAT_MAX;
  pet.state  = "normal";
  pet.lastInteractionTime = millis();
  pet.timesHealed++;
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
// Phase 5: Battery & Power
// ============================================================

void updateBatteryLevel(Pet &pet) {
  if (millis() - pet.lastBatteryCheck < SLEEP_CHECK_INTERVAL) return;
  pet.lastBatteryCheck = millis();

#ifdef BATTERY_ADC_PIN
  int raw = analogRead(BATTERY_ADC_PIN);
  float voltage = (raw / 4095.0) * 3.3 * 2.0;
  int pct = (int)((voltage - 3.0) / (4.2 - 3.0) * 100);
  if (pct < 0) pct = 0;
  if (pct > 100) pct = 100;
  pet.batteryLevel = pct;
  if (pct <= LOW_BATTERY_THRESHOLD) pet.lowBatteryWarning = true;
  else if (pct > LOW_BATTERY_THRESHOLD + 5) pet.lowBatteryWarning = false;
#else
  pet.batteryLevel = -1;
#endif
}

String getBatteryJson(const Pet &pet) {
  DynamicJsonDocument jsonDoc(256);
  jsonDoc["batteryLevel"] = pet.batteryLevel;
  jsonDoc["lowBatteryWarning"] = pet.lowBatteryWarning;
  jsonDoc["hasBatteryMonitor"] = (pet.batteryLevel >= 0);
  String result; serializeJson(jsonDoc, result);
  return result;
}
