#include <Arduino.h>
#include <unity.h>
#include "Pet.h"
#include "Achievements.h"
#include "config.h"

// ============================================================
// Phase 12.1: Achievements 2.0 Unit Tests
// Note: setUp/tearDown/main are in test_pet_statemachine.cpp
// ============================================================

static void resetAchievementStates(void) {
  for (int i = 0; i < ACHIEVEMENT_COUNT; i++) {
    achievementStates[i].progress = 0;
    achievementStates[i].tier = TIER_NONE;
    achievementStates[i].unlocked = false;
    achievementStates[i].notified = false;
  }
}

// --- Tier Calculation Tests ---

void test_tier_none_when_zero_progress(void) {
  TEST_ASSERT_EQUAL(TIER_NONE, calculateTier(0, 100));
}

void test_tier_bronze_at_1_percent(void) {
  TEST_ASSERT_EQUAL(TIER_BRONZE, calculateTier(1, 100));
}

void test_tier_bronze_at_24_percent(void) {
  TEST_ASSERT_EQUAL(TIER_BRONZE, calculateTier(24, 100));
}

void test_tier_silver_at_25_percent(void) {
  TEST_ASSERT_EQUAL(TIER_SILVER, calculateTier(25, 100));
}

void test_tier_silver_at_49_percent(void) {
  TEST_ASSERT_EQUAL(TIER_SILVER, calculateTier(49, 100));
}

void test_tier_gold_at_50_percent(void) {
  TEST_ASSERT_EQUAL(TIER_GOLD, calculateTier(50, 100));
}

void test_tier_gold_at_74_percent(void) {
  TEST_ASSERT_EQUAL(TIER_GOLD, calculateTier(74, 100));
}

void test_tier_platinum_at_75_percent(void) {
  TEST_ASSERT_EQUAL(TIER_PLATINUM, calculateTier(75, 100));
}

void test_tier_platinum_at_100_percent(void) {
  TEST_ASSERT_EQUAL(TIER_PLATINUM, calculateTier(100, 100));
}

void test_tier_platinum_capped_at_target(void) {
  TEST_ASSERT_EQUAL(TIER_PLATINUM, calculateTier(150, 100));
}

void test_tier_none_for_zero_target(void) {
  TEST_ASSERT_EQUAL(TIER_NONE, calculateTier(5, 0));
}

// --- Tier String Tests ---

void test_tier_to_string_none(void) {
  TEST_ASSERT_EQUAL_STRING("none", tierToString(TIER_NONE));
}

void test_tier_to_string_bronze(void) {
  TEST_ASSERT_EQUAL_STRING("bronze", tierToString(TIER_BRONZE));
}

void test_tier_to_string_silver(void) {
  TEST_ASSERT_EQUAL_STRING("silver", tierToString(TIER_SILVER));
}

void test_tier_to_string_gold(void) {
  TEST_ASSERT_EQUAL_STRING("gold", tierToString(TIER_GOLD));
}

void test_tier_to_string_platinum(void) {
  TEST_ASSERT_EQUAL_STRING("platinum", tierToString(TIER_PLATINUM));
}

// --- Category String Tests ---

void test_category_to_string_care(void) {
  TEST_ASSERT_EQUAL_STRING("care", categoryToString(CAT_CARE));
}

void test_category_to_string_evolution(void) {
  TEST_ASSERT_EQUAL_STRING("evolution", categoryToString(CAT_EVOLUTION));
}

void test_category_to_string_social(void) {
  TEST_ASSERT_EQUAL_STRING("social", categoryToString(CAT_SOCIAL));
}

void test_category_to_string_exploration(void) {
  TEST_ASSERT_EQUAL_STRING("exploration", categoryToString(CAT_EXPLORATION));
}

// --- Record Progress Tests ---

void test_record_progress_increments_progress(void) {
  resetAchievementStates();
  recordAchievementProgress(ACH_FED_10X, 5);
  TEST_ASSERT_EQUAL(5, achievementStates[0].progress);
}

void test_record_progress_caps_at_target(void) {
  resetAchievementStates();
  recordAchievementProgress(ACH_NAMED_PET, 5); // target is 1
  int idx = -1;
  for (int i = 0; i < ACHIEVEMENT_COUNT; i++) {
    if (strcmp(achievementDefs[i].id, ACH_NAMED_PET) == 0) { idx = i; break; }
  }
  TEST_ASSERT_TRUE(idx >= 0);
  TEST_ASSERT_EQUAL(1, achievementStates[idx].progress);
  TEST_ASSERT_TRUE(achievementStates[idx].unlocked);
}

void test_record_progress_updates_tier(void) {
  resetAchievementStates();
  // ACH_FED_10X target=10, 5 progress = 50% = GOLD
  recordAchievementProgress(ACH_FED_10X, 5);
  TEST_ASSERT_EQUAL(TIER_GOLD, achievementStates[0].tier);
}

void test_record_progress_ignores_invalid_id(void) {
  resetAchievementStates();
  recordAchievementProgress("nonexistent_id", 10);
  TEST_ASSERT_EQUAL(0, achievementStates[0].progress);
}

