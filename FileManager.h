// core/FileManager.h
#pragma once

#include <ClearCore.h>
#include <SPI.h>
#include <SD.h>
#include "SettingsManager.h"            // Brings in the `Settings` struct

class FileManager {
public:
    static FileManager& Instance();

    /// Mount the SD card
    bool init();

    /// Read/write the settings CSV file
    bool loadSettings(Settings& s);
    bool saveSettings(const Settings& s);

private:
    FileManager() = default;
    static constexpr const char* SETTINGS_FILE = "/settings.txt";
};
