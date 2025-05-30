// SettingsManager.h
#pragma once

#include <ClearCore.h>

// Holds all user-configurable settings
struct Settings {
    float bladeDiameter;
    float bladeThickness;
    float defaultRPM;
    float feedRate;
    float rapidRate;
    float manualOverrideRPM; 
    float cutPressure = 0.0f; // Add this line
    float feedRateIncrement = 0.5f; // Default increment for feed rate adjustment
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
