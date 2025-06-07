#include "SemiAutoScreen.h"
#include "screenmanager.h" 
#include "MotionController.h"
#include "UIInputManager.h"
#include "MPGJogManager.h"
#include "SettingsManager.h" 
#include "JogUtilities.h"  // Added include for JogUtilities
#include <ClearCore.h>
#include "Config.h" // Ensure LEDDIGITS_FEED_OVERRIDE and LEDDIGITS_DISTANCE_TO_GO_F2 are available

#ifndef GENIE_OBJ_LED_DIGITS
#define GENIE_OBJ_LED_DIGITS 15
#endif

extern Genie genie;

// NOTE: Ensure STATE_PAUSED is defined in your header file (e.g., in your SemiAutoScreenState enum)
// enum { STATE_READY, STATE_CUTTING, STATE_PAUSED, STATE_ADJUSTING_PRESSURE };

SemiAutoScreen::SemiAutoScreen(ScreenManager& mgr) : _mgr(mgr) {}

void SemiAutoScreen::onShow() {
    // Just clean up fields
    UIInputManager::Instance().unbindField();

    // Set spindle button state
    auto& mc = MotionController::Instance();
    bool spindleActive = mc.IsSpindleRunning();
    genie.WriteObject(GENIE_OBJ_WINBUTTON, WINBUTTON_SPINDLE_ON, spindleActive ? 1 : 0);

    // Reset to ready state
    _currentState = STATE_READY;
    genie.WriteObject(GENIE_OBJ_LED, LED_READY, 1);
    genie.WriteObject(GENIE_OBJ_WINBUTTON, WINBUTTON_FEED_TO_STOP, 0);
    genie.WriteObject(GENIE_OBJ_WINBUTTON, WINBUTTON_FEED_HOLD, 0);
    genie.WriteObject(GENIE_OBJ_WINBUTTON, WINBUTTON_EXIT_FEED_HOLD, 0);
    genie.WriteObject(GENIE_OBJ_WINBUTTON, WINBUTTON_ADJUST_CUT_PRESSURE, 0);

    // Display the current cut pressure setting
#ifdef SETTINGS_HAS_CUT_PRESSURE
    float cutPressure = SettingsManager::Instance().settings().cutPressure;
#else
    float cutPressure = 70.0f;
#endif
    genie.WriteObject(GENIE_OBJ_LED_DIGITS, LEDDIGITS_CUT_PRESSURE,
        static_cast<uint16_t>(cutPressure * 10.0f)); // Scale as needed

    // Initialize the gauge to show the target torque
    genie.WriteObject(GENIE_OBJ_IGAUGE, IGAUGE_SEMIAUTO_CUT_PRESSURE,
        static_cast<uint16_t>(0)); // Start at zero

    // Get the thickness directly from CutData
    auto& cutData = _mgr.GetCutData();
    UpdateThicknessLed(cutData.thickness);
}

void SemiAutoScreen::onHide() {
    // Stop any ongoing operations
    if (_currentState == STATE_CUTTING) {
        MotionController::Instance().abortTorqueControlledFeed(AXIS_Y);
    }
    // If adjusting cut pressure, exit that mode
    if (_currentState == STATE_ADJUSTING_PRESSURE) {
        MPGJogManager::Instance().setEnabled(false);
    }

    UIInputManager::Instance().unbindField();

    // Clear all visual indicators
    genie.WriteObject(GENIE_OBJ_LED, LED_READY, 0);
    genie.WriteObject(GENIE_OBJ_WINBUTTON, WINBUTTON_FEED_TO_STOP, 0);
    genie.WriteObject(GENIE_OBJ_WINBUTTON, WINBUTTON_FEED_HOLD, 0);
    genie.WriteObject(GENIE_OBJ_WINBUTTON, WINBUTTON_EXIT_FEED_HOLD, 0);
    genie.WriteObject(GENIE_OBJ_WINBUTTON, WINBUTTON_ADJUST_CUT_PRESSURE, 0);
}

