#pragma once
#include "Screen.h"
#include "FeedHoldManager.h"
#include "SpindleLoadMeter.h"
#include "TorqueControlUI.h"
#include <stdint.h>

class ScreenManager;

class SemiAutoScreen : public Screen {
public:
    SemiAutoScreen(ScreenManager& mgr);

    void onShow() override;
    void onHide() override;
    void handleEvent(const genieFrame& e) override;
    void update() override;

private:
    enum SemiAutoScreenState {
        STATE_READY,
        STATE_CUTTING,
        STATE_PAUSED,
        STATE_RETURNING,
        STATE_ADJUSTING_PRESSURE,    // Kept for compatibility
        STATE_ADJUSTING_FEED_RATE    // Kept for compatibility
    };

    void startFeedToStop();
    void feedHold();
    void exitFeedHold();
    void adjustCutPressure();
    void adjustMaxFeedRate();
    void advanceIncrement();
    void updateButtonState(uint16_t buttonId, bool state, const char* logMessage = nullptr, uint16_t delayMs = 0);
    void UpdateThicknessLed(float thickness);
    void updateFeedRateDisplay();

    ScreenManager& _mgr;
    FeedHoldManager _feedHoldManager;
    SpindleLoadMeter _spindleLoadMeter;
    TorqueControlUI _torqueControlUI;

    SemiAutoScreenState _currentState = STATE_READY;
    bool _isReturningToStart = false;
    float m_lastThickness = -1.0f;

    // No longer needed - handled by TorqueControlUI
    // float _tempCutPressure = 70.0f;
    // float _tempFeedRate = 1.0f;
};