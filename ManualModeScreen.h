// ManualModeScreen.h
#pragma once

#include "Screen.h"
#include <genieArduinoDEV.h>
class ScreenManager;

class ManualModeScreen : public Screen {
public:
    void onShow() override;
    void onHide() override;
    void handleEvent(const genieFrame& e) override;
    void update() override;
	ManualModeScreen(ScreenManager& mgr);  
private:
    ScreenManager& _mgr;
};