void SemiAutoScreen::startFeedToStop() {
    auto& motion = MotionController::Instance();
    auto& cutData = _mgr.GetCutData();

    // Store the current Y position as the feed start position BEFORE any movement
    float startPos = motion.getAbsoluteAxisPosition(AXIS_Y);
    _feedHoldManager.setStartPosition(startPos);

    ClearCore::ConnectorUsb.Send("[SemiAuto] Setting initial feed start position: ");
    ClearCore::ConnectorUsb.SendLine(startPos);

    // Get settings (using correct settings() method)
    auto& settings = SettingsManager::Instance().settings();
    float feedRate = settings.feedRate / 25.0f; // Scale to 0-1 range (assuming max feed is 25)

    // Use cutPressure if available, otherwise default to 70%
    float torqueTarget = 70.0f; // Default torque target
#ifdef SETTINGS_HAS_CUT_PRESSURE
    torqueTarget = settings.cutPressure;
#endif

    // Store the target pressure so we can display it consistently
    _tempCutPressure = torqueTarget;

    // Ensure feedRate is within reasonable bounds
    if (feedRate < 0.1f) feedRate = 0.1f;
    if (feedRate > 1.0f) feedRate = 1.0f;

    // Set torque target
    motion.setTorqueTarget(AXIS_Y, torqueTarget);

    // Log the operation
    ClearCore::ConnectorUsb.Send("[SemiAuto] Starting feed to ");
    ClearCore::ConnectorUsb.Send(cutData.cutEndPoint);
    ClearCore::ConnectorUsb.Send(" inches with torque target ");
    ClearCore::ConnectorUsb.Send(torqueTarget);
    ClearCore::ConnectorUsb.SendLine("%");

    // Start torque-controlled feed to the cut end position
    if (motion.startTorqueControlledFeed(AXIS_Y, cutData.cutEndPoint, feedRate)) {
        _currentState = STATE_CUTTING;

        // Update UI to show cutting state
        genie.WriteObject(GENIE_OBJ_LED, LED_READY, 0); // Turn off ready LED
        genie.WriteObject(GENIE_OBJ_WINBUTTON, WINBUTTON_FEED_TO_STOP, 1); // Highlight feed button
        genie.WriteObject(GENIE_OBJ_WINBUTTON, WINBUTTON_FEED_HOLD, 0); // Reset feed hold button
        genie.WriteObject(GENIE_OBJ_WINBUTTON, WINBUTTON_EXIT_FEED_HOLD, 0); // Hide exit button initially

        // Reset the cut pressure adjustment button
        genie.WriteObject(GENIE_OBJ_WINBUTTON, WINBUTTON_ADJUST_CUT_PRESSURE, 0);

        // Initialize gauge with a reasonable value based on target
        genie.WriteObject(GENIE_OBJ_IGAUGE, IGAUGE_SEMIAUTO_CUT_PRESSURE, 50); // Start at 50% to avoid red

        // Make sure the cut pressure display shows the target value
        genie.WriteObject(GENIE_OBJ_LED_DIGITS, LEDDIGITS_CUT_PRESSURE,
            static_cast<uint16_t>(_tempCutPressure * 10.0f)); // Scale as needed
    }
}

void SemiAutoScreen::feedHold() {
    // First, ensure the exit button is visible BEFORE toggling pause state
    // This helps prevent the red X issue by setting button state early
    genie.WriteObject(GENIE_OBJ_WINBUTTON, WINBUTTON_EXIT_FEED_HOLD, 1);
    delay(20);  // Small delay to let UI settle

    // Delegate to FeedHoldManager for pause/resume logic
    _feedHoldManager.toggleFeedHold();

    // Update UI based on paused state
    if (_feedHoldManager.isPaused()) {
        _currentState = STATE_PAUSED;

        // Set feed hold button active first
        genie.WriteObject(GENIE_OBJ_WINBUTTON, WINBUTTON_FEED_HOLD, 1);

        // Make another attempt at ensuring exit button is visible
        delay(20); // Small delay for UI stability
        genie.WriteObject(GENIE_OBJ_WINBUTTON, WINBUTTON_EXIT_FEED_HOLD, 1);

        ClearCore::ConnectorUsb.SendLine("[SemiAuto] Feed paused");
    }
    else {
        _currentState = STATE_CUTTING;
        genie.WriteObject(GENIE_OBJ_WINBUTTON, WINBUTTON_FEED_HOLD, 0);

        ClearCore::ConnectorUsb.SendLine("[SemiAuto] Feed resumed");
    }
}

