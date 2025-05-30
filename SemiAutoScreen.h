#pragma once
#include "SemiAutoCycle.h"
#include "Screen.h"
#include "CycleManager.h"
#include "SettingsManager.h"
#include "cutData.h"

class ScreenManager;

class SemiAutoScreen : public Screen {
public:
    void onShow() override;
    void onHide() override;
    void handleEvent(const genieFrame& e) override;
    void update() override;
    SemiAutoScreen(ScreenManager& mgr);

private:
    void updateDisplays();
    void updateButtonStates();
    SemiAutoCycle* _cycle = nullptr;
    bool _feedRateAdjustActive = false;
    int _mpgLastValue = 0;



    ScreenManager& _mgr;
};
