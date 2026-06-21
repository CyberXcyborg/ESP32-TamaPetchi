#ifndef PET_H
#define PET_H

#include <Arduino.h>
#include "PetAI_Types.h"

// ============================================================
// Pet Evolution Stages
// ============================================================
enum PetStage { BABY, CHILD, ADULT, ELDER };

// ============================================================
// Pet Types (different sprite sets)
// ============================================================
enum PetType { BLOB, CAT, DOG };

// ============================================================
// Pet State
// ============================================================

struct Pet {
  int  hunger;
  int  happiness;
  int  health;
  int  energy;
  int  cleanliness;
  int  age;              // age in minutes
  bool isAlive;
  String state;          // normal, eating, playing, sleeping, sick, hungry, critical, dying, dead
  unsigned long stateChangeTime;
  unsigned long lastInteractionTime;

  // --- Phase 2 fields ---
  PetStage stage;        // evolution stage
  bool isNight;          // day/night cycle flag
  unsigned long virtualMinutes;  // virtual time in minutes since start

  // --- Phase 3 fields ---
  String name;           // pet name (max 16 chars)
  bool soundEnabled;     // buzzer on/off
  PetType type;          // pet sprite type (BLOB, CAT, DOG)

  // --- Achievement tracking ---
  int  feedCount;        // total times fed
  int  playCount;        // total times played
  bool hasBeenNamed;     // true if user set a custom name
  bool elderAchieved;    // true once elder stage reached

  // --- Phase 4 fields ---
  bool isDying;              // true when in dying state (30s window before death)
  unsigned long dyingStartTime; // when dying started
  unsigned long lastReviveTime; // last revive timestamp (cooldown)
  bool isEvolving;           // true during evolution animation
  PetStage previousStage;    // stage before evolution (for animation)
  unsigned long evolutionStartTime; // when evolution animation started
  int weather;               // 0=sunny, 1=cloudy, 2=rainy, 3=stormy, 4=snowy
  unsigned long weatherChangeTime; // when weather last changed
  bool musicEnabled;         // background music on/off
  int difficulty;            // 0=easy(80%), 1=normal(100%), 2=hard(120%)
  int gameCooldown;          // seconds until next game available
  int activeGame;            // 0=none, 1=memory, 2=catch, 3=quiz
  int gameScore;             // current game score
  int gameRound;             // current game round
  int memorySequence[10];    // memory game sequence
  int catchTargetX, catchTargetY; // catch game target position

  // --- Phase 5: Statistics ---
  unsigned long totalPlayTime;    // total play time in seconds
  unsigned long totalSleepTime;   // total sleep time in seconds
  int timesFed;
  int timesPlayed;
  int timesSlept;
  int timesCleaned;
  int timesHealed;
  int highScore;
  unsigned long dailyActiveMinutes;
  unsigned long weeklyActiveMinutes;

  // --- Phase 5: Notifications ---
  int notificationCount;

  // --- Phase 5: Power ---
  int batteryLevel;              // 0-100 if ADC available, -1 if not
  bool lowBatteryWarning;
  unsigned long lastBatteryCheck;

  // --- Phase 7.5: Pet Mood System ---
  int mood;                   // 0=ecstatic, 1=happy, 2=content, 3=neutral, 4=sad, 5=upset, 6=miserable
  int personalityCheerful;    // 0-100: how quickly happiness decays/regenerates
  int personalityEnergetic;   // 0-100: how quickly energy decays/regenerates
  int personalityHungry;      // 0-100: how quickly hunger decays
  unsigned long lastMoodUpdate;

  // --- Phase 7.5: Scheduled Feeding --
  bool scheduledFeedEnabled;
  unsigned long scheduledFeedInterval;  // Interval in ms (default 4 hours)
  unsigned long lastScheduledFeed;
  int scheduledFeedAmount;              // How much hunger to restore (default 15)

  // --- Phase 12.2: Pet Lineage & Genealogy ---
  String parentIds[2];         // Parent device IDs (empty if original pet)
  int generation;              // Generation number (0 = original)
  unsigned long birthTimestamp; // When this pet was created
  String birthDeviceId;        // Device ID of birth device

  // --- Phase 12.3: Daily/Weekly Analytics ---
  int dailyFeedCount;          // Feeds today (reset at midnight)
  int dailyPlayCount;          // Plays today
  int dailySleepCount;         // Sleeps today
  unsigned long dailyPlayTimeSec;  // Play time today in seconds
  unsigned long weeklyPlayTimeSec; // Play time this week in seconds
  int dailyCleanCount;
  int dailyHealCount;
  unsigned long lastDayReset;  // When daily counters were last reset
  unsigned long lastWeekReset; // When weekly counters were last reset

  // --- Phase 12.4: Accessibility Settings --
  bool highContrastMode;       // High-contrast UI
  int fontSize;                // 0=small, 1=medium, 2=large
  bool reducedMotion;           // Disable animations
  int soundVolume;             // 0-100% volume for sound effects