void SemiAutoScreen::exitFeedHold() {
    // Log the start position for verification
    float returnPos = _feedHoldManager.getStartPosition();
    ClearCore::ConnectorUsb.Send("[SemiAuto] Exit feed hold, returning to position: ");
    ClearCore::ConnectorUsb.SendLine(returnPos);

    // Delegate to FeedHoldManager for abort/return logic
    _feedHoldManager.exitFeedHold();

    // Update UI to reflect ready state - ENSURE FEED_TO_STOP IS RESET!
    _currentState = STATE_READY;
    genie.WriteObject(GENIE_OBJ_WINBUTTON, WINBUTTON_FEED_TO_STOP, 0); // Reset feed button - important fix!
    genie.WriteObject(GENIE_OBJ_WINBUTTON, WINBUTTON_FEED_HOLD, 0);   // Reset feed hold button
    genie.WriteObject(GENIE_OBJ_WINBUTTON, WINBUTTON_EXIT_FEED_HOLD, 0); // Hide exit button
    genie.WriteObject(GENIE_OBJ_LED, LED_READY, 1); // Turn on ready LED
}

void SemiAutoScreen::adjustCutPressure() {
    // Toggle between normal mode and adjusting cut pressure
    if (_currentState == STATE_ADJUSTING_PRESSURE) {
        // Exit cut pressure adjustment mode
        _currentState = STATE_READY;
        MPGJogManager::Instance().setEnabled(false);

        // Save the adjusted value to settings
#ifdef SETTINGS_HAS_CUT_PRESSURE
        SettingsManager::Instance().settings().cutPressure = _tempCutPressure;
        SettingsManager::Instance().save();
#endif

        // Update the torque target if cutting is active
        if (MotionController::Instance().isInTorqueControlledFeed(AXIS_Y)) {
            MotionController::Instance().setTorqueTarget(AXIS_Y, _tempCutPressure);
        }

        // Reset UI
        genie.WriteObject(GENIE_OBJ_WINBUTTON, WINBUTTON_ADJUST_CUT_PRESSURE, 0);
        genie.WriteObject(GENIE_OBJ_LED, LED_READY, 1);

        ClearCore::ConnectorUsb.Send("[SemiAuto] Cut pressure set to: ");
        ClearCore::ConnectorUsb.SendLine(_tempCutPressure);
    }
    else if (_currentState == STATE_READY) {
        // Enter cut pressure adjustment mode
        _currentState = STATE_ADJUSTING_PRESSURE;

#ifdef SETTINGS_HAS_CUT_PRESSURE
        _tempCutPressure = SettingsManager::Instance().settings().cutPressure;
#else
        _tempCutPressure = 70.0f; // Default
#endif

        // Update UI: highlight adjustment button and turn off ready LED
        genie.WriteObject(GENIE_OBJ_WINBUTTON, WINBUTTON_ADJUST_CUT_PRESSURE, 1);
        genie.WriteObject(GENIE_OBJ_LED, LED_READY, 0);

        // Update display to show current value (scaled by 10 for display)
        genie.WriteObject(GENIE_OBJ_LED_DIGITS, LEDDIGITS_CUT_PRESSURE,
            static_cast<uint16_t>(_tempCutPressure * 10.0f));

        // Reset encoder tracking for clean start
        UIInputManager::Instance().resetRaw();

        ClearCore::ConnectorUsb.Send("[SemiAuto] Adjusting cut pressure, current value: ");
        ClearCore::ConnectorUsb.SendLine(_tempCutPressure);
    }
}

void SemiAutoScreen::advanceIncrement() {
    // Only handle increment in cut pressure adjustment mode
    if (_currentState == STATE_ADJUSTING_PRESSURE) {
        _tempCutPressure += CUT_PRESSURE_INCREMENT;
        if (_tempCutPressure > MAX_CUT_PRESSURE) {
            _tempCutPressure = MAX_CUT_PRESSURE;
        }

        // Update display
        genie.WriteObject(GENIE_OBJ_LED_DIGITS, LEDDIGITS_CUT_PRESSURE,
            static_cast<uint16_t>(_tempCutPressure * 10.0f)); // Scale for display
    }
}

