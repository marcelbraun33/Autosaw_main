#pragma once

#include "Screen.h"
#include "UIInputManager.h"
#include "SettingsManager.h"
#include "Config.h"
#include <genieArduinoDEV.h>

class SettingsScreen : public Screen {
public:
    void onShow() override;
    void onHide() override;
    void handleEvent(const genieFrame& e) override; // NOT a pointer
};
