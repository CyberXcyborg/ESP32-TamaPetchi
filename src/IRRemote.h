#ifndef IRREMOTE_H
#define IRREMOTE_H

// ============================================================
// Phase 7.5: IR Remote Control (NEC Protocol)
// ============================================================
// Supports standard NEC IR remote (32-bit protocol)
// Maps remote buttons to pet actions:
//   CH-  : Feed
//   CH   : Play
//   CH+  : Clean
//   |<<  : Sleep
//   >>|  : Wake / Toggle sound
//   >||  : Toggle mute
//   0    : Reset pet
//   1-9  : Select pet slot (multi-pet)
// ============================================================

#include <Arduino.h>

// IR receiver pin (default GPIO 15 — change in config.h)
#ifndef IR_RECEIVER_PIN
#define IR_RECEIVER_PIN 15
#endif

// NEC protocol constants
#define NEC_HEADER_MARK   9000
#define NEC_HEADER_SPACE  4500
#define NEC_BIT_MARK       560
#define NEC_ONE_SPACE     1690
#define NEC_ZERO_SPACE     560
#define NEC_TOLERANCE      200

// IR button codes (common NEC remote — adjust for your remote)
enum IRButton {
    IR_BTN_CH_MINUS  = 0x45,  // Channel minus
    IR_BTN_CH       = 0x46,  // Channel
    IR_BTN_CH_PLUS  = 0x47,  // Channel plus
    IR_BTN_PREV     = 0x44,  // Previous (rewind)
    IR_BTN_NEXT     = 0x40,  // Next (fast-forward)
    IR_BTN_PLAY     = 0x43,  // Play/Pause
    IR_BTN_VOL_MINUS = 0x15, // Volume minus
    IR_BTN_VOL_PLUS  = 0x07, // Volume plus
    IR_BTN_0        = 0x16,  // Number 0
    IR_BTN_1        = 0x0C,  // Number 1
    IR_BTN_2        = 0x18,  // Number 2
    IR_BTN_3        = 0x5E,  // Number 3
    IR_BTN_4        = 0x08,  // Number 4
    IR_BTN_5        = 0x1C,  // Number 5
    IR_BTN_6        = 0x5A,  // Number 6
    IR_BTN_7        = 0x42,  // Number 7
    IR_BTN_8        = 0x52,  // Number 8
    IR_BTN_9        = 0x4A,  // Number 9
    IR_BTN_NONE     = 0xFF   // No button / unknown
};

// IR event callback type
typedef void (*IRCommandCallback)(IRButton button, uint32_t rawCode);

// Initialize IR receiver
void setupIRRemote();

// Check for IR input — call this in loop()
void checkIRRemote();

// Set callback for IR commands
void setIRCommandCallback(IRCommandCallback cb);

// Get the last received button (for polling)
IRButton getLastIRButton();

// Get the last raw code
uint32_t getLastIRRawCode();

// Enable/disable IR remote
void enableIRRemote(bool enabled);

// Check if IR remote is enabled
bool isIRRemoteEnabled();

#endif // IRREMOTE_H