void SemiAutoScreen::update() {
    auto& motion = MotionController::Instance();

    // Update RPM display
    uint16_t rpm = motion.IsSpindleRunning() ? (uint16_t)motion.CommandedRPM() : 0;
    genie.WriteObject(GENIE_OBJ_LED_DIGITS, LEDDIGITS_RPM_DISPLAY, rpm);

    // Get access to CutData
    auto& cutData = _mgr.GetCutData();

    // Update thickness display if it has changed
    if (m_lastThickness != cutData.thickness) {
        UpdateThicknessLed(cutData.thickness);
    }

    // --- Distance to Go: Show remaining distance to cut end point (counts down to zero) ---
    float currentPos = motion.getAbsoluteAxisPosition(AXIS_Y);
    float distanceToGo = cutData.cutEndPoint - currentPos;
    if (distanceToGo < 0.0f) distanceToGo = 0.0f; // Clamp at zero if past endpoint
    genie.WriteObject(GENIE_OBJ_LED_DIGITS, LEDDIGITS_DISTANCE_TO_GO_F2,
        static_cast<uint16_t>(distanceToGo * 1000));

    // --- Feed Rate Display: Show real-time feed rate during torque-controlled cycle ---
    if (motion.isInTorqueControlledFeed(AXIS_Y)) {
        // Get current feed rate (0.0-1.0), scale to percent for display
        float currentFeedRate = MotionController::Instance().YAxisInstance().DebugGetCurrentFeedRate();
        genie.WriteObject(GENIE_OBJ_LED_DIGITS, LEDDIGITS_FEED_OVERRIDE,
            static_cast<uint16_t>(currentFeedRate * 100.0f)); // Show as percent
    }
    else {
        // Not in torque feed: clear or show zero
        genie.WriteObject(GENIE_OBJ_LED_DIGITS, LEDDIGITS_FEED_OVERRIDE, 0);
    }

    // If in cutting state or paused, update torque display
    if (_currentState == STATE_CUTTING || _currentState == STATE_PAUSED) {
        float torque = motion.getTorquePercent(AXIS_Y);
        float targetTorque = motion.getTorqueTarget(AXIS_Y);
        float torquePercentage = (torque / targetTorque) * 100.0f;

        if (torquePercentage > 125.0f) torquePercentage = 125.0f;
        if (torquePercentage < 0.0f) torquePercentage = 0.0f;

        uint16_t gaugeValue = static_cast<uint16_t>(torquePercentage);
        genie.WriteObject(GENIE_OBJ_IGAUGE, IGAUGE_SEMIAUTO_CUT_PRESSURE, gaugeValue);
        genie.WriteObject(GENIE_OBJ_LED_DIGITS, LEDDIGITS_CUT_PRESSURE,
            static_cast<uint16_t>(targetTorque * 10.0f)); // Always show target pressure

        // Check if feed completed
        if (!motion.isInTorqueControlledFeed(AXIS_Y) && !motion.isAxisMoving(AXIS_Y)) {
            _currentState = STATE_READY;
            genie.WriteObject(GENIE_OBJ_LED, LED_READY, 1); // Turn on ready LED
            genie.WriteObject(GENIE_OBJ_WINBUTTON, WINBUTTON_FEED_TO_STOP, 0); // Reset feed button
            genie.WriteObject(GENIE_OBJ_WINBUTTON, WINBUTTON_EXIT_FEED_HOLD, 0); // Hide exit button

#ifdef SETTINGS_HAS_CUT_PRESSURE
            float cutPressure = SettingsManager::Instance().settings().cutPressure;
#else
            float cutPressure = 70.0f;
#endif
            genie.WriteObject(GENIE_OBJ_LED_DIGITS, LEDDIGITS_CUT_PRESSURE,
                static_cast<uint16_t>(cutPressure * 10.0f));
        }
    }

    // Special handling for STATE_PAUSED - ensure exit button stays visible
    if (_currentState == STATE_PAUSED) {
        static uint32_t lastExitButtonCheck = 0;
        uint32_t now = ClearCore::TimingMgr.Milliseconds();

        // Periodically check and refresh exit button to prevent red X
        if (now - lastExitButtonCheck > 250) { // Four times per second for more reliability
            lastExitButtonCheck = now;
            // Force exit button to remain visible
            genie.WriteObject(GENIE_OBJ_WINBUTTON, WINBUTTON_EXIT_FEED_HOLD, 1);
        }
    }

    // Else if in adjusting pressure mode, check for encoder movement directly
    else if (_currentState == STATE_ADJUSTING_PRESSURE) {
        static int32_t lastEncoderPos = ClearCore::EncoderIn.Position();
        int32_t currentEncoderPos = ClearCore::EncoderIn.Position();
        int32_t delta = currentEncoderPos - lastEncoderPos;

        if (abs(delta) >= ENCODER_COUNTS_PER_CLICK) {
            lastEncoderPos = currentEncoderPos;
            float changeAmount = (delta > 0) ? CUT_PRESSURE_INCREMENT : -CUT_PRESSURE_INCREMENT;
            _tempCutPressure += changeAmount;

            if (_tempCutPressure < MIN_CUT_PRESSURE) _tempCutPressure = MIN_CUT_PRESSURE;
            if (_tempCutPressure > MAX_CUT_PRESSURE) _tempCutPressure = MAX_CUT_PRESSURE;

            ClearCore::ConnectorUsb.Send("[SemiAuto] Cut pressure adjusted to: ");
            ClearCore::ConnectorUsb.SendLine(_tempCutPressure);

            genie.WriteObject(GENIE_OBJ_LED_DIGITS, LEDDIGITS_CUT_PRESSURE,
                static_cast<uint16_t>(_tempCutPressure * 10.0f));

#ifdef SETTINGS_HAS_CUT_PRESSURE
            auto& settings = SettingsManager::Instance().settings();
            settings.cutPressure = _tempCutPressure;
#endif

            if (motion.isInTorqueControlledFeed(AXIS_Y)) {
                motion.setTorqueTarget(AXIS_Y, _tempCutPressure);
            }
        }
    }
}

