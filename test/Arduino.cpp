// Arduino mock implementation for native unit tests
#include "Arduino.h"
#include <sys/time.h>
#include <unistd.h>
#include <cstdlib>

static bool _rand_seeded = false;

void _ensure_seed() {
  if (!_rand_seeded) {
    srand(time(nullptr));
    _rand_seeded = true;
  }
}

MockSerial Serial;

unsigned long millis() {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return (unsigned long)(tv.tv_sec * 1000UL + tv.tv_usec / 1000UL);
}

void delay(unsigned long ms) {
  usleep(ms * 1000);
}

void pinMode(uint8_t pin, uint8_t mode) {}
void digitalWrite(uint8_t pin, uint8_t val) {}
int digitalRead(uint8_t pin) { return 0; }
int analogRead(uint8_t pin) { return 0; }
void analogWrite(uint8_t pin, int val) {}

// Arduino-compatible random() overloads
long random(long max) { _ensure_seed(); return rand() % max; }
long random(long min, long max) { _ensure_seed(); return min + rand() % (max - min); }

// tone/noTone stubs
void tone(uint8_t pin, unsigned int frequency, unsigned long duration) {}
void noTone(uint8_t pin) {}

// ESP32 stubs
namespace ESP {
  unsigned long getFreeHeap() { return 327680; }
  unsigned long getMinFreeHeap() { return 300000; }
}
