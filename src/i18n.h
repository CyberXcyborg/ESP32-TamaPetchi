#ifndef I18N_H
#define I18N_H

// ============================================================
// i18n Multi-Language Support (Phase 10.3)
//
// Server-side language detection and locale management.
// Locale JSON files are stored in data/locales/ and served
// to the web UI for client-side translation.
// ============================================================

#include <Arduino.h>

// Supported languages
enum Language {
  LANG_EN = 0,  // English (default)
  LANG_ZH = 1,  // Chinese (Simplified)
  LANG_JA = 2,  // Japanese
  LANG_COUNT
};

// Language codes (ISO 639-1)
#define LANG_CODE_EN "en"
#define LANG_CODE_ZH "zh"
#define LANG_CODE_JA "ja"

// Default language
#define DEFAULT_LANGUAGE LANG_EN

// Get/Set current language (stored in AppState)
Language getCurrentLanguage();
void setCurrentLanguage(Language lang);

// Get language code string for a language
const char* getLanguageCode(Language lang);

// Parse language from string (e.g., "en", "zh", "ja")
// Returns LANG_EN if not recognized
Language parseLanguage(const String &code);

// Detect language from Accept-Language HTTP header
// Returns best match from supported languages
Language detectLanguage(const String &acceptLanguage);

// Get the locale file path for a language
// e.g., "/locales/en.json"
String getLocaleFilePath(Language lang);

// Load locale JSON from SPIFFS for a language
// Returns empty string if file not found
String loadLocale(Language lang);

#endif // I18N_H
