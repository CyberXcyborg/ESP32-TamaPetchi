#ifndef POWER_MANAGER_H
#define POWER_MANAGER_H

#include <Arduino.h>
#include "Pet.h"

// ============================================================
// Power Management
// Deep sleep, WiFi power reduction, battery monitoring
// ============================================================

void setupPowerManager();
void handlePowerManager(Pet &pet, unsigned long currentMillis);

// Battery
int  getBatteryPercent();   // 0-100, or -1 if not available
bool isLowBattery();

// Deep sleep
void enterDeepSleep();
bool shouldSleep(const Pet &pet);
void enableSleepWakeup();

// WiFi power
void reduceWiFiPower();
void restoreWiFiPower();

// Battery JSON for web API
String getBatteryJson(const Pet &pet);

#endif // POWER_MANAGER_H
