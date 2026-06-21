#include <Arduino.h>
#include <unity.h>
#include "Pet.h"
#include "Achievements.h"
#include "config.h"

// ============================================================
// Pet State Machine Unit Tests (Phase 7.2)
// Tests core logic without hardware dependencies
// ============================================================

// Forward declarations for SPIFFS migration tests (test_spiffs_migration.cpp)
void test_v1_format_minimal_fields(void);
void test_v2_format_phase2_fields(void);
void test_v3_format_phase3_fields(void);
void test_v4_format_phase4_fields(void);
void test_v5_format_phase5_fields(void);
void test_full_modern_format(void);
void test_corrupted_values_get_clamped(void);
void test_invalid_enums_get_defaulted(void);
void test_empty_name_gets_default(void);
void test_migration_preserves_alive_state(void);

// Forward declarations for Phase 12.1: Achievements 2.0 tests (test_achievements.cpp)
void test_tier_none_when_zero_progress(void);
void test_tier_bronze_at_1_percent(void);
void test_tier_bronze_at_24_percent(void);
void test_tier_silver_at_25_percent(void);
void test_tier_silver_at_49_percent(void);
void test_tier_gold_at_50_percent(void);
void test_tier_gold_at_74_percent(void);
void test_tier_platinum_at_75_percent(void);
void test_tier_platinum_at_100_percent(void);
void test_tier_platinum_capped_at_target(void);
void test_tier_none_for_zero_target(void);
void test_tier_to_string_none(void);
void test_tier_to_string_bronze(void);
void test_tier_to_string_silver(void);
void test_tier_to_string_gold(void);
void test_tier_to_string_platinum(void);
void test_category_to_string_care(void);
void test_category_to_string_evolution(void);
void test_category_to_string_social(void);
void test_category_to_string_exploration(void);
void test_record_progress_increments_progress(void);
void test_record_progress_caps_at_target(void);
void test_record_progress_updates_tier(void);
void test_record_progress_ignores_invalid_id(void);
void test_record_progress_ignores_already_unlocked(void);
void test_record_progress_sets_notified_false_on_new_tier(void);
void test_achievement_count_is_16(void);
void test_achievement_have_valid_targets(void);
void test_achievement_categories_valid(void);
void test_achievements_progress_json_format(void);
void test_achievements_progress_json_contains_all_achievements(void);
void test_newly_unlocked_json_empty_when_none(void);
void test_newly_unlocked_json_returns_unnotified(void);
void test_unlocked_rewards_empty_when_none_unlocked(void);
void test_unlocked_rewards_includes_unlocked_rewards(void);
void test_legacy_achievements_json_format(void);

// Forward declarations for Phase 12.2: Pet Lineage tests (test_lineage.cpp)
void test_initLineage_sets_default_values(void);
void test_initLineage_birthDeviceId_not_empty(void);
void test_inheritTraits_averages_personality(void);
void test_inheritTraits_sets_parent_ids(void);
void test_inheritTraits_increments_generation(void);
void test_inheritTraits_zero_generation(void);
void test_inheritTraits_personality_within_bounds(void);
void test_getLineageJson_format(void);
void test_getLineageJson_with_parents(void);
void test_getLineageJson_generation_zero(void);
void test_recordTradeLineage_outgoing(void);
void test_recordTradeLineage_incoming(void);
void test_recordTradeLineage_updates_birth_timestamp(void);

// Forward declarations for Phase 12.3: Analytics tests (test_stats.cpp)
void test_initAnalytics_resets_all_counters(void);
void test_initAnalytics_sets_reset_timestamps(void);
void test_analyticsOnFeed_increments_daily_feed_count(void);
void test_analyticsOnPlay_increments_daily_play_count(void);
void test_analyticsOnSleep_increments_daily_sleep_count(void);
void test_analyticsOnClean_increments_daily_clean_count(void);
void test_analyticsOnHeal_increments_daily_heal_count(void);
void test_resetDailyCounters_resets_all_daily_fields(void);
void test_resetDailyCounters_preserves_weekly_play_time(void);
void test_resetWeeklyCounters_resets_weekly_play_time(void);
void test_resetWeeklyCounters_preserves_daily_counts(void);
void test_getAnalyticsTrendsJson_format(void);
void test_getAnalyticsTrendsJson_today_values(void);
void test_getAnalyticsCsv_format(void);
void test_getAnalyticsCsv_contains_all_metrics(void);
void test_analyticsTick_no_reset_before_day_passed(void);
void test_getAnalyticsJson_alias_for_trends(void);

