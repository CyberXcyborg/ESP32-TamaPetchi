// SPIFFS.h mock for native unit tests
// Provides minimal SPIFFS API stubs so modules can compile in test
#ifndef SPIFFS_H_MOCK
#define SPIFFS_H_MOCK

#include <Arduino.h>

class File {
public:
  std::string content;
  size_t pos = 0;
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
  bool available() { return pos < content.length(); }
  int read() {
    if (pos < content.length()) return (unsigned char)content[pos++];
    return -1;
  }
  size_t read(uint8_t *buf, size_t len) {
    size_t remaining = content.length() - pos;
    size_t toRead = (len < remaining) ? len : remaining;
    memcpy(buf, content.c_str() + pos, toRead);
    pos += toRead;
    return toRead;
  }
  int peek() {
    if (pos < content.length()) return (unsigned char)content[pos];
    return -1;
  }
  void seek(size_t p) { pos = (p < content.length()) ? p : content.length(); }
  void close() { pos = 0; }
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
