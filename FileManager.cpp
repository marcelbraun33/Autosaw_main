// core/FileManager.cpp
#include <SPI.h>
#include <SD.h>
#include "Config.h"
#include "FileManager.h"

FileManager& FileManager::Instance() {
    static FileManager inst;
    return inst;
}

bool FileManager::init() {
    // Mirror the ReadWrite example:
    Serial.print("Initializing SD card...");
    if (!SD.begin()) {
        Serial.println("initialization failed!");
        return false;
    }
    Serial.println("initialization done.");
    return true;
}

bool FileManager::loadSettings(Settings& s) {
    // Ensure the card is up
    if (!init()) return false;

    Serial.print("Opening ");
    Serial.print(SETTINGS_FILE);
    Serial.println(" for read...");

    File myFile = SD.open(SETTINGS_FILE, FILE_READ);
    if (!myFile) {
        Serial.println("error opening settings file");
        return false;
    }

    // Read one line (up to newline)
    String line = myFile.readStringUntil('\n');
    myFile.close();

    // Parse CSV: bladeDia,bladeThk,defRPM,feed,rapid,overrideRPM
    float vals[6] = { 0 };
    int idx = 0;
    char* token = strtok(const_cast<char*>(line.c_str()), ",");
    while (token && idx < 6) {
        vals[idx++] = atof(token);
        token = strtok(nullptr, ",");
    }

    if (idx == 6) {
        s.bladeDiameter = vals[0];
        s.bladeThickness = vals[1];
        s.defaultRPM = vals[2];
        s.feedRate = vals[3];
        s.rapidRate = vals[4];
        s.manualOverrideRPM = vals[5];
        Serial.println("Settings loaded from SD");
        return true;
    }

    Serial.println("Settings parse error");
    return false;
}

bool FileManager::saveSettings(const Settings& s) {
    if (!init()) return false;

    Serial.print("Opening ");
    Serial.print(SETTINGS_FILE);
    Serial.println(" for write...");

    File myFile = SD.open(SETTINGS_FILE, FILE_WRITE);
    if (!myFile) {
        Serial.println("error opening settings file for write");
        return false;
    }

    // Write CSV line
    myFile.print(s.bladeDiameter);     myFile.print(',');
    myFile.print(s.bladeThickness);    myFile.print(',');
    myFile.print(s.defaultRPM);        myFile.print(',');
    myFile.print(s.feedRate);          myFile.print(',');
    myFile.print(s.rapidRate);         myFile.print(',');
    myFile.print(s.manualOverrideRPM); myFile.println();

    myFile.close();
    Serial.println("Settings saved to SD");
    return true;
}