// Forward declarations for Phase 12.4: Accessibility tests (test_accessibility.cpp)
void test_initAccessibility_defaults(void);
void test_getAccessibilityJson_format(void);
void test_getAccessibilityJson_default_values(void);
void test_setAccessibilityFromJson_highContrast(void);
void test_setAccessibilityFromJson_fontSize(void);
void test_setAccessibilityFromJson_reducedMotion(void);
void test_setAccessibilityFromJson_soundVolume(void);
void test_setAccessibilityFromJson_clamps_fontSize(void);
void test_setAccessibilityFromJson_clamps_soundVolume(void);
void test_setAccessibilityFromJson_ignores_invalid_json(void);
void test_setAccessibilityFromJson_full_settings(void);
void test_setAccessibilityFromJson_partial_settings(void);

// Forward declarations for Phase 12.5 / 15.3: Backup & Restore tests (test_backup.cpp)
void test_backup_json_contains_version(void);
void test_backup_json_contains_pet_object(void);
void test_backup_json_contains_checksum(void);
void test_backup_json_contains_timestamp(void);
void test_backup_json_contains_achievements_array(void);
void test_backup_json_contains_stats_object(void);
void test_backup_json_contains_settings_object(void);
void test_backup_json_contains_core_stats(void);
void test_backup_restore_roundtrip_pet_name(void);
void test_backup_restore_roundtrip_accessibility(void);
void test_backup_restore_roundtrip_achievements(void);
void test_backup_restore_roundtrip_core_stats(void);
void test_backup_checksum_changes_with_stats(void);
void test_backup_checksum_same_for_identical_pets(void);
void test_backup_empty_name(void);
void test_backup_no_achievements_unlocked(void);
void test_backup_generation_preserved(void);
void test_backup_verify_valid_backup(void);
void test_backup_verify_invalid_json(void);
void test_backup_verify_missing_pet(void);
void test_backup_verify_checksum_mismatch(void);
void test_backup_restore_roundtrip_full_pet(void);
void test_backup_version_string(void);

void setUp(void) {
  // Called before each test
}

void tearDown(void) {
  // Called after each test
}

// --- Lifecycle Tests ---

void test_initPet_sets_default_values(void) {
  Pet pet;
  initPet(pet);
  TEST_ASSERT_EQUAL(50, pet.hunger);
  TEST_ASSERT_EQUAL(50, pet.happiness);
  TEST_ASSERT_EQUAL(80, pet.health);
  TEST_ASSERT_EQUAL(100, pet.energy);
  TEST_ASSERT_EQUAL(80, pet.cleanliness);
  TEST_ASSERT_EQUAL(0, pet.age);
  TEST_ASSERT_TRUE(pet.isAlive);
  TEST_ASSERT_EQUAL_STRING("normal", pet.state.c_str());
  TEST_ASSERT_EQUAL(BABY, pet.stage);
  TEST_ASSERT_FALSE(pet.isNight);
  TEST_ASSERT_EQUAL(BLOB, pet.type);
}

void test_initPet_resets_all_phase_fields(void) {
  Pet pet;
  // Set some non-default values
  pet.hunger = 0;
  pet.health = 0;
  pet.isAlive = false;
  pet.stage = ELDER;
  pet.isDying = true;
  pet.weather = 3;
  pet.musicEnabled = false;
  pet.difficulty = 2;
  pet.activeGame = 3;
  pet.gameScore = 10;
  pet.timesFed = 100;

  initPet(pet);

  TEST_ASSERT_EQUAL(50, pet.hunger);
  TEST_ASSERT_EQUAL(80, pet.health);
  TEST_ASSERT_TRUE(pet.isAlive);
  TEST_ASSERT_EQUAL(BABY, pet.stage);
  TEST_ASSERT_FALSE(pet.isDying);
  TEST_ASSERT_EQUAL(0, pet.weather);
  TEST_ASSERT_TRUE(pet.musicEnabled);
  TEST_ASSERT_EQUAL(1, pet.difficulty);
  TEST_ASSERT_EQUAL(0, pet.activeGame);
  TEST_ASSERT_EQUAL(0, pet.gameScore);
  TEST_ASSERT_EQUAL(0, pet.timesFed);
}

