/usr/bin/bash: warning: setlocale: LC_ALL: cannot change locale (en_US.UTF-8): No such file or directory
#include "Pet.h"
#include "config.h"

// --- Helpers ---
static int clampStat(int value) {
  if (value < STAT_MIN) return STAT_MIN;
  if (value > STAT_MAX) return STAT_MAX;
  return value;
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
}

void updatePet(Pet &pet) {
  if (!pet.isAlive) return;

  if (pet.state == "sleeping") {
    pet.hunger      = clampStat(pet.hunger      - HUNGER_DECAY_SLEEP);
    pet.happiness   = clampStat(pet.happiness   - HAPPINESS_DECAY_SLEEP);
    pet.energy      = clampStat(pet.energy      + ENERGY_REGEN_SLEEP);
    pet.cleanliness = clampStat(pet.cleanliness - CLEANLINESS_DECAY_SLEEP);

    // Auto wake-up when energy is full
    if (pet.energy >= STAT_MAX) {
      pet.state          = "normal";
      pet.stateChangeTime = millis();
      if (pet.soundEnabled) soundWake();
      Serial.println("Pet woke up automatically after full rest");
    }
  } else {
    pet.hunger      = clampStat(pet.hunger      - HUNGER_DECAY_NORMAL);
    pet.happiness   = clampStat(pet.happiness   - HAPPINESS_DECAY_NORMAL);
    pet.energy      = clampStat(pet.energy      - ENERGY_DECAY_NORMAL);
    pet.cleanliness = clampStat(pet.cleanliness - CLEANLINESS_DECAY_NORMAL);
  }

  // Health decreases if hunger or cleanliness too low
  if (pet.hunger < HEALTH_DECAY_THRESHOLD || pet.cleanliness < HEALTH_DECAY_THRESHOLD) {
    pet.health = clampStat(pet.health - HEALTH_DECAY_AMOUNT);
  }

  // Check if pet is sick
  if (pet.health < SICK_THRESHOLD && pet.state != "sick" &&
      pet.state != "dead" && pet.state != "sleeping") {
    pet.state          = "sick";
    pet.stateChangeTime = millis();
  }
  // Check if pet is hungry
  else if (pet.hunger < HEALTH_DECAY_THRESHOLD && pet.state != "sick" &&
           pet.state != "dead" && pet.state != "sleeping") {
    pet.state          = "hungry";
    pet.stateChangeTime = millis();
  }
  // Return to normal after eating / playing timeout
  else if ((pet.state == "eating" || pet.state == "playing") &&
           millis() - pet.stateChangeTime > PET_STATE_TIMEOUT) {
    pet.state = "normal";
  }

  // Check if pet died
  if (pet.health <= 0) {
    pet.isAlive = false;
    pet.state   = "dead";
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

  if (pet.soundEnabled) soundFeed();

  // If very hungry, improve health too
  if (pet.hunger < HEALTH_DECAY_THRESHOLD) {
    pet.health = clampStat(pet.health + FEED_HEALTH_BONUS);
  }
}

void playPet(Pet &pet) {
  if (!pet.isAlive) return;
  if (pet.energy < PLAY_ENERGY_MIN) return;

  pet.happiness = clampStat(pet.happiness + PLAY_HAPPINESS_GAIN);
  pet.energy    = clampStat(pet.energy    - PLAY_ENERGY_COST);
  pet.hunger    = clampStat(pet.hunger    - PLAY_HUNGER_COST);
  pet.state     = "playing";
  pet.stateChangeTime     = millis();
  pet.lastInteractionTime = millis();

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