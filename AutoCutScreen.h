#pragma once
#include "Screen.h"
#include "cutData.h"
#include "AutoCutCycle.h" // Add this include
#include "CycleManager.h" // Add this include

class ScreenManager;

class AutoCutScreen : public Screen {
public:
    AutoCutScreen(ScreenManager& mgr);

    // Required virtual methods
    void onShow() override;
    void onHide() override;
    void handleEvent(const genieFrame& e) override;
    void update() override;

    // Cycle controls
    void pauseCycle();
    void resumeCycle();
    void cancelCycle();

    // UI updates
    void updateDisplays();

private:
    ScreenManager& _mgr;
    AutoCutCycle* _cycle = nullptr;
};
