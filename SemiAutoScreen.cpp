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

SemiAutoScreen::SemiAutoScreen(ScreenManager& mgr) : _mgr(mgr) {}

void SemiAutoScreen::updateButtonState(uint16_t buttonId, bool state, const char* logMessage, uint16_t delayMs) {
    showButtonSafe(buttonId, state ? 1 : 0, delayMs);

    if (logMessage) {
        ClearCore::ConnectorUsb.SendLine(logMessage);
    }
}

void SemiAutoScreen::onShow() {
    // Just clean up fields
    UIInputManager::Instance().unbindField();

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

    // Display the current feed rate setting
    float feedRate = SettingsManager::Instance().settings().feedRate / 25.0f; // Scale to 0-1 range
    _tempFeedRate = feedRate; // Store for later use

    // Update the target feed rate display (as a percentage)
    genie.WriteObject(GENIE_OBJ_LED_DIGITS, LEDDIGITS_TARGET_FEEDRATE,
        static_cast<uint16_t>(feedRate * 100.0f)); // Display as percentage

    // Get the thickness directly from CutData
    auto& cutData = _mgr.GetCutData();
    UpdateThicknessLed(cutData.thickness);
}

void SemiAutoScreen::onHide() {
    // Stop any ongoing operations
    if (_currentState == STATE_CUTTING || _currentState == STATE_PAUSED || _currentState == STATE_RETURNING) {
        MotionController::Instance().abortTorqueControlledFeed(AXIS_Y);
    }
    // If adjusting cut pressure or feed rate, exit that mode
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
    _tempFeedRate = feedRate; // Store feed rate for adjustment

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
        updateButtonState(WINBUTTON_ADJUST_CUT_PRESSURE, false, nullptr, 10);
        updateButtonState(WINBUTTON_ADJUST_MAX_SPEED, false, nullptr, 10);

        // Initialize gauge with a reasonable value based on target
        genie.WriteObject(GENIE_OBJ_IGAUGE, IGAUGE_SEMIAUTO_CUT_PRESSURE, 50); // Start at 50% to avoid red

        // Make sure the cut pressure display shows the target value
        genie.WriteObject(GENIE_OBJ_LED_DIGITS, LEDDIGITS_CUT_PRESSURE,
            static_cast<uint16_t>(_tempCutPressure * 10.0f)); // Scale as needed

        // Make sure the target feed rate display shows the current target
        genie.WriteObject(GENIE_OBJ_LED_DIGITS, LEDDIGITS_TARGET_FEEDRATE,
            static_cast<uint16_t>(_tempFeedRate * 100.0f)); // Display as percentage
    }
}

