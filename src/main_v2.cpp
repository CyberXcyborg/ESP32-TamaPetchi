// ============================================================
// ESP32-TamaPetchi v2.0 — Main Entry Point
// LVGL Display + Touch + Pet Engine
// ============================================================

#include <Arduino.h>
#include <SPI.h>
#include <WiFi.h>
#include <lvgl.h>

#include "config_v2.h"
#include "AppState_v2.h"
#include "Pet_v2.h"
#include "Storage_v2.h"
#include "DisplayDriver.h"

#include "TouchDriver.h"
#include "HAL_v2.h"

// ============================================================
// Globals
// ============================================================

static PetEngine pet;
static uint32_t last_pet_update = 0;
static uint32_t last_pet_save = 0;
#define MIN_SAVE_INTERVAL 300000UL  // 5 minutes between saves
static uint32_t last_ui_update = 0;

// LVGL UI elements
static lv_obj_t *screen_main = NULL;
static lv_obj_t *label_title = NULL;
static lv_obj_t *label_status = NULL;
static lv_obj_t *bar_hunger = NULL;
static lv_obj_t *bar_happiness = NULL;
static lv_obj_t *bar_energy = NULL;
static lv_obj_t *bar_health = NULL;
static lv_obj_t *pet_canvas = NULL;

// ============================================================
// UI Helpers
// ============================================================