// --- Evolution Tests ---

void test_updateStage_baby_to_child(void) {
  Pet pet;
  initPet(pet);
  pet.age = BABY_MAX_MINUTES + 1; // 61 minutes
  updateStage(pet);
  TEST_ASSERT_EQUAL(CHILD, pet.stage);
}

void test_updateStage_child_to_adult(void) {
  Pet pet;
  initPet(pet);
  pet.age = CHILD_MAX_MINUTES + 1; // 481 minutes
  updateStage(pet);
  TEST_ASSERT_EQUAL(ADULT, pet.stage);
}

void test_updateStage_adult_to_elder(void) {
  Pet pet;
  initPet(pet);
  pet.age = ADULT_MAX_MINUTES + 1; // 1441 minutes
  updateStage(pet);
  TEST_ASSERT_EQUAL(ELDER, pet.stage);
}

void test_updateStage_stays_baby_at_boundary(void) {
  Pet pet;
  initPet(pet);
  pet.age = BABY_MAX_MINUTES; // exactly 60
  updateStage(pet);
  TEST_ASSERT_EQUAL(BABY, pet.stage);
}

void test_getStageDecayMultiplier_baby(void) {
  Pet pet;
  initPet(pet);
  pet.stage = BABY;
  TEST_ASSERT_EQUAL(BABY_DECAY_MULT, getStageDecayMultiplier(pet));
}

void test_getStageDecayMultiplier_elder(void) {
  Pet pet;
  initPet(pet);
  pet.stage = ELDER;
  TEST_ASSERT_EQUAL(ELDER_DECAY_MULT, getStageDecayMultiplier(pet));
}

// --- Action Tests ---

void test_feedPet_increases_hunger(void) {
  Pet pet;
  initPet(pet);
  pet.hunger = 30;
  feedPet(pet);
  TEST_ASSERT_EQUAL(50, pet.hunger); // 30 + FEED_HUNGER_GAIN(20)
}

void test_feedPet_clamps_to_max(void) {
  Pet pet;
  initPet(pet);
  pet.hunger = 95;
  feedPet(pet);
  TEST_ASSERT_EQUAL(STAT_MAX, pet.hunger); // clamped at 100
}

void test_feedPet_sets_eating_state(void) {
  Pet pet;
  initPet(pet);
  feedPet(pet);
  TEST_ASSERT_EQUAL_STRING("eating", pet.state.c_str());
}

void test_feedPet_increments_feedCount(void) {
  Pet pet;
  initPet(pet);
  TEST_ASSERT_EQUAL(0, pet.feedCount);
  feedPet(pet);
  TEST_ASSERT_EQUAL(1, pet.feedCount);
}

void test_feedPet_health_bonus_when_hungry(void) {
  Pet pet;
  initPet(pet);
  pet.hunger = 10; // Below HEALTH_DECAY_THRESHOLD(20)
  pet.health = 50;
  feedPet(pet);
  // FEED_HEALTH_BONUS only granted when hunger < 20 BEFORE feeding
  // After feed: hunger = 10 + 20 = 30, health = 50 + 5 = 55
  TEST_ASSERT_EQUAL(55, pet.health);
}

void test_playPet_increases_happiness(void) {
  Pet pet;
  initPet(pet);
  pet.happiness = 50;
  playPet(pet);
  TEST_ASSERT_EQUAL(65, pet.happiness); // 50 + PLAY_HAPPINESS_GAIN(15)
}

void test_playPet_reduces_energy(void) {
  Pet pet;
  initPet(pet);
  playPet(pet);
  TEST_ASSERT_EQUAL(85, pet.energy); // 100 - PLAY_ENERGY_COST(15)
}

