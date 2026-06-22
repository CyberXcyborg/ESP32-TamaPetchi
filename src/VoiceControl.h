#ifndef VOICECONTROL_H
#define VOICECONTROL_H

#include <Arduino.h>

// ============================================================
// Phase 17.2: Voice Control Integration
// Lightweight keyword-based voice command parser.
// Supports Alexa Smart Home and Google Home directives.
// ============================================================

// Voice command types
enum VoiceCommand {
    VC_NONE = 0,
    VC_FEED,
    VC_PLAY,
    VC_CLEAN,
    VC_SLEEP,
    VC_WAKE,
    VC_STATUS,
    VC_HEAL,
    VC_RESET,
    VC_UNKNOWN
};

// Parse a voice command string into a command type
VoiceCommand parseVoiceCommand(const char* input);

// Get human-readable command name
const char* voiceCommandToString(VoiceCommand cmd);

// Get command from Alexa directive name
VoiceCommand parseAlexaDirective(const char* directive);

// Get command from Google Home trait
VoiceCommand parseGoogleTrait(const char* trait, const char* value);

// Format pet status for voice assistants (Alexa/Google)
String formatVoiceStatus(class Pet& pet);

// Execute a voice command on the pet
// Returns true if command was executed successfully
bool executeVoiceCommand(VoiceCommand cmd, class Pet& pet);

#endif // VOICECONTROL_H
