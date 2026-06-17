#include "RGB_LED.h"
#include "config.h"

// ============================================================
// RGB LED Status Indicator (Phase 6.3)
// Uses common cathode RGB LED on GPIO 12/13/14
// ============================================================

static unsigned long lastBlinkTime = 0;
static bool blinkState = false;

void setupRGBLED() {
  pinMode(RGB_LED_RED_PIN, OUTPUT);
  pinMode(RGB_LED_GREEN_PIN, OUTPUT);
  pinMode(RGB_LED_BLUE_PIN, OUTPUT);
  // Start with green (healthy)
  digitalWrite(RGB_LED_RED_PIN, LOW);
  digitalWrite(RGB_LED_GREEN_PIN, HIGH);
  digitalWrite(RGB_LED_BLUE_PIN, LOW);
  Serial.println("RGB LED initialized");
}

static void setColor(bool red, bool green, bool blue) {
  digitalWrite(RGB_LED_RED_PIN, red ? HIGH : LOW);
  digitalWrite(RGB_LED_GREEN_PIN, green ? HIGH : LOW);
  digitalWrite(RGB_LED_BLUE_PIN, blue ? HIGH : LOW);
}

void updateRGBLED(const Pet &pet) {
  if (!pet.isAlive) {
    // Dead: slow red blink
    if (millis() - lastBlinkTime > 1000) {
      lastBlinkTime = millis();
      blinkState = !blinkState;
      setColor(blinkState, false, false);
    }
    return;
  }

  if (pet.isDying) {
    // Dying: fast red blink
    if (millis() - lastBlinkTime > 200) {
      lastBlinkTime = millis();
      blinkState = !blinkState;
      setColor(blinkState, false, false);
    }
    return;
  }

  if (pet.state == "sleeping") {
    // Sleeping: dim blue
    setColor(false, false, true);
    return;
  }

  if (pet.health <= 10 || pet.hunger <= 10) {
    // Critical: solid red
    setColor(true, false, false);
    return;
  }

  if (pet.health <= 30 || pet.hunger <= 20 || pet.state == "sick") {
    // Warning: yellow (red + green)
    setColor(true, true, false);
    return;
  }

  // Healthy: green
  setColor(false, true, false);
}