void test_playPet_reduces_hunger(void) {
  Pet pet;
  initPet(pet);
  playPet(pet);
  TEST_ASSERT_EQUAL(40, pet.hunger); // 50 - PLAY_HUNGER_COST(10)
}

void test_playPet_refuses_when_tired(void) {
  Pet pet;
  initPet(pet);
  pet.energy = 5; // Below PLAY_ENERGY_MIN(10)
  playPet(pet);
  TEST_ASSERT_EQUAL(5, pet.energy); // unchanged
  TEST_ASSERT_EQUAL_STRING("normal", pet.state.c_str()); // state unchanged
}

void test_cleanPet_maximizes_cleanliness(void) {
  Pet pet;
  initPet(pet);
  pet.cleanliness = 30;
  cleanPet(pet);
  TEST_ASSERT_EQUAL(STAT_MAX, pet.cleanliness);
}

void test_cleanPet_boosts_health(void) {
  Pet pet;
  initPet(pet);
  pet.health = 70;
  cleanPet(pet);
  TEST_ASSERT_EQUAL(75, pet.health); // 70 + CLEAN_HEALTH_BONUS(5)
}

void test_healPet_restores_health(void) {
  Pet pet;
  initPet(pet);
  pet.health = 30;
  healPet(pet);
  TEST_ASSERT_EQUAL(STAT_MAX, pet.health);
  TEST_ASSERT_EQUAL_STRING("normal", pet.state.c_str());
}

void test_healPet_saves_from_dying(void) {
  Pet pet;
  initPet(pet);
  pet.isDying = true;
  pet.health = 5;
  healPet(pet);
  TEST_ASSERT_FALSE(pet.isDying);
  TEST_ASSERT_EQUAL(50, pet.health);
  TEST_ASSERT_EQUAL_STRING("normal", pet.state.c_str());
}

void test_sleepPet_toggles_sleeping(void) {
  Pet pet;
  initPet(pet);
  TEST_ASSERT_EQUAL_STRING("normal", pet.state.c_str());
  sleepPet(pet);
  TEST_ASSERT_EQUAL_STRING("sleeping", pet.state.c_str());
  sleepPet(pet);
  TEST_ASSERT_EQUAL_STRING("normal", pet.state.c_str());
}

// --- Death & Revive Tests ---

void test_triggerDying_sets_flag(void) {
  Pet pet;
  initPet(pet);
  triggerDying(pet);
  TEST_ASSERT_TRUE(pet.isDying);
  TEST_ASSERT_EQUAL_STRING("dying", pet.state.c_str());
}

void test_canRevive_returns_false_when_alive(void) {
  Pet pet;
  initPet(pet);
  TEST_ASSERT_FALSE(canRevive(pet));
}

void test_canRevive_returns_false_when_dying(void) {
  Pet pet;
  initPet(pet);
  pet.isDying = true;
  TEST_ASSERT_FALSE(canRevive(pet));
}

void test_canRevive_returns_true_when_dead_and_cooldown_expired(void) {
  Pet pet;
  initPet(pet);
  pet.isAlive = false;
  pet.state = "dead";
  pet.lastReviveTime = 0; // No previous revive
  TEST_ASSERT_TRUE(canRevive(pet));
}

void test_canRevive_returns_false_during_cooldown(void) {
  Pet pet;
  initPet(pet);
  pet.isAlive = false;
  pet.state = "dead";
  pet.lastReviveTime = millis(); // Just revived
  TEST_ASSERT_FALSE(canRevive(pet));
}

void test_revivePet_restores_pet(void) {
  Pet pet;
  initPet(pet);
  pet.isAlive = false;
  pet.state = "dead";
  pet.lastReviveTime = 0;
  revivePet(pet);
  TEST_ASSERT_TRUE(pet.isAlive);
  TEST_ASSERT_FALSE(pet.isDying);
  TEST_ASSERT_EQUAL(30, pet.health);
  TEST_ASSERT_EQUAL(30, pet.hunger);
  TEST_ASSERT_EQUAL(20, pet.happiness);
  TEST_ASSERT_EQUAL(50, pet.energy);
  TEST_ASSERT_EQUAL(50, pet.cleanliness);
  TEST_ASSERT_EQUAL_STRING("normal", pet.state.c_str());
}

