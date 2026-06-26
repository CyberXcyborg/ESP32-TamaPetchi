#include "IRRemote.h"
#include "config.h"

// ============================================================
// NEC IR Protocol Decoder
// ============================================================
// Uses edge-triggered interrupt via PCINT or polling.
// For simplicity and portability, we use a polling approach
// with micros() timing. This is adequate for NEC protocol
// at ~38kHz carrier.
// ============================================================

static volatile bool irDataReady = false;
static volatile uint32_t irRawCode = 0;
static volatile uint32_t irLastTime = 0;

static IRButton lastButton = IR_BTN_NONE;
static uint32_t lastRawCode = 0;
static bool irEnabled = true;
static IRCommandCallback irCallback = nullptr;

// NEC decode state
static volatile uint8_t irBitIndex = 0;
static volatile uint32_t irRawData = 0;
static volatile bool irReceiving = false;

// Timing tracking
static unsigned long lastIRCheck = 0;
static const unsigned long IR_POLL_INTERVAL = 50; // Poll every 50ms

// Button-to-command mapping
static IRButton decodeNEC(uint32_t rawCode) {
    // Extract the command byte (bits 16-23, inverted in NEC)
    uint8_t cmd = (rawCode >> 16) & 0xFF;
    uint8_t cmdInv = (rawCode >> 24) & 0xFF;

    // Verify: command + ~command should equal 0xFF for valid NEC
    if ((cmd ^ cmdInv) != 0xFF) {
        return IR_BTN_NONE;
    }

    return (IRButton)cmd;
}

void setupIRRemote() {
    pinMode(IR_RECEIVER_PIN, INPUT_PULLUP);
    irEnabled = true;
    irDataReady = false;
    irReceiving = false;
    irBitIndex = 0;
    irRawData = 0;
    lastButton = IR_BTN_NONE;
    lastRawCode = 0;
    irCallback = nullptr;
    Serial.printf("[IR] Initialized on GPIO %d\n", IR_RECEIVER_PIN);
}

void checkIRRemote() {
    if (!irEnabled) return;

    unsigned long now = millis();
    if (now - lastIRCheck < IR_POLL_INTERVAL) return;
    lastIRCheck = now;

    // Read the IR receiver pin
    // Most IR receiver modules output LOW when receiving carrier
    static bool lastState = HIGH;
    static unsigned long lastEdgeTime = 0;
    static uint8_t bitCount = 0;
    static uint32_t rawData = 0;
    static bool receiving = false;

    bool currentState = digitalRead(IR_RECEIVER_PIN);

    if (lastState != currentState) {
        unsigned long edgeTime = micros();
        unsigned long duration = edgeTime - lastEdgeTime;
        lastEdgeTime = edgeTime;

        if (currentState == LOW) {
            // Falling edge — carrier detected (mark)
            if (duration > (NEC_HEADER_MARK - NEC_TOLERANCE) &&
                duration < (NEC_HEADER_MARK + NEC_TOLERANCE) && !receiving) {
                // Start of NEC frame
                receiving = true;
                bitCount = 0;
                rawData = 0;
            } else if (receiving && duration > (NEC_BIT_MARK - NEC_TOLERANCE) &&
                       duration < (NEC_BIT_MARK + NEC_TOLERANCE)) {
                // Bit mark received — next space determines 0 or 1
                // Just mark received, wait for space
            }
        } else {
            // Rising edge — carrier stopped (space)
            if (receiving) {
                if (duration > (NEC_HEADER_SPACE - NEC_TOLERANCE) &&
                    duration < (NEC_HEADER_SPACE + NEC_TOLERANCE) && bitCount == 0) {
                    // Header space — start receiving data bits
                    // Do nothing, wait for first bit mark
                } else if (duration > (NEC_ONE_SPACE - NEC_TOLERANCE) &&
                           duration < (NEC_ONE_SPACE + NEC_TOLERANCE)) {
                    // Logic '1'
                    rawData |= (1UL << bitCount);
                    bitCount++;
                } else if (duration > (NEC_ZERO_SPACE - NEC_TOLERANCE) &&
                           duration < (NEC_ZERO_SPACE + NEC_TOLERANCE)) {
                    // Logic '0' — bit already 0
                    bitCount++;
                } else {
                    // Invalid timing — reset
                    receiving = false;
                    bitCount = 0;
                    rawData = 0;
                }

                // NEC frame has 32 bits (addr + ~addr + cmd + ~cmd)
                if (bitCount >= 32) {
                    receiving = false;
                    bitCount = 0;

                    // Decode and validate
                    IRButton btn = decodeNEC(rawData);
                    if (btn != IR_BTN_NONE) {
                        lastButton = btn;
                        lastRawCode = rawData;
                        irDataReady = true;

                        Serial.printf("[IR] Button: 0x%02X (raw: 0x%08X)\n", btn, rawData);

                        if (irCallback) {
                            irCallback(btn, rawData);
                        }
                    }
                    rawData = 0;
                }
            }
        }

        lastState = currentState;
    }
}

void setIRCommandCallback(IRCommandCallback cb) {
    irCallback = cb;
}

IRButton getLastIRButton() {
    if (irDataReady) {
        irDataReady = false;
        return lastButton;
    }
    return IR_BTN_NONE;
}

uint32_t getLastIRRawCode() {
    return lastRawCode;
}

void enableIRRemote(bool enabled) {
    irEnabled = enabled;
    if (!enabled) {
        irDataReady = false;
        irReceiving = false;
    }
}

bool isIRRemoteEnabled() {
    return irEnabled;
}