static void ui_create() {
    screen_main = lv_scr_act();
    
    // Background
    lv_obj_set_style_bg_color(screen_main, lv_color_hex(0x1a1a2e), 0);
    
    // Title
    label_title = lv_label_create(screen_main);
    lv_label_set_text(label_title, "TamaPetchi v2.0");
    lv_obj_set_style_text_font(label_title, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(label_title, lv_color_hex(0xffffff), 0);
    lv_obj_align(label_title, LV_ALIGN_TOP_MID, 0, 8);
    
    // Status
    label_status = lv_label_create(screen_main);
    lv_label_set_text(label_status, "Initializing...");
    lv_obj_set_style_text_font(label_status, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(label_status, lv_color_hex(0xaaaaaa), 0);
    lv_obj_align(label_status, LV_ALIGN_TOP_MID, 0, 30);
    
    // Pet sprite placeholder (colored circle)
    pet_canvas = lv_obj_create(screen_main);
    lv_obj_set_size(pet_canvas, 64, 64);
    lv_obj_align(pet_canvas, LV_ALIGN_CENTER, 0, -30);
    lv_obj_set_style_bg_color(pet_canvas, lv_palette_main(LV_PALETTE_GREEN), 0);
    lv_obj_set_style_radius(pet_canvas, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_width(pet_canvas, 2, 0);
    lv_obj_set_style_border_color(pet_canvas, lv_color_hex(0xffffff), 0);
    
    // Stats bars
    lv_obj_t *lbl_hunger = lv_label_create(screen_main);
    lv_label_set_text(lbl_hunger, "Hunger");
    lv_obj_set_style_text_font(lbl_hunger, &lv_font_montserrat_10, 0);
    lv_obj_align(lbl_hunger, LV_ALIGN_BOTTOM_MID, -100, -70);
    
    bar_hunger = lv_bar_create(screen_main);
    lv_obj_set_size(bar_hunger, 80, 10);
    lv_obj_align(bar_hunger, LV_ALIGN_BOTTOM_MID, 50, -70);
    lv_bar_set_range(bar_hunger, 0, 100);
    lv_obj_set_style_bg_color(bar_hunger, lv_palette_main(LV_PALETTE_RED), LV_PART_INDICATOR);
    
    lv_obj_t *lbl_happy = lv_label_create(screen_main);
    lv_label_set_text(lbl_happy, "Happy");
    lv_obj_set_style_text_font(lbl_happy, &lv_font_montserrat_10, 0);
    lv_obj_align(lbl_happy, LV_ALIGN_BOTTOM_MID, -100, -50);
    
    bar_happiness = lv_bar_create(screen_main);
    lv_obj_set_size(bar_happiness, 80, 10);
    lv_obj_align(bar_happiness, LV_ALIGN_BOTTOM_MID, 50, -50);
    lv_bar_set_range(bar_happiness, 0, 100);
    lv_obj_set_style_bg_color(bar_happiness, lv_palette_main(LV_PALETTE_YELLOW), LV_PART_INDICATOR);
    
    lv_obj_t *lbl_energy = lv_label_create(screen_main);
    lv_label_set_text(lbl_energy, "Energy");
    lv_obj_set_style_text_font(lbl_energy, &lv_font_montserrat_10, 0);
    lv_obj_align(lbl_energy, LV_ALIGN_BOTTOM_MID, -100, -30);
    
    bar_energy = lv_bar_create(screen_main);
    lv_obj_set_size(bar_energy, 80, 10);
    lv_obj_align(bar_energy, LV_ALIGN_BOTTOM_MID, 50, -30);
    lv_bar_set_range(bar_energy, 0, 100);
    lv_obj_set_style_bg_color(bar_energy, lv_palette_main(LV_PALETTE_BLUE), LV_PART_INDICATOR);
    
    lv_obj_t *lbl_health = lv_label_create(screen_main);
    lv_label_set_text(lbl_health, "Health");
    lv_obj_set_style_text_font(lbl_health, &lv_font_montserrat_10, 0);
    lv_obj_align(lbl_health, LV_ALIGN_BOTTOM_MID, -100, -10);
    
    bar_health = lv_bar_create(screen_main);
    lv_obj_set_size(bar_health, 80, 10);
    lv_obj_align(bar_health, LV_ALIGN_BOTTOM_MID, 50, -10);
    lv_bar_set_range(bar_health, 0, 100);
    lv_obj_set_style_bg_color(bar_health, lv_palette_main(LV_PALETTE_GREEN), LV_PART_INDICATOR);
}

static void ui_update_stats() {
    const PetData &data = pet.getData();
    
    if (label_status) {
        const char *stage_names[] = {"Baby", "Child", "Adult", "Elder"};
        char buf[64];
        snprintf(buf, sizeof(buf), "%s | %s | Age: %lum", 
                 data.name.c_str(), stage_names[data.stage], data.age_minutes);
        lv_label_set_text(label_status, buf);
    }
    
    if (bar_hunger) lv_bar_set_value(bar_hunger, data.hunger, LV_ANIM_ON);
    if (bar_happiness) lv_bar_set_value(bar_happiness, data.happiness, LV_ANIM_ON);
    if (bar_energy) lv_bar_set_value(bar_energy, data.energy, LV_ANIM_ON);
    if (bar_health) lv_bar_set_value(bar_health, data.health, LV_ANIM_ON);
    
    // Update pet color based on health
    if (pet_canvas) {
        lv_color_t color;
        if (data.health > 70) color = lv_palette_main(LV_PALETTE_GREEN);
        else if (data.health > 40) color = lv_palette_main(LV_PALETTE_YELLOW);
        else if (data.health > 10) color = lv_palette_main(LV_PALETTE_ORANGE);
        else color = lv_palette_main(LV_PALETTE_RED);
        lv_obj_set_style_bg_color(pet_canvas, color, 0);
    }
}

// ============================================================
// LVGL Tick Timer
// ============================================================

static void lv_tick_cb(void *arg) {
    lv_tick_inc(LVGL_TICK_PERIOD_MS);
}

// ============================================================
// Setup & Loop
// ============================================================

void setup() {
    Serial.begin(SERIAL_BAUD);
    delay(100);
    DEBUG_PRINTLN("\n\n=== TamaPetchi v2.0 ===");
    DEBUG_PRINTLN("ESP32-S3 + LVGL + ST7789");
    
    // Initialize HAL
    HAL_V2::begin();
    DEBUG_PRINTF("[System] CPU: %u MHz, Free heap: %u bytes\n", 
                 HAL_V2::getCpuFreqMHz(), HAL_V2::getFreeHeap());
    DEBUG_PRINTF("[System] Reset reason: %s\n", HAL_V2::getResetReason().c_str());
    
    // Initialize AppState
    g_state.update();
    
    // Initialize LittleFS
    if (StorageV2::begin()) {
        DEBUG_PRINTLN("[Storage] LittleFS mounted");
    } else {
        DEBUG_PRINTLN("[Storage] LittleFS mount failed!");
    }
    
    // Initialize LVGL
    lv_init();
    
    // Initialize display driver
    if (DisplayDriver::begin()) {
        g_state.displayReady = true;
        DEBUG_PRINTLN("[Display] Ready");
    } else {
        DEBUG_PRINTLN("[Display] FAILED!");
    }
    
    // Initialize touch driver
    if (TouchDriver::begin()) {
        g_state.touchReady = true;
        DEBUG_PRINTLN("[Touch] Ready");
    }
    
    // Create UI
    ui_create();
    
    // Initialize pet
    pet.init();
    
    // Try to load pet from storage
    if (StorageV2::exists(PET_DATA_FILE)) {
        String saved = StorageV2::read(PET_DATA_FILE);
        if (saved.length() > 0 && pet.fromJson(saved)) {
            DEBUG_PRINTLN("[Pet] Loaded from storage");
        }
    }
    
    ui_update_stats();
    
    DEBUG_PRINTLN("[System] Setup complete. Starting main loop...");
    DEBUG_PRINTF("[System] Free heap: %u bytes\n", HAL_V2::getFreeHeap());
}

void loop() {
    // Handle LVGL tasks
    lv_timer_handler();
    
    // Pet engine update
    uint32_t now = millis();
    if (now - last_pet_update >= PET_UPDATE_INTERVAL) {
        pet.update();
        ui_update_stats();
        last_pet_update = now;
        
        // Save pet state (throttled to MIN_SAVE_INTERVAL)
        if (now - last_pet_save >= MIN_SAVE_INTERVAL) {
            StorageV2::write(PET_DATA_FILE, pet.toJson());
            last_pet_save = now;
        }
        
        DEBUG_PRINTF("[Pet] H=%d Hp=%d E=%d HP=%d Stage=%d\n",
                     pet.getData().hunger, pet.getData().happiness,
                     pet.getData().energy, pet.getData().health,
                     pet.getData().stage);
    }
    
    // AppState update (every 10s)
    static uint32_t last_state_update = 0;
    if (now - last_state_update >= 10000) {
        g_state.update();
        last_state_update = now;
    }
    
    delay(LVGL_TICK_PERIOD_MS);
}
