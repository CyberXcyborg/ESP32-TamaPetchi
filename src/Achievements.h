#ifndef ACHIEVEMENTS_H
#define ACHIEVEMENTS_H

#include <Arduino.h>
#include "Pet.h"

// ============================================================
// Achievements System — Phase 12.1: Achievements 2.0
// Tracks milestones with tiers, categories, progress, and rewards
// Persists to SPIFFS /achievements.json
// ============================================================

// --- Achievement Tiers ---
enum AchievementTier {
  TIER_NONE = 0,
  TIER_BRONZE = 1,
  TIER_SILVER = 2,
  TIER_GOLD = 3,
  TIER_PLATINUM = 4
};

// --- Achievement Categories ---
enum AchievementCategory {
  CAT_CARE = 0,
  CAT_EVOLUTION = 1,
  CAT_SOCIAL = 2,
  CAT_EXPLORATION = 3
};

// --- Achievement IDs (Care) ---
#define ACH_FED_10X         "fed_10x"
#define ACH_FED_100X        "fed_100x"
#define ACH_CLEANED_10X     "cleaned_10x"
#define ACH_HEALED_5X       "healed_5x"
#define ACH_PLAYED_10X      "played_10x"
#define ACH_PLAYED_100X     "played_100x"
#define ACH_NAMED_PET       "named_pet"
#define ACH_SCHEDULED_FEED  "scheduled_feed"

// --- Achievement IDs (Evolution) ---
#define ACH_REACHED_CHILD   "reached_child"
#define ACH_REACHED_ADULT   "reached_adult"
#define ACH_REACHED_ELDER   "reached_elder"
#define ACH_FULLY_EVOLVED   "fully_evolved"

// --- Achievement IDs (Social) ---
#define ACH_TRADE_COMPLETED "trade_completed"
#define ACH_TRADE_RECEIVED  "trade_received"

// --- Achievement IDs (Exploration) ---
#define ACH_GAME_WON_1X     "game_won_1x"
#define ACH_GAME_WON_10X    "game_won_10x"
#define ACH_ALL_WEATHERS    "all_weathers"
#define ACH_SURVIVED_24H    "survived_24h"
#define ACH_SURVIVED_7D     "survived_7d"

#define ACHIEVEMENTS_FILE   "/achievements.json"

// --- Achievement Definition ---
struct AchievementDef {
  const char* id;
  const char* name;
  AchievementCategory category;
  int target;            // target value for completion
  const char* rewardId;  // reward unlocked (pet skin/accessory ID)
};

// --- Achievement State (runtime + persisted) ---
struct AchievementState {
  int progress;          // current progress value
  AchievementTier tier;  // current tier
  bool unlocked;         // fully unlocked (platinum)
  bool notified;         // WebSocket notification sent
};

// --- Achievement Definitions Table ---
// 16 achievements across 4 categories
#define ACHIEVEMENT_COUNT 16
extern const AchievementDef achievementDefs[ACHIEVEMENT_COUNT];

// --- Runtime State ---
extern AchievementState achievementStates[ACHIEVEMENT_COUNT];

// --- Tier Thresholds ---
// Bronze: 0-25%, Silver: 25-50%, Gold: 50-75%, Platinum: 75-100%
inline AchievementTier calculateTier(int progress, int target) {
  if (target <= 0) return TIER_NONE;
  int pct = (progress * 100) / target;
  if (pct >= 75) return TIER_PLATINUM;
  if (pct >= 50) return TIER_GOLD;
  if (pct >= 25) return TIER_SILVER;
  if (pct > 0)   return TIER_BRONZE;
  return TIER_NONE;
}

inline const char* tierToString(AchievementTier tier) {
  switch (tier) {
    case TIER_BRONZE:   return "bronze";
    case TIER_SILVER:   return "silver";
    case TIER_GOLD:     return "gold";
    case TIER_PLATINUM: return "platinum";
    default:            return "none";
  }
}

inline const char* categoryToString(AchievementCategory cat) {
  switch (cat) {
    case CAT_CARE:       return "care";
    case CAT_EVOLUTION:  return "evolution";
    case CAT_SOCIAL:     return "social";
    case CAT_EXPLORATION:return "exploration";
    default:             return "unknown";
  }
}

// --- Load/Save ---
void loadAchievements(Pet &pet);
void saveAchievements(const Pet &pet);

// --- Check & Unlock ---
void checkAchievements(Pet &pet);

// --- Get achievements as JSON ---
// Phase 12.1: Returns full progress data
// { achievements: [{id, name, tier, category, progress, target, reward, unlocked}] }
String getAchievementsJson(const Pet &pet);

// --- Phase 12.1: Get progress JSON for /api/achievements/progress ---
String getAchievementsProgressJson(const Pet &pet);

// --- Phase 12.1: Get newly unlocked achievements (clears notified flags) ---
String getNewlyUnlockedJson();

// --- Phase 12.1: Record specific achievement progress ---
void recordAchievementProgress(const char* achId, int amount);

// --- Phase 12.1: Get unlocked rewards ---
String getUnlockedRewardsJson();
// --- Helper (needed by WebHandlers for restore) ---
int findAchievementIndex(const char* id);

// --- Legacy compatibility ---
// These maintain backward compatibility with existing code
#define ACH_SURVIVED_1H     "survived_1h"  // legacy alias

#endif // ACHIEVEMENTS_H
