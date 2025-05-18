// FileManager.h
#pragma once

class Settings; // Forward declaration if not including the full header

class FileManager {
public:
    static FileManager& Instance();
    
    bool saveSettings(const Settings& s);
    bool loadSettings(Settings& s);
    
private:
    FileManager() = default;
    // Add any private members needed
};
