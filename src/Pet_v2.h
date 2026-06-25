// ============================================================
// Pet_v2.h — Minimal Pet Engine for v2.0 Foundation
// PetStage/PetState enums are prefixed (PET_STAGE_*, PET_STATE_*)
// to avoid conflict with v1 Pet.h which defines unscoped BABY/CHILD/etc.
// ============================================================

#ifndef PET_V2_H
#define PET_V2_H

#include <Arduino.h>
#include "config_v2.h"

enum PetStage {
    PET_STAGE_BABY = 0,
    PET_STAGE_CHILD,
    PET_STAGE_ADULT,
    PET_STAGE_ELDER,
    PET_STAGE_COUNT
};

enum PetState {
    PET_STATE_IDLE = 0,
    PET_STATE_EATING,
    PET_STATE_PLAYING,
    PET_STATE_SLEEPING,
    PET_STATE_SICK,
    PET_STATE_DEAD
};

struct PetData {
    String name;
    uint8_t hunger;
    uint8_t happiness;
    uint8_t energy;
    uint8_t cleanliness;
    uint8_t health;
    uint32_t age_minutes;
    uint32_t birth_timestamp;
    uint8_t stage;
    uint8_t state;
    uint8_t generation;
};

class PetEngine {
public:
    PetEngine();

    void init();
    void update();

    void feed();
    void play();
    void clean();
    void sleep();
    void wake();
    void heal();

    const PetData& getData() const { return _data; }
    uint8_t getStage() const { return _data.stage; }
    uint8_t getHealth() const { return _data.health; }
    bool isAlive() const { return _data.health > 0; }
    bool isSleeping() const { return _data.state == PET_STATE_SLEEPING; }

    String toJson() const;
    bool fromJson(const String &json);

private:
    PetData _data;

    void decay_stats();
    void check_health();
    void evolve();
    uint8_t get_decay_mult() const;
};

#endif // PET_V2_H
