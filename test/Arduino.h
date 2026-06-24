// Arduino.h mock for native unit tests (PlatformIO native environment)
// Provides minimal definitions needed to compile test code without ESP32 toolchain
#ifndef ARDUINO_H_MOCK
#define ARDUINO_H_MOCK

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <ctime>

// Type aliases
typedef uint8_t byte;
typedef uint16_t word;
typedef bool boolean;

// Pin constants
#define HIGH 1
#define LOW 0
#define INPUT 0
#define OUTPUT 1
#define INPUT_PULLUP 2
#define HEX 16
#define DEC 10
#define OCT 8
#define BIN 2

// Mock Serial
class MockSerial {
public:
  void begin(unsigned long baud) {}
  void print(const char* s) {}
  void println(const char* s) {}
  void printf(const char* fmt, ...) {}
  int available(void) { return 0; }
  int read(void) { return -1; }
  void write(const uint8_t* buf, size_t len) {}
  void flush() {}
};
extern MockSerial Serial;

// Mock millis/delay
unsigned long millis(void);
void delay(unsigned long ms);

// Mock pinMode/digitalWrite
void pinMode(uint8_t pin, uint8_t mode);
void digitalWrite(uint8_t pin, uint8_t val);
int digitalRead(uint8_t pin);
int analogRead(uint8_t pin);
void analogWrite(uint8_t pin, int val);

// PROGMEM mock
#define PROGMEM
#define F(x) (x)
#define pgm_read_byte(addr) (*(const unsigned char*)(addr))
#define pgm_read_word(addr) (*(const unsigned short*)(addr))
#define pgm_read_dword(addr) (*(const unsigned long*)(addr))
#define pgm_read_float(addr) (*(const float*)(addr))
#define pgm_read_ptr(addr) (*(const void**)(addr))

// Flash string helper
class __FlashStringHelper {};
#define FPSTR(pstr_pointer) (pstr_pointer)

// min/max/constrain as inline functions to avoid macro conflicts with std::
#ifndef min
inline int min(int a, int b) { return a < b ? a : b; }
#endif
#ifndef max
inline int max(int a, int b) { return a > b ? a : b; }
#endif
inline int constrain(int x, int lo, int hi) {
  return x < lo ? lo : (x > hi ? hi : x);
}

// map function (Arduino-compatible)
inline long map(long x, long in_min, long in_max, long out_min, long out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

// Arduino random() overloads
long random(long max);
long random(long min, long max);

// tone/noTone
void tone(uint8_t pin, unsigned int frequency, unsigned long duration = 0);
void noTone(uint8_t pin);

// ESP32 stubs (inline for header visibility)
namespace ESP {
  inline unsigned long getFreeHeap() { return 327680; }
  inline unsigned long getMinFreeHeap() { return 300000; }
  inline uint64_t getEfuseMac() { return 0x123456789ABCDEF0ULL; }
  inline void restart() {}
}

// String class (simplified wrapper around std::string)
#include <string>
class String {
public:
  std::string str;
  String() {}
  String(const char* s) : str(s ? s : "") {}
  String(const std::string& s) : str(s) {}
  String(int n) : str(std::to_string(n)) {}
  String(long n) : str(std::to_string(n)) {}
  String(unsigned long n) : str(std::to_string(n)) {}
  String(float n) : str(std::to_string(n)) {}
  String(double n) : str(std::to_string(n)) {}
  String(uint32_t n, int base) : str(std::to_string(n)) {} // HEX format stub
  operator const char*() const { return str.c_str(); }
  operator const char*() { return str.c_str(); }
  const char* c_str() const { return str.c_str(); }
  int length() const { return (int)str.length(); }
  bool operator==(const String& other) const { return str == other.str; }
  bool operator==(const char* other) const { return str == other; }
  bool equals(const String& other) const { return str == other.str; }
  bool equals(const char* other) const { return str == other; }
  String& operator+=(const String& other) { str += other.str; return *this; }
  String& operator+=(const char* other) { str += other; return *this; }
  String substring(int from, int to = -1) const {
    if (to < 0) return String(str.substr(from));
    return String(str.substr(from, to - from));
  }
  int indexOf(char c, int from = 0) const {
    size_t pos = str.find(c, from);
    return (pos == std::string::npos) ? -1 : (int)pos;
  }
  int indexOf(const char* s, int from = 0) const {
    size_t pos = str.find(s, from);
    return (pos == std::string::npos) ? -1 : (int)pos;
  }
  void trim() {
    while (!str.empty() && (str.front() == ' ' || str.front() == '\t')) str.erase(str.begin());
    while (!str.empty() && (str.back() == ' ' || str.back() == '\t')) str.pop_back();
  }
  void replace(const char* from, const char* to) {
    size_t pos = 0;
    while ((pos = str.find(from, pos)) != std::string::npos) {
      str.replace(pos, strlen(from), to);
      pos += strlen(to);
    }
  }
  void remove(unsigned int index, unsigned int count = 1) {
    if (index < str.length()) str.erase(index, count);
  }
  unsigned char charAt(unsigned int index) const { return (unsigned char)str[index]; }
  void setCharAt(unsigned int index, char c) { str[index] = c; }
  long toInt() const { return strtol(str.c_str(), nullptr, 10); }
  float toFloat() const { return strtof(str.c_str(), nullptr); }

  // ESP32 compatibility
  void toUpperCase() {
    for (auto &c : str) {
      if (c >= 'a' && c <= 'z') c = c - 'a' + 'A';
    }
  }
  void toLowerCase() {
    for (auto &c : str) {
      if (c >= 'A' && c <= 'Z') c = c - 'A' + 'a';
    }
  }

  // ArduinoJson compatibility
  size_t write(uint8_t c) { str += (char)c; return 1; }
  size_t write(const uint8_t* s, size_t n) { str.append((const char*)s, n); return n; }
  int available() { return 0; }
  int read() { return -1; }
  int peek() { return -1; }
  void flush() {}
};

// Non-member operators for String concatenation
inline String operator+(const char* lhs, const String& rhs) { return String(lhs) + rhs; }
inline String operator+(const String& lhs, const String& rhs) { return String(lhs.str + rhs.str); }
inline String operator+(const String& lhs, const char* rhs) { return String(lhs.str + rhs); }

#endif // ARDUINO_H_MOCK