void SemiAutoScreen::handleEvent(const genieFrame& e) {
    if (e.reportObject.cmd != GENIE_REPORT_EVENT) {
        return;
    }

    switch (e.reportObject.object) {
    case GENIE_OBJ_WINBUTTON:
        switch (e.reportObject.index) {
        case WINBUTTON_FEED_TO_STOP:
            if (_currentState == STATE_READY) {
                startFeedToStop();
            }
            break;

        case WINBUTTON_FEED_HOLD:
            if (_currentState == STATE_CUTTING || _currentState == STATE_PAUSED) {
                feedHold();
            }
            break;

        case WINBUTTON_EXIT_FEED_HOLD:
            // Automatically use FeedHoldManager's state rather than UI state
            if (_feedHoldManager.isPaused()) {
                exitFeedHold();
            }
            break;

        case WINBUTTON_ADJUST_CUT_PRESSURE:
            if (_currentState == STATE_READY || _currentState == STATE_ADJUSTING_PRESSURE) {
                adjustCutPressure();
            }
            break;

        case WINBUTTON_SPINDLE_ON:
            if (MotionController::Instance().IsSpindleRunning()) {
                MotionController::Instance().StopSpindle();
                genie.WriteObject(GENIE_OBJ_WINBUTTON, WINBUTTON_SPINDLE_ON, 0);
            }
            else {
                float rpm = 3000.0f;
#ifdef SETTINGS_HAS_SPINDLE_RPM
                rpm = SettingsManager::Instance().settings().spindleRPM;
#else
                rpm = SettingsManager::Instance().settings().defaultRPM;
#endif
                MotionController::Instance().StartSpindle(rpm);
                genie.WriteObject(GENIE_OBJ_WINBUTTON, WINBUTTON_SPINDLE_ON, 1);
            }
            break;

        case WINBUTTON_INC_PLUS_F2:
            JogUtilities::Increment(_mgr.GetCutData(), AXIS_X);
            break;

        case WINBUTTON_INC_MINUS_F2:
            if (_currentState == STATE_ADJUSTING_PRESSURE) {
                _tempCutPressure -= CUT_PRESSURE_INCREMENT;
                if (_tempCutPressure < MIN_CUT_PRESSURE) {
                    _tempCutPressure = MIN_CUT_PRESSURE;
                }
                genie.WriteObject(GENIE_OBJ_LED_DIGITS, LEDDIGITS_CUT_PRESSURE,
                    static_cast<uint16_t>(_tempCutPressure * 10.0f));
#ifdef SETTINGS_HAS_CUT_PRESSURE
                SettingsManager::Instance().settings().cutPressure = _tempCutPressure;
#endif
            }
            else {
                JogUtilities::Decrement(_mgr.GetCutData(), AXIS_X);
            }
            break;

        case WINBUTTON_SETTINGS_SEMI:
            ScreenManager::Instance().ShowSettings();
            break;

        case WINBUTTON_HOME_F2:
            JogUtilities::GoToZero(_mgr.GetCutData(), AXIS_X);
            break;

        case WINBUTTON_STOCK_ZERO:
            JogUtilities::GoToZero(_mgr.GetCutData(), AXIS_X);
            showButtonSafe(WINBUTTON_STOCK_ZERO, 1);
            delay(200);
            showButtonSafe(WINBUTTON_STOCK_ZERO, 0);
            break;
        }
        break;
    }
}

void SemiAutoScreen::UpdateThicknessLed(float thickness) {
    m_lastThickness = thickness;
    int32_t scaled = static_cast<int32_t>(round(thickness * 1000.0f));
    genie.WriteObject(GENIE_OBJ_LED_DIGITS, LEDDIGITS_THICKNESS_F2, static_cast<uint16_t>(scaled));
}
