#include "VoiceControl.h"
#include "Pet.h"
#include "config.h"

// ============================================================
// Phase 17.2: Voice Command Parser
// Case-insensitive keyword matching for voice commands.
// ============================================================

VoiceCommand parseVoiceCommand(const char* input) {
    if (!input || strlen(input) == 0) return VC_NONE;

    // Convert to lowercase for matching
    String lower = String(input);
    lower.toLowerCase();

    // Feed commands
    if (lower.indexOf("feed") >= 0 || lower.indexOf("eat") >= 0 ||
        lower.indexOf("food") >= 0 || lower.indexOf("hungry") >= 0 ||
        lower.indexOf("snack") >= 0) {
        return VC_FEED;
    }

    // Play commands
    if (lower.indexOf("play") >= 0 || lower.indexOf("fun") >= 0 ||
        lower.indexOf("game") >= 0 || lower.indexOf("toy") >= 0) {
        return VC_PLAY;
    }

    // Clean commands
    if (lower.indexOf("clean") >= 0 || lower.indexOf("wash") >= 0 ||
        lower.indexOf("bath") >= 0 || lower.indexOf("shower") >= 0) {
        return VC_CLEAN;
    }

    // Sleep commands
    if (lower.indexOf("sleep") >= 0 || lower.indexOf("rest") >= 0 ||
        lower.indexOf("nap") >= 0 || lower.indexOf("tired") >= 0 ||
        lower.indexOf("bed") >= 0) {
        return VC_SLEEP;
    }

    // Wake commands
    if (lower.indexOf("wake") >= 0 || lower.indexOf("awake") >= 0 ||
        lower.indexOf("rise") >= 0 || lower.indexOf("morning") >= 0) {
        return VC_WAKE;
    }

    // Status commands
    if (lower.indexOf("status") >= 0 || lower.indexOf("how") >= 0 ||
        lower.indexOf("doing") >= 0 || lower.indexOf("feeling") >= 0 ||
        lower.indexOf("check") >= 0) {
        return VC_STATUS;
    }

    // Heal commands
    if (lower.indexOf("heal") >= 0 || lower.indexOf("medicine") >= 0 ||
        lower.indexOf("sick") >= 0 || lower.indexOf("cure") >= 0 ||
        lower.indexOf("doctor") >= 0) {
        return VC_HEAL;
    }

    // Reset commands
    if (lower.indexOf("reset") >= 0 || lower.indexOf("restart") >= 0 ||
        lower.indexOf("revive") >= 0 || lower.indexOf("new pet") >= 0) {
        return VC_RESET;
    }

    return VC_UNKNOWN;
}

const char* voiceCommandToString(VoiceCommand cmd) {
    switch (cmd) {
        case VC_FEED:    return "feed";
        case VC_PLAY:    return "play";
        case VC_CLEAN:   return "clean";
        case VC_SLEEP:   return "sleep";
        case VC_WAKE:    return "wake";
        case VC_STATUS:  return "status";
        case VC_HEAL:    return "heal";
        case VC_RESET:   return "reset";
        case VC_UNKNOWN: return "unknown";
        default:         return "none";
    }
}

// ============================================================
// Alexa Smart Home Directive Parser
// Maps Alexa.PowerController and custom directives
// ============================================================

VoiceCommand parseAlexaDirective(const char* directive) {
    if (!directive) return VC_NONE;

    String dir = String(directive);
    dir.toLowerCase();

    // Alexa built-in intents
    if (dir.indexOf("feed") >= 0)  return VC_FEED;
    if (dir.indexOf("play") >= 0)  return VC_PLAY;
    if (dir.indexOf("clean") >= 0) return VC_CLEAN;
    if (dir.indexOf("sleep") >= 0) return VC_SLEEP;
    if (dir.indexOf("wake") >= 0)  return VC_WAKE;
    if (dir.indexOf("status") >= 0) return VC_STATUS;
    if (dir.indexOf("heal") >= 0)  return VC_HEAL;
    if (dir.indexOf("reset") >= 0) return VC_RESET;

    return VC_UNKNOWN;
}

// ============================================================
// Google Home Trait Parser
// Maps Google Home traits to commands
// ============================================================

VoiceCommand parseGoogleTrait(const char* trait, const char* value) {
    if (!trait) return VC_NONE;

    String t = String(trait);
    t.toLowerCase();

    if (value) {
        String v = String(value);
        v.toLowerCase();

        if (t.indexOf("action.devices.traits.OnOff") >= 0) {
            if (v.indexOf("on") >= 0) return VC_WAKE;
            if (v.indexOf("off") >= 0) return VC_SLEEP;
        }

        if (t.indexOf("action.devices.commands") >= 0) {
            if (v.indexOf("feed") >= 0)  return VC_FEED;
            if (v.indexOf("play") >= 0)  return VC_PLAY;
            if (v.indexOf("clean") >= 0) return VC_CLEAN;
            if (v.indexOf("heal") >= 0)  return VC_HEAL;
        }
    }

    return VC_UNKNOWN;
}

