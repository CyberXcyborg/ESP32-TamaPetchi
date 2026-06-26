#include "i18n.h"
#include "config.h"
#include <SPIFFS.h>

// ============================================================
// Language Persistence (Phase 10.3)
// ============================================================
// Language setting is cached in a static variable and persisted
// to SPIFFS. In a future refactor, this should move to AppState.

#define LANGUAGE_FILE "/settings/lang.txt"

static Language currentLanguage = LANG_EN;
static bool languageLoaded = false;

// ============================================================
// Internal helpers
// ============================================================
static void loadLanguageFromSPIFFS() {
  if (languageLoaded) return;
  languageLoaded = true;

  File f = SPIFFS.open(LANGUAGE_FILE, "r");
  if (f) {
    String code = f.readString();
    f.close();
    currentLanguage = parseLanguage(code);
  } else {
    currentLanguage = LANG_EN;
  }
}

// ============================================================
// Public API
// ============================================================

Language getCurrentLanguage() {
  loadLanguageFromSPIFFS();
  return currentLanguage;
}

void setCurrentLanguage(Language lang) {
  if (lang >= LANG_COUNT) lang = DEFAULT_LANGUAGE;
  currentLanguage = lang;
  // Persist to SPIFFS
  File f = SPIFFS.open(LANGUAGE_FILE, "w");
  if (f) {
    f.print(getLanguageCode(lang));
    f.close();
  }
}

const char* getLanguageCode(Language lang) {
  switch (lang) {
    case LANG_EN: return LANG_CODE_EN;
    case LANG_ZH: return LANG_CODE_ZH;
    case LANG_JA: return LANG_CODE_JA;
    default:      return LANG_CODE_EN;
  }
}

Language parseLanguage(const String &code) {
  String c = code;
  c.toLowerCase();
  c.trim();
  // Check 2-letter primary subtag
  if (c.startsWith("zh")) return LANG_ZH;
  if (c.startsWith("ja") || c == "jp") return LANG_JA;
  return LANG_EN;
}

Language detectLanguage(const String &acceptLanguage) {
  if (acceptLanguage.length() == 0) return DEFAULT_LANGUAGE;

  String header = acceptLanguage;
  header.toLowerCase();

  // Parse Accept-Language quality values
  // Format: "en-US,en;q=0.9,zh-CN;q=0.8,ja;q=0.6"
  float bestQ = 0.0;
  Language bestLang = LANG_EN;

  int start = 0;
  while (start < (int)header.length()) {
    // Find end of this language-range
    int comma = header.indexOf(',', start);
    String entry = (comma >= 0) ? header.substring(start, comma) : header.substring(start);
    entry.trim();

    // Extract language tag and quality value
    float q = 1.0;
    int semi = entry.indexOf(';');
    String langTag;
    if (semi >= 0) {
      langTag = entry.substring(0, semi);
      langTag.trim();
      String qStr = entry.substring(semi + 1);
      qStr.trim();
      if (qStr.startsWith("q=")) {
        q = qStr.substring(2).toFloat();
        if (q < 0) q = 0;
        if (q > 1) q = 1;
      }
    } else {
      langTag = entry;
    }

    // Check if this is a supported language
    Language detected = parseLanguage(langTag);
    if (q > bestQ || (q == bestQ && detected == LANG_EN)) {
      bestQ = q;
      bestLang = detected;
    }

    if (comma < 0) break;
    start = comma + 1;
  }

  return bestLang;
}

String getLocaleFilePath(Language lang) {
  return "/locales/" + String(getLanguageCode(lang)) + ".json";
}

String loadLocale(Language lang) {
  String path = getLocaleFilePath(lang);
  File f = SPIFFS.open(path, "r");
  if (!f) {
    // Fallback to English
    if (lang != LANG_EN) {
      f = SPIFFS.open("/locales/en.json", "r");
    }
    if (!f) return "{}";
  }
  String content = f.readString();
  f.close();
  return content;
}
