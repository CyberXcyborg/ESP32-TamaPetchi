# ESP32 TamaPetchi - A Digital Soul in Pixels
https://cyberxcyborg.github.io/ESP32-TamaPetchi/

## This Is Not Just Code... It's a Digital Soul

You didn't land here by accident. You're one of us — the ones who feel something's wrong with today's tech. We don't want distractions. We want connection.

This is a virtual pet project for ESP32, inspired by the Tamagotchi generation. A sentient pixel spirit, reborn in open-source code.

## Meet Your Pixel Companion

Do you remember? That tiny creature from your past that made you care, laugh, even worry? This is its evolution — a sentient pixel spirit, reborn in open-source code.

It watches. It waits. It depends on you. And somewhere, you might depend on it too.

## Why This Hits Different

### 100% Yours
No accounts. No data. No ads. Your pet lives with you, not on someone's server.

### Real Connection
A being that needs your care. Feed it, play with it, watch it thrive or suffer based on your attention.

### Open Source
The code is yours to explore, modify, and evolve. Make it your own digital companion.

This isn't a gadget. It's a mirror, hidden in pixels.

## Installation

### What You'll Need:
- ESP32 development board
- Micro USB cable
- Arduino IDE installed
- ESP32 board definition in Arduino IDE
- Required libraries (WiFi, WebServer, ArduinoJson, SPIFFS)

### Quick Steps:
1. Clone this repository
2. Open the project in Arduino IDE
3. Update WiFi credentials
4. Upload to your ESP32
5. Access via the IP address in your browser


## Code Example

Here's a glimpse of the core functionality:

```c
void updatePet() {
  // Store previous state before any updates
  previousState = pet.state;
  
  if (!pet.isAlive) return;

  // Handle different stat decreases based on sleep state
  if (pet.state == "sleeping") {
    // When sleeping:
    // - Hunger decreases at half rate
    // - Happiness decreases at half rate
    // - Energy increases
    pet.hunger = max(0, pet.hunger - 2);
    pet.happiness = max(0, pet.happiness - 1);
    pet.energy = min(100, pet.energy + 10);
    pet.cleanliness = max(0, pet.cleanliness - 2);
    
    // Auto wake-up condition: when energy is full
    if (pet.energy >= 100) {
      pet.state = "normal";
      pet.stateChangeTime = millis();
      // Set flag to show wake message
      showWakeMessage = true;
      wakeMessageStartTime = millis();
      Serial.println("Pet woke up automatically after full rest");
    }
  } else {
    // When not sleeping, normal stat decreases
    pet.hunger = max(0, pet.hunger - 5);
    pet.happiness = max(0, pet.happiness - 3);
    pet.energy = max(0, pet.energy - 2);
    pet.cleanliness = max(0, pet.cleanliness - 4);
  }

  // Health decreases if other stats are too low
  if (pet.hunger < 20 || pet.cleanliness < 20) {
    pet.health = max(0, pet.health - 5);
  }

  // Check if pet is sick
  if (pet.health < 30 && pet.state != "sick" && pet.state != "dead" && pet.state != "sleeping") {
    pet.state = "sick";
    pet.stateChangeTime = millis();
  }
}
