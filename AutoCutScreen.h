#pragma once
#include "Screen.h"
#include "FeedHoldManager.h"

class ScreenManager;

class AutoCutScreen : public Screen {
public:
    void onShow() override;
    void onHide() override;
    void handleEvent(const genieFrame& e) override;
    AutoCutScreen(ScreenManager& mgr);
    void pauseCycle();
    void resumeCycle();
    void cancelCycle();
private:
    ScreenManager& _mgr;
    FeedHoldManager _feedHoldManager; // Add FeedHoldManager instance
};
