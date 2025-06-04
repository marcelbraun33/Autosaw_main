// Updated SettingsScreen.h
#pragma once

#include "Screen.h"
#include "UIInputManager.h"
#include "SettingsManager.h"
#include "Config.h"
#include <genieArduinoDEV.h>
class ScreenManager;

class SettingsScreen : public Screen {
public:
    void onShow() override;
    void onHide() override;
    void handleEvent(const genieFrame& e) override;
    void update() override;  // Added update method for encoder polling
    SettingsScreen(ScreenManager& mgr);
private:
    ScreenManager& _mgr;

    // Cut pressure adjustment with MPG
    bool _adjustingCutPressure = false;
    float _tempCutPressure = 70.0f;
    int32_t _lastEncoderPos = 0;
};
