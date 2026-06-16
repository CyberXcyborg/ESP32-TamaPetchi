#ifndef CONFIG_H
#define CONFIG_H

// ============================================================
// ESP32 TamaPetchi — Configuration
// ============================================================

// --- WiFi Credentials ---
#define WIFI_SSID     "TEST"
#define WIFI_PASSWORD "TEST"

// --- Network ---
#define WEB_SERVER_PORT  80

// --- SPIFFS ---
#define PET_DATA_FILE    "/pet_data.json"

// --- Timing (millis) ---
#define PET_UPDATE_INTERVAL   60000   // Stat tick every 60 s
#define PET_STATE_TIMEOUT      5000   // Eating / playing state duration

// --- Stat Bounds ---
#define STAT_MIN    0
#define STAT_MAX  100

// --- Decay Rates (per tick, normal state) ---
#define HUNGER_DECAY_NORMAL       5
#define HAPPINESS_DECAY_NORMAL    3
#define ENERGY_DECAY_NORMAL       2
#define CLEANLINESS_DECAY_NORMAL  4

// --- Decay Rates (per tick, sleeping state) ---
#define HUNGER_DECAY_SLEEP        2
#define HAPPINESS_DECAY_SLEEP     1
#define ENERGY_REGEN_SLEEP       10
#define CLEANLINESS_DECAY_SLEEP   2

// --- Health ---
#define HEALTH_DECAY_THRESHOLD   20   // hunger or cleanliness below this triggers health loss
#define HEALTH_DECAY_AMOUNT       5
#define SICK_THRESHOLD           30   // health below this → sick state

// --- Action: Feed ---
#define FEED_HUNGER_GAIN   20
#define FEED_ENERGY_COST    5
#define FEED_HEALTH_BONUS   5   // granted only when hunger < 20

// --- Action: Play ---
#define PLAY_HAPPINESS_GAIN  15
#define PLAY_ENERGY_COST     15
#define PLAY_HUNGER_COST     10
#define PLAY_ENERGY_MIN      10   // refuse if energy below this

// --- Action: Clean ---
#define CLEAN_HEALTH_BONUS    5

// --- Serial ---
#define SERIAL_BAUD  115200

// --- Buzzer ---
#define BUZZER_PIN    25
#define BUZZER_CHANNEL 0

// --- OLED (SSD1306) ---
// Enable with: -DENABLE_OLED in platformio.ini or build flags
#define OLED_SDA    21
#define OLED_SCL    22
#define OLED_ADDRESS 0x3C

// --- Evolution Stage Thresholds (in minutes) ---
#define BABY_MAX_MINUTES    60     // 0-60 min = baby
#define CHILD_MAX_MINUTES   480    // 61-480 min = child
#define ADULT_MAX_MINUTES   1440   // 481-1440 min = adult
// > 1440 min = elder

// --- Stage Decay Multipliers (percentage of base rate) ---
#define BABY_DECAY_MULT     80     // Baby decays slower
#define CHILD_DECAY_MULT    100    // Normal
#define ADULT_DECAY_MULT    110    // Slightly faster
#define ELDER_DECAY_MULT    130    // Decays faster

// --- Virtual Time ---
#define VIRTUAL_MINUTES_PER_HOUR  60UL
#define VIRTUAL_MINUTES_PER_DAY   (24UL * VIRTUAL_MINUTES_PER_HOUR)

// --- Day/Night Cycle ---
#define NIGHT_START_HOUR    22     // Night starts at 22:00
#define DAY_START_HOUR      6      // Day starts at 06:00

// --- Warning State Thresholds ---
#define DYING_HEALTH_MAX    10     // health 0-10 → dying state
#define DYING_HEALTH_MIN    0
#define CRITICAL_HEALTH_MAX 30     // health 11-30 → critical state
#define CRITICAL_HEALTH_MIN 11

// --- Phase 6: Wear Leveling ---
#define MIN_SAVE_INTERVAL   300000UL  // 5 minutes minimum between SPIFFS saves

// --- Phase 6: Compile-Time Feature Flags ---
// Uncomment to disable features and reduce flash usage
// #define DISABLE_OTA          // Remove OTA update support
// #define DISABLE_WIFI_MANAGER // Remove WiFi Manager captive portal
// #define DISABLE_MULTIPET     // Remove multi-pet support (single pet only)
// #define DISABLE_STATS        // Remove statistics tracking
// #define DISABLE_NOTIFICATIONS // Remove notification system
// #define DISABLE_ACHIEVEMENTS // Remove achievements system
// #define DISABLE_WEATHER      // Remove weather system
// #define DISABLE_GAMES        // Remove mini-games
// #define DISABLE_MUSIC        // Remove buzzer melodies
// #define DISABLE_OLED         // Remove OLED display support
// #define DISABLE_BUTTONS      // Remove physical button support
// #define DISABLE_RGB_LED      // Remove RGB LED indicator

#endif // CONFIG_H
