#include "SemiAutoScreen.h"
#include "screenmanager.h" 
#include "MotionController.h"
#include "UIInputManager.h"
#include "MPGJogManager.h"
#include "SettingsManager.h" 
#include "JogUtilities.h"
#include <ClearCore.h>
#include "Config.h"

#ifndef GENIE_OBJ_LED_DIGITS
#define GENIE_OBJ_LED_DIGITS 15
#endif

extern Genie genie;

SemiAutoScreen::SemiAutoScreen(ScreenManager& mgr)
    : _mgr(mgr), _spindleLoadMeter(IGAUGE_SEMIAUTO_LOAD_METER) {
}

void SemiAutoScreen::updateButtonState(uint16_t buttonId, bool state, const char* logMessage, uint16_t delayMs) {
    showButtonSafe(buttonId, state ? 1 : 0, delayMs);

    if (logMessage) {
        ClearCore::ConnectorUsb.SendLine(logMessage);
    }
}

void SemiAutoScreen::onShow() {
    // Clean up fields
    UIInputManager::Instance().unbindField();

    // Initialize torque control UI with Form2 display IDs
    _torqueControlUI.init(
        LEDDIGITS_CUT_PRESSURE,         // Cut pressure display
        LEDDIGITS_TARGET_FEEDRATE,      // Target feed rate display  
        IGAUGE_SEMIAUTO_CUT_PRESSURE,   // Torque gauge
        LEDDIGITS_FEED_OVERRIDE         // Live feed rate display
    );

    _torqueControlUI.onShow();

    // Set spindle button state
    auto& mc = MotionController::Instance();
    bool spindleActive = mc.IsSpindleRunning();
    genie.WriteObject(GENIE_OBJ_WINBUTTON, WINBUTTON_SPINDLE_ON, spindleActive ? 1 : 0);

    // Reset to ready state
    _currentState = STATE_READY;
    _isReturningToStart = false;
    genie.WriteObject(GENIE_OBJ_LED, LED_READY, 1);
    genie.WriteObject(GENIE_OBJ_WINBUTTON, WINBUTTON_FEED_TO_STOP, 0);
    genie.WriteObject(GENIE_OBJ_WINBUTTON, WINBUTTON_FEED_HOLD, 0);
    genie.WriteObject(GENIE_OBJ_WINBUTTON, WINBUTTON_EXIT_FEED_HOLD, 0);
    genie.WriteObject(GENIE_OBJ_WINBUTTON, WINBUTTON_ADJUST_CUT_PRESSURE, 0);
    genie.WriteObject(GENIE_OBJ_WINBUTTON, WINBUTTON_ADJUST_MAX_SPEED, 0);

    // Get the thickness directly from CutData
    auto& cutData = _mgr.GetCutData();
    UpdateThicknessLed(cutData.thickness);
}

void SemiAutoScreen::onHide() {
    // Stop any ongoing operations
    if (_currentState == STATE_CUTTING || _currentState == STATE_PAUSED || _currentState == STATE_RETURNING) {
        MotionController::Instance().abortTorqueControlledFeed(AXIS_Y);
    }

    // Let torque control UI clean up
    _torqueControlUI.onHide();

    // If adjusting cut pressure or feed rate through MPG, disable it
    if (_currentState == STATE_ADJUSTING_PRESSURE || _currentState == STATE_ADJUSTING_FEED_RATE) {
        MPGJogManager::Instance().setEnabled(false);
    }

    UIInputManager::Instance().unbindField();

    // Clear all visual indicators
    genie.WriteObject(GENIE_OBJ_LED, LED_READY, 0);
    genie.WriteObject(GENIE_OBJ_WINBUTTON, WINBUTTON_FEED_TO_STOP, 0);
    genie.WriteObject(GENIE_OBJ_WINBUTTON, WINBUTTON_FEED_HOLD, 0);
    genie.WriteObject(GENIE_OBJ_WINBUTTON, WINBUTTON_EXIT_FEED_HOLD, 0);
    genie.WriteObject(GENIE_OBJ_WINBUTTON, WINBUTTON_ADJUST_CUT_PRESSURE, 0);
    genie.WriteObject(GENIE_OBJ_WINBUTTON, WINBUTTON_ADJUST_MAX_SPEED, 0);
}

