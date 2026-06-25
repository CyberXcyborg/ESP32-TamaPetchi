// ============================================================
// Pet_v2.cpp — Minimal Pet Engine for v2.0 Foundation
// ============================================================

#include "Pet_v2.h"
#include <ArduinoJson.h>

PetEngine::PetEngine() {
    memset(&_data, 0, sizeof(PetData));
}

void PetEngine::init() {
    _data.name = "Tama";
    _data.hunger = 80;
    _data.happiness = 70;
    _data.energy = 90;
    _data.cleanliness = 85;
    _data.health = 100;
    _data.age_minutes = 0;
    _data.birth_timestamp = millis();
    _data.stage = PET_STAGE_BABY;
    _data.state = PET_STATE_IDLE;
    _data.generation = 1;
}

void PetEngine::update() {
    if (_data.state == PET_STATE_DEAD) return;
    
    decay_stats();
    check_health();
    evolve();
    _data.age_minutes++;
}

void PetEngine::decay_stats() {
    uint8_t mult = get_decay_mult();
    bool sleeping = (_data.state == PET_STATE_SLEEPING);
    
    // Hunger decay
    uint8_t hunger_decay = sleeping ? 
        (HUNGER_DECAY_SLEEP * mult / 100) : 
        (HUNGER_DECAY_NORMAL * mult / 100);
    if (_data.hunger > hunger_decay) _data.hunger -= hunger_decay;
    else _data.hunger = 0;
    
    // Happiness decay
    uint8_t happy_decay = sleeping ? 
        (HAPPINESS_DECAY_SLEEP * mult / 100) : 
        (HAPPINESS_DECAY_NORMAL * mult / 100);
    if (_data.happiness > happy_decay) _data.happiness -= happy_decay;
    else _data.happiness = 0;
    
    // Energy
    if (sleeping) {
        uint8_t regen = ENERGY_REGEN_SLEEP * mult / 100;
        if (_data.energy + regen <= STAT_MAX) _data.energy += regen;
        else _data.energy = STAT_MAX;
    } else {
        uint8_t energy_decay = ENERGY_DECAY_NORMAL * mult / 100;
        if (_data.energy > energy_decay) _data.energy -= energy_decay;
        else _data.energy = 0;
    }
    
    // Cleanliness
    uint8_t clean_decay = sleeping ?
        (CLEANLINESS_DECAY_SLEEP * mult / 100) :
        (CLEANLINESS_DECAY_NORMAL * mult / 100);
    if (_data.cleanliness > clean_decay) _data.cleanliness -= clean_decay;
    else _data.cleanliness = 0;
}

void PetEngine::check_health() {
    if (_data.hunger < HEALTH_DECAY_THRESHOLD || _data.cleanliness < HEALTH_DECAY_THRESHOLD) {
        if (_data.health > HEALTH_DECAY_AMOUNT) _data.health -= HEALTH_DECAY_AMOUNT;
        else _data.health = 0;
    }
    
    if (_data.health == 0) {
        _data.state = PET_STATE_DEAD;
    } else if (_data.health <= DYING_HEALTH_MAX) {
        _data.state = PET_STATE_SICK;  // Reusing SICK for dying
    } else if (_data.health <= CRITICAL_HEALTH_MAX) {
        _data.state = PET_STATE_SICK;
    }
}

void PetEngine::evolve() {
    if (_data.age_minutes > ADULT_MAX_MINUTES) _data.stage = PET_STAGE_ELDER;
    else if (_data.age_minutes > CHILD_MAX_MINUTES) _data.stage = PET_STAGE_ADULT;
    else if (_data.age_minutes > BABY_MAX_MINUTES) _data.stage = PET_STAGE_CHILD;
}

uint8_t PetEngine::get_decay_mult() const {
    switch (_data.stage) {
        case PET_STAGE_BABY: return BABY_DECAY_MULT;
        case PET_STAGE_CHILD: return CHILD_DECAY_MULT;
        case PET_STAGE_ADULT: return ADULT_DECAY_MULT;
        case PET_STAGE_ELDER: return ELDER_DECAY_MULT;
        default: return 100;
    }
}

void PetEngine::feed() {
    if (_data.state == PET_STATE_DEAD || _data.state == PET_STATE_SLEEPING) return;
    
    _data.state = PET_STATE_EATING;
    
    if (_data.hunger + FEED_HUNGER_GAIN <= STAT_MAX) _data.hunger += FEED_HUNGER_GAIN;
    else _data.hunger = STAT_MAX;
    
    if (_data.energy > FEED_ENERGY_COST) _data.energy -= FEED_ENERGY_COST;
    
    if (_data.hunger < HEALTH_DECAY_THRESHOLD) {
        if (_data.health + FEED_HEALTH_BONUS <= STAT_MAX) _data.health += FEED_HEALTH_BONUS;
        else _data.health = STAT_MAX;
    }
}

void PetEngine::play() {
    if (_data.state == PET_STATE_DEAD || _data.state == PET_STATE_SLEEPING) return;
    if (_data.energy < PLAY_ENERGY_MIN) return;
    
    _data.state = PET_STATE_PLAYING;
    _data.energy -= PLAY_ENERGY_COST;
    if (_data.hunger > PLAY_HUNGER_COST) _data.hunger -= PLAY_HUNGER_COST;
    
    if (_data.happiness + PLAY_HAPPINESS_GAIN <= STAT_MAX) _data.happiness += PLAY_HAPPINESS_GAIN;
    else _data.happiness = STAT_MAX;
}

void PetEngine::clean() {
    if (_data.state == PET_STATE_DEAD) return;
    _data.cleanliness = STAT_MAX;
    _data.health += CLEAN_HEALTH_BONUS;
    if (_data.health > STAT_MAX) _data.health = STAT_MAX;
}

void PetEngine::sleep() {
    if (_data.state == PET_STATE_DEAD) return;
    _data.state = PET_STATE_SLEEPING;
}

void PetEngine::wake() {
    if (_data.state == PET_STATE_SLEEPING) _data.state = PET_STATE_IDLE;
}

void PetEngine::heal() {
    if (_data.state == PET_STATE_DEAD) return;
    _data.health = STAT_MAX;
    _data.state = PET_STATE_IDLE;
}

String PetEngine::toJson() const {
    StaticJsonDocument<256> doc;
    doc["name"] = _data.name.c_str();
    doc["hunger"] = _data.hunger;
    doc["happiness"] = _data.happiness;
    doc["energy"] = _data.energy;
    doc["cleanliness"] = _data.cleanliness;
    doc["health"] = _data.health;
    doc["age_minutes"] = _data.age_minutes;
    doc["stage"] = _data.stage;
    doc["state"] = _data.state;
    doc["generation"] = _data.generation;
    
    String output;
    serializeJson(doc, output);
    return output;
}

bool PetEngine::fromJson(const String &json) {
    StaticJsonDocument<256> doc;
    DeserializationError err = deserializeJson(doc, json);
    if (err) return false;
    
    _data.name = doc["name"] | "Tama";
    _data.hunger = doc["hunger"] | 80;
    _data.happiness = doc["happiness"] | 70;
    _data.energy = doc["energy"] | 90;
    _data.cleanliness = doc["cleanliness"] | 85;
    _data.health = doc["health"] | 100;
    _data.age_minutes = doc["age_minutes"] | 0;
    _data.stage = doc["stage"] | PET_STAGE_BABY;
    _data.state = doc["state"] | PET_STATE_IDLE;
    _data.generation = doc["generation"] | 1;
    
    return true;
}
