#include <Arduino.h>
#include <unity.h>
#include <ArduinoJson.h>
#include "Pet.h"
#include "Achievements.h"
#include "config.h"

// ============================================================
// Phase 12.5: Backup & Restore Unit Tests
// Note: setUp/tearDown/main are in test_pet_statemachine.cpp
// ============================================================

// --- Backup JSON Format Tests ---

static String createBackupJson(const Pet &pet) {
  // Simulate what handleGetBackup does (without server dependency)
  DynamicJsonDocument jsonDoc(8192);
  jsonDoc["version"] = "1.2.0";
  jsonDoc["timestamp"] = millis();

  JsonObject petObj = jsonDoc.createNestedObject("pet");
  petObj["name"] = pet.name.c_str();
  petObj["type"] = (int)pet.type;
  petObj["stage"] = (int)pet.stage;
  petObj["generation"] = pet.generation;
  petObj["birthDeviceId"] = pet.birthDeviceId.c_str();
  petObj["highContrastMode"] = pet.highContrastMode;
  petObj["fontSize"] = pet.fontSize;
  petObj["reducedMotion"] = pet.reducedMotion;
  petObj["soundVolume"] = pet.soundVolume;

  JsonArray achArr = jsonDoc.createNestedArray("achievements");
  for (int i = 0; i < ACHIEVEMENT_COUNT; i++) {
    if (achievementStates[i].progress > 0 || achievementStates[i].unlocked) {
      JsonObject obj = achArr.createNestedObject();
      obj["id"] = achievementDefs[i].id;
      obj["progress"] = achievementStates[i].progress;
      obj["unlocked"] = achievementStates[i].unlocked;
    }
  }

  unsigned long checksum = 0;
  checksum += pet.hunger + pet.happiness + pet.health;
  checksum += pet.energy + pet.cleanliness + pet.age;
  checksum += pet.feedCount + pet.playCount;
  jsonDoc["checksum"] = checksum;

  String backup;
  serializeJson(jsonDoc, backup);
  return backup;
}

void test_backup_json_contains_version(void) {
  Pet pet;
  initPet(pet);
  String backup = createBackupJson(pet);
  TEST_ASSERT_TRUE(backup.indexOf("\"version\":\"1.2.0\"") >= 0);
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

// --- Backup Round-Trip Tests ---

void test_backup_restore_roundtrip_pet_name(void) {
  Pet pet;
  initPet(pet);
  pet.name = "TestPet";
  pet.hasBeenNamed = true;

  String backup = createBackupJson(pet);

  // Parse backup and restore
  DynamicJsonDocument jsonDoc(8192);
  deserializeJson(jsonDoc, backup);
  JsonObject petObj = jsonDoc["pet"];
  String restoredName = petObj["name"] | "";

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
  deserializeJson(jsonDoc, backup);
  JsonObject petObj = jsonDoc["pet"];

  TEST_ASSERT_EQUAL(true, (bool)(petObj["highContrastMode"] | false));
  TEST_ASSERT_EQUAL(2, (int)(petObj["fontSize"] | 1));
  TEST_ASSERT_EQUAL(true, (bool)(petObj["reducedMotion"] | false));
  TEST_ASSERT_EQUAL(42, (int)(petObj["soundVolume"] | 80));
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
  deserializeJson(jsonDoc, backup);
  JsonArray achArr = jsonDoc["achievements"];
  TEST_ASSERT_TRUE(achArr.size() >= 2);

  // Verify achievement data
  bool foundFed10x = false;
  bool foundFed100x = false;
  for (JsonObject obj : achArr) {
    if (strcmp(obj["id"], ACH_FED_10X) == 0) {
      TEST_ASSERT_EQUAL(7, (int)(obj["progress"] | 0));
      TEST_ASSERT_EQUAL(false, (bool)(obj["unlocked"] | false));
      foundFed10x = true;
    }
    if (strcmp(obj["id"], ACH_FED_100X) == 0) {
      TEST_ASSERT_EQUAL(50, (int)(obj["progress"] | 0));
      TEST_ASSERT_EQUAL(true, (bool)(obj["unlocked"] | false));
      foundFed100x = true;
    }
  }
  TEST_ASSERT_TRUE(foundFed10x);
  TEST_ASSERT_TRUE(foundFed100x);
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
  deserializeJson(doc1, backup1);
  deserializeJson(doc2, backup2);

  TEST_ASSERT_NOT_EQUAL((unsigned long)(doc1["checksum"] | 0), (unsigned long)(doc2["checksum"] | 0));
}

void test_backup_checksum_same_for_identical_pets(void) {
  Pet pet;
  initPet(pet);

  String backup1 = createBackupJson(pet);
  String backup2 = createBackupJson(pet);

  DynamicJsonDocument doc1(8192), doc2(8192);
  deserializeJson(doc1, backup1);
  deserializeJson(doc2, backup2);

  TEST_ASSERT_EQUAL((unsigned long)(doc1["checksum"] | 0), (unsigned long)(doc2["checksum"] | 0));
}

// --- Edge Case Tests ---

void test_backup_empty_name(void) {
  Pet pet;
  initPet(pet);
  pet.name = "";

  String backup = createBackupJson(pet);
  DynamicJsonDocument jsonDoc(8192);
  deserializeJson(jsonDoc, backup);
  String name = jsonDoc["pet"]["name"] | "default";
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
  deserializeJson(jsonDoc, backup);
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
  deserializeJson(jsonDoc, backup);

  TEST_ASSERT_EQUAL(5, (int)(jsonDoc["pet"]["generation"] | 0));
  TEST_ASSERT_TRUE(String("test_device_123") == (jsonDoc["pet"]["birthDeviceId"] | ""));
}
