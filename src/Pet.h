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

#endif // PET_H
