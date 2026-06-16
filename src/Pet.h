#ifndef PET_H
#define PET_H

#include <Arduino.h>

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
  bool isDying;
  unsigned long dyingStartTime;
  unsigned long lastReviveTime;
  bool isEvolving;
  PetStage previousStage;
  unsigned long evolutionStartTime;
  int weather;
  unsigned long weatherChangeTime;
  bool musicEnabled;
  int difficulty;
  int gameCooldown;
  int activeGame;
  int gameScore;
  int gameRound;
  int memorySequence[10];
  int catchTargetX, catchTargetY;

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

// --- Lifecycle ---
void initPet(Pet &pet);
void updatePet(Pet &pet);

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

// --- Actions ---
void feedPet(Pet &pet);
void playPet(Pet &pet);
void cleanPet(Pet &pet);
void sleepPet(Pet &pet);
void healPet(Pet &pet);
void resetPet(Pet &pet);

// --- Phase 5: Power Management ---
void updateBatteryLevel(Pet &pet);
String getBatteryJson(const Pet &pet);

#endif // PET_H
