#pragma once
#include "Screen.h"
#include "cutData.h"
#include "FeedHoldManager.h" // Add this include
#include "SpindleLoadMeter.h" // Add include for SpindleLoadMeter

class ScreenManager;

// Define screen states
enum SemiAutoScreenState {
    STATE_READY = 0,
    STATE_CUTTING = 1,
    STATE_PAUSED = 2,
    STATE_ADJUSTING_PRESSURE = 3,
    STATE_RETURNING = 4,  // State for returning to start position
    STATE_ADJUSTING_FEED_RATE = 5  // New state for feed rate adjustment
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
    void adjustCutPressure();      // Method to handle cut pressure adjustment
    void adjustMaxFeedRate();      // New method to handle feed rate adjustment

    // Helper method for button state management
    void updateButtonState(uint16_t buttonId, bool state, const char* logMessage = nullptr, uint16_t delayMs = 20);

    ScreenManager& _mgr;
    SemiAutoScreenState _currentState = STATE_READY;
    float _tempCutPressure = 70.0f;   // Temporary cut pressure value for MPG adjustment
    float _tempFeedRate = 0.5f;       // Temporary feed rate value (0.0-1.0 scale)
    bool _isReturningToStart = false; // Flag to track return motion
    SpindleLoadMeter _spindleLoadMeter; // SpindleLoadMeter instance

    // Constants for cut pressure adjustment
    static constexpr float CUT_PRESSURE_INCREMENT = 1.0f;
    static constexpr float MIN_CUT_PRESSURE = 1.0f;
    static constexpr float MAX_CUT_PRESSURE = 100.0f;

    // Constants for feed rate adjustment
    static constexpr float FEED_RATE_INCREMENT = 0.05f;  // 5% increment
    static constexpr float MIN_FEED_RATE = 0.05f;        // 5% minimum
    static constexpr float MAX_FEED_RATE = 1.0f;         // 100% maximum

    FeedHoldManager _feedHoldManager; // FeedHoldManager instance

protected:
    // Optionally, store the last thickness value for display
    float m_lastThickness = 0.0f;
};
