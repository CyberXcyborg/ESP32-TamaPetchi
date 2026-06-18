// Arduino mock implementation for native unit tests
#include "Arduino.h"
#include <sys/time.h>
#include <unistd.h>

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