  // --- Phase 16.1: Pet AI ---
  PetMemory memory;            // Last 10 actions memory buffer
  AIModifiers aiMods;          // Computed AI behavior modifiers
  unsigned long lastAIUpdate;  // Last time AI was updated
  int aiActivityLevel;         // 0-100: how active the pet has been recently
};

// --- Lifecycle ---
void initPet(Pet &pet);
void updatePet(Pet &pet);

// --- Persistence (Phase 6: force save for critical events) ---
extern void savePetDataForce(const Pet &pet);

// --- Evolution ---
void updateStage(Pet &pet);
int  getStageDecayMultiplier(Pet &pet);

// --- Day/Night ---
void updateDayNightCycle(Pet &pet);
bool isNightTime(unsigned long virtualMin);

// --- Randomness ---
int randomVariation(int base);

// --- Sound Helpers ---
void soundFeed();
void soundPlay();
void soundDeath();
void soundWake();

// --- Evolution & Death ---
void checkEvolution(Pet &pet);
void triggerEvolution(Pet &pet);
void triggerDying(Pet &pet);
bool canRevive(const Pet &pet);
void revivePet(Pet &pet);
String getEvolutionAnimation(const Pet &pet);
String getDeathAnimation(const Pet &pet);

// --- Weather ---
void updateWeather(Pet &pet);
String getWeatherName(int weather);
String getWeatherSVG(int weather);
int getWeatherHappinessMod(int weather);
int getWeatherEnergyMod(int weather);
int getWeatherCleanlinessMod(int weather);

// --- Mini-Games ---
void startGame(Pet &pet, int gameType);
void endGame(Pet &pet, bool won);
void generateMemorySequence(Pet &pet);
bool checkMemoryInput(Pet &pet, int input);
void updateCatchTarget(Pet &pet);
String getGameStateJSON(const Pet &pet);

// --- Music ---
void playMelody(const int *melody, int length, int tempo);
void playStateMelody(const Pet &pet);
void stopMusic();

// --- Buzzer Melody Configuration (Phase 6.3) ---
// User-selectable melody IDs per event
#define MELODY_HAPPY    0
#define MELODY_SLEEP    1
#define MELODY_SICK     2
#define MELODY_DYING    3
#define MELODY_EVOLVE   4
#define MELODY_FEED     5
#define MELODY_PLAY     6
#define MELODY_DEATH    7
#define MELODY_COUNT    8

// Default melody assignments (can be changed at runtime via web UI)
extern int melodyConfig[MELODY_COUNT];  // maps event -> melody index

// Built-in melody library
extern const int *melodyLibrary[];
extern const int melodyLengths[];
extern const char *melodyNames[];
extern const int melodyCount;

void playMelodyById(int melodyId);
void setMelodyConfig(int event, int melodyIndex);
int getMelodyConfig(int event);
String getMelodyConfigJson();
void setMelodyConfigFromJson(const String &json);

// --- Settings ---
int getDifficultyMultiplier(int difficulty);
String getDifficultyName(int difficulty);

// --- Actions ---
void feedPet(Pet &pet);
void playPet(Pet &pet);
void cleanPet(Pet &pet);
void sleepPet(Pet &pet);
void healPet(Pet &pet);
void resetPet(Pet &pet);

// --- Phase 5: Power Management ---
void updateBatteryLevel(Pet &pet);

// --- Phase 7.5: Pet Mood System ---
void initMoodSystem(Pet &pet);
void updateMood(Pet &pet);
String getMoodName(int mood);
String getMoodEmoji(int mood);
int getMoodHappinessMod(int mood);

// --- Phase 7.5: Scheduled Feeding ---
void initScheduledFeed(Pet &pet);
void checkScheduledFeed(Pet &pet);
void setScheduledFeed(bool enabled, unsigned long intervalMs, int amount);
String getScheduledFeedJson(Pet &pet);

// --- Phase 12.2: Pet Lineage & Genealogy ---
void initLineage(Pet &pet);
void inheritTraits(Pet &pet, const Pet &parent1, const Pet &parent2);
String getLineageJson(const Pet &pet);
void recordTradeLineage(Pet &pet, const String &partnerDeviceId, bool isOutgoing);

// --- Phase 12.3: Analytics ---
void initAnalytics(Pet &pet);
void resetDailyCounters(Pet &pet);
void resetWeeklyCounters(Pet &pet);
void analyticsOnFeed(Pet &pet);
void analyticsOnPlay(Pet &pet);
void analyticsOnSleep(Pet &pet);
void analyticsOnClean(Pet &pet);
void analyticsOnHeal(Pet &pet);
void analyticsTick(Pet &pet, unsigned long intervalMs);
String getAnalyticsTrendsJson(const Pet &pet, const String &range);
String getAnalyticsCsv(const Pet &pet, const String &range);
String getAnalyticsJson(const Pet &pet, const String &range);

// --- Phase 12.4: Accessibility --
void initAccessibility(Pet &pet);
String getAccessibilityJson(const Pet &pet);
void setAccessibilityFromJson(Pet &pet, const String &json);

#endif // PET_H
