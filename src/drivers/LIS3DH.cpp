// ============================================================
// LIS3DH.cpp — LIS3DH Accelerometer Driver (I2C)
// Phase 21.4: Accelerometer
// ============================================================

#include "drivers/LIS3DH.h"
#include <Wire.h>
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// ============================================================
// Shake detection thresholds (in g deviation from 1g)
// ============================================================
static const float SHAKE_THRESHOLD_LOW = 0.3f;
static const float SHAKE_THRESHOLD_MEDIUM = 0.5f;
static const float SHAKE_THRESHOLD_HIGH = 0.8f;

static const float SHAKE_AGGRESSIVE_THRESHOLD = 1.5f;
static const uint32_t SHAKE_DEBOUNCE_MS = 500;

// ============================================================
// Singleton
// ============================================================
LIS3DH& LIS3DH::getInstance() {
    static LIS3DH instance;
    return instance;
}

LIS3DH::LIS3DH()
    : _address(LIS3DH_ADDR_LOW), _initialized(false),
      _range(LIS3DH_RANGE_2G), _odr(LIS3DH_ODR_100HZ),
      _shakeEnabled(false), _shakeSensitivity(SHAKE_SENSITIVITY_MEDIUM),
      _lastShakeTime(0), _lastShakeIntensity(SHAKE_GENTLE),
      _tiltEnabled(false), _tiltThreshold(30.0f),
      _lastTilt(TILT_NONE), _lastTiltAngle(0.0f),
      _shakeCB(nullptr), _tiltCB(nullptr), _lastUpdate(0) {
}

LIS3DH::~LIS3DH() {
    end();
}

// ============================================================
// Public API
// ============================================================
bool LIS3DH::begin(uint8_t address) {
    _address = address;

    // Verify WHO_AM_I
    uint8_t whoAmI = 0;
    if (!readRegister(LIS3DH_REG_WHO_AM_I, whoAmI)) {
        DEBUG_PRINTLN("[LIS3DH] WHO_AM_I read failed");
        return false;
    }

    if (whoAmI != LIS3DH_WHO_AM_I_VALUE) {
        DEBUG_PRINTF("[LIS3DH] Unexpected WHO_AM_I: 0x%02X (expected 0x%02X)\n",
            whoAmI, LIS3DH_WHO_AM_I_VALUE);
        return false;
    }

    // CTRL_REG1: Enable XYZ axes, set ODR to 100Hz
    uint8_t ctrl1 = 0x57; // 100Hz, XYZ enabled
    if (!writeRegister(LIS3DH_REG_CTRL_REG1, ctrl1)) return false;

    // CTRL_REG4: ±2g range, high-resolution mode
    uint8_t ctrl4 = 0x08; // ±2g, high-res
    if (!writeRegister(LIS3DH_REG_CTRL_REG4, ctrl4)) return false;

    _initialized = true;
    DEBUG_PRINTLN("[LIS3DH] Initialized");
    return true;
}

void LIS3DH::end() {
    if (!_initialized) return;
    // Power down
    writeRegister(LIS3DH_REG_CTRL_REG1, 0x00);
    _initialized = false;
    DEBUG_PRINTLN("[LIS3DH] Shutdown");
}

bool LIS3DH::read(int16_t& x, int16_t& y, int16_t& z) {
    if (!_initialized) return false;

    uint8_t data[6];
    if (!readMultiple(LIS3DH_REG_OUT_X_L | 0x80, data, 6)) return false;

    // LIS3DH outputs are 16-bit left-justified in 10-bit or 16-bit mode
    // In high-res mode (12-bit), we right-shift by 4
    x = (int16_t)(data[0] | (data[1] << 8)) >> 4;
    y = (int16_t)(data[2] | (data[3] << 8)) >> 4;
    z = (int16_t)(data[4] | (data[5] << 8)) >> 4;

    return true;
}

bool LIS3DH::readG(float& gx, float& gy, float& gz) {
    int16_t x, y, z;
    if (!read(x, y, z)) return false;

    float lsbPerG = LIS3DH_LSB_PER_G[_range];
    // In high-res mode (12-bit), sensitivity is 1mg/LSB at ±2g
    // Already right-shifted by 4 in read(), so use 12-bit sensitivity
    gx = (float)x / 1000.0f; // 1mg/LSB at ±2g, so x/1000 = g
    gy = (float)y / 1000.0f;
    gz = (float)z / 1000.0f;

    return true;
}

void LIS3DH::setRange(LIS3DHRange range) {
    _range = range;
    if (!_initialized) return;

    uint8_t ctrl4 = 0;
    readRegister(LIS3DH_REG_CTRL_REG4, ctrl4);
    ctrl4 &= ~0x30; // Clear FS bits
    ctrl4 |= (range << 4); // Set new range
    writeRegister(LIS3DH_REG_CTRL_REG4, ctrl4);
}

void LIS3DH::setODR(LIS3DHODR odr) {
    _odr = odr;
    if (!_initialized) return;

    uint8_t ctrl1 = 0;
    readRegister(LIS3DH_REG_CTRL_REG1, ctrl1);
    ctrl1 &= ~0xF0; // Clear ODR bits
    ctrl1 |= (odr << 4); // Set new ODR
    writeRegister(LIS3DH_REG_CTRL_REG1, ctrl1);
}

LIS3DHRange LIS3DH::getRange() const {
    return _range;
}

void LIS3DH::setShakeSensitivity(ShakeSensitivity sens) {
    _shakeSensitivity = sens;
}