void SemiAutoScreen::startFeedToStop() {
    auto& motion = MotionController::Instance();
    auto& cutData = _mgr.GetCutData();

    // Store the current Y position as the feed start position BEFORE any movement
    float startPos = motion.getAbsoluteAxisPosition(AXIS_Y);
    _feedHoldManager.setStartPosition(startPos);

    ClearCore::ConnectorUsb.Send("[SemiAuto] Setting initial feed start position: ");
    ClearCore::ConnectorUsb.SendLine(startPos);

    // Get current values from torque control UI
    float feedRate = _torqueControlUI.getCurrentFeedRate();
    float torqueTarget = _torqueControlUI.getCurrentCutPressure();

    // Notify torque control UI that cutting is active
    _torqueControlUI.setCuttingActive(true);

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

        // Update UI to show cutting state - use safe update methods for buttons
        genie.WriteObject(GENIE_OBJ_LED, LED_READY, 0); // Turn off ready LED
        updateButtonState(WINBUTTON_FEED_TO_STOP, true, nullptr, 10);
        updateButtonState(WINBUTTON_FEED_HOLD, false, nullptr, 10);
        updateButtonState(WINBUTTON_EXIT_FEED_HOLD, false, nullptr, 10);

        // Only disable adjustment buttons if not currently adjusting
        if (!_torqueControlUI.isAdjusting()) {
            updateButtonState(WINBUTTON_ADJUST_CUT_PRESSURE, false, nullptr, 10);
            updateButtonState(WINBUTTON_ADJUST_MAX_SPEED, false, nullptr, 10);
        }

        // Initialize gauge with a reasonable value based on target
        genie.WriteObject(GENIE_OBJ_IGAUGE, IGAUGE_SEMIAUTO_CUT_PRESSURE, 50); // Start at 50% to avoid red
    }
}

void SemiAutoScreen::feedHold() {
    // Store current state before changing it
    SemiAutoScreenState previousState = _currentState;

    // Delegate to FeedHoldManager for pause/resume logic first
    _feedHoldManager.toggleFeedHold();

    // Update UI based on the resulting state
    if (_feedHoldManager.isPaused()) {
        // Only change state if we're not in an adjustment state
        if (previousState != STATE_ADJUSTING_PRESSURE && previousState != STATE_ADJUSTING_FEED_RATE) {
            _currentState = STATE_PAUSED;
        }
        else {
            ClearCore::ConnectorUsb.SendLine("[SemiAuto] Pausing while maintaining adjustment mode");
        }

        // Set feed hold button active
        updateButtonState(WINBUTTON_FEED_HOLD, true, "[SemiAuto] Feed paused", 20);

        // Then show exit button (inactive state for latching button)
        updateButtonState(WINBUTTON_EXIT_FEED_HOLD, false, nullptr, 50);
    }
    else {
        // If we're resuming and were in an adjustment state, store that knowledge
        bool wasAdjusting = (_currentState == STATE_ADJUSTING_PRESSURE ||
            _currentState == STATE_ADJUSTING_FEED_RATE);

        _currentState = STATE_CUTTING;
        updateButtonState(WINBUTTON_FEED_HOLD, false, "[SemiAuto] Feed resumed", 20);

        // Hide exit button when resumed
        updateButtonState(WINBUTTON_EXIT_FEED_HOLD, false, nullptr, 0);

        // Restore adjustment button state if we were adjusting
        if (wasAdjusting) {
            _torqueControlUI.updateButtonStates(
                WINBUTTON_ADJUST_CUT_PRESSURE,
                WINBUTTON_ADJUST_MAX_SPEED
            );
        }
    }
}

