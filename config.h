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

#endif // CONFIG_H
