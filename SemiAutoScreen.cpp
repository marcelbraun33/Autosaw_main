




#include "SemiAutoScreen.h"
#include "screenmanager.h" 
#include "MotionController.h"
#include "UIInputManager.h"
#include "MPGJogManager.h"
#include "SettingsManager.h" 
#include <ClearCore.h>

#ifndef GENIE_OBJ_LED_DIGITS
#define GENIE_OBJ_LED_DIGITS 15
#endif

extern Genie genie;

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
    if (_currentState == STATE_CUTTING) {
        // Abort the torque controlled feed
        MotionController::Instance().abortTorqueControlledFeed(AXIS_Y);
        _currentState = STATE_FEED_HOLD;

        // Update UI
        genie.WriteObject(GENIE_OBJ_WINBUTTON, WINBUTTON_FEED_HOLD, 1); // Highlight feed hold button
        genie.WriteObject(GENIE_OBJ_WINBUTTON, WINBUTTON_FEED_TO_STOP, 0); // Reset feed button
        genie.WriteObject(GENIE_OBJ_WINBUTTON, WINBUTTON_EXIT_FEED_HOLD, 1); // Show exit button
    }
}

void SemiAutoScreen::exitFeedHold() {
    if (_currentState == STATE_FEED_HOLD) {
        _currentState = STATE_READY;

        // Update UI
        genie.WriteObject(GENIE_OBJ_WINBUTTON, WINBUTTON_FEED_HOLD, 0); // Reset feed hold button
        genie.WriteObject(GENIE_OBJ_WINBUTTON, WINBUTTON_EXIT_FEED_HOLD, 0); // Hide exit button
        genie.WriteObject(GENIE_OBJ_LED, LED_READY, 1); // Turn on ready LED
    }
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

        // Get current setting
#ifdef SETTINGS_HAS_CUT_PRESSURE
        _tempCutPressure = SettingsManager::Instance().settings().cutPressure;
#else
        _tempCutPressure = 70.0f; // Default
#endif

        // Update UI
        genie.WriteObject(GENIE_OBJ_WINBUTTON, WINBUTTON_ADJUST_CUT_PRESSURE, 1);
        genie.WriteObject(GENIE_OBJ_LED, LED_READY, 0);
        
        // Make sure display shows current value (scaled by 10 for display)
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

    // Get and display current Y position
    float currentPos = motion.getAbsoluteAxisPosition(AXIS_Y);
    genie.WriteObject(GENIE_OBJ_LED_DIGITS, LEDDIGITS_DISTANCE_TO_GO_F2,
        static_cast<uint16_t>(currentPos * 1000));

    // If in cutting state, update torque display and check for completion
    if (_currentState == STATE_CUTTING) {
        // Get current torque and target torque
        float torque = motion.getTorquePercent(AXIS_Y);
        float targetTorque = motion.getTorqueTarget(AXIS_Y);

        // Calculate torque percentage relative to target (0-100%)
        float torquePercentage = (torque / targetTorque) * 100.0f;

        // Clamp the percentage to avoid extreme values causing gauge problems
        if (torquePercentage > 200.0f) torquePercentage = 200.0f;
        if (torquePercentage < 0.0f) torquePercentage = 0.0f;

        // Update gauge to show percentage of torque target
        uint16_t gaugeValue = static_cast<uint16_t>(torquePercentage);

        // Gauge value over 100% will display in red (assuming gauge is configured for this)
        genie.WriteObject(GENIE_OBJ_IGAUGE, IGAUGE_SEMIAUTO_CUT_PRESSURE, gaugeValue);

        // For the digital display, always show the TARGET pressure value during cutting
        // This provides a stable reference value rather than showing the fluctuating actual torque
        genie.WriteObject(GENIE_OBJ_LED_DIGITS, LEDDIGITS_CUT_PRESSURE,
            static_cast<uint16_t>(targetTorque * 10.0f)); // Scale as needed

        // Check if feed completed
        if (!motion.isInTorqueControlledFeed(AXIS_Y) && !motion.isAxisMoving(AXIS_Y)) {
            // Feed completed
            _currentState = STATE_READY;
            genie.WriteObject(GENIE_OBJ_LED, LED_READY, 1); // Turn on ready LED
            genie.WriteObject(GENIE_OBJ_WINBUTTON, WINBUTTON_FEED_TO_STOP, 0); // Reset feed button

            // Reset display to show current pressure setting from settings
#ifdef SETTINGS_HAS_CUT_PRESSURE
            float cutPressure = SettingsManager::Instance().settings().cutPressure;
#else
            float cutPressure = 70.0f;
#endif
            genie.WriteObject(GENIE_OBJ_LED_DIGITS, LEDDIGITS_CUT_PRESSURE,
                static_cast<uint16_t>(cutPressure * 10.0f)); // Scale as needed
        }
    }
    // Rest of your existing code for adjusting pressure mode...

  
    // If in adjust pressure mode, check for encoder movement directly
    else if (_currentState == STATE_ADJUSTING_PRESSURE) {
        // Get the current encoder position directly from the hardware
        static int32_t lastEncoderPos = ClearCore::EncoderIn.Position();
        int32_t currentEncoderPos = ClearCore::EncoderIn.Position();

        // Calculate delta in encoder counts
        int32_t delta = currentEncoderPos - lastEncoderPos;

        // Only process significant changes to avoid noise
        if (abs(delta) >= ENCODER_COUNTS_PER_CLICK) {
            // Update the last position for next time
            lastEncoderPos = currentEncoderPos;

            // Determine direction
            float changeAmount = (delta > 0) ? CUT_PRESSURE_INCREMENT : -CUT_PRESSURE_INCREMENT;

            // Update the cut pressure value
            _tempCutPressure += changeAmount;

            // Clamp to valid range
            if (_tempCutPressure < MIN_CUT_PRESSURE) _tempCutPressure = MIN_CUT_PRESSURE;
            if (_tempCutPressure > MAX_CUT_PRESSURE) _tempCutPressure = MAX_CUT_PRESSURE;

            // Debug output
            ClearCore::ConnectorUsb.Send("[SemiAuto] Cut pressure adjusted to: ");
            ClearCore::ConnectorUsb.SendLine(_tempCutPressure);

            // Update display
            genie.WriteObject(GENIE_OBJ_LED_DIGITS, LEDDIGITS_CUT_PRESSURE,
                static_cast<uint16_t>(_tempCutPressure * 10.0f));

            // Save the value to settings immediately
#ifdef SETTINGS_HAS_CUT_PRESSURE
            auto& settings = SettingsManager::Instance().settings();
            settings.cutPressure = _tempCutPressure;
            // Don't call save() every update as it might cause excessive EEPROM writes
#endif

            // If cutting is active, update the torque target immediately for real-time adjustment
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
            if (_currentState == STATE_CUTTING) {
                feedHold();
            }
            break;

        case WINBUTTON_EXIT_FEED_HOLD:
            if (_currentState == STATE_FEED_HOLD) {
                exitFeedHold();
            }
            break;

        case WINBUTTON_ADJUST_CUT_PRESSURE:
            // Toggle cut pressure adjustment mode
            if (_currentState == STATE_READY || _currentState == STATE_ADJUSTING_PRESSURE) {
                adjustCutPressure();
            }
            break;

        case WINBUTTON_SPINDLE_ON:
            // Toggle spindle
            if (MotionController::Instance().IsSpindleRunning()) {
                MotionController::Instance().StopSpindle();
                genie.WriteObject(GENIE_OBJ_WINBUTTON, WINBUTTON_SPINDLE_ON, 0);
            }
            else {
                // Get RPM from settings with default
                float rpm = 3000.0f; // Default RPM
#ifdef SETTINGS_HAS_SPINDLE_RPM
                rpm = SettingsManager::Instance().settings().spindleRPM;
#else
                // Fall back to defaultRPM if available
                rpm = SettingsManager::Instance().settings().defaultRPM;
#endif

                MotionController::Instance().StartSpindle(rpm);
                genie.WriteObject(GENIE_OBJ_WINBUTTON, WINBUTTON_SPINDLE_ON, 1);
            }
            break;

        case WINBUTTON_INC_PLUS_F2:
            // Increment setting
            advanceIncrement();
            break;

        case WINBUTTON_INC_MINUS_F2:
            // Decrement setting in adjustment mode
            if (_currentState == STATE_ADJUSTING_PRESSURE) {
                _tempCutPressure -= CUT_PRESSURE_INCREMENT;
                if (_tempCutPressure < MIN_CUT_PRESSURE) {
                    _tempCutPressure = MIN_CUT_PRESSURE;
                }

                // Update display
                genie.WriteObject(GENIE_OBJ_LED_DIGITS, LEDDIGITS_CUT_PRESSURE,
                    static_cast<uint16_t>(_tempCutPressure * 10.0f)); // Scale for display

                // Save to settings
#ifdef SETTINGS_HAS_CUT_PRESSURE
                SettingsManager::Instance().settings().cutPressure = _tempCutPressure;
#endif
            }
            break;

        case WINBUTTON_SETTINGS_SEMI:
            // Switch to settings screen
            ScreenManager::Instance().ShowSettings();
            break;

        case WINBUTTON_HOME_F2:
            // Home Y axis
            MotionController::Instance().StartHomingAxis(AXIS_Y);
            break;
        }
        break;
    }
}




