// ManualModeScreen.h
#pragma once

#include "Screen.h"
#include <genieArduinoDEV.h>
// Forward declaration
class ScreenManager;

class ManualModeScreen : public Screen {
public:
    void onShow() override;
    void onHide() override;
    void handleEvent(const genieFrame& e) override;
    void update() override;
    ManualModeScreen(ScreenManager& mgr);

    // Methods for handling manual mode functionality (removed togglePendant)
    void toggleSpindle();
    void activateHoming();
    void goToSettings();

private:
    ScreenManager& _mgr;
};
