#ifndef ANALYTICS_H
#define ANALYTICS_H

#include <Arduino.h>
#include "Pet.h"
#include "Stats.h"

// ============================================================
// Phase 17.3: Advanced Analytics & Insights
// Care pattern analysis, health predictions, and reports.
// Designed for minimal flash impact (~1.5% estimated).
// ============================================================

// Care pattern summary
struct CarePattern {
    float avgFeedIntervalMin;    // Average minutes between feeds
    float avgPlayIntervalMin;    // Average minutes between plays
    float avgSleepDurationMin;   // Average sleep duration
    int totalActions;            // Total care actions in period
    float careScore;             // 0-100 overall care quality score
};

// Health prediction
struct HealthPrediction {
    int minutesUntilHungry;      // Estimated minutes until hunger < 30
    int minutesUntilTired;       // Estimated minutes until energy < 30
    int minutesUntilUnhappy;     // Estimated minutes until happiness < 30
    int minutesUntilDirty;       // Estimated minutes until cleanliness < 30
    bool healthDeclining;        // True if health is trending down
};

// Weekly/Monthly report
struct CareReport {
    int periodDays;              // 7 for weekly, 30 for monthly
    int totalFeeds;
    int totalPlays;
    int totalCleans;
    int totalSleeps;
    int totalHeals;
    float avgHealth;
    float avgHappiness;
    float avgHunger;
    float avgEnergy;
    float avgCleanliness;
    float careScore;
    int achievementsUnlocked;
    int rankPercentile;          // 0-100, how owner compares
};

// Analyze care patterns from current stats
CarePattern analyzeCarePatterns(const GameStats& stats, unsigned long uptimeMinutes);

// Predict future pet state based on current stats
HealthPrediction predictHealth(const Pet& pet, const GameStats& stats);

// Generate a care report for the given period
CareReport generateReport(const Pet& pet, const GameStats& stats, int periodDays);

// JSON output for API
String getCarePatternsJson(const GameStats& stats, unsigned long uptimeMinutes);
String getHealthPredictionsJson(const Pet& pet, const GameStats& stats);
String getCareReportJson(const Pet& pet, const GameStats& stats, int periodDays);

#endif // ANALYTICS_H
