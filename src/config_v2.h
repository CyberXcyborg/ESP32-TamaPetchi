#ifndef CONFIG_V2_H
#define CONFIG_V2_H

// ============================================================
// ESP32 TamaPetchi v2.0 — Configuration
// Target: ESP32-S3 + LVGL + ST7789 TFT
// ============================================================

// --- Hardware: ESP32-S3 ---
#define CHIP_ESP32_S3
#define CPU_FREQ_MHZ        240
#define SRAM_SIZE_KB        512
#define PSRAM_SIZE_MB       8
#define FLASH_SIZE_MB       8

// --- WiFi Credentials ---
#define WIFI_SSID     "TEST"
#define WIFI_PASSWORD "TEST"

// --- Network ---
#define WEB_SERVER_PORT  80

// --- LittleFS (replaces SPIFFS) ---
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
#define HEALTH_DECAY_THRESHOLD   20
#define HEALTH_DECAY_AMOUNT       5
#define SICK_THRESHOLD           30

// --- Action: Feed ---
#define FEED_HUNGER_GAIN   20
#define FEED_ENERGY_COST    5
#define FEED_HEALTH_BONUS   5

// --- Action: Play ---
#define PLAY_HAPPINESS_GAIN  15
#define PLAY_ENERGY_COST     15
#define PLAY_HUNGER_COST     10
#define PLAY_ENERGY_MIN      10

// --- Action: Clean ---
#define CLEAN_HEALTH_BONUS    5

// --- Serial ---
#define SERIAL_BAUD  115200

// --- Debug Output Control ---
#ifdef DISABLE_DEBUG
  #define DEBUG_PRINT(x)
  #define DEBUG_PRINTLN(x)
  #define DEBUG_PRINTF(fmt, ...)
#else
  #define DEBUG_PRINT(x)    Serial.print(x)
  #define DEBUG_PRINTLN(x)  Serial.println(x)
  #define DEBUG_PRINTF(fmt, ...) Serial.printf(fmt, ##__VA_ARGS__)
#endif

// --- Evolution Stage Thresholds (in minutes) ---
#define BABY_MAX_MINUTES    60
#define CHILD_MAX_MINUTES   480
#define ADULT_MAX_MINUTES   1440

// --- Stage Decay Multipliers (percentage of base rate) ---
#define BABY_DECAY_MULT     80
#define CHILD_DECAY_MULT    100
#define ADULT_DECAY_MULT    110
#define ELDER_DECAY_MULT    130

// --- Virtual Time ---
#define VIRTUAL_MINUTES_PER_HOUR  60UL
#define VIRTUAL_MINUTES_PER_DAY   (24UL * VIRTUAL_MINUTES_PER_HOUR)

// --- Day/Night Cycle ---
#define NIGHT_START_HOUR    22
#define DAY_START_HOUR      6

// --- Warning State Thresholds ---
#define DYING_HEALTH_MAX    10
#define DYING_HEALTH_MIN    0
#define CRITICAL_HEALTH_MAX 30
#define CRITICAL_HEALTH_MIN 11

// --- Wear Leveling ---
#define MIN_SAVE_INTERVAL   300000UL

// ============================================================
// v2.0 Display Configuration (LVGL + ST7789)
// ============================================================

// --- TFT Display (ST7789 via SPI) ---
#define TFT_WIDTH           240
#define TFT_HEIGHT          240
#define TFT_COLOR_DEPTH     16      // RGB565
#define TFT_SPI_HOST        SPI2_HOST
#define TFT_SPI_FREQ        40000000  // 40 MHz
#define TFT_PIN_MOSI        11
#define TFT_PIN_SCLK        12
#define TFT_PIN_CS          10
#define TFT_PIN_DC          14
#define TFT_PIN_RST         13
#define TFT_PIN_BL          9       // Backlight (optional)

// --- Touch (XPT2046 resistive, shares SPI bus) ---
#define TOUCH_SPI_HOST      SPI2_HOST  // Same SPI as display
#define TOUCH_PIN_CS        15
#define TOUCH_PIN_IRQ       -1      // No IRQ
#define TOUCH_CALIBRATION   true

