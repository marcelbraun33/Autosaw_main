#pragma once
// No change needed to SemiAutoScreen.h - it already has the FeedHoldManager instance
#include "Screen.h"
#include "cutData.h"
#include "FeedHoldManager.h" // Add this include
class ScreenManager;

// Define screen states
enum SemiAutoScreenState {
    STATE_READY = 0,
    STATE_CUTTING = 1,
    STATE_PAUSED = 2,
    STATE_ADJUSTING_PRESSURE = 3  // New state for pressure adjustment
};

class SemiAutoScreen : public Screen {
public:
    void onShow() override;
    void onHide() override;
    void handleEvent(const genieFrame& e) override;
    void update() override;
    SemiAutoScreen(ScreenManager& mgr);
    void updateFeedRateDisplay();

    // Update the thickness LED display (IDs 20 and 8)
    void UpdateThicknessLed(float thickness);

private:
    void startFeedToStop();
    void advanceIncrement();
    void feedHold();
    void exitFeedHold();
    void adjustCutPressure();      // New method to handle cut pressure adjustment

    ScreenManager& _mgr;
    SemiAutoScreenState _currentState = STATE_READY;
    float _tempCutPressure = 70.0f;   // Temporary cut pressure value for MPG adjustment

    // Constants for cut pressure adjustment
    static constexpr float CUT_PRESSURE_INCREMENT = 1.0f;
    static constexpr float MIN_CUT_PRESSURE = 1.0f;  // Changed from 20.0f to 1.0f
    static constexpr float MAX_CUT_PRESSURE = 100.0f;

    FeedHoldManager _feedHoldManager; // FeedHoldManager instance

protected:
    // Optionally, store the last thickness value for display
    float m_lastThickness = 0.0f;
};
