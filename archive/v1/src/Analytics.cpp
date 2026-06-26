#include "Analytics.h"
#include "config.h"
#include "Storage.h"

// ============================================================
// Phase 17.3: Advanced Analytics Implementation
// ============================================================

// --- Care Pattern Analysis ---

CarePattern analyzeCarePatterns(const GameStats& stats, unsigned long uptimeMinutes) {
    CarePattern p = {0};

    if (uptimeMinutes == 0) return p;

    p.totalActions = stats.totalFeeds + stats.totalPlays + stats.totalCleans +
                     stats.totalSleeps + stats.totalHeals;

    // Average intervals (in minutes)
    if (stats.totalFeeds > 0)
        p.avgFeedIntervalMin = (float)uptimeMinutes / stats.totalFeeds;
    else
        p.avgFeedIntervalMin = 999.0f;

    if (stats.totalPlays > 0)
        p.avgPlayIntervalMin = (float)uptimeMinutes / stats.totalPlays;
    else
        p.avgPlayIntervalMin = 999.0f;

    if (stats.totalSleeps > 0)
        p.avgSleepDurationMin = (float)uptimeMinutes / stats.totalSleeps;
    else
        p.avgSleepDurationMin = 0;

    // Care score calculation (0-100)
    // Based on: action frequency, balance of care types, low deaths
    float actionFreq = min(100.0f, (float)p.totalActions / (uptimeMinutes / 60.0f) * 10.0f);
    float feedBalance = 0;
    if (p.avgFeedIntervalMin < 120) feedBalance += 25;  // Fed within 2h = good
    else if (p.avgFeedIntervalMin < 240) feedBalance += 15;
    else feedBalance += 5;

    float playBalance = 0;
    if (p.avgPlayIntervalMin < 240) playBalance += 25;
    else if (p.avgPlayIntervalMin < 480) playBalance += 15;
    else playBalance += 5;

    float deathPenalty = max(0.0f, 25.0f - stats.deaths * 5.0f);

    p.careScore = min(100.0f, actionFreq * 0.3f + feedBalance + playBalance + deathPenalty);

    return p;
}

// --- Health Predictions ---

HealthPrediction predictHealth(const Pet& pet, const GameStats& stats) {
    HealthPrediction pred = {0};

    if (!pet.isAlive) {
        pred.minutesUntilHungry = 0;
        pred.minutesUntilTired = 0;
        pred.minutesUntilUnhappy = 0;
        pred.minutesUntilDirty = 0;
        pred.healthDeclining = false;
        return pred;
    }

    // Use decay rates from config.h to estimate time until thresholds
    // Hunger: decays at HUNGER_DECAY_NORMAL per PET_UPDATE_INTERVAL minutes
    float hungerPerMin = (float)HUNGER_DECAY_NORMAL / (PET_UPDATE_INTERVAL / 60000.0f);
    if (hungerPerMin > 0 && pet.hunger > 30)
        pred.minutesUntilHungry = (int)((pet.hunger - 30) / hungerPerMin);
    else if (pet.hunger <= 30)
        pred.minutesUntilHungry = 0;
    else
        pred.minutesUntilHungry = 999;

    // Energy: decays at ENERGY_DECAY_NORMAL per tick (or regenerates during sleep)
    bool isSleeping = (pet.state == "sleeping");
    float energyPerMin;
    if (isSleeping) {
        energyPerMin = (float)ENERGY_REGEN_SLEEP / (PET_UPDATE_INTERVAL / 60000.0f);
        if (energyPerMin > 0 && pet.energy < 30)
            pred.minutesUntilTired = (int)((30 - pet.energy) / energyPerMin);  // Time to recover
        else
            pred.minutesUntilTired = 999;
    } else {
        energyPerMin = (float)ENERGY_DECAY_NORMAL / (PET_UPDATE_INTERVAL / 60000.0f);
        if (energyPerMin > 0 && pet.energy > 30)
            pred.minutesUntilTired = (int)((pet.energy - 30) / energyPerMin);
        else if (pet.energy <= 30)
            pred.minutesUntilTired = 0;
        else
            pred.minutesUntilTired = 999;
    }

    // Happiness: decays at HAPPINESS_DECAY_NORMAL per tick
    float happyPerMin = (float)HAPPINESS_DECAY_NORMAL / (PET_UPDATE_INTERVAL / 60000.0f);
    if (happyPerMin > 0 && pet.happiness > 30)
        pred.minutesUntilUnhappy = (int)((pet.happiness - 30) / happyPerMin);
    else if (pet.happiness <= 30)
        pred.minutesUntilUnhappy = 0;
    else
        pred.minutesUntilUnhappy = 999;

    // Cleanliness: decays at CLEANLINESS_DECAY_NORMAL per tick
    float cleanPerMin = (float)CLEANLINESS_DECAY_NORMAL / (PET_UPDATE_INTERVAL / 60000.0f);
    if (cleanPerMin > 0 && pet.cleanliness > 30)
        pred.minutesUntilDirty = (int)((pet.cleanliness - 30) / cleanPerMin);
    else if (pet.cleanliness <= 30)
        pred.minutesUntilDirty = 0;
    else
        pred.minutesUntilDirty = 999;

    // Health declining if any critical stat is low
    pred.healthDeclining = (pet.hunger < HEALTH_DECAY_THRESHOLD ||
                            pet.cleanliness < HEALTH_DECAY_THRESHOLD);

    return pred;
}

