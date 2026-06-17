#include "OLED.h"
#include "config.h"

#ifdef ENABLE_OLED

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT  64
#define OLED_RESET     -1
#define OLED_ADDRESS   0x3C

static Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

void setupOLED() {
  Wire.begin(21, 22); // SDA=21, SCL=22
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS)) {
    Serial.println("SSD1306 OLED allocation failed");
    return;
  }
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("TamaPetchi");
  display.display();
  Serial.println("OLED initialized");
}

void updateOLED(const Pet &pet) {
  display.clearDisplay();

  // Header: pet name + state
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print(pet.name);
  display.print(" - ");

  // State text
  if (!pet.isAlive) {
    display.println("DEAD");
  } else if (pet.state == "sleeping") {
    display.println("Zzz...");
  } else if (pet.state == "eating") {
    display.println("Eating");
  } else if (pet.state == "playing") {
    display.println("Playing");
  } else if (pet.state == "sick") {
    display.println("Sick!");
  } else if (pet.state == "hungry") {
    display.println("Hungry!");
  } else if (pet.state == "critical") {
    display.println("Critical!");
  } else if (pet.state == "dying") {
    display.println("Dying!");
  } else {
    display.println("Happy");
  }

  // Separator line
  display.drawLine(0, 10, SCREEN_WIDTH, 10, SSD1306_WHITE);

  // Stat bars: hunger, health, energy
  const char* labels[] = {"Hunger", "Health", "Energy"};
  int values[] = {pet.hunger, pet.health, pet.energy};

  for (int i = 0; i < 3; i++) {
    int y = 14 + i * 16;
    display.setCursor(0, y);
    display.setTextSize(1);
    display.print(labels[i]);
    display.print(": ");
    display.print(values[i]);
    display.print("%");

    // Bar background
    int barX = 0;
    int barY = y + 10;
    int barW = SCREEN_WIDTH;
    int barH = 4;
    display.drawRect(barX, barY, barW, barH, SSD1306_WHITE);

    // Bar fill
    int fillW = (values[i] * barW) / 100;
    if (fillW > 0) {
      display.fillRect(barX, barY, fillW, barH, SSD1306_WHITE);
    }
  }

  // Stage indicator at bottom
  display.setCursor(0, 62);
  display.setTextSize(1);
  display.print("Age:");
  display.print(pet.age);
  display.print("m ");
  switch (pet.stage) {
    case BABY:  display.print("[Baby]");  break;
    case CHILD: display.print("[Child]"); break;
    case ADULT: display.print("[Adult]"); break;
    case ELDER: display.print("[Elder]"); break;
  }

  // Phase 6: Battery level display (if ADC available)
  // Read battery voltage from GPIO 35 (common on ESP32 dev boards)
  int raw = analogRead(35);
  float voltage = (raw / 4095.0) * 3.3 * 2; // Assuming voltage divider
  int battPercent = constrain(map((int)(voltage * 100), 330, 420, 0, 100), 0, 100);
  display.setCursor(90, 62);
  display.print(battPercent);
  display.print("%");

  display.display();
}

#endif // ENABLE_OLED
