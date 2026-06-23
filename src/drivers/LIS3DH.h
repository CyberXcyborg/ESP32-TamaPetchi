// ============================================================
// LIS3DH.h — LIS3DH Accelerometer Driver (I2C)
// Phase 21.4: Accelerometer
// ============================================================
// I2C address: 0x18 or 0x19 (configurable)
// ±2g/±4g/±8g/±16g range, 100Hz ODR
// Shake detection, tilt detection, face-up/down detection
// ============================================================

#ifndef LIS3DH_H
#define LIS3DH_H

#include <Arduino.h>
#include "config_v2.h"

// LIS3DH I2C Addresses
#define LIS3DH_ADDR_LOW     0x18    // SDO/SA0 pin low
#define LIS3DH_ADDR_HIGH    0x19    // SDO/SA0 pin high

// LIS3DH Registers
#define LIS3DH_REG_STATUS_REG_AUX  0x07
#define LIS3DH_REG_OUT_ADC1_L      0x08
#define LIS3DH_REG_WHO_AM_I        0x0F
#define LIS3DH_REG_CTRL_REG0       0x1E
#define LIS3DH_REG_TEMP_CFG_REG    0x1F
#define LIS3DH_REG_CTRL_REG1       0x20
#define LIS3DH_REG_CTRL_REG2       0x1E  // Note: some docs use 0x21
#define LIS3DH_REG_CTRL_REG3       0x22
#define LIS3DH_REG_CTRL_REG4       0x23
#define LIS3DH_REG_CTRL_REG5       0x24
#define LIS3DH_REG_CTRL_REG6       0x25
#define LIS3DH_REG_REFERENCE       0x26
#define LIS3DH_REG_STATUS_REG      0x27
#define LIS3DH_REG_OUT_X_L         0x28
#define LIS3DH_REG_OUT_X_H         0x29
#define LIS3DH_REG_OUT_Y_L         0x2A
#define LIS3DH_REG_OUT_Y_H         0x2B
#define LIS3DH_REG_OUT_Z_L         0x2C
#define LIS3DH_REG_OUT_Z_H         0x2D
#define LIS3DH_REG_FIFO_CTRL_REG   0x2E
#define LIS3DH_REG_FIFO_SRC_REG    0x2F
#define LIS3DH_REG_INT1_CFG        0x30
#define LIS3DH_REG_INT1_SRC        0x31
#define LIS3DH_REG_INT1_THS        0x32
#define LIS3DH_REG_INT1_DURATION   0x33
#define LIS3DH_REG_CLICK_CFG       0x38
#define LIS3DH_REG_CLICK_SRC       0x39
#define LIS3DH_REG_CLICK_THS       0x3A
#define LIS3DH_REG_TIME_LIMIT      0x3B
#define LIS3DH_REG_TIME_LATENCY    0x3C
#define LIS3DH_REG_TIME_WINDOW     0x3D

// Who Am I expected value
#define LIS3DH_WHO_AM_I_VALUE   0x33

// Range settings (for CTRL_REG4)
enum LIS3DHRange {
    LIS3DH_RANGE_2G = 0,    // ±2g, 16384 LSB/g
    LIS3DH_RANGE_4G = 1,    // ±4g, 8192 LSB/g
    LIS3DH_RANGE_8G = 2,    // ±8g, 4096 LSB/g
    LIS3DH_RANGE_16G = 3    // ±16g, 2048 LSB/g
};

// ODR settings (for CTRL_REG1)
enum LIS3DHODR {
    LIS3DH_ODR_POWER_DOWN = 0,
    LIS3DH_ODR_1HZ = 1,
    LIS3DH_ODR_10HZ = 2,
    LIS3DH_ODR_25HZ = 3,
    LIS3DH_ODR_50HZ = 4,
    LIS3DH_ODR_100HZ = 5,
    LIS3DH_ODR_200HZ = 6,
    LIS3DH_ODR_400HZ = 7
};

// Shake sensitivity
enum ShakeSensitivity {
    SHAKE_SENSITIVITY_LOW = 0,     // Office use
    SHAKE_SENSITIVITY_MEDIUM = 1,  // Handheld
    SHAKE_SENSITIVITY_HIGH = 2     // Active use
};

// Tilt direction
enum TiltDirection {
    TILT_NONE = 0,
    TILT_LEFT = 1,
    TILT_RIGHT = 2,
    TILT_UP = 3,
    TILT_DOWN = 4,
    TILT_FACE_UP = 5,
    TILT_FACE_DOWN = 6
};

// Shake intensity
enum ShakeIntensity {
    SHAKE_GENTLE = 0,
    SHAKE_VIGOROUS = 1,
    SHAKE_AGGRESSIVE = 2
};

// Callback types
typedef void (*ShakeCallback)(uint8_t intensity);
typedef void (*TiltCallback)(uint8_t direction, float angle);

// LSB/g for each range
static const float LIS3DH_LSB_PER_G[] = {
    16384.0f,  // ±2g
    8192.0f,   // ±4g
    4096.0f,   // ±8g
    2048.0f    // ±16g
};

class LIS3DH {
public:
    static LIS3DH& getInstance();

    // Initialize with I2C address and default ±2g range, 100Hz ODR
    bool begin(uint8_t address = LIS3DH_ADDR_LOW);
    
    // Shutdown
    end();

    // Read raw XYZ values (raw ADC counts)
    bool read(int16_t& x, int16_t& y, int16_t& z);

    // Read acceleration in g
    bool readG(float& gx, float& gy, float& gz);

    // Set range
    void setRange(LIS3DHRange range);

    // Set output data rate
    void setODR(LIS3DHODR odr);

    // Get current range
    LIS3DHRange getRange() const;

    // Shake detection
    void setShakeSensitivity(ShakeSensitivity sens);
    void enableShakeDetection(bool enable);
    bool isShakeEnabled() const;

    // Tilt detection
    void setTiltThreshold(float degrees);  // default 30°
    float getTiltThreshold() const;
    void enableTiltDetection(bool enable);
    bool isTiltEnabled() const;

    // Register callbacks
    void onShake(ShakeCallback cb);
    void onTilt(TiltCallback cb);

    // Process accelerometer data (call at ~10Hz from main loop)
    void update();

    // Get last detected tilt direction
    TiltDirection getLastTilt() const;

    // Get last shake intensity
    ShakeIntensity getLastShakeIntensity() const;

    // Check if device is face-down
    bool isFaceDown() const;

    // Check if initialized
    bool isInitialized() const;

private:
    LIS3DH();
    ~LIS3DH();
    LIS3DH(const LIS3DH&) = delete;
    LIS3DH& operator=(const LIS3DH&) = delete;

    // I2C register read/write
    bool writeRegister(uint8_t reg, uint8_t value);
    bool readRegister(uint8_t reg, uint8_t& value);
    bool readMultiple(uint8_t reg, uint8_t* data, size_t len);

    // Internal state
    uint8_t _address;
    bool _initialized;
    LIS3DHRange _range;
    LIS3DHODR _odr;

    // Shake detection state
    bool _shakeEnabled;
    ShakeSensitivity _shakeSensitivity;
    uint32_t _lastShakeTime;
    ShakeIntensity _lastShakeIntensity;

    // Tilt detection state
    bool _tiltEnabled;
    float _tiltThreshold;
    TiltDirection _lastTilt;
    float _lastTiltAngle;

    // Callbacks
    ShakeCallback _shakeCB;
    TiltCallback _tiltCB;

    // Timing
    uint32_t _lastUpdate;
};

#endif // LIS3DH_H
