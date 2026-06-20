// SPIFFS.h mock for native unit tests
// Provides minimal SPIFFS API stubs so Community.cpp can compile in test
#ifndef SPIFFS_H_MOCK
#define SPIFFS_H_MOCK

#include <Arduino.h>
#include <string>

class File {
public:
  std::string content;
  bool writing = false;
  operator bool() { return true; }
  size_t write(const uint8_t *buf, size_t len) {
    content.append((const char *)buf, len);
    return len;
  }
  size_t print(const String &s) {
    content += s.str;
    return s.length();
  }
  String readString() { return String(content.c_str()); }
  size_t size() { return content.length(); }
  bool available() { return false; }
  void close() {}
};

class SPIFFSClass {
public:
  bool begin(bool = false) { return true; }
  bool exists(const char *) { return false; }
  File open(const char *, const char *) { return File(); }
  bool remove(const char *) { return true; }
  bool format() { return true; }
};

extern SPIFFSClass SPIFFS;

#endif // SPIFFS_H_MOCK