// --- Report Generation ---

CareReport generateReport(const Pet& pet, const GameStats& stats, int periodDays) {
    CareReport r = {0};
    r.periodDays = periodDays;

    r.totalFeeds = stats.totalFeeds;
    r.totalPlays = stats.totalPlays;
    r.totalCleans = stats.totalCleans;
    r.totalSleeps = stats.totalSleeps;
    r.totalHeals = stats.totalHeals;

    r.avgHealth = pet.health;
    r.avgHappiness = pet.happiness;
    r.avgHunger = pet.hunger;
    r.avgEnergy = pet.energy;
    r.avgCleanliness = pet.cleanliness;

    // Care score
    CarePattern p = analyzeCarePatterns(stats,periodDays * 24 * 60);
    r.careScore = p.careScore;
    r.achievementsUnlocked = stats.evolutions;
    // Simplified ranking (based on care score)
    r.rankPercentile = (int)r.careScore;

    return r;
}

// --- JSON Output ---

String getCarePatternsJson(const GameStats& stats, unsigned long uptimeMinutes) {
    CarePattern p = analyzeCarePatterns(stats, uptimeMinutes);
    String json = "{";
    json += "\"avgFeedIntervalMin\":" + String(p.avgFeedIntervalMin, 1) + ",";
    json += "\"avgPlayIntervalMin\":" + String(p.avgPlayIntervalMin, 1) + ",";
    json += "\"avgSleepDurationMin\":" + String(p.avgSleepDurationMin, 1) + ",";
    json += "\"totalActions\":" + String(p.totalActions) + ",";
    json += "\"careScore\":" + String(p.careScore, 1);
    json += "}";
    return json;
}

String getHealthPredictionsJson(const Pet& pet, const GameStats& stats) {
    HealthPrediction p = predictHealth(pet, stats);
    String json = "{";
    json += "\"minutesUntilHungry\":" + String(p.minutesUntilHungry) + ",";
    json += "\"minutesUntilTired\":" + String(p.minutesUntilTired) + ",";
    json += "\"minutesUntilUnhappy\":" + String(p.minutesUntilUnhappy) + ",";
    json += "\"minutesUntilDirty\":" + String(p.minutesUntilDirty) + ",";
    json += "\"healthDeclining\":" + String(p.healthDeclining ? "true" : "false");
    json += "}";
    return json;
}

String getCareReportJson(const Pet& pet, const GameStats& stats, int periodDays) {
    CareReport r = generateReport(pet, stats, periodDays);
    String json = "{";
    json += "\"periodDays\":" + String(r.periodDays) + ",";
    json += "\"totalFeeds\":" + String(r.totalFeeds) + ",";
    json += "\"totalPlays\":" + String(r.totalPlays) + ",";
    json += "\"totalCleans\":" + String(r.totalCleans) + ",";
    json += "\"totalSleeps\":" + String(r.totalSleeps) + ",";
    json += "\"totalHeals\":" + String(r.totalHeals) + ",";
    json += "\"avgHealth\":" + String(r.avgHealth, 1) + ",";
    json += "\"avgHappiness\":" + String(r.avgHappiness, 1) + ",";
    json += "\"avgHunger\":" + String(r.avgHunger, 1) + ",";
    json += "\"avgEnergy\":" + String(r.avgEnergy, 1) + ",";
    json += "\"avgCleanliness\":" + String(r.avgCleanliness, 1) + ",";
    json += "\"careScore\":" + String(r.careScore, 1) + ",";
    json += "\"achievementsUnlocked\":" + String(r.achievementsUnlocked) + ",";
    json += "\"rankPercentile\":" + String(r.rankPercentile);
    json += "}";
    return json;
}