void test_record_progress_ignores_already_unlocked(void) {
  resetAchievementStates();
  recordAchievementProgress(ACH_NAMED_PET, 1);
  int idx = -1;
  for (int i = 0; i < ACHIEVEMENT_COUNT; i++) {
    if (strcmp(achievementDefs[i].id, ACH_NAMED_PET) == 0) { idx = i; break; }
  }
  TEST_ASSERT_TRUE(achievementStates[idx].unlocked);
  recordAchievementProgress(ACH_NAMED_PET, 5);
  TEST_ASSERT_EQUAL(1, achievementStates[idx].progress);
}

void test_record_progress_sets_notified_false_on_new_tier(void) {
  resetAchievementStates();
  achievementStates[0].notified = true;
  achievementStates[0].tier = TIER_NONE;
  // Adding progress to reach bronze should reset notified
  recordAchievementProgress(ACH_FED_10X, 1); // 1/10 = 10% = BRONZE
  TEST_ASSERT_FALSE(achievementStates[0].notified);
}

// --- Achievement Definitions Tests ---

void test_achievement_count_is_16(void) {
  TEST_ASSERT_EQUAL(16, ACHIEVEMENT_COUNT);
}

void test_achievement_have_valid_targets(void) {
  for (int i = 0; i < ACHIEVEMENT_COUNT; i++) {
    TEST_ASSERT_TRUE(achievementDefs[i].target > 0);
    TEST_ASSERT_TRUE(achievementDefs[i].id != NULL);
    TEST_ASSERT_TRUE(achievementDefs[i].name != NULL);
  }
}

void test_achievement_categories_valid(void) {
  for (int i = 0; i < ACHIEVEMENT_COUNT; i++) {
    TEST_ASSERT_TRUE(achievementDefs[i].category >= CAT_CARE);
    TEST_ASSERT_TRUE(achievementDefs[i].category <= CAT_EXPLORATION);
  }
}

// --- Progress JSON Tests ---

void test_achievements_progress_json_format(void) {
  Pet pet;
  initPet(pet);
  resetAchievementStates();

  achievementStates[0].progress = 5;
  achievementStates[0].tier = TIER_GOLD;

  String json = getAchievementsProgressJson(pet);
  TEST_ASSERT_TRUE(json.length() > 0);
  TEST_ASSERT_TRUE(json.indexOf("\"achievements\"") >= 0);
  TEST_ASSERT_TRUE(json.indexOf("\"tier\"") >= 0);
  TEST_ASSERT_TRUE(json.indexOf("\"progress\"") >= 0);
  TEST_ASSERT_TRUE(json.indexOf("\"target\"") >= 0);
}

void test_achievements_progress_json_contains_all_achievements(void) {
  Pet pet;
  initPet(pet);
  resetAchievementStates();

  String json = getAchievementsProgressJson(pet);
  // Should contain all 16 achievement IDs
  for (int i = 0; i < ACHIEVEMENT_COUNT; i++) {
    TEST_ASSERT_TRUE(json.indexOf(achievementDefs[i].id) >= 0);
  }
}

// --- Newly Unlocked JSON Tests ---

void test_newly_unlocked_json_empty_when_none(void) {
  resetAchievementStates();
  String json = getNewlyUnlockedJson();
  TEST_ASSERT_TRUE(json.indexOf("\"newAchievements\":[]") >= 0);
}

void test_newly_unlocked_json_returns_unnotified(void) {
  resetAchievementStates();
  achievementStates[0].tier = TIER_BRONZE;
  achievementStates[0].notified = false;
  String json = getNewlyUnlockedJson();
  TEST_ASSERT_TRUE(json.indexOf(achievementDefs[0].id) >= 0);
  // After call, notified should be true
  TEST_ASSERT_TRUE(achievementStates[0].notified);
}

// --- Unlocked Rewards Tests ---

void test_unlocked_rewards_empty_when_none_unlocked(void) {
  resetAchievementStates();
  String json = getUnlockedRewardsJson();
  TEST_ASSERT_TRUE(json.indexOf("\"rewards\":[]") >= 0);
}

void test_unlocked_rewards_includes_unlocked_rewards(void) {
  resetAchievementStates();
  // ACH_FED_10X has reward "skin_green"
  achievementStates[0].unlocked = true;
  achievementStates[0].tier = TIER_PLATINUM;
  String json = getUnlockedRewardsJson();
  TEST_ASSERT_TRUE(json.indexOf("skin_green") >= 0);
}

// --- Legacy JSON Test ---

void test_legacy_achievements_json_format(void) {
  Pet pet;
  initPet(pet);
  pet.age = 120; // > 60 for survived_1h
  pet.feedCount = 15;
  pet.playCount = 20;
  pet.hasBeenNamed = true;
  pet.elderAchieved = true;

  String json = getAchievementsJson(pet);
  TEST_ASSERT_TRUE(json.length() > 0);
  TEST_ASSERT_TRUE(json.indexOf("\"achievements\"") >= 0);
  TEST_ASSERT_TRUE(json.indexOf("survived_1h") >= 0);
  TEST_ASSERT_TRUE(json.indexOf("fed_10x") >= 0);
  TEST_ASSERT_TRUE(json.indexOf("played_10x") >= 0);
  TEST_ASSERT_TRUE(json.indexOf("named_pet") >= 0);
  TEST_ASSERT_TRUE(json.indexOf("reached_elder") >= 0);
}
