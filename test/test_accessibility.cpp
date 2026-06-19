#include <Arduino.h>
#include <unity.h>
#include "Pet.h"
#include "config.h"

// ============================================================
// Phase 12.4: Accessibility & UX Unit Tests
// Note: setUp/tearDown/main are in test_pet_statemachine.cpp
// ============================================================

// --- Init Tests ---

void test_initAccessibility_defaults(void) {
  Pet pet;
  initPet(pet);
  initAccessibility(pet);

  TEST_ASSERT_FALSE(pet.highContrastMode);
  TEST_ASSERT_EQUAL(1, pet.fontSize);       // medium
  TEST_ASSERT_FALSE(pet.reducedMotion);
  TEST_ASSERT_EQUAL(80, pet.soundVolume);
}

// --- JSON Serialization Tests ---

void test_getAccessibilityJson_format(void) {
  Pet pet;
  initPet(pet);
  initAccessibility(pet);

  String json = getAccessibilityJson(pet);
  TEST_ASSERT_TRUE(json.length() > 0);
  TEST_ASSERT_TRUE(json.indexOf("\"highContrastMode\"") >= 0);
  TEST_ASSERT_TRUE(json.indexOf("\"fontSize\"") >= 0);
  TEST_ASSERT_TRUE(json.indexOf("\"reducedMotion\"") >= 0);
  TEST_ASSERT_TRUE(json.indexOf("\"soundVolume\"") >= 0);
}

void test_getAccessibilityJson_default_values(void) {
  Pet pet;
  initPet(pet);
  initAccessibility(pet);

  String json = getAccessibilityJson(pet);
  TEST_ASSERT_TRUE(json.indexOf("\"highContrastMode\":false") >= 0);
  TEST_ASSERT_TRUE(json.indexOf("\"fontSize\":1") >= 0);
  TEST_ASSERT_TRUE(json.indexOf("\"reducedMotion\":false") >= 0);
  TEST_ASSERT_TRUE(json.indexOf("\"soundVolume\":80") >= 0);
}

// --- JSON Deserialization Tests ---

void test_setAccessibilityFromJson_highContrast(void) {
  Pet pet;
  initPet(pet);
  initAccessibility(pet);

  String json = "{\"highContrastMode\":true}";
  setAccessibilityFromJson(pet, json);
  TEST_ASSERT_TRUE(pet.highContrastMode);
}

void test_setAccessibilityFromJson_fontSize(void) {
  Pet pet;
  initPet(pet);
  initAccessibility(pet);

  setAccessibilityFromJson(pet, "{\"fontSize\":2}");
  TEST_ASSERT_EQUAL(2, pet.fontSize);
}

void test_setAccessibilityFromJson_reducedMotion(void) {
  Pet pet;
  initPet(pet);
  initAccessibility(pet);

  setAccessibilityFromJson(pet, "{\"reducedMotion\":true}");
  TEST_ASSERT_TRUE(pet.reducedMotion);
}

void test_setAccessibilityFromJson_soundVolume(void) {
  Pet pet;
  initPet(pet);
  initAccessibility(pet);

  setAccessibilityFromJson(pet, "{\"soundVolume\":50}");
  TEST_ASSERT_EQUAL(50, pet.soundVolume);
}

// --- Validation Tests ---

void test_setAccessibilityFromJson_clamps_fontSize(void) {
  Pet pet;
  initPet(pet);
  initAccessibility(pet);

  setAccessibilityFromJson(pet, "{\"fontSize\":5}");
  TEST_ASSERT_EQUAL(2, pet.fontSize); // clamped to max 2

  setAccessibilityFromJson(pet, "{\"fontSize\":-1}");
  TEST_ASSERT_EQUAL(0, pet.fontSize); // clamped to min 0
}

void test_setAccessibilityFromJson_clamps_soundVolume(void) {
  Pet pet;
  initPet(pet);
  initAccessibility(pet);

  setAccessibilityFromJson(pet, "{\"soundVolume\":150}");
  TEST_ASSERT_EQUAL(100, pet.soundVolume); // clamped to max 100

  setAccessibilityFromJson(pet, "{\"soundVolume\":-10}");
  TEST_ASSERT_EQUAL(0, pet.soundVolume); // clamped to min 0
}

void test_setAccessibilityFromJson_ignores_invalid_json(void) {
  Pet pet;
  initPet(pet);
  initAccessibility(pet);
  pet.highContrastMode = false;
  pet.fontSize = 1;

  setAccessibilityFromJson(pet, "not valid json{{{");
  // Should not crash, values unchanged
  TEST_ASSERT_FALSE(pet.highContrastMode);
  TEST_ASSERT_EQUAL(1, pet.fontSize);
}

void test_setAccessibilityFromJson_full_settings(void) {
  Pet pet;
  initPet(pet);
  initAccessibility(pet);

  setAccessibilityFromJson(pet, "{\"highContrastMode\":true,\"fontSize\":2,\"reducedMotion\":true,\"soundVolume\":30}");
  TEST_ASSERT_TRUE(pet.highContrastMode);
  TEST_ASSERT_EQUAL(2, pet.fontSize);
  TEST_ASSERT_TRUE(pet.reducedMotion);
  TEST_ASSERT_EQUAL(30, pet.soundVolume);
}

void test_setAccessibilityFromJson_partial_settings(void) {
  Pet pet;
  initPet(pet);
  initAccessibility(pet);

  // Only set highContrastMode, others should remain unchanged
  setAccessibilityFromJson(pet, "{\"highContrastMode\":true}");
  TEST_ASSERT_TRUE(pet.highContrastMode);
  TEST_ASSERT_EQUAL(1, pet.fontSize);       // unchanged
  TEST_ASSERT_FALSE(pet.reducedMotion);      // unchanged
  TEST_ASSERT_EQUAL(80, pet.soundVolume);    // unchanged
}