void SemiAutoScreen::exitFeedHold() {
    // When exit button is pressed, set returning flag
    _isReturningToStart = true;
    _currentState = STATE_RETURNING;

    // Log the start position for verification
    float returnPos = _feedHoldManager.getStartPosition();
    ClearCore::ConnectorUsb.Send("[SemiAuto] Exit feed hold, returning to position: ");
    ClearCore::ConnectorUsb.SendLine(returnPos);

    // The exit button should already be in active state (1) from the user pressing it
    // Just ensure it stays visible during return
    updateButtonState(WINBUTTON_EXIT_FEED_HOLD, true, nullptr, 20);

    // Delegate to FeedHoldManager for abort/return logic
    _feedHoldManager.exitFeedHold();
}

void SemiAutoScreen::adjustCutPressure() {
    // Delegate to TorqueControlUI
    _torqueControlUI.toggleCutPressureAdjustment();

    // Update our state based on the new mode
    auto newMode = _torqueControlUI.getCurrentMode();
    if (newMode == TorqueControlUI::ADJUSTMENT_CUT_PRESSURE) {
        _currentState = STATE_ADJUSTING_PRESSURE;
        genie.WriteObject(GENIE_OBJ_LED, LED_READY, 0);
    }
    else {
        _currentState = STATE_READY;
        genie.WriteObject(GENIE_OBJ_LED, LED_READY, 1);
    }

    // Update button states
    _torqueControlUI.updateButtonStates(
        WINBUTTON_ADJUST_CUT_PRESSURE,
        WINBUTTON_ADJUST_MAX_SPEED
    );
}

void SemiAutoScreen::adjustMaxFeedRate() {
    // Delegate to TorqueControlUI
    _torqueControlUI.toggleFeedRateAdjustment();

    // Update our state based on the new mode
    auto newMode = _torqueControlUI.getCurrentMode();
    if (newMode == TorqueControlUI::ADJUSTMENT_FEED_RATE) {
        _currentState = STATE_ADJUSTING_FEED_RATE;
        genie.WriteObject(GENIE_OBJ_LED, LED_READY, 0);
    }
    else {
        _currentState = STATE_READY;
        genie.WriteObject(GENIE_OBJ_LED, LED_READY, 1);
    }

    // Update button states
    _torqueControlUI.updateButtonStates(
        WINBUTTON_ADJUST_CUT_PRESSURE,
        WINBUTTON_ADJUST_MAX_SPEED
    );
}

