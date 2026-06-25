// ============================================================
// HAL_v2.h — Hardware Abstraction Layer for ESP32-S3 v2.0
// ============================================================

#ifndef HAL_V2_H
#define HAL_V2_H

#include <Arduino.h>
#include "config_v2.h"

class HAL_V2 {
public:
    static bool begin();
    
    // GPIO
    static void pinMode(uint8_t pin, uint8_t mode);
    static void digitalWrite(uint8_t pin, uint8_t val);
    static int digitalRead(uint8_t pin);
    static uint16_t analogRead(uint8_t pin);
    
    // SPI
    static void spiBegin();
    static uint8_t spiTransfer(uint8_t data);
    
    // I2C
    static void i2cBegin();
    static bool i2cWrite(uint8_t addr, uint8_t *data, size_t len);
    static bool i2cRead(uint8_t addr, uint8_t *data, size_t len);
    
    // Timing
    static uint32_t millis();
    static uint32_t micros();
    static void delay(uint32_t ms);
    static void delayMicroseconds(uint32_t us);
    
    // System
    static uint32_t getFreeHeap();
    static uint32_t getCpuFreqMHz();
    static void restart();
    static void deepSleep(uint64_t us);
    
    // Battery
    static uint8_t getBatteryPercent();
    static float getBatteryVoltage();
    
    // Reset reason
    static String getResetReason();
    
private:
    static bool _initialized;
    #ifdef ESP32
    static SemaphoreHandle_t _spiMutex;
    #endif
};

#endif // HAL_V2_H
