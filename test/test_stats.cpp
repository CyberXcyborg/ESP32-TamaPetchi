#include <Arduino.h>
#include <unity.h>
#include "Pet.h"
#include "config.h"

// ============================================================
// Phase 12.3: Data Dashboard & Analytics Unit Tests
// Note: setUp/tearDown/main are in test_pet_statemachine.cpp
// ============================================================

// --- Analytics Init Tests ---

void test_initAnalytics_resets_all_counters(void) {
  Pet pet;
  initPet(pet);
  pet.dailyFeedCount = 10;
  pet.dailyPlayCount = 20;
  pet.dailySleepCount = 5;
  pet.dailyPlayTimeSec = 3600;
  pet.weeklyPlayTimeSec = 7200;
  pet.dailyCleanCount = 3;
  pet.dailyHealCount = 2;

  initAnalytics(pet);

  TEST_ASSERT_EQUAL(0, pet.dailyFeedCount);
  TEST_ASSERT_EQUAL(0, pet.dailyPlayCount);
  TEST_ASSERT_EQUAL(0, pet.dailySleepCount);
  TEST_ASSERT_EQUAL(0UL, pet.dailyPlayTimeSec);
  TEST_ASSERT_EQUAL(0UL, pet.weeklyPlayTimeSec);
  TEST_ASSERT_EQUAL(0, pet.dailyCleanCount);
  TEST_ASSERT_EQUAL(0, pet.dailyHealCount);
}

void test_initAnalytics_sets_reset_timestamps(void) {
  Pet pet;
  initPet(pet);
  initAnalytics(pet);
  TEST_ASSERT_GREATER_THAN(0UL, pet.lastDayReset);
  TEST_ASSERT_GREATER_THAN(0UL, pet.lastWeekReset);
}

// --- Counter Increment Tests ---

void test_analyticsOnFeed_increments_daily_feed_count(void) {
  Pet pet;
  initPet(pet);
  initAnalytics(pet);
  analyticsOnFeed(pet);
  analyticsOnFeed(pet);
  analyticsOnFeed(pet);
  TEST_ASSERT_EQUAL(3, pet.dailyFeedCount);
}

void test_analyticsOnPlay_increments_daily_play_count(void) {
  Pet pet;
  initPet(pet);
  initAnalytics(pet);
  analyticsOnPlay(pet);
  analyticsOnPlay(pet);
  TEST_ASSERT_EQUAL(2, pet.dailyPlayCount);
}

void test_analyticsOnSleep_increments_daily_sleep_count(void) {
  Pet pet;
  initPet(pet);
  initAnalytics(pet);
  analyticsOnSleep(pet);
  TEST_ASSERT_EQUAL(1, pet.dailySleepCount);
}

void test_analyticsOnClean_increments_daily_clean_count(void) {
  Pet pet;
  initPet(pet);
  initAnalytics(pet);
  analyticsOnClean(pet);
  TEST_ASSERT_EQUAL(1, pet.dailyCleanCount);
}

void test_analyticsOnHeal_increments_daily_heal_count(void) {
  Pet pet;
  initPet(pet);
  initAnalytics(pet);
  analyticsOnHeal(pet);
  TEST_ASSERT_EQUAL(1, pet.dailyHealCount);
}

// --- Daily Reset Tests ---

void test_resetDailyCounters_resets_all_daily_fields(void) {
  Pet pet;
  initPet(pet);
  initAnalytics(pet);
  pet.dailyFeedCount = 5;
  pet.dailyPlayCount = 3;
  pet.dailySleepCount = 2;
  pet.dailyPlayTimeSec = 1800;
  pet.dailyCleanCount = 1;
  pet.dailyHealCount = 1;

  resetDailyCounters(pet);

  TEST_ASSERT_EQUAL(0, pet.dailyFeedCount);
  TEST_ASSERT_EQUAL(0, pet.dailyPlayCount);
  TEST_ASSERT_EQUAL(0, pet.dailySleepCount);
  TEST_ASSERT_EQUAL(0UL, pet.dailyPlayTimeSec);
  TEST_ASSERT_EQUAL(0, pet.dailyCleanCount);
  TEST_ASSERT_EQUAL(0, pet.dailyHealCount);
}

