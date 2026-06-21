#include <Arduino.h>
#include <unity.h>
#include <ArduinoJson.h>
#include "Pet.h"
#include "Achievements.h"
#include "config.h"
#include "Backup.h"
#include "ErrorCode.h"

// ============================================================
// Phase 12.5 / 15.3: Backup & Restore Unit Tests
// Note: setUp/tearDown/main are in test_pet_statemachine.cpp
// ============================================================

// --- Backup JSON Format Tests ---

void test_backup_json_contains_version(void) {
  Pet pet;
  initPet(pet);
  String backup = createBackupJson(pet);
  TEST_ASSERT_TRUE(backup.indexOf("\"version\":\"" BACKUP_VERSION "\"") >= 0);
}

void test_backup_json_contains_pet_object(void) {
  Pet pet;
  initPet(pet);
  String backup = createBackupJson(pet);
  TEST_ASSERT_TRUE(backup.indexOf("\"pet\"") >= 0);
}

void test_backup_json_contains_checksum(void) {
  Pet pet;
  initPet(pet);
  String backup = createBackupJson(pet);
  TEST_ASSERT_TRUE(backup.indexOf("\"checksum\"") >= 0);
}

void test_backup_json_contains_timestamp(void) {
  Pet pet;
  initPet(pet);
  String backup = createBackupJson(pet);
  TEST_ASSERT_TRUE(backup.indexOf("\"timestamp\"") >= 0);
}

void test_backup_json_contains_achievements_array(void) {
  Pet pet;
  initPet(pet);
  String backup = createBackupJson(pet);
  TEST_ASSERT_TRUE(backup.indexOf("\"achievements\"") >= 0);
}

void test_backup_json_contains_stats_object(void) {
  Pet pet;
  initPet(pet);
  String backup = createBackupJson(pet);
  TEST_ASSERT_TRUE(backup.indexOf("\"stats\"") >= 0);
}

void test_backup_json_contains_settings_object(void) {
  Pet pet;
  initPet(pet);
  String backup = createBackupJson(pet);
  TEST_ASSERT_TRUE(backup.indexOf("\"settings\"") >= 0);
}

void test_backup_json_contains_core_stats(void) {
  Pet pet;
  initPet(pet);
  pet.hunger = 75;
  pet.happiness = 80;
  pet.health = 90;
  pet.energy = 70;
  pet.cleanliness = 85;
  pet.age = 100;
  String backup = createBackupJson(pet);
  TEST_ASSERT_TRUE(backup.indexOf("\"hunger\"") >= 0);
  TEST_ASSERT_TRUE(backup.indexOf("\"happiness\"") >= 0);
  TEST_ASSERT_TRUE(backup.indexOf("\"health\"") >= 0);
  TEST_ASSERT_TRUE(backup.indexOf("\"energy\"") >= 0);
  TEST_ASSERT_TRUE(backup.indexOf("\"cleanliness\"") >= 0);
  TEST_ASSERT_TRUE(backup.indexOf("\"age\"") >= 0);
}

// --- Backup Round-Trip Tests ---

// Helper: deserialize from String using .c_str() to work around ArduinoJson
// 6.18.5 native test String incompatibility
static void deserializeFromString(DynamicJsonDocument &doc, const String &str) {
  deserializeJson(doc, str.c_str());
}

void test_backup_restore_roundtrip_pet_name(void) {
  Pet pet;
  initPet(pet);
  pet.name = "TestPet";
  pet.hasBeenNamed = true;

  String backup = createBackupJson(pet);

  DynamicJsonDocument jsonDoc(8192);
  deserializeFromString(jsonDoc, backup);
  const char* namePtr = jsonDoc["pet"]["name"];
  String restoredName = namePtr ? namePtr : "";

  TEST_ASSERT_EQUAL_STRING("TestPet", restoredName.c_str());
}

void test_backup_restore_roundtrip_accessibility(void) {
  Pet pet;
  initPet(pet);
  pet.highContrastMode = true;
  pet.fontSize = 2;
  pet.reducedMotion = true;
  pet.soundVolume = 42;

  String backup = createBackupJson(pet);

  DynamicJsonDocument jsonDoc(8192);
  deserializeFromString(jsonDoc, backup);
  TEST_ASSERT_EQUAL(true, jsonDoc["pet"]["highContrastMode"].as<bool>());
  TEST_ASSERT_EQUAL(2, jsonDoc["pet"]["fontSize"].as<int>());
  TEST_ASSERT_EQUAL(true, jsonDoc["pet"]["reducedMotion"].as<bool>());
  TEST_ASSERT_EQUAL(42, jsonDoc["pet"]["soundVolume"].as<int>());
}

