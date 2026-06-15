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
// Balanced for ~24h survival with feeding every 4-6 hours
// At 1 tick/min: hunger drops ~120/day, feeding every 4h = 6 feeds = +120
#define HUNGER_DECAY_NORMAL       2
#define HAPPINESS_DECAY_NORMAL    1
#define ENERGY_DECAY_NORMAL       1
#define CLEANLINESS_DECAY_NORMAL  1

// --- Decay Rates (per tick, sleeping state) ---
#define HUNGER_DECAY_SLEEP        1
#define HAPPINESS_DECAY_SLEEP     1
#define ENERGY_REGEN_SLEEP        5
#define CLEANLINESS_DECAY_SLEEP   1

// --- Stage Decay Multipliers (percentage of base rate) ---
#define BABY_DECAY_MULT     80   // 80% of normal (slower decay for babies)
#define CHILD_DECAY_MULT   100   // 100% of normal
#define ADULT_DECAY_MULT   100   // 100% of normal
#define ELDER_DECAY_MULT   130   // 130% of normal (faster decay for elders)

// --- Health ---
#define HEALTH_DECAY_THRESHOLD   20   // hunger or cleanliness below this triggers health loss
#define HEALTH_DECAY_AMOUNT       2
#define SICK_THRESHOLD           30   // health below this → sick state

// --- Warning States ---
#define CRITICAL_HEALTH_MIN       1   // critical state: health 1-15
#define CRITICAL_HEALTH_MAX      15
#define DYING_HEALTH_MIN          0   // dying state: health 0-5
#define DYING_HEALTH_MAX          5

// --- Action: Feed ---
#define FEED_HUNGER_GAIN   20
#define FEED_ENERGY_COST    5
#define FEED_HEALTH_BONUS   5   // granted only when hunger < 20

// --- Action: Play ---
#define PLAY_HAPPINESS_GAIN  15
#define PLAY_ENERGY_COST     10
#define PLAY_HUNGER_COST      5
#define PLAY_ENERGY_MIN      10   // refuse if energy below this

// --- Action: Clean ---
#define CLEAN_HEALTH_BONUS    5

// --- Evolution Stage Thresholds (in minutes) ---
#define BABY_MAX_MINUTES      1440   // 0-1440 minutes (24h) → BABY
#define CHILD_MAX_MINUTES    10080   // 1441-10080 minutes (7 days) → CHILD
#define ADULT_MAX_MINUTES    43200   // 10081-43200 minutes (30 days) → ADULT
// 43201+ minutes → ELDER

// --- Day/Night Cycle ---
// Virtual time: 1 real second = 1 virtual minute
// Virtual day = 1440 virtual minutes = 1440 real seconds = 24 real minutes
#define DAY_START_HOUR        6    // 6:00 virtual time → day begins
#define NIGHT_START_HOUR     18    // 18:00 virtual time → night begins
#define VIRTUAL_MINUTES_PER_HOUR  60
#define VIRTUAL_MINUTES_PER_DAY   1440

// --- Serial ---
#define SERIAL_BAUD  115200

#endif // CONFIG_H