void LIS3DH::enableShakeDetection(bool enable) {
    _shakeEnabled = enable;
    if (!enable) {
        _lastShakeIntensity = SHAKE_GENTLE;
    }
}

bool LIS3DH::isShakeEnabled() const {
    return _shakeEnabled;
}

void LIS3DH::setTiltThreshold(float degrees) {
    _tiltThreshold = degrees;
}

float LIS3DH::getTiltThreshold() const {
    return _tiltThreshold;
}

void LIS3DH::enableTiltDetection(bool enable) {
    _tiltEnabled = enable;
    if (!enable) {
        _lastTilt = TILT_NONE;
        _lastTiltAngle = 0.0f;
    }
}

bool LIS3DH::isTiltEnabled() const {
    return _tiltEnabled;
}

void LIS3DH::onShake(ShakeCallback cb) {
    _shakeCB = cb;
}

void LIS3DH::onTilt(TiltCallback cb) {
    _tiltCB = cb;
}

void LIS3DH::update() {
    if (!_initialized) return;

    float gx, gy, gz;
    if (!readG(gx, gy, gz)) return;

    uint32_t now = millis();

    // --- Shake Detection ---
    if (_shakeEnabled) {
        float magnitude = sqrtf(gx * gx + gy * gy + gz * gz);
        float deviation = fabsf(magnitude - 1.0f); // deviation from 1g

        float threshold = SHAKE_THRESHOLD_MEDIUM;
        switch (_shakeSensitivity) {
            case SHAKE_SENSITIVITY_LOW:    threshold = SHAKE_THRESHOLD_LOW; break;
            case SHAKE_SENSITIVITY_MEDIUM: threshold = SHAKE_THRESHOLD_MEDIUM; break;
            case SHAKE_SENSITIVITY_HIGH:   threshold = SHAKE_THRESHOLD_HIGH; break;
        }

        if (deviation > threshold && (now - _lastShakeTime) > SHAKE_DEBOUNCE_MS) {
            _lastShakeTime = now;

            // Determine intensity
            if (deviation > SHAKE_AGGRESSIVE_THRESHOLD) {
                _lastShakeIntensity = SHAKE_AGGRESSIVE;
            } else if (deviation > threshold * 1.5f) {
                _lastShakeIntensity = SHAKE_VIGOROUS;
            } else {
                _lastShakeIntensity = SHAKE_GENTLE;
            }

            if (_shakeCB) {
                _shakeCB((uint8_t)_lastShakeIntensity);
            }
        }
    }

    // --- Tilt Detection ---
    if (_tiltEnabled) {
        // Calculate pitch and roll
        float pitch = atan2f(-gx, sqrtf(gy * gy + gz * gz)) * 180.0f / M_PI;
        float roll = atan2f(gy, gz) * 180.0f / M_PI;

        TiltDirection dir = TILT_NONE;
        float angle = 0.0f;

        // Check face-down first (z pointing down)
        if (gz < -0.7f) {
            dir = TILT_FACE_DOWN;
            angle = 180.0f;
        } else if (gz > 0.7f && fabsf(gx) < 0.3f && fabsf(gy) < 0.3f) {
            dir = TILT_FACE_UP;
            angle = 0.0f;
        } else if (fabsf(roll) > _tiltThreshold || fabsf(pitch) > _tiltThreshold) {
            if (fabsf(pitch) > fabsf(roll)) {
                if (pitch > 0) {
                    dir = TILT_UP;
                    angle = pitch;
                } else {
                    dir = TILT_DOWN;
                    angle = -pitch;
                }
            } else {
                if (roll > 0) {
                    dir = TILT_RIGHT;
                    angle = roll;
                } else {
                    dir = TILT_LEFT;
                    angle = -roll;
                }
            }
        }

        if (dir != _lastTilt) {
            _lastTilt = dir;
            _lastTiltAngle = angle;
            if (_tiltCB && dir != TILT_NONE) {
                _tiltCB((uint8_t)dir, angle);
            }
        }
    }

    _lastUpdate = now;
}

TiltDirection LIS3DH::getLastTilt() const {
    return _lastTilt;
}

ShakeIntensity LIS3DH::getLastShakeIntensity() const {
    return _lastShakeIntensity;
}

bool LIS3DH::isFaceDown() const {
    return _lastTilt == TILT_FACE_DOWN;
}

bool LIS3DH::isInitialized() const {
    return _initialized;
}

// ============================================================
// Private Methods
// ============================================================
bool LIS3DH::writeRegister(uint8_t reg, uint8_t value) {
    Wire.beginTransmission(_address);
    Wire.write(reg);
    Wire.write(value);
    return Wire.endTransmission() == 0;
}

bool LIS3DH::readRegister(uint8_t reg, uint8_t& value) {
    Wire.beginTransmission(_address);
    Wire.write(reg);
    if (Wire.endTransmission(false) != 0) return false;

    Wire.requestFrom(_address, (uint8_t)1);
    if (!Wire.available()) return false;
    value = Wire.read();
    return true;
}

bool LIS3DH::readMultiple(uint8_t reg, uint8_t* data, size_t len) {
    Wire.beginTransmission(_address);
    Wire.write(reg | 0x80); // Auto-increment bit for multi-byte read
    if (Wire.endTransmission(false) != 0) return false;

    Wire.requestFrom(_address, (uint8_t)len);
    size_t i = 0;
    while (Wire.available() && i < len) {
        data[i++] = Wire.read();
    }
    return i == len;
}
