#include "Notifications.h"
#include "config.h"
#include <SPIFFS.h>
#include <ArduinoJson.h>

// ============================================================
// Module-level state - per slot
// ============================================================
#define MAX_SLOTS 3

static Notification g_notifications[MAX_SLOTS][MAX_NOTIFICATIONS];
static int g_notificationCount[MAX_SLOTS] = {0, 0, 0};

static String getNotifFile(int petSlot) {
  return "/notifications_" + String(petSlot) + ".json";
}

// ============================================================
// Persistence
// ============================================================
void saveNotifications(int petSlot) {
  if (petSlot < 0 || petSlot >= MAX_SLOTS) return;

  File file = SPIFFS.open(getNotifFile(petSlot), "w");
  if (!file) return;

  DynamicJsonDocument doc(2048);
  JsonArray arr = doc.createNestedArray("notifications");
  for (int i = 0; i < g_notificationCount[petSlot] && i < MAX_NOTIFICATIONS; i++) {
    JsonObject n = arr.createNestedObject();
    n["type"]      = (int)g_notifications[petSlot][i].type;
    n["message"]   = g_notifications[petSlot][i].message;
    n["timestamp"] = g_notifications[petSlot][i].timestamp;
    n["read"]      = g_notifications[petSlot][i].read;
  }

  serializeJson(doc, file);
  file.close();
}

void loadNotifications(int petSlot) {
  if (petSlot < 0 || petSlot >= MAX_SLOTS) return;

  g_notificationCount[petSlot] = 0;

  if (!SPIFFS.exists(getNotifFile(petSlot))) return;

  File file = SPIFFS.open(getNotifFile(petSlot), "r");
  if (!file) return;

  DynamicJsonDocument doc(2048);
  DeserializationError err = deserializeJson(doc, file);
  file.close();
  if (err) return;

  JsonArray arr = doc["notifications"];
  for (int i = 0; i < (int)arr.size() && i < MAX_NOTIFICATIONS; i++) {
    g_notifications[petSlot][i].type      = (NotificationType)(int)arr[i]["type"];
    g_notifications[petSlot][i].message   = arr[i]["message"].as<String>();
    g_notifications[petSlot][i].timestamp = arr[i]["timestamp"];
    g_notifications[petSlot][i].read      = arr[i]["read"] | false;
    g_notificationCount[petSlot]++;
  }
}

// ============================================================
// Notification Management
// ============================================================
void addNotification(int petSlot, NotificationType type, const String &message) {
  if (petSlot < 0 || petSlot >= MAX_SLOTS) return;

  if (g_notificationCount[petSlot] >= MAX_NOTIFICATIONS) {
    for (int i = 0; i < MAX_NOTIFICATIONS - 1; i++) {
      g_notifications[petSlot][i] = g_notifications[petSlot][i + 1];
    }
    g_notificationCount[petSlot] = MAX_NOTIFICATIONS - 1;
  }

  Notification &n = g_notifications[petSlot][g_notificationCount[petSlot]];
  n.type      = type;
  n.message   = message;
  n.timestamp = millis();
  n.read      = false;
  g_notificationCount[petSlot]++;

  saveNotifications(petSlot);
  playNotificationBuzzer(type);
}

String getNotificationsJson(int petSlot) {
  if (petSlot < 0 || petSlot >= MAX_SLOTS) return "{\"notifications\":[],\"unread\":0}";

  DynamicJsonDocument doc(2048);
  JsonArray arr = doc.createNestedArray("notifications");
  for (int i = 0; i < g_notificationCount[petSlot]; i++) {
    JsonObject n = arr.createNestedObject();
    n["type"]      = (int)g_notifications[petSlot][i].type;
    n["message"]   = g_notifications[petSlot][i].message;
    n["timestamp"] = g_notifications[petSlot][i].timestamp;
    n["read"]      = g_notifications[petSlot][i].read;
  }
  doc["unread"] = getUnreadCount(petSlot);

  String result;
  serializeJson(doc, result);
  return result;
}

void clearNotifications(int petSlot) {
  if (petSlot < 0 || petSlot >= MAX_SLOTS) return;
  g_notificationCount[petSlot] = 0;
  saveNotifications(petSlot);
}

int getUnreadCount(int petSlot) {
  if (petSlot < 0 || petSlot >= MAX_SLOTS) return 0;
  int count = 0;
  for (int i = 0; i < g_notificationCount[petSlot]; i++) {
    if (!g_notifications[petSlot][i].read) count++;
  }
  return count;
}

// ============================================================
// Buzzer Patterns
// ============================================================
void buzzerLowHealth() {
  for (int i = 0; i < 3; i++) {
    tone(BUZZER_PIN, 800, 150);
    delay(200);
  }
  noTone(BUZZER_PIN);
}

void buzzerEvolutionReady() {
  for (int freq = 400; freq <= 1200; freq += 200) {
    tone(BUZZER_PIN, freq, 100);
    delay(120);
  }
  noTone(BUZZER_PIN);
}

void buzzerAchievement() {
  tone(BUZZER_PIN, 1000, 200);
  delay(250);
  tone(BUZZER_PIN, 1500, 300);
  delay(350);
  noTone(BUZZER_PIN);
}

void buzzerDeath() {
  for (int freq = 1000; freq >= 200; freq -= 100) {
    tone(BUZZER_PIN, freq, 80);
    delay(100);
  }
  noTone(BUZZER_PIN);
}

void buzzerWake() {
  for (int freq = 300; freq <= 800; freq += 100) {
    tone(BUZZER_PIN, freq, 80);
    delay(100);
  }
  noTone(BUZZER_PIN);
}

void buzzerLowBattery() {
  for (int i = 0; i < 5; i++) {
    tone(BUZZER_PIN, 500, 100);
    delay(500);
  }
  noTone(BUZZER_PIN);
}

void playNotificationBuzzer(NotificationType type) {
  switch (type) {
    case NOTIF_LOW_HEALTH:      buzzerLowHealth();      break;
    case NOTIF_EVOLUTION_READY: buzzerEvolutionReady();  break;
    case NOTIF_ACHIEVEMENT:     buzzerAchievement();     break;
    case NOTIF_DEATH:           buzzerDeath();           break;
    case NOTIF_WAKE:            buzzerWake();            break;
    case NOTIF_LOW_BATTERY:     buzzerLowBattery();      break;
    default: break;
  }
}
