
// SettingsManager.cpp
#include "SettingsManager.h"
#include <genieArduinoDEV.h>
#include <ClearCore.h>

SettingsManager& SettingsManager::Instance() {
    static SettingsManager inst;
    return inst;
}

SettingsManager::SettingsManager() {
    // Default values
    settings_.bladeDiameter = 1.0f;
    settings_.bladeThickness = 0.05f;
    settings_.defaultRPM = 3600.0f;
    settings_.feedRate = 10.0f;
    settings_.rapidRate = 50.0f;
}

void SettingsManager::load() {
    // TODO: read from SD card or EEPROM
}

void SettingsManager::save() {
    // TODO: write to SD card or EEPROM
}