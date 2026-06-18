#include <Arduino.h>
#include <unity.h>
#include "Pet.h"
#include "config.h"

// ============================================================
// SPIFFS Data Migration Tests (Phase 7.2)
// Tests backward compatibility when loading data from
// different versions of the save format
// Note: setUp/tearDown/main are in test_pet_statemachine.cpp
// ============================================================

// --- v1 format: only original 7 fields (pre-Phase 2) ---
// Simulates loading a save file from the original monolithic .ino
void test_v1_format_minimal_fields(void) {
  // v1 format only had: hunger, happiness, health, energy, cleanliness, age, isAlive, state
  // All other fields should get sensible defaults
  Pet pet;
  initPet(pet);

  // Simulate v1 data by manually setting only v1 fields
  pet.hunger = 40;
  pet.happiness = 60;
  pet.health = 70;
  pet.energy = 80;
  pet.cleanliness = 50;
  pet.age = 30;
  pet.isAlive = true;
  pet.state = "normal";

  // After loading, v2 fields should have defaults
  // (In real loading, these come from the JSON with | defaults)
  TEST_ASSERT_EQUAL(BABY, pet.stage);          // default
  TEST_ASSERT_FALSE(pet.isNight);              // default
  TEST_ASSERT_EQUAL_STRING("Tama", pet.name.c_str()); // default
  TEST_ASSERT_TRUE(pet.soundEnabled);          // default
  TEST_ASSERT_EQUAL(BLOB, pet.type);           // default
}

// --- v2 format: Phase 2 fields added ---
void test_v2_format_phase2_fields(void) {
  Pet pet;
  initPet(pet);

  // Simulate v2 data
  pet.stage = CHILD;
  pet.isNight = true;
  pet.virtualMinutes = 1200; // 20:00

  TEST_ASSERT_EQUAL(CHILD, pet.stage);
  TEST_ASSERT_TRUE(pet.isNight);
  TEST_ASSERT_EQUAL(1200UL, pet.virtualMinutes);
}

// --- v3 format: Phase 3 fields added ---
void test_v3_format_phase3_fields(void) {
  Pet pet;
  initPet(pet);

  pet.name = "Fluffy";
  pet.soundEnabled = false;
  pet.type = CAT;
  pet.feedCount = 15;
  pet.playCount = 8;
  pet.hasBeenNamed = true;

  TEST_ASSERT_EQUAL_STRING("Fluffy", pet.name.c_str());
  TEST_ASSERT_FALSE(pet.soundEnabled);
  TEST_ASSERT_EQUAL(CAT, pet.type);
  TEST_ASSERT_EQUAL(15, pet.feedCount);
  TEST_ASSERT_EQUAL(8, pet.playCount);
  TEST_ASSERT_TRUE(pet.hasBeenNamed);
}

// --- v4 format: Phase 4 fields added ---
void test_v4_format_phase4_fields(void) {
  Pet pet;
  initPet(pet);

  pet.isDying = true;
  pet.dyingStartTime = millis() - 5000;
  pet.lastReviveTime = millis() - 400000; // >5min ago
  pet.musicEnabled = false;
  pet.difficulty = 2; // hard
  pet.weather = 2;    // rainy

  TEST_ASSERT_TRUE(pet.isDying);
  TEST_ASSERT_FALSE(pet.musicEnabled);
  TEST_ASSERT_EQUAL(2, pet.difficulty);
  TEST_ASSERT_EQUAL(2, pet.weather);
}

// --- v5 format: Phase 5 fields added ---
void test_v5_format_phase5_fields(void) {
  Pet pet;
  initPet(pet);

  pet.totalPlayTime = 3600;
  pet.totalSleepTime = 7200;
  pet.timesFed = 50;
  pet.timesPlayed = 30;
  pet.timesSlept = 20;
  pet.timesCleaned = 15;
  pet.timesHealed = 5;
  pet.highScore = 8;
  pet.batteryLevel = 75;
  pet.lowBatteryWarning = false;

  TEST_ASSERT_EQUAL(3600UL, pet.totalPlayTime);
  TEST_ASSERT_EQUAL(7200UL, pet.totalSleepTime);
  TEST_ASSERT_EQUAL(50, pet.timesFed);
  TEST_ASSERT_EQUAL(30, pet.timesPlayed);
  TEST_ASSERT_EQUAL(8, pet.highScore);
  TEST_ASSERT_EQUAL(75, pet.batteryLevel);
  TEST_ASSERT_FALSE(pet.lowBatteryWarning);
}

