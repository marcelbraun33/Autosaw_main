// SettingsManager.h
#pragma once

#include <ClearCore.h>

// Define feature flags for optional settings
#define SETTINGS_HAS_CUT_PRESSURE


// Holds all user-configurable settings
struct Settings {
    float bladeDiameter;
    float bladeThickness;

    float feedRate;
    float rapidRate;
    float manualOverrideRPM; 
    
    // Added new settings
    float cutPressure = 70.0f;     // Default cut pressure/torque target (%)
    float spindleRPM = 3000.0f;    // Default spindle speed (RPM)
};

class SettingsManager {
public:
    // Singleton
    static SettingsManager& Instance();

    // Persist/load settings (stubbed for now)
    void load();
    void save();

    // Access the settings
    Settings& settings() { return settings_; }

private:
    SettingsManager();
    Settings settings_;
};