void test_backup_restore_roundtrip_achievements(void) {
  Pet pet;
  initPet(pet);

  // Set some achievement progress
  for (int i = 0; i < ACHIEVEMENT_COUNT; i++) {
    achievementStates[i].progress = 0;
    achievementStates[i].unlocked = false;
  }
  achievementStates[0].progress = 7;
  achievementStates[0].unlocked = false;
  achievementStates[1].progress = 50;
  achievementStates[1].unlocked = true;

  String backup = createBackupJson(pet);

  DynamicJsonDocument jsonDoc(8192);
  deserializeFromString(jsonDoc, backup);
  JsonArray achArr = jsonDoc["achievements"];
  TEST_ASSERT_TRUE(achArr.size() >= 2);

  // Verify achievement data
  bool foundFed10x = false;
  bool foundFed100x = false;
  for (JsonObject obj : achArr) {
    const char* id = obj["id"];
    if (id && strcmp(id, ACH_FED_10X) == 0) {
      TEST_ASSERT_EQUAL(7, obj["progress"].as<int>());
      TEST_ASSERT_EQUAL(false, obj["unlocked"].as<bool>());
      foundFed10x = true;
    }
    if (id && strcmp(id, ACH_FED_100X) == 0) {
      TEST_ASSERT_EQUAL(50, obj["progress"].as<int>());
      TEST_ASSERT_EQUAL(true, obj["unlocked"].as<bool>());
      foundFed100x = true;
    }
  }
  TEST_ASSERT_TRUE(foundFed10x);
  TEST_ASSERT_TRUE(foundFed100x);
}

void test_backup_restore_roundtrip_core_stats(void) {
  Pet pet;
  initPet(pet);
  pet.hunger = 75;
  pet.happiness = 80;
  pet.health = 90;
  pet.energy = 70;
  pet.cleanliness = 85;
  pet.age = 100;

  String backup = createBackupJson(pet);

  DynamicJsonDocument jsonDoc(8192);
  deserializeFromString(jsonDoc, backup);
  TEST_ASSERT_EQUAL(75, jsonDoc["pet"]["hunger"].as<int>());
  TEST_ASSERT_EQUAL(80, jsonDoc["pet"]["happiness"].as<int>());
  TEST_ASSERT_EQUAL(90, jsonDoc["pet"]["health"].as<int>());
  TEST_ASSERT_EQUAL(70, jsonDoc["pet"]["energy"].as<int>());
  TEST_ASSERT_EQUAL(85, jsonDoc["pet"]["cleanliness"].as<int>());
  TEST_ASSERT_EQUAL(100, jsonDoc["pet"]["age"].as<int>());
}

// --- Checksum Tests ---

void test_backup_checksum_changes_with_stats(void) {
  Pet pet1, pet2;
  initPet(pet1);
  initPet(pet2);
  pet2.hunger = 99;

  String backup1 = createBackupJson(pet1);
  String backup2 = createBackupJson(pet2);

  DynamicJsonDocument doc1(8192), doc2(8192);
  deserializeFromString(doc1, backup1);
  deserializeFromString(doc2, backup2);

  TEST_ASSERT_NOT_EQUAL(doc1["checksum"].as<unsigned long>(), doc2["checksum"].as<unsigned long>());
}

void test_backup_checksum_same_for_identical_pets(void) {
  Pet pet;
  initPet(pet);

  String backup1 = createBackupJson(pet);
  String backup2 = createBackupJson(pet);

  DynamicJsonDocument doc1(8192), doc2(8192);
  deserializeFromString(doc1, backup1);
  deserializeFromString(doc2, backup2);

  TEST_ASSERT_EQUAL(doc1["checksum"].as<unsigned long>(), doc2["checksum"].as<unsigned long>());
}

// --- Edge Case Tests ---

void test_backup_empty_name(void) {
  Pet pet;
  initPet(pet);
  pet.name = "";

  String backup = createBackupJson(pet);
  DynamicJsonDocument jsonDoc(8192);
  deserializeFromString(jsonDoc, backup);
  const char* namePtr = jsonDoc["pet"]["name"];
  String name = namePtr ? namePtr : "";
  TEST_ASSERT_EQUAL_STRING("", name.c_str());
}

void test_backup_no_achievements_unlocked(void) {
  Pet pet;
  initPet(pet);
  for (int i = 0; i < ACHIEVEMENT_COUNT; i++) {
    achievementStates[i].progress = 0;
    achievementStates[i].unlocked = false;
  }

  String backup = createBackupJson(pet);
  DynamicJsonDocument jsonDoc(8192);
  deserializeFromString(jsonDoc, backup);
  JsonArray achArr = jsonDoc["achievements"];
  TEST_ASSERT_EQUAL(0, achArr.size());
}