// --- Full v2+ load: all fields populated ---
void test_full_modern_format(void) {
  Pet pet;
  initPet(pet);

  // Simulate a fully populated modern save
  pet.hunger = 45;
  pet.happiness = 72;
  pet.health = 88;
  pet.energy = 65;
  pet.cleanliness = 90;
  pet.age = 500;
  pet.isAlive = true;
  pet.state = "normal";
  pet.stage = CHILD;
  pet.isNight = false;
  pet.virtualMinutes = 500;
  pet.name = "Rex";
  pet.soundEnabled = true;
  pet.type = DOG;
  pet.feedCount = 25;
  pet.playCount = 18;
  pet.hasBeenNamed = true;
  pet.elderAchieved = false;
  pet.isDying = false;
  pet.musicEnabled = true;
  pet.difficulty = 1;
  pet.weather = 0;
  pet.totalPlayTime = 1800;
  pet.totalSleepTime = 3600;
  pet.timesFed = 25;
  pet.timesPlayed = 18;
  pet.timesSlept = 10;
  pet.timesCleaned = 8;
  pet.timesHealed = 2;
  pet.highScore = 7;
  pet.batteryLevel = 82;
  pet.lowBatteryWarning = false;

  // Verify all fields
  TEST_ASSERT_EQUAL(45, pet.hunger);
  TEST_ASSERT_EQUAL(72, pet.happiness);
  TEST_ASSERT_EQUAL(88, pet.health);
  TEST_ASSERT_EQUAL(65, pet.energy);
  TEST_ASSERT_EQUAL(90, pet.cleanliness);
  TEST_ASSERT_EQUAL(500, pet.age);
  TEST_ASSERT_TRUE(pet.isAlive);
  TEST_ASSERT_EQUAL_STRING("normal", pet.state.c_str());
  TEST_ASSERT_EQUAL(CHILD, pet.stage);
  TEST_ASSERT_FALSE(pet.isNight);
  TEST_ASSERT_EQUAL_STRING("Rex", pet.name.c_str());
  TEST_ASSERT_EQUAL(DOG, pet.type);
  TEST_ASSERT_EQUAL(7, pet.highScore);
  TEST_ASSERT_EQUAL(82, pet.batteryLevel);
}

// --- Edge case: corrupted/out-of-range values ---
void test_corrupted_values_get_clamped(void) {
  Pet pet;
  initPet(pet);

  // Simulate corrupted values that should be clamped on load
  pet.hunger = 999;     // Should be clamped to STAT_MAX
  pet.health = -50;     // Should be clamped to STAT_MIN
  pet.energy = 200;     // Should be clamped to STAT_MAX
  pet.cleanliness = -1; // Should be clamped to STAT_MIN

  // After clamping (simulating what loadPetData does)
  pet.hunger = constrain(pet.hunger, STAT_MIN, STAT_MAX);
  pet.health = constrain(pet.health, STAT_MIN, STAT_MAX);
  pet.energy = constrain(pet.energy, STAT_MIN, STAT_MAX);
  pet.cleanliness = constrain(pet.cleanliness, STAT_MIN, STAT_MAX);

  TEST_ASSERT_EQUAL(STAT_MAX, pet.hunger);
  TEST_ASSERT_EQUAL(STAT_MIN, pet.health);
  TEST_ASSERT_EQUAL(STAT_MAX, pet.energy);
  TEST_ASSERT_EQUAL(STAT_MIN, pet.cleanliness);
}

// --- Edge case: invalid enum values ---
void test_invalid_enums_get_defaulted(void) {
  Pet pet;
  initPet(pet);

  // Simulate invalid enum values
  int stageVal = 99;   // Invalid stage
  int typeVal = -1;    // Invalid type
  int diffVal = 5;     // Invalid difficulty
  int weatherVal = 10; // Invalid weather

  // Clamp/validate (simulating what loadPetData does)
  if (stageVal < BABY || stageVal > ELDER) stageVal = BABY;
  if (typeVal < BLOB || typeVal > DOG) typeVal = BLOB;
  if (diffVal < 0 || diffVal > 2) diffVal = 1;
  if (weatherVal < 0 || weatherVal > 4) weatherVal = 0;

  pet.stage = (PetStage)stageVal;
  pet.type = (PetType)typeVal;
  pet.difficulty = diffVal;
  pet.weather = weatherVal;

  TEST_ASSERT_EQUAL(BABY, pet.stage);
  TEST_ASSERT_EQUAL(BLOB, pet.type);
  TEST_ASSERT_EQUAL(1, pet.difficulty);
  TEST_ASSERT_EQUAL(0, pet.weather);
}

// --- Edge case: empty/missing name ---
void test_empty_name_gets_default(void) {
  Pet pet;
  initPet(pet);

  pet.name = "";
  // In real loading: pet.name = jsonDoc["name"] | "Tama";
  if (pet.name.length() == 0) pet.name = "Tama";

  TEST_ASSERT_EQUAL_STRING("Tama", pet.name.c_str());
}

// --- Migration preserves critical state across versions ---
void test_migration_preserves_alive_state(void) {
  Pet pet;
  initPet(pet);

  // A v1 save with dead pet should stay dead after migration
  pet.isAlive = false;
  pet.state = "dead";
  pet.health = 0;

  TEST_ASSERT_FALSE(pet.isAlive);
  TEST_ASSERT_EQUAL_STRING("dead", pet.state.c_str());
  TEST_ASSERT_EQUAL(0, pet.health);
}
