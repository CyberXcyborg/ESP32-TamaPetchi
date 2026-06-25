// ============================================================
// HAL_v2.cpp — HAL Implementation for ESP32-S3
// ============================================================

#include "HAL_v2.h"
#include <esp_system.h>
#include <esp_sleep.h>
#include <SPI.h>
#include <Wire.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

bool HAL_V2::_initialized = false;
SemaphoreHandle_t HAL_V2::_spiMutex = NULL;

bool HAL_V2::begin() {
    if (_initialized) return true;
    
    // Initialize SPI
    SPI.begin(TFT_PIN_SCLK, -1, TFT_PIN_MOSI, -1);
    
    // Create SPI mutex for thread-safe display/touch bus access
    if (_spiMutex == NULL) {
        _spiMutex = xSemaphoreCreateMutex();
    }
    
    // Initialize I2C (for touch, accelerometer, NFC)
    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
    
    _initialized = true;
    return true;
}

void HAL_V2::pinMode(uint8_t pin, uint8_t mode) {
    ::pinMode(pin, mode);
}

void HAL_V2::digitalWrite(uint8_t pin, uint8_t val) {
    ::digitalWrite(pin, val);
}

int HAL_V2::digitalRead(uint8_t pin) {
    return ::digitalRead(pin);
}

uint16_t HAL_V2::analogRead(uint8_t pin) {
    return ::analogRead(pin);
}

void HAL_V2::spiBegin() {
    if (_spiMutex != NULL) {
        xSemaphoreTake(_spiMutex, portMAX_DELAY);
    }
    SPI.begin();
}

uint8_t HAL_V2::spiTransfer(uint8_t data) {
    uint8_t result = SPI.transfer(data);
    if (_spiMutex != NULL) {
        xSemaphoreGive(_spiMutex);
    }
    return result;
}

void HAL_V2::i2cBegin() {
    Wire.begin();
}

bool HAL_V2::i2cWrite(uint8_t addr, uint8_t *data, size_t len) {
    Wire.beginTransmission(addr);
    Wire.write(data, len);
    return Wire.endTransmission() == 0;
}

bool HAL_V2::i2cRead(uint8_t addr, uint8_t *data, size_t len) {
    Wire.requestFrom(addr, len);
    size_t i = 0;
    while (Wire.available() && i < len) {
        data[i++] = Wire.read();
    }
    return i == len;
}

uint32_t HAL_V2::millis() {
    return ::millis();
}

uint32_t HAL_V2::micros() {
    return ::micros();
}

void HAL_V2::delay(uint32_t ms) {
    ::delay(ms);
}

void HAL_V2::delayMicroseconds(uint32_t us) {
    ::delayMicroseconds(us);
}

uint32_t HAL_V2::getFreeHeap() {
    return ESP.getFreeHeap();
}

uint32_t HAL_V2::getCpuFreqMHz() {
    return ESP.getCpuFreqMHz();
}

void HAL_V2::restart() {
    ESP.restart();
}

void HAL_V2::deepSleep(uint64_t us) {
    esp_sleep_enable_ext0_wakeup((gpio_num_t)BUTTON_FEED_PIN, 0);
    esp_deep_sleep_start();
}

uint8_t HAL_V2::getBatteryPercent() {
    float voltage = getBatteryVoltage();
    if (voltage >= BATTERY_VOLTAGE_MAX) return 100;
    if (voltage <= BATTERY_VOLTAGE_MIN) return 0;
    return (uint8_t)((voltage - BATTERY_VOLTAGE_MIN) / (BATTERY_VOLTAGE_MAX - BATTERY_VOLTAGE_MIN) * 100);
}

float HAL_V2::getBatteryVoltage() {
    // ESP32-S3 ADC: 12-bit, 0-3.3V range
    // With voltage divider (typically 2:1 for LiPo)
    uint32_t raw = 0;
    for (int i = 0; i < 16; i++) {
        raw += analogRead(BATTERY_ADC_PIN);
    }
    raw /= 16;
    float voltage = (raw / 4095.0f) * 3.3f * 2.0f;  // 2x for voltage divider
    return voltage;
}

String HAL_V2::getResetReason() {
    esp_reset_reason_t reason = esp_reset_reason();
    switch (reason) {
        case ESP_RST_POWERON: return "Power-on";
        case ESP_RST_EXT: return "External reset";
        case ESP_RST_SW: return "Software reset";
        case ESP_RST_PANIC: return "Exception/panic";
        case ESP_RST_INT_WDT: return "Interrupt watchdog";
        case ESP_RST_TASK_WDT: return "Task watchdog";
        case ESP_RST_WDT: return "Other watchdog";
        case ESP_RST_DEEPSLEEP: return "Deep sleep wake";
        case ESP_RST_BROWNOUT: return "Brownout";
        case ESP_RST_SDIO: return "SDIO reset";
        default: return "Unknown";
    }
}