// --- Weather Tests ---

void test_getWeatherName_sunny(void) {
  TEST_ASSERT_EQUAL_STRING("sunny", getWeatherName(0).c_str());
}

void test_getWeatherName_stormy(void) {
  TEST_ASSERT_EQUAL_STRING("stormy", getWeatherName(3).c_str());
}

void test_getWeatherName_default(void) {
  TEST_ASSERT_EQUAL_STRING("sunny", getWeatherName(99).c_str());
}

void test_getWeatherHappinessMod_sunny(void) {
  TEST_ASSERT_EQUAL(2, getWeatherHappinessMod(0));
}

void test_getWeatherHappinessMod_rainy(void) {
  TEST_ASSERT_EQUAL(-3, getWeatherHappinessMod(2));
}

// --- Difficulty Tests ---

void test_getDifficultyMultiplier_easy(void) {
  TEST_ASSERT_EQUAL(80, getDifficultyMultiplier(0));
}

void test_getDifficultyMultiplier_normal(void) {
  TEST_ASSERT_EQUAL(100, getDifficultyMultiplier(1));
}

void test_getDifficultyMultiplier_hard(void) {
  TEST_ASSERT_EQUAL(120, getDifficultyMultiplier(2));
}

void test_getDifficultyName_easy(void) {
  TEST_ASSERT_EQUAL_STRING("easy", getDifficultyName(0).c_str());
}

void test_getDifficultyName_hard(void) {
  TEST_ASSERT_EQUAL_STRING("hard", getDifficultyName(2).c_str());
}

// --- Game Tests ---

void test_startGame_sets_active_game(void) {
  Pet pet;
  initPet(pet);
  startGame(pet, 1);
  TEST_ASSERT_EQUAL(1, pet.activeGame);
  TEST_ASSERT_EQUAL(0, pet.gameScore);
}

void test_startGame_refuses_during_cooldown(void) {
  Pet pet;
  initPet(pet);
  pet.gameCooldown = 30;
  startGame(pet, 1);
  TEST_ASSERT_EQUAL(0, pet.activeGame);
}

void test_startGame_refuses_when_dead(void) {
  Pet pet;
  initPet(pet);
  pet.isAlive = false;
  startGame(pet, 1);
  TEST_ASSERT_EQUAL(0, pet.activeGame);
}

void test_endGame_won_boosts_happiness(void) {
  Pet pet;
  initPet(pet);
  pet.activeGame = 1;
  pet.gameScore = 5;
  pet.happiness = 50;
  endGame(pet, true);
  // bonus = 5 * 2 = 10, happiness = 50 + 10 = 60
  TEST_ASSERT_EQUAL(60, pet.happiness);
  TEST_ASSERT_EQUAL(0, pet.activeGame);
  TEST_ASSERT_EQUAL(60, pet.gameCooldown);
}

void test_endGame_won_high_score_boosts_health(void) {
  Pet pet;
  initPet(pet);
  pet.activeGame = 1;
  pet.gameScore = 5;
  pet.health = 70;
  endGame(pet, true);
  // health = 70 + 5 = 75
  TEST_ASSERT_EQUAL(75, pet.health);
}

// --- Memory Game Tests ---

void test_memory_game_correct_input(void) {
  Pet pet;
  initPet(pet);
  startGame(pet, 1);
  int correct = pet.memorySequence[0];
  bool result = checkMemoryInput(pet, correct);
  TEST_ASSERT_TRUE(result);
  TEST_ASSERT_EQUAL(1, pet.gameScore);
}

void test_memory_game_wrong_input(void) {
  Pet pet;
  initPet(pet);
  startGame(pet, 1);
  int wrong = (pet.memorySequence[0] + 1) % 4;
  bool result = checkMemoryInput(pet, wrong);
  TEST_ASSERT_FALSE(result);
  TEST_ASSERT_EQUAL(0, pet.activeGame); // Game ended
}

// --- Stat Clamping Tests ---

void test_clampStat_below_min(void) {
  Pet pet;
  initPet(pet);
  pet.hunger = -10;
  feedPet(pet); // This will clamp
  TEST_ASSERT_GREATER_OR_EQUAL(STAT_MIN, pet.hunger);
}