// --- LVGL Configuration ---
#define LVGL_TICK_PERIOD_MS  5      // LVGL tick interval
#define LVGL_BUFFER_SIZE     (TFT_WIDTH * TFT_HEIGHT / 10)  // 1/10 screen per buffer
#define LVGL_USE_PSRAM       true

// --- I2S Audio (MAX98357A) ---
#define I2S_BCLK_PIN        4
#define I2S_LRC_PIN         5
#define I2S_DIN_PIN         6
#define I2S_SAMPLE_RATE     44100
#define I2S_BITS_PER_SAMPLE  16

// --- Physical Buttons (optional, active-low) ---
#define BUTTON_FEED_PIN      0     // BOOT button on ESP32-S3
#define BUTTON_PLAY_PIN      -1    // Not connected
#define BUTTON_CLEAN_PIN     -1    // Not connected
#define BUTTON_SLEEP_PIN     -1    // Not connected

// --- I2C (for touch, accelerometer, NFC) ---
#define I2C_SDA_PIN         21
#define I2C_SCL_PIN         22

// --- Battery ADC ---
#define BATTERY_ADC_PIN      1     // GPIO1 (ADC1_CH0 on ESP32-S3)
#define BATTERY_VOLTAGE_MAX  4.2f
#define BATTERY_VOLTAGE_MIN  3.3f
#define LOW_BATTERY_THRESHOLD   20     // Battery percentage
#define CRITICAL_BATTERY_THRESHOLD 5    // Critical shutdown threshold
#define CHARGE_DETECT_PIN   -1     // GPIO for charge detection (TP4056), -1 = not connected
#define BATTERY_FILTER_SAMPLES 10    // ADC oversampling for noise reduction

// --- Power Management (Phase 23) ---
#define SLEEP_CHECK_INTERVAL    60000  // Check sleep conditions every 60s
#define IDLE_TIMEOUT_MS         300000 // Enter light sleep after 5min idle
#define DISPLAY_DIM_BATTERY     30     // Dim display to N% on low battery
#define DISPLAY_OFF_BATTERY     10     // Turn off display on critical battery
#define MAX_SLEEP_DURATION_MS   3600000 // Max light sleep: 1 hour
#define WAKEUP_BLE_TIMEOUT_MS   30000  // BLE wakeup advertising duration

// --- Watchdog Timer ---
#define WATCHDOG_TIMEOUT_MS     10000  // 10s watchdog for main loop
#define RENDER_WATCHDOG_MS      5000   // 5s watchdog for LVGL render

// --- OTA v2 ---
#define OTA_SIGNATURE_KEY       "TamaPetchi_v2_ed25519"  // Simple signature check key
#define OTA_MAX_PROGRESS        100
#define OTA_ROLLBACK_TIMEOUT_MS 30000  // 30s before auto-rollback on failed boot

// ============================================================
// v2.0 Compile-Time Assertions
// ============================================================

#if STAT_MAX <= STAT_MIN
  #error "STAT_MAX must be greater than STAT_MIN"
#endif

#if HUNGER_DECAY_NORMAL < 0 || HAPPINESS_DECAY_NORMAL < 0 || ENERGY_DECAY_NORMAL < 0
  #error "Decay rates must be non-negative"
#endif

#if BABY_MAX_MINUTES >= CHILD_MAX_MINUTES
  #error "BABY_MAX_MINUTES must be less than CHILD_MAX_MINUTES"
#endif
#if CHILD_MAX_MINUTES >= ADULT_MAX_MINUTES
  #error "CHILD_MAX_MINUTES must be less than ADULT_MAX_MINUTES"
#endif

#if SICK_THRESHOLD >= STAT_MAX
  #error "SICK_THRESHOLD must be less than STAT_MAX"
#endif

// v2.0 display assertions
#if TFT_WIDTH < 120 || TFT_WIDTH > 480
  #error "TFT_WIDTH must be between 120 and 480"
#endif
#if TFT_HEIGHT < 120 || TFT_HEIGHT > 480
  #error "TFT_HEIGHT must be between 120 and 480"
#endif
#if TFT_COLOR_DEPTH != 8 && TFT_COLOR_DEPTH != 16
  #error "TFT_COLOR_DEPTH must be 8 or 16"
#endif

#endif // CONFIG_V2_H