void test_resetDailyCounters_preserves_weekly_play_time(void) {
  Pet pet;
  initPet(pet);
  initAnalytics(pet);
  pet.weeklyPlayTimeSec = 5000;
  resetDailyCounters(pet);
  TEST_ASSERT_EQUAL(5000UL, pet.weeklyPlayTimeSec);
}

// --- Weekly Reset Tests ---

void test_resetWeeklyCounters_resets_weekly_play_time(void) {
  Pet pet;
  initPet(pet);
  initAnalytics(pet);
  pet.weeklyPlayTimeSec = 10000;
  resetWeeklyCounters(pet);
  TEST_ASSERT_EQUAL(0UL, pet.weeklyPlayTimeSec);
}

void test_resetWeeklyCounters_preserves_daily_counts(void) {
  Pet pet;
  initPet(pet);
  initAnalytics(pet);
  pet.dailyFeedCount = 5;
  resetWeeklyCounters(pet);
  TEST_ASSERT_EQUAL(5, pet.dailyFeedCount);
}

// --- JSON Export Tests ---

void test_getAnalyticsTrendsJson_format(void) {
  Pet pet;
  initPet(pet);
  initAnalytics(pet);
  pet.dailyFeedCount = 3;
  pet.dailyPlayCount = 2;

  String json = getAnalyticsTrendsJson(pet, "7d");
  TEST_ASSERT_TRUE(json.length() > 0);
  TEST_ASSERT_TRUE(json.indexOf("\"range\":\"7d\"") >= 0);
  TEST_ASSERT_TRUE(json.indexOf("\"today\"") >= 0);
  TEST_ASSERT_TRUE(json.indexOf("\"week\"") >= 0);
  TEST_ASSERT_TRUE(json.indexOf("\"cumulative\"") >= 0);
}

void test_getAnalyticsTrendsJson_today_values(void) {
  Pet pet;
  initPet(pet);
  initAnalytics(pet);
  pet.dailyFeedCount = 5;
  pet.dailyPlayCount = 3;

  String json = getAnalyticsTrendsJson(pet, "7d");
  TEST_ASSERT_TRUE(json.indexOf("\"feeds\":5") >= 0);
  TEST_ASSERT_TRUE(json.indexOf("\"plays\":3") >= 0);
}

// --- CSV Export Tests ---

void test_getAnalyticsCsv_format(void) {
  Pet* pet = new Pet();
  initPet(*pet);
  initAnalytics(*pet);
  pet->dailyFeedCount = 2;

  String csv = getAnalyticsCsv(*pet, "7d");
  TEST_ASSERT_TRUE(csv.indexOf("metric,value") == 0);
  TEST_ASSERT_TRUE(csv.indexOf("dailyFeeds,2") >= 0);
  delete pet;
}

void test_getAnalyticsCsv_contains_all_metrics(void) {
  Pet* pet = new Pet();
  initPet(*pet);
  initAnalytics(*pet);

  String csv = getAnalyticsCsv(*pet, "7d");
  TEST_ASSERT_TRUE(csv.indexOf("dailyFeeds") >= 0);
  TEST_ASSERT_TRUE(csv.indexOf("dailyPlays") >= 0);
  TEST_ASSERT_TRUE(csv.indexOf("totalFeeds") >= 0);
  TEST_ASSERT_TRUE(csv.indexOf("totalPlays") >= 0);
  TEST_ASSERT_TRUE(csv.indexOf("totalPlayTimeSec") >= 0);
  delete pet;
}

// --- Analytics Tick Tests ---

void test_analyticsTick_no_reset_before_day_passed(void) {
  Pet pet;
  initPet(pet);
  initAnalytics(pet);
  pet.dailyFeedCount = 3;
  pet.lastDayReset = millis(); // Just reset

  analyticsTick(pet, 1000);
  TEST_ASSERT_EQUAL(3, pet.dailyFeedCount);
}

// --- JSON Export Alias Test ---

void test_getAnalyticsJson_alias_for_trends(void) {
  Pet pet;
  initPet(pet);
  initAnalytics(pet);
  pet.dailyFeedCount = 7;

  String json1 = getAnalyticsJson(pet, "30d");
  String json2 = getAnalyticsTrendsJson(pet, "30d");
  TEST_ASSERT_EQUAL_STRING(json2.c_str(), json1.c_str());
}
