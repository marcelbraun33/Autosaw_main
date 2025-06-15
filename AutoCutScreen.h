#pragma once
#include "Screen.h"
#include "FeedHoldManager.h"
#include "TorqueControlUI.h"

class ScreenManager;

class AutoCutScreen : public Screen {
public:
    AutoCutScreen(ScreenManager& mgr);

    void onShow() override;
    void onHide() override;
    void handleEvent(const genieFrame& e) override;
    void update() override;

    void startCycle();
    void pauseCycle();
    void resumeCycle();
    void cancelCycle();
    void exitFeedHold();
    void adjustCutPressure();
    void adjustMaxSpeed();
    void moveToStartPosition();
    void toggleSpindle();
    void openSettings();

private:
    void updateDisplay();
    void updateButtonState(uint16_t buttonId, bool state, const char* logMessage = nullptr, uint16_t delayMs = 0);

    enum RapidToZeroState {
        RapidIdle,
        MovingYToRetract,
        MovingXToZero
    };
    RapidToZeroState _rapidState = RapidIdle;

    ScreenManager& _mgr;
    FeedHoldManager _feedHoldManager;
    TorqueControlUI _torqueControlUI;
};