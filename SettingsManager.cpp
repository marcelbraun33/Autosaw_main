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
    settings_.defaultRPM = 3600.0f;
    settings_.feedRate = 10.0f;
    settings_.rapidRate = 50.0f;
    settings_.cutPressure = 0.0f; // Initialize cutPressure
}

void SettingsManager::load() {
    // TODO: read from SD card or EEPROM
    // Example (pseudo-code):
    // settings_.bladeDiameter = FileManager::ReadFloat("bladeDiameter", 1.0f);
    // settings_.bladeThickness = FileManager::ReadFloat("bladeThickness", 0.05f);
    // settings_.defaultRPM = FileManager::ReadFloat("defaultRPM", 3600.0f);
    // settings_.feedRate = FileManager::ReadFloat("feedRate", 10.0f);
    // settings_.rapidRate = FileManager::ReadFloat("rapidRate", 50.0f);
    // settings_.cutPressure = FileManager::ReadFloat("cutPressure", 0.0f);
}

void SettingsManager::save() {
    // Clamp values to valid ranges
    if (settings_.defaultRPM > 4000) settings_.defaultRPM = 4000;
    if (settings_.defaultRPM < 0)    settings_.defaultRPM = 0;

    if (settings_.bladeDiameter > 10.0f) settings_.bladeDiameter = 10.0f;
    if (settings_.bladeDiameter < 0.1f)  settings_.bladeDiameter = 0.1f;

    if (settings_.bladeThickness > 0.5f)   settings_.bladeThickness = 0.5f;
    if (settings_.bladeThickness < 0.001f) settings_.bladeThickness = 0.001f;

    if (settings_.feedRate > 25.0f) settings_.feedRate = 25.0f;
    if (settings_.feedRate < 0.0f)  settings_.feedRate = 0.0f;

    if (settings_.rapidRate > 300.0f) settings_.rapidRate = 300.0f;
    if (settings_.rapidRate < 0.0f)   settings_.rapidRate = 0.0f;

    if (settings_.cutPressure > 100.0f) settings_.cutPressure = 100.0f;
    if (settings_.cutPressure < 0.0f)   settings_.cutPressure = 0.0f;

    // TODO: write to SD card or EEPROM
    // Example (pseudo-code):
    // FileManager::WriteFloat("bladeDiameter", settings_.bladeDiameter);
    // FileManager::WriteFloat("bladeThickness", settings_.bladeThickness);
    // FileManager::WriteFloat("defaultRPM", settings_.defaultRPM);
    // FileManager::WriteFloat("feedRate", settings_.feedRate);
    // FileManager::WriteFloat("rapidRate", settings_.rapidRate);
    // FileManager::WriteFloat("cutPressure", settings_.cutPressure);
}
