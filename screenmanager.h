#pragma once

#include <genieArduinoDEV.h>
#include "Config.h"
#include "Screen.h"
#include "SettingsScreen.h"
#include "ManualModeScreen.h"

class ScreenManager {
public:
    static ScreenManager& Instance();

    void Init();

    void ShowSplash();
    void ShowManualMode();
    void ShowHoming();
    void ShowJogX();
    void ShowJogY();
    void ShowJogZ();
    void ShowSemiAuto();
    void ShowAutoCut();
    void ShowSettings();
    void Back();

    void SetHomed(bool homed);
    uint8_t currentForm() const { return _currentForm; }

    Screen* currentScreen();

private:
    ScreenManager();
    void writeForm(uint8_t formId);

    uint8_t _currentForm = 255;
    uint8_t _lastForm = FORM_MANUAL_MODE;
    bool _homed = false;
    ManualModeScreen _manualModeScreen;

    Screen* _currentScreen = nullptr;
    SettingsScreen _settingsScreen;

};
