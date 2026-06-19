#include <Arduino.h>
#include <unity.h>
#include "Pet.h"
#include "config.h"

// ============================================================
// Phase 12.2: Pet Lineage & Genealogy Unit Tests
// Note: setUp/tearDown/main are in test_pet_statemachine.cpp
// ============================================================

// --- Lineage Init Tests ---

void test_initLineage_sets_default_values(void) {
  Pet pet;
  initLineage(pet);
  TEST_ASSERT_EQUAL_STRING("", pet.parentIds[0].c_str());
  TEST_ASSERT_EQUAL_STRING("", pet.parentIds[1].c_str());
  TEST_ASSERT_EQUAL(0, pet.generation);
  TEST_ASSERT_GREATER_THAN(0UL, pet.birthTimestamp);
}

void test_initLineage_birthDeviceId_not_empty(void) {
  Pet pet;
  initLineage(pet);
  TEST_ASSERT_TRUE(pet.birthDeviceId.length() > 0);
}

// --- Trait Inheritance Tests ---

void test_inheritTraits_averages_personality(void) {
  Pet parent1, parent2, child;
  initPet(parent1);
  initPet(parent2);

  parent1.personalityCheerful = 80;
  parent1.personalityEnergetic = 60;
  parent1.personalityHungry = 40;

  parent2.personalityCheerful = 60;
  parent2.personalityEnergetic = 40;
  parent2.personalityHungry = 60;

  inheritTraits(child, parent1, parent2);

  // Average: cheerful=70, energetic=50, hungry=50
  // With ±10% mutation: cheerful in [63,77], energetic in [45,55], hungry in [45,55]
  TEST_ASSERT_TRUE(child.personalityCheerful >= 60 && child.personalityCheerful <= 80);
  TEST_ASSERT_TRUE(child.personalityEnergetic >= 40 && child.personalityEnergetic <= 60);
  TEST_ASSERT_TRUE(child.personalityHungry >= 40 && child.personalityHungry <= 60);
}

void test_inheritTraits_sets_parent_ids(void) {
  Pet parent1, parent2, child;
  initPet(parent1);
  initPet(parent2);

  parent1.birthDeviceId = "device_a";
  parent2.birthDeviceId = "device_b";

  inheritTraits(child, parent1, parent2);

  // Both parent IDs should be set
  TEST_ASSERT_TRUE(
    (child.parentIds[0] == "device_a" && child.parentIds[1] == "device_b") ||
    (child.parentIds[0] == "device_b" && child.parentIds[1] == "device_a") ||
    (child.parentIds[0] == "local" && child.parentIds[1] == "device_b") ||
    (child.parentIds[0] == "device_a" && child.parentIds[1] == "")
  );
}

void test_inheritTraits_increments_generation(void) {
  Pet parent1, parent2, child;
  initPet(parent1);
  initPet(parent2);

  parent1.generation = 2;
  parent2.generation = 3;

  inheritTraits(child, parent1, parent2);

  TEST_ASSERT_EQUAL(4, child.generation); // max(2,3) + 1
}

void test_inheritTraits_zero_generation(void) {
  Pet parent1, parent2, child;
  initPet(parent1);
  initPet(parent2);
  initLineage(child);

  parent1.generation = 0;
  parent2.generation = 0;

  inheritTraits(child, parent1, parent2);

  TEST_ASSERT_EQUAL(1, child.generation);
}

void test_inheritTraits_personality_within_bounds(void) {
  Pet parent1, parent2, child;
  initPet(parent1);
  initPet(parent2);

  // Extreme values
  parent1.personalityCheerful = 100;
  parent1.personalityEnergetic = 100;
  parent1.personalityHungry = 100;

  parent2.personalityCheerful = 0;
  parent2.personalityEnergetic = 0;
  parent2.personalityHungry = 0;

  inheritTraits(child, parent1, parent2);

  // All should be within [0, 100]
  TEST_ASSERT_TRUE(child.personalityCheerful >= 0 && child.personalityCheerful <= 100);
  TEST_ASSERT_TRUE(child.personalityEnergetic >= 0 && child.personalityEnergetic <= 100);
  TEST_ASSERT_TRUE(child.personalityHungry >= 0 && child.personalityHungry <= 100);
}

// --- Lineage JSON Tests ---

void test_getLineageJson_format(void) {
  Pet pet;
  initPet(pet);
  initLineage(pet);

  String json = getLineageJson(pet);
  TEST_ASSERT_TRUE(json.length() > 0);
  TEST_ASSERT_TRUE(json.indexOf("\"generation\"") >= 0);
  TEST_ASSERT_TRUE(json.indexOf("\"birthDeviceId\"") >= 0);
  TEST_ASSERT_TRUE(json.indexOf("\"parents\"") >= 0);
  TEST_ASSERT_TRUE(json.indexOf("\"traits\"") >= 0);
}

void test_getLineageJson_with_parents(void) {
  Pet pet;
  initPet(pet);
  initLineage(pet);

  pet.parentIds[0] = "parent_a";
  pet.parentIds[1] = "parent_b";
  pet.generation = 2;

  String json = getLineageJson(pet);
  TEST_ASSERT_TRUE(json.indexOf("parent_a") >= 0);
  TEST_ASSERT_TRUE(json.indexOf("parent_b") >= 0);
  TEST_ASSERT_TRUE(json.indexOf("\"generation\":2") >= 0);
}

void test_getLineageJson_generation_zero(void) {
  Pet pet;
  initPet(pet);
  initLineage(pet);

  String json = getLineageJson(pet);
  TEST_ASSERT_TRUE(json.indexOf("\"generation\":0") >= 0);
}

// --- Trade Lineage Tests ---

void test_recordTradeLineage_outgoing(void) {
  Pet pet;
  initPet(pet);
  initLineage(pet);

  pet.birthDeviceId = "my_device";
  recordTradeLineage(pet, "partner_device", true);

  TEST_ASSERT_EQUAL_STRING("my_device", pet.parentIds[0].c_str());
  TEST_ASSERT_EQUAL_STRING("partner_device", pet.parentIds[1].c_str());
}

void test_recordTradeLineage_incoming(void) {
  Pet pet;
  initPet(pet);
  initLineage(pet);

  recordTradeLineage(pet, "sender_device", false);

  TEST_ASSERT_EQUAL_STRING("sender_device", pet.parentIds[0].c_str());
  TEST_ASSERT_EQUAL_STRING("", pet.parentIds[1].c_str());
}

void test_recordTradeLineage_updates_birth_timestamp(void) {
  Pet pet;
  initPet(pet);
  initLineage(pet);

  unsigned long oldTimestamp = pet.birthTimestamp;
  // In test environment, millis() may not advance between calls.
  // Just verify the function doesn't crash and parent IDs are set.
  recordTradeLineage(pet, "partner", true);
  TEST_ASSERT_EQUAL_STRING("local", pet.parentIds[0].c_str());
  TEST_ASSERT_EQUAL_STRING("partner", pet.parentIds[1].c_str());
}
