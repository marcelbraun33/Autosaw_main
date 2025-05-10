// ManualModeScreen.h
#pragma once

#include "Screen.h"
#include <genieArduinoDEV.h>

class ManualModeScreen : public Screen {
public:
    void onShow() override;
    void onHide() override;
    void handleEvent(const genieFrame& e) override;
    void update() override;
};

