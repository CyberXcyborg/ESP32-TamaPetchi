#ifndef NOTIFICATIONS_H
#define NOTIFICATIONS_H

#include <Arduino.h>
#include "Pet.h"

// ============================================================
// Notification System
// Buzzer patterns + web UI notification badges
// ============================================================

enum NotificationType {
  NOTIF_LOW_HEALTH,
  NOTIF_EVOLUTION_READY,
  NOTIF_ACHIEVEMENT,
  NOTIF_DEATH,
  NOTIF_WAKE,
  NOTIF_SICK,
  NOTIF_HUNGRY,
  NOTIF_LOW_BATTERY
};

struct Notification {
  NotificationType type;
  String message;
  unsigned long timestamp;
  bool read;
};

// Add a notification for a specific pet slot
void addNotification(int petSlot, NotificationType type, const String &message);

// Get all notifications as JSON for a pet slot
String getNotificationsJson(int petSlot);

// Clear all notifications for a pet slot
void clearNotifications(int petSlot);

// Get unread count for a pet slot
int getUnreadCount(int petSlot);

// Load/save notifications for a pet slot
void loadNotifications(int petSlot);
void saveNotifications(int petSlot);

// Buzzer patterns
void buzzerLowHealth();
void buzzerEvolutionReady();
void buzzerAchievement();
void buzzerDeath();
void buzzerWake();
void buzzerLowBattery();

// Play buzzer pattern for notification type
void playNotificationBuzzer(NotificationType type);

#endif // NOTIFICATIONS_H
