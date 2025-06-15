#pragma once

#include <ClearCore.h>
#include <genieArduinoDEV.h>
#include "Config.h"

// Forward declarations
class MotionController;
class SettingsManager;

/// Shared component for torque control UI across different screens
/// Handles cut pressure adjustment, feed rate adjustment, gauge updates, and encoder input
class TorqueControlUI {
public:
    // UI State for torque control adjustments
    enum AdjustmentMode {
        ADJUSTMENT_NONE,
        ADJUSTMENT_CUT_PRESSURE,
        ADJUSTMENT_FEED_RATE
    };

    // Constructor
    TorqueControlUI();

    // Initialize with display object IDs for this screen's UI elements
    void init(uint16_t cutPressureLedId, uint16_t targetFeedRateLedId,
        uint16_t torqueGaugeId, uint16_t liveFeedRateLedId = 0);

    // Call when screen is shown to initialize displays
    void onShow();

    // Call when screen is hidden to cleanup
    void onHide();

    // Update displays and handle encoder input - call from screen's update()
    void update();

    // Button handlers - call from screen's handleEvent()
    void toggleCutPressureAdjustment();
    void toggleFeedRateAdjustment();
    void incrementValue();
    void decrementValue();

    // Query current adjustment state
    AdjustmentMode getCurrentMode() const { return _currentMode; }
    bool isAdjusting() const { return _currentMode != ADJUSTMENT_NONE; }

    // Get current values (for display or saving)
    float getCurrentCutPressure() const { return _tempCutPressure; }
    float getCurrentFeedRate() const { return _tempFeedRate; }

    // Force exit adjustment mode and save settings
    void exitAdjustmentMode();

    // Update button states (call these when entering/exiting cut cycles)
    void setCuttingActive(bool active);
    void updateButtonStates(uint16_t cutPressureButtonId, uint16_t feedRateButtonId);

private:
    // Display object IDs
    uint16_t _cutPressureLedId;
    uint16_t _targetFeedRateLedId;
    uint16_t _torqueGaugeId;
    uint16_t _liveFeedRateLedId;

    // Current adjustment mode
    AdjustmentMode _currentMode;

    // Temporary values during adjustment
    float _tempCutPressure;
    float _tempFeedRate;

    // Encoder tracking
    int32_t _lastEncoderPos;

    // State tracking
    bool _isCuttingActive;

    // Constants for adjustment
    static constexpr float CUT_PRESSURE_INCREMENT = 0.5f;
    static constexpr float FEED_RATE_INCREMENT = 0.01f;
    static constexpr float MIN_CUT_PRESSURE = 10.0f;
    static constexpr float MAX_CUT_PRESSURE = 100.0f;
    static constexpr float MIN_FEED_RATE = 0.1f;
    static constexpr float MAX_FEED_RATE = 1.0f;

    // Helper methods
    void updateCutPressureDisplay();
    void updateFeedRateDisplay();
    void updateTorqueGauge();
    void handleEncoderInput();
    void applyLiveAdjustments();
};