// ============================================================
// Voice Status Formatter
// Returns a JSON object optimized for voice assistant responses
// ============================================================

String formatVoiceStatus(Pet& pet) {
    String result = "{";

    // Pet name and alive status
    result += "\"name\":\"" + String(pet.name) + "\",";
    result += "\"alive\":" + String(pet.isAlive ? "true" : "false") + ",";

    if (pet.isAlive) {
        // Current state
        result += "\"state\":\"" + String(pet.state) + "\",";

        // Health summary (for voice: good/fair/poor/critical)
        const char* healthLevel;
        if (pet.health >= 70) healthLevel = "good";
        else if (pet.health >= 40) healthLevel = "fair";
        else if (pet.health >= 20) healthLevel = "poor";
        else healthLevel = "critical";
        result += "\"health\":\"" + String(healthLevel) + "\",";

        // Hunger summary
        const char* hungerLevel;
        if (pet.hunger >= 70) hungerLevel = "full";
        else if (pet.hunger >= 40) hungerLevel = "okay";
        else if (pet.hunger >= 20) hungerLevel = "hungry";
        else hungerLevel = "starving";
        result += "\"hunger\":\"" + String(hungerLevel) + "\",";

        // Happiness summary
        const char* happyLevel;
        if (pet.happiness >= 70) happyLevel = "happy";
        else if (pet.happiness >= 40) happyLevel = "okay";
        else if (pet.happiness >= 20) happyLevel = "sad";
        else happyLevel = "unhappy";
        result += "\"happiness\":\"" + String(happyLevel) + "\",";

        // Energy summary
        const char* energyLevel;
        if (pet.energy >= 70) energyLevel = "energetic";
        else if (pet.energy >= 40) energyLevel = "okay";
        else if (pet.energy >= 20) energyLevel = "tired";
        else energyLevel = "exhausted";
        result += "\"energy\":\"" + String(energyLevel) + "\",";

        // Needs attention?
        bool needsAttention = (pet.hunger < 30 || pet.happiness < 30 ||
                               pet.energy < 30 || pet.cleanliness < 30 ||
                               pet.health < 40);
        result += "\"needsAttention\":" + String(needsAttention ? "true" : "false") + ",";

        // Speech-friendly summary
        String summary = "Your pet " + String(pet.name) + " is ";
        if (!pet.isAlive) {
            summary += "not alive. You can revive them.";
        } else {
            summary += String(pet.state) + ". ";
            summary += "Health is " + String(healthLevel) + ". ";
            if (pet.hunger < 30) summary += "They are hungry. ";
            if (pet.happiness < 30) summary += "They need playtime. ";
            if (pet.energy < 30) summary += "They are tired. ";
            if (pet.cleanliness < 30) summary += "They need cleaning. ";
            if (!needsAttention) summary += "All stats look good!";
        }
        result += "\"summary\":\"" + summary + "\"";
    } else {
        result += "\"summary\":\"Your pet is not alive. Say reset to revive them.\"";
    }

    result += "}";
    return result;
}

// ============================================================
// Voice Command Executor
// ============================================================

bool executeVoiceCommand(VoiceCommand cmd, Pet& pet) {
    switch (cmd) {
        case VC_FEED:
            if (pet.isAlive) {
                feedPet(pet);
                return true;
            }
            return false;

        case VC_PLAY:
            if (pet.isAlive && pet.energy >= PLAY_ENERGY_MIN) {
                pet.state = "playing";
                pet.happiness = min(STAT_MAX, pet.happiness + PLAY_HAPPINESS_GAIN);
                pet.energy = max(STAT_MIN, pet.energy - PLAY_ENERGY_COST);
                pet.hunger = max(STAT_MIN, pet.hunger - PLAY_HUNGER_COST);
                return true;
            }
            return false;

        case VC_CLEAN:
            if (pet.isAlive) {
                cleanPet(pet);
                return true;
            }
            return false;

        case VC_SLEEP:
            if (pet.isAlive && pet.state != "sleeping") {
                pet.state = "sleeping";
                return true;
            }
            return false;

        case VC_WAKE:
            if (pet.isAlive && pet.state == "sleeping") {
                pet.state = "normal";
                return true;
            }
            return false;

        case VC_HEAL:
            if (pet.isAlive && pet.health < STAT_MAX) {
                pet.health = min(STAT_MAX, pet.health + 20);
                return true;
            }
            return false;

        case VC_RESET:
            if (!pet.isAlive) {
                pet.isAlive = true;
                pet.state = "normal";
                pet.health = 50;
                pet.hunger = 50;
                pet.happiness = 50;
                pet.energy = 50;
                pet.cleanliness = 50;
                pet.isDying = false;
                return true;
            }
            return false;

        case VC_STATUS:
            // Status is read-only, always succeeds
            return true;

        default:
            return false;
    }
}