void test_backup_generation_preserved(void) {
  Pet pet;
  initPet(pet);
  pet.generation = 5;
  pet.birthDeviceId = "test_device_123";

  String backup = createBackupJson(pet);
  DynamicJsonDocument jsonDoc(8192);
  deserializeFromString(jsonDoc, backup);

  TEST_ASSERT_EQUAL(5, jsonDoc["pet"]["generation"].as<int>());
  const char* devId = jsonDoc["pet"]["birthDeviceId"];
  TEST_ASSERT_TRUE(devId && String("test_device_123") == devId);
}

// --- Phase 15.3: Verify & Restore API Tests ---

void test_backup_verify_valid_backup(void) {
  Pet pet;
  initPet(pet);
  pet.name = "VerifyTest";
  pet.hunger = 60;

  String backup = createBackupJson(pet);
  int result = verifyBackupJson(backup);
  TEST_ASSERT_EQUAL(ERR_OK, result);
}

void test_backup_verify_invalid_json(void) {
  String invalidJson = "not valid json {{{";
  int result = verifyBackupJson(invalidJson);
  TEST_ASSERT_NOT_EQUAL(ERR_OK, result);
}

void test_backup_verify_missing_pet(void) {
  String badBackup = "{\"version\":\"1.5.0\",\"checksum\":123}";
  int result = verifyBackupJson(badBackup);
  TEST_ASSERT_EQUAL(ERR_BACKUP_NO_PET, result);
}

void test_backup_verify_checksum_mismatch(void) {
  Pet pet;
  initPet(pet);
  String backup = createBackupJson(pet);

  // Tamper with the backup by changing a stat value
  DynamicJsonDocument jsonDoc(8192);
  deserializeFromString(jsonDoc, backup);
  jsonDoc["pet"]["hunger"] = 999;  // Tampered
  String tampered;
  serializeJson(jsonDoc, tampered);

  int result = verifyBackupJson(tampered);
  TEST_ASSERT_EQUAL(ERR_BACKUP_CHECKSUM_MISMATCH, result);
}

void test_backup_restore_roundtrip_full_pet(void) {
  Pet pet;
  initPet(pet);
  pet.name = "FullTest";
  pet.hasBeenNamed = true;
  pet.hunger = 65;
  pet.happiness = 70;
  pet.health = 80;
  pet.energy = 55;
  pet.cleanliness = 90;
  pet.age = 42;
  pet.isAlive = true;
  pet.soundEnabled = false;
  pet.feedCount = 15;
  pet.playCount = 20;
  pet.highContrastMode = true;
  pet.fontSize = 2;
  pet.reducedMotion = true;
  pet.soundVolume = 30;
  pet.mood = 2;
  pet.personalityCheerful = 80;
  pet.personalityEnergetic = 60;
  pet.personalityHungry = 40;

  String backup = createBackupJson(pet);

  // Verify first
  TEST_ASSERT_EQUAL(ERR_OK, verifyBackupJson(backup));

  // Restore into a fresh pet
  Pet restored;
  initPet(restored);
  int result = restoreBackupJson(backup, restored);
  TEST_ASSERT_EQUAL(ERR_OK, result);

  // Verify all fields
  TEST_ASSERT_EQUAL_STRING("FullTest", restored.name.c_str());
  TEST_ASSERT_EQUAL(true, restored.hasBeenNamed);
  TEST_ASSERT_EQUAL(65, restored.hunger);
  TEST_ASSERT_EQUAL(70, restored.happiness);
  TEST_ASSERT_EQUAL(80, restored.health);
  TEST_ASSERT_EQUAL(55, restored.energy);
  TEST_ASSERT_EQUAL(90, restored.cleanliness);
  TEST_ASSERT_EQUAL(42, restored.age);
  TEST_ASSERT_EQUAL(true, restored.isAlive);
  TEST_ASSERT_EQUAL(false, restored.soundEnabled);
  TEST_ASSERT_EQUAL(15, restored.feedCount);
  TEST_ASSERT_EQUAL(20, restored.playCount);
  TEST_ASSERT_EQUAL(true, restored.highContrastMode);
  TEST_ASSERT_EQUAL(2, restored.fontSize);
  TEST_ASSERT_EQUAL(true, restored.reducedMotion);
  TEST_ASSERT_EQUAL(30, restored.soundVolume);
  TEST_ASSERT_EQUAL(2, restored.mood);
  TEST_ASSERT_EQUAL(80, restored.personalityCheerful);
  TEST_ASSERT_EQUAL(60, restored.personalityEnergetic);
  TEST_ASSERT_EQUAL(40, restored.personalityHungry);
}

void test_backup_version_string(void) {
  Pet pet;
  initPet(pet);
  String backup = createBackupJson(pet);
  String ver = getBackupVersion(backup);
  TEST_ASSERT_EQUAL_STRING(BACKUP_VERSION, ver.c_str());
}