void test_clampStat_above_max(void) {
  Pet pet;
  initPet(pet);
  pet.hunger = 95;
  feedPet(pet); // 95 + 20 = 115, should clamp to 100
  TEST_ASSERT_EQUAL(STAT_MAX, pet.hunger);
}

// --- Day/Night Tests ---

void test_isNightTime_at_midnight(void) {
  // 0 minutes = 00:00 = night
  TEST_ASSERT_TRUE(isNightTime(0));
}

void test_isNightTime_at_noon(void) {
  // 720 minutes = 12:00 = day
  TEST_ASSERT_FALSE(isNightTime(720));
}

void test_isNightTime_at_22(void) {
  // 22*60 = 1320 minutes = 22:00 = night
  TEST_ASSERT_TRUE(isNightTime(1320));
}

void test_isNightTime_at_06(void) {
  // 6*60 = 360 minutes = 06:00 = day start
  TEST_ASSERT_FALSE(isNightTime(360));
}

// --- Run all tests ---

int run_pet_statemachine_tests() {
  UNITY_BEGIN();

  // Lifecycle
  RUN_TEST(test_initPet_sets_default_values);
  RUN_TEST(test_initPet_resets_all_phase_fields);

  // Evolution
  RUN_TEST(test_updateStage_baby_to_child);
  RUN_TEST(test_updateStage_child_to_adult);
  RUN_TEST(test_updateStage_adult_to_elder);
  RUN_TEST(test_updateStage_stays_baby_at_boundary);
  RUN_TEST(test_getStageDecayMultiplier_baby);
  RUN_TEST(test_getStageDecayMultiplier_elder);

  // Actions
  RUN_TEST(test_feedPet_increases_hunger);
  RUN_TEST(test_feedPet_clamps_to_max);
  RUN_TEST(test_feedPet_sets_eating_state);
  RUN_TEST(test_feedPet_increments_feedCount);
  RUN_TEST(test_feedPet_health_bonus_when_hungry);
  RUN_TEST(test_playPet_increases_happiness);
  RUN_TEST(test_playPet_reduces_energy);
  RUN_TEST(test_playPet_reduces_hunger);
  RUN_TEST(test_playPet_refuses_when_tired);
  RUN_TEST(test_cleanPet_maximizes_cleanliness);
  RUN_TEST(test_cleanPet_boosts_health);
  RUN_TEST(test_healPet_restores_health);
  RUN_TEST(test_healPet_saves_from_dying);
  RUN_TEST(test_sleepPet_toggles_sleeping);

  // Death & Revive
  RUN_TEST(test_triggerDying_sets_flag);
  RUN_TEST(test_canRevive_returns_false_when_alive);
  RUN_TEST(test_canRevive_returns_false_when_dying);
  RUN_TEST(test_canRevive_returns_true_when_dead_and_cooldown_expired);
  RUN_TEST(test_canRevive_returns_false_during_cooldown);
  RUN_TEST(test_revivePet_restores_pet);

  // Weather
  RUN_TEST(test_getWeatherName_sunny);
  RUN_TEST(test_getWeatherName_stormy);
  RUN_TEST(test_getWeatherName_default);
  RUN_TEST(test_getWeatherHappinessMod_sunny);
  RUN_TEST(test_getWeatherHappinessMod_rainy);

  // Difficulty
  RUN_TEST(test_getDifficultyMultiplier_easy);
  RUN_TEST(test_getDifficultyMultiplier_normal);
  RUN_TEST(test_getDifficultyMultiplier_hard);
  RUN_TEST(test_getDifficultyName_easy);
  RUN_TEST(test_getDifficultyName_hard);

  // Games
  RUN_TEST(test_startGame_sets_active_game);
  RUN_TEST(test_startGame_refuses_during_cooldown);
  RUN_TEST(test_startGame_refuses_when_dead);
  RUN_TEST(test_endGame_won_boosts_happiness);
  RUN_TEST(test_endGame_won_high_score_boosts_health);
  RUN_TEST(test_memory_game_correct_input);
  RUN_TEST(test_memory_game_wrong_input);

  // Stat clamping
  RUN_TEST(test_clampStat_below_min);
  RUN_TEST(test_clampStat_above_max);

  // Day/Night
  RUN_TEST(test_isNightTime_at_midnight);
  RUN_TEST(test_isNightTime_at_noon);
  RUN_TEST(test_isNightTime_at_22);
  RUN_TEST(test_isNightTime_at_06);

  // SPIFFS Migration tests (defined in test_spiffs_migration.cpp)
  RUN_TEST(test_v1_format_minimal_fields);
  RUN_TEST(test_v2_format_phase2_fields);
  RUN_TEST(test_v3_format_phase3_fields);
  RUN_TEST(test_v4_format_phase4_fields);
  RUN_TEST(test_v5_format_phase5_fields);
  RUN_TEST(test_full_modern_format);
  RUN_TEST(test_corrupted_values_get_clamped);
  RUN_TEST(test_invalid_enums_get_defaulted);
  RUN_TEST(test_empty_name_gets_default);
  RUN_TEST(test_migration_preserves_alive_state);

  // Phase 12.1: Achievements 2.0 tests (defined in test_achievements.cpp)
  RUN_TEST(test_tier_none_when_zero_progress);
  RUN_TEST(test_tier_bronze_at_1_percent);
  RUN_TEST(test_tier_bronze_at_24_percent);
  RUN_TEST(test_tier_silver_at_25_percent);
  RUN_TEST(test_tier_silver_at_49_percent);
  RUN_TEST(test_tier_gold_at_50_percent);
  RUN_TEST(test_tier_gold_at_74_percent);
  RUN_TEST(test_tier_platinum_at_75_percent);
  RUN_TEST(test_tier_platinum_at_100_percent);
  RUN_TEST(test_tier_platinum_capped_at_target);
  RUN_TEST(test_tier_none_for_zero_target);
  RUN_TEST(test_tier_to_string_none);
  RUN_TEST(test_tier_to_string_bronze);
  RUN_TEST(test_tier_to_string_silver);
  RUN_TEST(test_tier_to_string_gold);
  RUN_TEST(test_tier_to_string_platinum);
  RUN_TEST(test_category_to_string_care);
  RUN_TEST(test_category_to_string_evolution);
  RUN_TEST(test_category_to_string_social);
  RUN_TEST(test_category_to_string_exploration);
  RUN_TEST(test_record_progress_increments_progress);
  RUN_TEST(test_record_progress_caps_at_target);
  RUN_TEST(test_record_progress_updates_tier);
  RUN_TEST(test_record_progress_ignores_invalid_id);
  RUN_TEST(test_record_progress_ignores_already_unlocked);
  RUN_TEST(test_record_progress_sets_notified_false_on_new_tier);
  RUN_TEST(test_achievement_count_is_16);
  RUN_TEST(test_achievement_have_valid_targets);
  RUN_TEST(test_achievement_categories_valid);
  RUN_TEST(test_achievements_progress_json_format);
  RUN_TEST(test_achievements_progress_json_contains_all_achievements);
  RUN_TEST(test_newly_unlocked_json_empty_when_none);
  RUN_TEST(test_newly_unlocked_json_returns_unnotified);
  RUN_TEST(test_unlocked_rewards_empty_when_none_unlocked);
  RUN_TEST(test_unlocked_rewards_includes_unlocked_rewards);
  RUN_TEST(test_legacy_achievements_json_format);

  // Phase 12.2: Pet Lineage tests (defined in test_lineage.cpp)
  RUN_TEST(test_initLineage_sets_default_values);
  RUN_TEST(test_initLineage_birthDeviceId_not_empty);
  RUN_TEST(test_inheritTraits_averages_personality);
  RUN_TEST(test_inheritTraits_sets_parent_ids);
  RUN_TEST(test_inheritTraits_increments_generation);
  RUN_TEST(test_inheritTraits_zero_generation);
  RUN_TEST(test_inheritTraits_personality_within_bounds);
  RUN_TEST(test_getLineageJson_format);
  RUN_TEST(test_getLineageJson_with_parents);
  RUN_TEST(test_getLineageJson_generation_zero);
  RUN_TEST(test_recordTradeLineage_outgoing);
  RUN_TEST(test_recordTradeLineage_incoming);
  RUN_TEST(test_recordTradeLineage_updates_birth_timestamp);

  // Phase 12.3: Analytics tests (defined in test_stats.cpp)
  RUN_TEST(test_initAnalytics_resets_all_counters);
  RUN_TEST(test_initAnalytics_sets_reset_timestamps);
  RUN_TEST(test_analyticsOnFeed_increments_daily_feed_count);
  RUN_TEST(test_analyticsOnPlay_increments_daily_play_count);
  RUN_TEST(test_analyticsOnSleep_increments_daily_sleep_count);
  RUN_TEST(test_analyticsOnClean_increments_daily_clean_count);
  RUN_TEST(test_analyticsOnHeal_increments_daily_heal_count);
  RUN_TEST(test_resetDailyCounters_resets_all_daily_fields);
  RUN_TEST(test_resetDailyCounters_preserves_weekly_play_time);
  RUN_TEST(test_resetWeeklyCounters_resets_weekly_play_time);
  RUN_TEST(test_resetWeeklyCounters_preserves_daily_counts);
  RUN_TEST(test_getAnalyticsTrendsJson_format);
  RUN_TEST(test_getAnalyticsTrendsJson_today_values);
  RUN_TEST(test_getAnalyticsCsv_format);
  RUN_TEST(test_getAnalyticsCsv_contains_all_metrics);
  RUN_TEST(test_analyticsTick_no_reset_before_day_passed);
  RUN_TEST(test_getAnalyticsJson_alias_for_trends);

  // Phase 12.4: Accessibility tests (defined in test_accessibility.cpp)
  RUN_TEST(test_initAccessibility_defaults);
  RUN_TEST(test_getAccessibilityJson_format);
  RUN_TEST(test_getAccessibilityJson_default_values);
  RUN_TEST(test_setAccessibilityFromJson_highContrast);
  RUN_TEST(test_setAccessibilityFromJson_fontSize);
  RUN_TEST(test_setAccessibilityFromJson_reducedMotion);
  RUN_TEST(test_setAccessibilityFromJson_soundVolume);
  RUN_TEST(test_setAccessibilityFromJson_clamps_fontSize);
  RUN_TEST(test_setAccessibilityFromJson_clamps_soundVolume);
  RUN_TEST(test_setAccessibilityFromJson_ignores_invalid_json);
  RUN_TEST(test_setAccessibilityFromJson_full_settings);
  RUN_TEST(test_setAccessibilityFromJson_partial_settings);

  // Phase 12.5 / 15.3: Backup & Restore tests (defined in test_backup.cpp)
  RUN_TEST(test_backup_json_contains_version);
  RUN_TEST(test_backup_json_contains_pet_object);
  RUN_TEST(test_backup_json_contains_checksum);
  RUN_TEST(test_backup_json_contains_timestamp);
  RUN_TEST(test_backup_json_contains_achievements_array);
  RUN_TEST(test_backup_json_contains_stats_object);
  RUN_TEST(test_backup_json_contains_settings_object);
  RUN_TEST(test_backup_json_contains_core_stats);
  RUN_TEST(test_backup_restore_roundtrip_pet_name);
  RUN_TEST(test_backup_restore_roundtrip_accessibility);
  RUN_TEST(test_backup_restore_roundtrip_achievements);
  RUN_TEST(test_backup_restore_roundtrip_core_stats);
  RUN_TEST(test_backup_checksum_changes_with_stats);
  RUN_TEST(test_backup_checksum_same_for_identical_pets);
  RUN_TEST(test_backup_empty_name);
  RUN_TEST(test_backup_no_achievements_unlocked);
  RUN_TEST(test_backup_generation_preserved);
  RUN_TEST(test_backup_verify_valid_backup);
  RUN_TEST(test_backup_verify_invalid_json);
  RUN_TEST(test_backup_verify_missing_pet);
  RUN_TEST(test_backup_verify_checksum_mismatch);
  RUN_TEST(test_backup_restore_roundtrip_full_pet);
  RUN_TEST(test_backup_version_string);

  UNITY_END();
  return 0;
}