void SemiAutoScreen::feedHold() {
    // Delegate to FeedHoldManager for pause/resume logic first
    _feedHoldManager.toggleFeedHold();

    // Update UI based on the resulting state
    if (_feedHoldManager.isPaused()) {
        _currentState = STATE_PAUSED;

        // Set feed hold button active
        updateButtonState(WINBUTTON_FEED_HOLD, true, "[SemiAuto] Feed paused", 20);

        // Then show exit button (inactive state for latching button)
        updateButtonState(WINBUTTON_EXIT_FEED_HOLD, false, nullptr, 50);
    }
    else {
        _currentState = STATE_CUTTING;
        updateButtonState(WINBUTTON_FEED_HOLD, false, "[SemiAuto] Feed resumed", 20);

        // Hide exit button when resumed
        updateButtonState(WINBUTTON_EXIT_FEED_HOLD, false, nullptr, 0);
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
        updateButtonState(WINBUTTON_ADJUST_CUT_PRESSURE, false, "[SemiAuto] Cut pressure set", 20);
        genie.WriteObject(GENIE_OBJ_LED, LED_READY, 1);

        ClearCore::ConnectorUsb.Send("[SemiAuto] Cut pressure set to: ");
        ClearCore::ConnectorUsb.SendLine(_tempCutPressure);
    }
    else if (_currentState == STATE_READY || _currentState == STATE_CUTTING || _currentState == STATE_PAUSED) {
        // If adjusting feed rate, exit that mode first
        if (_currentState == STATE_ADJUSTING_FEED_RATE) {
            // Save the current feed rate before switching
            auto& settings = SettingsManager::Instance().settings();
            settings.feedRate = _tempFeedRate * 25.0f;
            SettingsManager::Instance().save();

            // Turn off feed rate adjustment button
            updateButtonState(WINBUTTON_ADJUST_MAX_SPEED, false, "[SemiAuto] Exiting feed rate adjustment", 20);

            ClearCore::ConnectorUsb.SendLine("[SemiAuto] Switching from feed rate to cut pressure adjustment");
        }

        // Enter cut pressure adjustment mode
        _currentState = STATE_ADJUSTING_PRESSURE;

#ifdef SETTINGS_HAS_CUT_PRESSURE
        _tempCutPressure = SettingsManager::Instance().settings().cutPressure;
#else
        _tempCutPressure = 70.0f; // Default
#endif

        // Update UI: highlight adjustment button and turn off ready LED
        updateButtonState(WINBUTTON_ADJUST_CUT_PRESSURE, true, "[SemiAuto] Adjusting cut pressure", 20);
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

void SemiAutoScreen::adjustMaxFeedRate() {
    // Toggle between normal mode and adjusting feed rate
    if (_currentState == STATE_ADJUSTING_FEED_RATE) {
        // Exit feed rate adjustment mode
        _currentState = STATE_READY;
        MPGJogManager::Instance().setEnabled(false);

        // Save the adjusted feed rate to settings
        auto& settings = SettingsManager::Instance().settings();
        settings.feedRate = _tempFeedRate * 25.0f; // Convert back from 0-1 scale to actual value
        SettingsManager::Instance().save();

        // Reset UI
        updateButtonState(WINBUTTON_ADJUST_MAX_SPEED, false, "[SemiAuto] Feed rate set", 20);
        genie.WriteObject(GENIE_OBJ_LED, LED_READY, 1);

        ClearCore::ConnectorUsb.Send("[SemiAuto] Feed rate set to: ");
        ClearCore::ConnectorUsb.Send(_tempFeedRate * 100.0f, 0);
        ClearCore::ConnectorUsb.SendLine("%");
    }
    else if (_currentState == STATE_READY || _currentState == STATE_CUTTING || _currentState == STATE_PAUSED) {
        // If adjusting cut pressure, exit that mode first
        if (_currentState == STATE_ADJUSTING_PRESSURE) {
            // Save the current cut pressure before switching
#ifdef SETTINGS_HAS_CUT_PRESSURE
            SettingsManager::Instance().settings().cutPressure = _tempCutPressure;
            SettingsManager::Instance().save();
#endif

            // Turn off cut pressure adjustment button
            updateButtonState(WINBUTTON_ADJUST_CUT_PRESSURE, false, "[SemiAuto] Exiting cut pressure adjustment", 20);

            ClearCore::ConnectorUsb.SendLine("[SemiAuto] Switching from cut pressure to feed rate adjustment");
        }

        // Enter feed rate adjustment mode
        _currentState = STATE_ADJUSTING_FEED_RATE;

        // Get current feed rate from settings (0-1 scale)
        auto& settings = SettingsManager::Instance().settings();
        _tempFeedRate = settings.feedRate / 25.0f;

        // Update UI: highlight adjustment button and turn off ready LED
        updateButtonState(WINBUTTON_ADJUST_MAX_SPEED, true, "[SemiAuto] Adjusting feed rate", 20);
        genie.WriteObject(GENIE_OBJ_LED, LED_READY, 0);

        // Update display to show current value as percentage
        genie.WriteObject(GENIE_OBJ_LED_DIGITS, LEDDIGITS_TARGET_FEEDRATE,
            static_cast<uint16_t>(_tempFeedRate * 100.0f));

        // Reset encoder tracking for clean start
        UIInputManager::Instance().resetRaw();

        ClearCore::ConnectorUsb.Send("[SemiAuto] Adjusting feed rate, current value: ");
        ClearCore::ConnectorUsb.Send(_tempFeedRate * 100.0f, 0);
        ClearCore::ConnectorUsb.SendLine("%");
    }
}

void SemiAutoScreen::advanceIncrement() {
    // Handle increment based on current state
    if (_currentState == STATE_ADJUSTING_PRESSURE) {
        _tempCutPressure += CUT_PRESSURE_INCREMENT;
        if (_tempCutPressure > MAX_CUT_PRESSURE) {
            _tempCutPressure = MAX_CUT_PRESSURE;
        }

        // Update display
        genie.WriteObject(GENIE_OBJ_LED_DIGITS, LEDDIGITS_CUT_PRESSURE,
            static_cast<uint16_t>(_tempCutPressure * 10.0f)); // Scale for display

        // Apply immediately if in cutting mode
        if (MotionController::Instance().isInTorqueControlledFeed(AXIS_Y)) {
            MotionController::Instance().setTorqueTarget(AXIS_Y, _tempCutPressure);
        }
    }
    else if (_currentState == STATE_ADJUSTING_FEED_RATE) {
        _tempFeedRate += FEED_RATE_INCREMENT;
        if (_tempFeedRate > MAX_FEED_RATE) {
            _tempFeedRate = MAX_FEED_RATE;
        }

        // Update display
        genie.WriteObject(GENIE_OBJ_LED_DIGITS, LEDDIGITS_TARGET_FEEDRATE,
            static_cast<uint16_t>(_tempFeedRate * 100.0f)); // Scale for display

        // Apply immediately if in cutting mode
        if (MotionController::Instance().isInTorqueControlledFeed(AXIS_Y)) {
            MotionController::Instance().YAxisInstance().UpdateFeedRate(_tempFeedRate);
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
            else if (_currentState == STATE_CUTTING || _currentState == STATE_PAUSED || _currentState == STATE_RETURNING) {
                // If button is pressed during active cycle, force it back to active state
                updateButtonState(WINBUTTON_FEED_TO_STOP, true, "[SemiAuto] Feed cycle in progress", 0);
            }
            break;

        case WINBUTTON_FEED_HOLD:
            if (_currentState == STATE_CUTTING || _currentState == STATE_PAUSED) {
                feedHold(); // Normal operation
            }
            else {
                // If button is pressed when not in a cutting state, force it back to its proper state
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
            // Allow toggling in all states except when feed rate adjustment is active
            if (_currentState != STATE_ADJUSTING_FEED_RATE) {
                adjustCutPressure();
            }
            else {
                // If feed rate adjustment is active, switch to cut pressure adjustment
                adjustMaxFeedRate(); // This will exit feed rate adjustment first
                adjustCutPressure(); // Then enter cut pressure adjustment
            }
            break;

        case WINBUTTON_ADJUST_MAX_SPEED:
            // Allow toggling in all states except when cut pressure adjustment is active
            if (_currentState != STATE_ADJUSTING_PRESSURE) {
                adjustMaxFeedRate();
            }
            else {
                // If cut pressure adjustment is active, switch to feed rate adjustment
                adjustCutPressure(); // This will exit cut pressure adjustment first
                adjustMaxFeedRate(); // Then enter feed rate adjustment
            }
            break;



        case WINBUTTON_SPINDLE_ON:
            if (MotionController::Instance().IsSpindleRunning()) {
                MotionController::Instance().StopSpindle();
                updateButtonState(WINBUTTON_SPINDLE_ON, false, "[SemiAuto] Spindle stopped", 0);
            }
            else {
                float rpm = 3000.0f;
#ifdef SETTINGS_HAS_SPINDLE_RPM
                rpm = SettingsManager::Instance().settings().spindleRPM;
#else
                rpm = SettingsManager::Instance().settings().defaultRPM;
#endif
                MotionController::Instance().StartSpindle(rpm);
                updateButtonState(WINBUTTON_SPINDLE_ON, true, "[SemiAuto] Spindle started", 0);
            }
            break;

        case WINBUTTON_INC_PLUS_F2:
            if (_currentState == STATE_ADJUSTING_PRESSURE || _currentState == STATE_ADJUSTING_FEED_RATE) {
                advanceIncrement();
            }
            else {
                JogUtilities::Increment(_mgr.GetCutData(), AXIS_X);
            }
            break;

        case WINBUTTON_INC_MINUS_F2:
            if (_currentState == STATE_ADJUSTING_PRESSURE) {
                _tempCutPressure -= CUT_PRESSURE_INCREMENT;
                if (_tempCutPressure < MIN_CUT_PRESSURE) {
                    _tempCutPressure = MIN_CUT_PRESSURE;
                }
                genie.WriteObject(GENIE_OBJ_LED_DIGITS, LEDDIGITS_CUT_PRESSURE,
                    static_cast<uint16_t>(_tempCutPressure * 10.0f));

                // Apply immediately if in cutting mode
                if (MotionController::Instance().isInTorqueControlledFeed(AXIS_Y)) {
                    MotionController::Instance().setTorqueTarget(AXIS_Y, _tempCutPressure);
                }
            }
            else if (_currentState == STATE_ADJUSTING_FEED_RATE) {
                _tempFeedRate -= FEED_RATE_INCREMENT;
                if (_tempFeedRate < MIN_FEED_RATE) {
                    _tempFeedRate = MIN_FEED_RATE;
                }
                genie.WriteObject(GENIE_OBJ_LED_DIGITS, LEDDIGITS_TARGET_FEEDRATE,
                    static_cast<uint16_t>(_tempFeedRate * 100.0f));

                // Apply immediately if in cutting mode
                if (MotionController::Instance().isInTorqueControlledFeed(AXIS_Y)) {
                    MotionController::Instance().YAxisInstance().UpdateFeedRate(_tempFeedRate);
                }
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

    // Monitor return motion completion
    if (_isReturningToStart) {
        // Check if return motion is complete
        if (!motion.isInTorqueControlledFeed(AXIS_Y) && !motion.isAxisMoving(AXIS_Y)) {
            // Return is complete, reset state
            _isReturningToStart = false;
            _currentState = STATE_READY;

            // Toggle exit button back to inactive state
            updateButtonState(WINBUTTON_EXIT_FEED_HOLD, false, nullptr, 10);

            // Reset other UI elements
            updateButtonState(WINBUTTON_FEED_TO_STOP, false, nullptr, 10);
            updateButtonState(WINBUTTON_FEED_HOLD, false, nullptr, 10);

            // Turn on ready LED
            genie.WriteObject(GENIE_OBJ_LED, LED_READY, 1);

            ClearCore::ConnectorUsb.SendLine("[SemiAuto] Return complete, ready for new operation");

            // Update cut pressure display to normal value
#ifdef SETTINGS_HAS_CUT_PRESSURE
            float cutPressure = SettingsManager::Instance().settings().cutPressure;
#else
            float cutPressure = 70.0f;
#endif
            genie.WriteObject(GENIE_OBJ_LED_DIGITS, LEDDIGITS_CUT_PRESSURE,
                static_cast<uint16_t>(cutPressure * 10.0f));
        }
    }
    // Only process normal cutting/paused states if not in return motion
    else if (_currentState == STATE_CUTTING || _currentState == STATE_PAUSED) {
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
            // Exit any adjustment modes if they're active
            if (_currentState == STATE_ADJUSTING_PRESSURE) {
                // Save settings before exiting
#ifdef SETTINGS_HAS_CUT_PRESSURE
                SettingsManager::Instance().settings().cutPressure = _tempCutPressure;
                SettingsManager::Instance().save();
#endif
                updateButtonState(WINBUTTON_ADJUST_CUT_PRESSURE, false, "[SemiAuto] Cut pressure set on completion", 20);
            }
            else if (_currentState == STATE_ADJUSTING_FEED_RATE) {
                // Save settings before exiting
                auto& settings = SettingsManager::Instance().settings();
                settings.feedRate = _tempFeedRate * 25.0f;
                SettingsManager::Instance().save();
                updateButtonState(WINBUTTON_ADJUST_MAX_SPEED, false, "[SemiAuto] Feed rate set on completion", 20);
            }

            // Set state to ready
            _currentState = STATE_READY;
            genie.WriteObject(GENIE_OBJ_LED, LED_READY, 1); // Turn on ready LED
            updateButtonState(WINBUTTON_FEED_TO_STOP, false, nullptr, 10); // Reset feed button
            updateButtonState(WINBUTTON_EXIT_FEED_HOLD, false, nullptr, 10); // Hide exit button

#ifdef SETTINGS_HAS_CUT_PRESSURE
            float cutPressure = SettingsManager::Instance().settings().cutPressure;
#else
            float cutPressure = 70.0f;
#endif
            genie.WriteObject(GENIE_OBJ_LED_DIGITS, LEDDIGITS_CUT_PRESSURE,
                static_cast<uint16_t>(cutPressure * 10.0f));
        }

        // Ensure Feed to Stop button maintains proper state during active cycles
        static uint32_t lastFeedButtonCheck = 0;
        uint32_t now = ClearCore::TimingMgr.Milliseconds();

        // Periodically check button state (every 500ms is enough)
        if (now - lastFeedButtonCheck > 500) {
            lastFeedButtonCheck = now;

            // Force Feed to Stop button to stay in active state during cycle
            updateButtonState(WINBUTTON_FEED_TO_STOP, true, nullptr, 0);

            // Also enforce feed hold button state based on current pause status
            bool isPaused = _feedHoldManager.isPaused();
            updateButtonState(WINBUTTON_FEED_HOLD, isPaused, nullptr, 0);
        }
    }

    // Special handling for STATE_PAUSED - ensure exit button stays visible in correct state
    if (_currentState == STATE_PAUSED && !_isReturningToStart) {
        static uint32_t lastExitButtonCheck = 0;
        uint32_t now = ClearCore::TimingMgr.Milliseconds();

        // Periodically check and refresh exit button to prevent red X
        if (now - lastExitButtonCheck > 500) { // Twice per second is enough
            lastExitButtonCheck = now;

            // Keep button visible in inactive state (for latching button)
            updateButtonState(WINBUTTON_EXIT_FEED_HOLD, false, nullptr, 0);

            // Also ensure feed hold button is in active state
            updateButtonState(WINBUTTON_FEED_HOLD, true, nullptr, 0);
        }
    }

    // Check for encoder movement in pressure adjustment mode
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

            // Apply changes immediately if in active cutting mode
            if (motion.isInTorqueControlledFeed(AXIS_Y)) {
                motion.setTorqueTarget(AXIS_Y, _tempCutPressure);
            }
        }
    }

    // Check for encoder movement in feed rate adjustment mode
    else if (_currentState == STATE_ADJUSTING_FEED_RATE) {
        static int32_t lastEncoderPos = ClearCore::EncoderIn.Position();
        int32_t currentEncoderPos = ClearCore::EncoderIn.Position();
        int32_t delta = currentEncoderPos - lastEncoderPos;

        if (abs(delta) >= ENCODER_COUNTS_PER_CLICK) {
            lastEncoderPos = currentEncoderPos;
            float changeAmount = (delta > 0) ? FEED_RATE_INCREMENT : -FEED_RATE_INCREMENT;
            _tempFeedRate += changeAmount;

            if (_tempFeedRate < MIN_FEED_RATE) _tempFeedRate = MIN_FEED_RATE;
            if (_tempFeedRate > MAX_FEED_RATE) _tempFeedRate = MAX_FEED_RATE;

            ClearCore::ConnectorUsb.Send("[SemiAuto] Feed rate adjusted to: ");
            ClearCore::ConnectorUsb.Send(_tempFeedRate * 100.0f, 0);
            ClearCore::ConnectorUsb.SendLine("%");

            // Update display with new value
            genie.WriteObject(GENIE_OBJ_LED_DIGITS, LEDDIGITS_TARGET_FEEDRATE,
                static_cast<uint16_t>(_tempFeedRate * 100.0f));

            // Store in settings (but don't save yet)
            auto& settings = SettingsManager::Instance().settings();
            settings.feedRate = _tempFeedRate * 25.0f;

            // Apply changes immediately if in active cutting mode
            if (motion.isInTorqueControlledFeed(AXIS_Y)) {
                motion.YAxisInstance().UpdateFeedRate(_tempFeedRate);
            }
        }
    }
}


void SemiAutoScreen::UpdateThicknessLed(float thickness) {
    m_lastThickness = thickness;
    int32_t scaled = static_cast<int32_t>(round(thickness * 1000.0f));
    genie.WriteObject(GENIE_OBJ_LED_DIGITS, LEDDIGITS_THICKNESS_F2, static_cast<uint16_t>(scaled));
}

void SemiAutoScreen::updateFeedRateDisplay() {
    auto& motion = MotionController::Instance();

    // Get the current feed rate (if in torque controlled feed) or use settings value
    float feedRate;
    if (motion.isInTorqueControlledFeed(AXIS_Y)) {
        feedRate = motion.YAxisInstance().DebugGetCurrentFeedRate();
    }
    else {
        feedRate = SettingsManager::Instance().settings().feedRate / 25.0f;
    }

    // Update the target feed rate display
    genie.WriteObject(GENIE_OBJ_LED_DIGITS, LEDDIGITS_TARGET_FEEDRATE,
        static_cast<uint16_t>(feedRate * 100.0f)); // Show as percentage
}
