#include "Buttons.h"
#include "Storage.h"
#include "config.h"

// ============================================================
// Physical Button Handler (Phase 6.3)
// Short press = cycle through actions (feed → play → clean → sleep)
// Long press (2s) = confirm/execute current action
// ============================================================

static unsigned long buttonPressTime = 0;
static bool buttonWasPressed = false;
static int currentAction = 0; // 0=feed, 1=play, 2=clean, 3=sleep
static bool actionSelected = false;

void setupButtons() {
  pinMode(BUTTON_PIN, INPUT_PULLUP); // GPIO 0 has internal pull-up
  Serial.println("Buttons initialized (GPIO 0 BOOT button)");
}

void checkButtons(Pet &pet) {
  bool buttonPressed = (digitalRead(BUTTON_PIN) == LOW); // Active low

  // Debounce
  static unsigned long lastDebounceTime = 0;
  if (buttonPressed != buttonWasPressed) {
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) < BUTTON_DEBOUNCE_MS) {
    buttonWasPressed = buttonPressed;
    return;
  }

  // Button just pressed
  if (buttonPressed && !buttonWasPressed) {
    buttonPressTime = millis();
    buttonWasPressed = buttonPressed;
  }

  // Button just released
  if (!buttonPressed && buttonWasPressed) {
    unsigned long pressDuration = millis() - buttonPressTime;
    buttonWasPressed = buttonPressed;

    if (pressDuration >= BUTTON_LONG_PRESS_MS) {
      // Long press = execute current action
      if (!pet.isAlive && currentAction == 3) {
        // Special: long press sleep on dead pet = reset
        initPet(pet);
        savePetDataForce(pet);
        Serial.println("Pet reset via long press");
      } else {
        switch (currentAction) {
          case 0: // Feed
            if (pet.isAlive) {
              feedPet(pet);
              Serial.println("Button: Feed");
            }
            break;
          case 1: // Play
            if (pet.isAlive && pet.energy >= PLAY_ENERGY_MIN) {
              playPet(pet);
              Serial.println("Button: Play");
            }
            break;
          case 2: // Clean
            if (pet.isAlive) {
              cleanPet(pet);
              Serial.println("Button: Clean");
            }
            break;
          case 3: // Sleep toggle
            if (pet.isAlive) {
              sleepPet(pet);
              Serial.println("Button: Sleep toggle");
            }
            break;
        }
        savePetData(pet);
      }
      actionSelected = false;
    } else {
      // Short press = cycle action
      currentAction = (currentAction + 1) % 4;
      actionSelected = true;
      const char* actionNames[] = {"Feed", "Play", "Clean", "Sleep"};
      Serial.print("Button: Selected ");
      Serial.println(actionNames[currentAction]);
    }
  }

  // Long press detection while held
  if (buttonPressed && buttonWasPressed && !actionSelected) {
    if (millis() - buttonPressTime >= BUTTON_LONG_PRESS_MS) {
      // Visual/audio feedback could go here (buzzer beep)
      actionSelected = true; // Mark as executed to prevent repeat
    }
  }
}

// ============================================================
// Deep Sleep with State Restore (Phase 6.3)
// Saves critical pet state to RTC memory before.deep sleep
// On wake, restores from RTC + loads full state from SPIFFS
// ============================================================

void enterDeepSleep(Pet &pet) {
  // Save critical state to RTC memory (survives deep sleep)
  rtcMagic = RTC_MAGIC_VALUE;
  rtcPetAge = pet.age;
  rtcPetHunger = pet.hunger;
  rtcPetHappiness = pet.happiness;
  rtcPetHealth = pet.health;
  rtcPetEnergy = pet.energy;
  rtcPetCleanliness = pet.cleanliness;
  rtcPetAlive = pet.isAlive;
  rtcPetStage = (int)pet.stage;
  rtcPetType = (int)pet.type;
  rtcPetWeather = pet.weather;
  rtcPetMusic = pet.musicEnabled;
  rtcPetDifficulty = pet.difficulty;

  // Also save to SPIFFS as backup (in case RTC is corrupted)
  savePetDataForce(pet);

  Serial.println("Entering deep sleep... Will wake on button press.");
  Serial.flush();

  // Configure wake on GPIO 0 (BOOT button)
  esp_sleep_enable_ext0_wakeup((gpio_num_t)DEEP_SLEEP_PIN, LOW);
  esp_deep_sleep_start();
}

bool wasDeepSleepWake() {
  esp_sleep_wakeup_cause_t wakeupReason = esp_sleep_get_wakeup_cause();
  return (wakeupReason == ESP_SLEEP_WAKEUP_EXT0);
}

void restoreFromRTC(Pet &pet) {
  if (rtcMagic != RTC_MAGIC_VALUE) {
    Serial.println("RTC magic mismatch — cold boot, not deep sleep wake");
    return;
  }

  // Restore from RTC memory for fast boot
  pet.age = rtcPetAge;
  pet.hunger = constrain(rtcPetHunger, STAT_MIN, STAT_MAX);
  pet.happiness = constrain(rtcPetHappiness, STAT_MIN, STAT_MAX);
  pet.health = constrain(rtcPetHealth, STAT_MIN, STAT_MAX);
  pet.energy = constrain(rtcPetEnergy, STAT_MIN, STAT_MAX);
  pet.cleanliness = constrain(rtcPetCleanliness, STAT_MIN, STAT_MAX);
  pet.isAlive = rtcPetAlive;
  pet.stage = (PetStage)constrain(rtcPetStage, BABY, ELDER);
  pet.type = (PetType)constrain(rtcPetType, BLOB, DOG);
  pet.weather = constrain(rtcPetWeather, 0, 4);
  pet.musicEnabled = rtcPetMusic;
  pet.difficulty = constrain(rtcPetDifficulty, 0, 2);

  // Also try SPIFFS load for any missing fields
  // SPIFFS load will override RTC if file exists (SPIFFS is more complete)
  if (SPIFFS.exists(PET_DATA_FILE)) {
    // SPIFFS exists — load it. But preserve age and core stats from RTC
    // since SPIFFS might be older. We'll load SPIFFS but keep RTC age.
    unsigned long savedAge = pet.age;
    loadPetData(pet);
    // Use the more recent of RTC age vs SPIFFS age
    if (savedAge > pet.age) pet.age = savedAge;
  }

  // Clear magic to prevent re-restore on next boot
  rtcMagic = 0;

  Serial.println("Restored pet state from RTC + SPIFFS after deep sleep wake");
  Serial.print("Age: ");
  Serial.print(pet.age);
  Serial.print(" State: ");
  Serial.println(pet.state);
}
