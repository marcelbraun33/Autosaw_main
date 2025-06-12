
// === ScreenManager.h ===
#pragma once
#include <genieArduinoDEV.h>
#include "HomingScreen.h"
#include "Screen.h"
#include "ManualModeScreen.h"
#include "SettingsScreen.h"
#include "JogXScreen.h"
#include "JogYScreen.h"
#include "JogZScreen.h"
#include "SemiAutoScreen.h"
#include "AutoCutScreen.h"
#include "CutData.h"


class HomingScreen;
class ScreenManager {
public:
    static ScreenManager& Instance();

    /// Call once at startup
    void Init();

    // UI navigation
    void ShowSplash();
    void ShowManualMode();
    void ShowHoming();
    void ShowSettings();
    void ShowJogX();
    void ShowJogY();
    void ShowJogZ();
    void ShowSemiAuto();
    void ShowAutoCut();
    void Back();
   
    void clearAllLeds();
    JogXScreen& GetJogXScreen() { return _jogXScreen; }
    JogYScreen& GetJogYScreen() { return _jogYScreen; }
    uint8_t currentForm() const { return _currentForm; }
    Screen* currentScreen();
    CutData& GetCutData() { return _cutData; }

private:
    ScreenManager();
    void writeForm(uint8_t formId);

    uint8_t _currentForm = 255;
    uint8_t _lastForm = FORM_MANUAL_MODE;

    ManualModeScreen _manualScreen;
    SettingsScreen   _settingsScreen;
    HomingScreen     _homingScreen;
    JogXScreen       _jogXScreen;
    JogYScreen       _jogYScreen;
    JogZScreen       _jogZScreen;
    SemiAutoScreen   _semiAutoScreen;
    AutoCutScreen    _autoCutScreen;
    Screen* _currentScreen = nullptr;
    CutData _cutData;
};