void SemiAutoScreen::advanceIncrement() {
    // Handle increment based on whether torque UI is adjusting
    if (_torqueControlUI.isAdjusting()) {
        _torqueControlUI.incrementValue();
    }
    else {
        JogUtilities::Increment(_mgr.GetCutData(), AXIS_X);
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
            else if (_currentState == STATE_CUTTING || _currentState == STATE_PAUSED || _currentState == STATE_RETURNING) {
                // If button is pressed during active cycle, force it back to active state
                updateButtonState(WINBUTTON_FEED_TO_STOP, true, "[SemiAuto] Feed cycle in progress", 0);
            }
            break;

        case WINBUTTON_FEED_HOLD:
            if (_currentState == STATE_CUTTING || _currentState == STATE_PAUSED ||
                _currentState == STATE_ADJUSTING_PRESSURE || _currentState == STATE_ADJUSTING_FEED_RATE) {
                // Save current adjustment state if needed
                SemiAutoScreenState previousState = _currentState;

                // Apply feed hold action
                feedHold();

                // If we were in an adjustment state, preserve it while paused
                if ((previousState == STATE_ADJUSTING_PRESSURE || previousState == STATE_ADJUSTING_FEED_RATE)
                    && _currentState == STATE_PAUSED) {
                    _currentState = previousState;

                    // Make sure appropriate adjustment button stays active
                    _torqueControlUI.updateButtonStates(
                        WINBUTTON_ADJUST_CUT_PRESSURE,
                        WINBUTTON_ADJUST_MAX_SPEED
                    );

                    ClearCore::ConnectorUsb.SendLine("[SemiAuto] Maintaining adjustment mode while paused");
                }
            }
            else {
                // If button is pressed when not in a valid state, force it back to its proper state
                bool shouldBeActive = (_currentState == STATE_PAUSED);
                updateButtonState(WINBUTTON_FEED_HOLD, shouldBeActive, "[SemiAuto] Feed hold not available", 0);
            }
            break;

        case WINBUTTON_EXIT_FEED_HOLD:
            if (_feedHoldManager.isPaused() && !_isReturningToStart) {
                exitFeedHold(); // Normal operation
            }
            else {
                // If button is pressed when not paused or already returning, force back to proper state
                bool shouldBeActive = _isReturningToStart;
                updateButtonState(WINBUTTON_EXIT_FEED_HOLD, shouldBeActive, "[SemiAuto] Exit feed not available", 0);
            }
            break;

        case WINBUTTON_ADJUST_CUT_PRESSURE:
            adjustCutPressure();
            break;

        case WINBUTTON_ADJUST_MAX_SPEED:
            adjustMaxFeedRate();
            break;

        case WINBUTTON_SPINDLE_ON:
            if (MotionController::Instance().IsSpindleRunning()) {
                // Stop the spindle
                MotionController::Instance().StopSpindle();
                updateButtonState(WINBUTTON_SPINDLE_ON, false, "[SemiAuto] Spindle stopped", 0);
            }
            else {
                // Get RPM from settings
                float rpm = SettingsManager::Instance().settings().spindleRPM;

                ClearCore::ConnectorUsb.Send("[SemiAuto] Starting spindle at rpm: ");
                ClearCore::ConnectorUsb.SendLine(rpm);

                // Start the spindle with the RPM from settings
                MotionController::Instance().StartSpindle(rpm);
                updateButtonState(WINBUTTON_SPINDLE_ON, true, "[SemiAuto] Spindle started", 0);
            }
            break;

        case WINBUTTON_INC_PLUS_F2:
            advanceIncrement();
            break;

        case WINBUTTON_INC_MINUS_F2:
            if (_torqueControlUI.isAdjusting()) {
                _torqueControlUI.decrementValue();
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

void SemiAutoScreen::update() {
    auto& motion = MotionController::Instance();
    auto& cutData = _mgr.GetCutData();

    // Update torque control UI (handles gauge, encoder, displays)
    _torqueControlUI.update();

    // Update RPM display
    uint16_t rpm = motion.IsSpindleRunning() ? (uint16_t)motion.CommandedRPM() : 0;
    genie.WriteObject(GENIE_OBJ_LED_DIGITS, LEDDIGITS_RPM_DISPLAY, rpm);

    // Update the spindle load meter
    _spindleLoadMeter.Update();

    // Update thickness display if it has changed
    if (m_lastThickness != cutData.thickness) {
        UpdateThicknessLed(cutData.thickness);
    }

    // Distance to Go: Show remaining distance to cut end point
    float currentPos = motion.getAbsoluteAxisPosition(AXIS_Y);
    float distanceToGo = cutData.cutEndPoint - currentPos;
    if (distanceToGo < 0.0f) distanceToGo = 0.0f;
    genie.WriteObject(GENIE_OBJ_LED_DIGITS, LEDDIGITS_DISTANCE_TO_GO_F2,
        static_cast<uint16_t>(distanceToGo * 1000));

    // Check for feed cycle completion
    static bool wasCuttingOrPaused = false;

    if (_currentState == STATE_CUTTING || _currentState == STATE_PAUSED) {
        wasCuttingOrPaused = true;
    }

    if (wasCuttingOrPaused && !motion.isInTorqueControlledFeed(AXIS_Y) && !motion.isAxisMoving(AXIS_Y)) {
        wasCuttingOrPaused = false;

        // Exit any adjustment mode on completion
        if (_torqueControlUI.isAdjusting()) {
            _torqueControlUI.exitAdjustmentMode();
            _torqueControlUI.updateButtonStates(
                WINBUTTON_ADJUST_CUT_PRESSURE,
                WINBUTTON_ADJUST_MAX_SPEED
            );
        }

        // Notify torque control UI that cutting is no longer active
        _torqueControlUI.setCuttingActive(false);

        // Set state to ready
        _currentState = STATE_READY;
        genie.WriteObject(GENIE_OBJ_LED, LED_READY, 1);

        // Reset all buttons
        updateButtonState(WINBUTTON_FEED_TO_STOP, false, "[SemiAuto] Feed cycle completed", 10);
        updateButtonState(WINBUTTON_EXIT_FEED_HOLD, false, nullptr, 10);
        updateButtonState(WINBUTTON_FEED_HOLD, false, nullptr, 10);

        ClearCore::ConnectorUsb.SendLine("[SemiAuto] Feed cycle completed, all states reset");
    }

    // Monitor return motion completion
    if (_isReturningToStart) {
        if (!motion.isInTorqueControlledFeed(AXIS_Y) && !motion.isAxisMoving(AXIS_Y)) {
            _isReturningToStart = false;

            // Exit any adjustment mode
            if (_torqueControlUI.isAdjusting()) {
                _torqueControlUI.exitAdjustmentMode();
                _torqueControlUI.updateButtonStates(
                    WINBUTTON_ADJUST_CUT_PRESSURE,
                    WINBUTTON_ADJUST_MAX_SPEED
                );
            }

            _currentState = STATE_READY;
            updateButtonState(WINBUTTON_EXIT_FEED_HOLD, false, nullptr, 10);
            updateButtonState(WINBUTTON_FEED_TO_STOP, false, nullptr, 10);
            updateButtonState(WINBUTTON_FEED_HOLD, false, nullptr, 10);
            genie.WriteObject(GENIE_OBJ_LED, LED_READY, 1);

            ClearCore::ConnectorUsb.SendLine("[SemiAuto] Return complete, ready for new operation");
        }
    }

    // Periodic button state enforcement during active cycles
    if (_currentState == STATE_CUTTING || _currentState == STATE_PAUSED) {
        static uint32_t lastButtonCheck = 0;
        uint32_t now = ClearCore::TimingMgr.Milliseconds();

        if (now - lastButtonCheck > 500) {
            lastButtonCheck = now;
            updateButtonState(WINBUTTON_FEED_TO_STOP, true, nullptr, 0);
            bool isPaused = _feedHoldManager.isPaused();
            updateButtonState(WINBUTTON_FEED_HOLD, isPaused, nullptr, 0);
        }
    }

    // Special handling for STATE_PAUSED
    if (_currentState == STATE_PAUSED && !_isReturningToStart) {
        static uint32_t lastExitButtonCheck = 0;
        uint32_t now = ClearCore::TimingMgr.Milliseconds();

        if (now - lastExitButtonCheck > 500) {
            lastExitButtonCheck = now;
            updateButtonState(WINBUTTON_EXIT_FEED_HOLD, false, nullptr, 0);
            updateButtonState(WINBUTTON_FEED_HOLD, true, nullptr, 0);
        }
    }
}

void SemiAutoScreen::UpdateThicknessLed(float thickness) {
    m_lastThickness = thickness;
    int32_t scaled = static_cast<int32_t>(round(thickness * 1000.0f));
    genie.WriteObject(GENIE_OBJ_LED_DIGITS, LEDDIGITS_THICKNESS_F2, static_cast<uint16_t>(scaled));
}

void SemiAutoScreen::updateFeedRateDisplay() {
    // This method is now largely handled by TorqueControlUI
    // Kept for compatibility if needed
}