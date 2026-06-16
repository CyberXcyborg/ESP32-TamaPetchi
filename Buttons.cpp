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
