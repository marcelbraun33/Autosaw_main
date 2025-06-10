
// SettingsManager.cpp
#include "SettingsManager.h"
#include <genieArduinoDEV.h>
#include <ClearCore.h>
#include "FileManager.h"

SettingsManager& SettingsManager::Instance() {
    static SettingsManager inst;
    return inst;
}

SettingsManager::SettingsManager() {
    // Default values
    settings_.bladeDiameter = 1.0f;
    settings_.bladeThickness = 0.05f;

    settings_.feedRate = 10.0f;
    settings_.rapidRate = 50.0f;
}

void SettingsManager::load() {
    // TODO: read from SD card or EEPROM
}

void SettingsManager::save() {
    // Clamp values to valid ranges

    if (settings_.bladeDiameter > 10.0f) settings_.bladeDiameter = 10.0f;
    if (settings_.bladeDiameter < 0.1f)  settings_.bladeDiameter = 0.1f;

    if (settings_.bladeThickness > 0.5f)   settings_.bladeThickness = 0.5f;
    if (settings_.bladeThickness < 0.001f) settings_.bladeThickness = 0.001f;

    if (settings_.feedRate > 25.0f) settings_.feedRate = 25.0f;
    if (settings_.feedRate < 0.0f)  settings_.feedRate = 0.0f;

    if (settings_.rapidRate > 300.0f) settings_.rapidRate = 300.0f;
    if (settings_.rapidRate < 0.0f)   settings_.rapidRate = 0.0f;

    // TODO: write to SD card or EEPROM
}

