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

    // Cycle control methods
    void startCycle();
    void pauseCycle();
    void resumeCycle();
    void cancelCycle();
    void exitFeedHold();

    // New toggle method for pause/resume
    void togglePauseResume();        // For WINBUTTON_SLIDE_HOLD_F5

    // Adjustment methods
    void adjustCutPressure();
    void adjustMaxSpeed();

    // Movement methods
    void moveToStartPosition();      // WINBUTTON_MOVE_TO_START_POSITION (Rapid to Zero)

    // Setup method
    void openSetupAutocutScreen();      // For WINBUTTON_SETUP_AUTOCUT_F5

    // UI methods
    void toggleSpindle();
    void openSettings();

private:
    void updateDisplay();
    void updateButtonState(uint16_t buttonId, bool state, const char* logMessage = nullptr, uint16_t delayMs = 0);
    void flashButtonError(uint16_t buttonId);  // Helper for error feedback

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