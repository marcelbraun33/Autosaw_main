#include "JogYScreen.h"
#include "Config.h"
#include "MotionController.h"
#include "MPGJogManager.h"
#include "UIInputmanager.h"

// Define Genie object types if not already defined
#ifndef GENIE_OBJ_LEDDIGITS
#define GENIE_OBJ_LEDDIGITS 15
#endif

#ifndef GENIE_OBJ_WINBUTTON
#define GENIE_OBJ_WINBUTTON 10
#endif

#ifndef GENIE_OBJ_LED
#define GENIE_OBJ_LED 16
#endif

// Fixed MPG increment for cut length and retract (5 thousandths)
#define MPG_FIXED_INCREMENT 0.005f

extern Genie genie;

// Static member initialization
float JogYScreen::_tempLength = 0.0f;
float JogYScreen::_tempRetract = 0.0f;

void JogYScreen::onShow() {
    // Initialize displays
    updateAllDisplays();

    // Check if position is within 0.002 inches of the cut start point
    float pos = MotionController::Instance().getAxisPosition(AXIS_Y);
    float distanceToStart = fabs(pos - _cutStartPoint);
    if (distanceToStart <= 0.002f) {
        genie.WriteObject(GENIE_OBJ_LED, LED_AT_START_POSITION_Y, 1);
    }
    else {
        genie.WriteObject(GENIE_OBJ_LED, LED_AT_START_POSITION_Y, 0);
    }

    // Disable MPG editing modes
    _mpgSetLengthMode = false;
    _mpgSetRetractMode = false;

    // Reset button states
    showButtonSafe(WINBUTTON_SET_WITH_MPG_F6, 0);
    showButtonSafe(WINBUTTON_SET_RETRACT_WITH_MPG_F6, 0);

    // Ensure jog is initially disabled
    auto& mpg = MPGJogManager::Instance();
    mpg.setEnabled(false);
    showButtonSafe(WINBUTTON_ACTIVATE_JOG_Y_F6, 0);
}

void JogYScreen::onHide() {
    // If we're in MPG edit mode, exit it
    auto& ui = UIInputManager::Instance();
    if (_mpgSetLengthMode || _mpgSetRetractMode) {
        ui.unbindField();
        _mpgSetLengthMode = false;
        _mpgSetRetractMode = false;
    }

    // Disable jog mode when leaving screen
    auto& mpg = MPGJogManager::Instance();
    if (mpg.isEnabled()) {
        mpg.setEnabled(false);
    }
}

void JogYScreen::handleEvent(const genieFrame& e) {
    if (e.reportObject.cmd != GENIE_REPORT_EVENT) return;

    switch (e.reportObject.index) {
    case WINBUTTON_CAPTURE_CUT_START_F6:
        captureCutStart();
        break;

    case WINBUTTON_CAPTURE_CUT_END_F6:
        captureCutEnd();
        break;

    case WINBUTTON_JOG_TO_START:
        jogToStartPosition();
        break;

    case WINBUTTON_JOG_TO_END:
        jogToEndPosition();
        break;

    case WINBUTTON_JOG_TO_RETRACT:
        jogToRetractPosition();
        break;

    case WINBUTTON_SET_WITH_MPG_F6:
        setLengthWithMPG();
        break;

    case WINBUTTON_SET_RETRACT_WITH_MPG_F6:
        setRetractWithMPG();
        break;

    case WINBUTTON_ACTIVATE_JOG_Y_F6:
        toggleJogMode();
        break;

    default:
        break;
    }
}

void JogYScreen::captureCutStart() {
    // Get current position in machine coordinates
    _cutStartPoint = MotionController::Instance().getAxisPosition(AXIS_Y);

    // Update cut length when start point changes
    _cutLength = _cutEndPoint - _cutStartPoint;
    if (_cutLength < 0) _cutLength = 0;

    // Update displays
    genie.WriteObject(GENIE_OBJ_LEDDIGITS,
        LEDDIGITS_STOCK_END_Y,
        static_cast<uint16_t>(_cutStartPoint * 1000));
    updateCutLengthDisplay();

    // Since we're at the start position, illuminate LED
    genie.WriteObject(GENIE_OBJ_LED, LED_AT_START_POSITION_Y, 1);

    // Visual feedback
    showButtonSafe(WINBUTTON_CAPTURE_CUT_START_F6, 1);
    delay(200);
    showButtonSafe(WINBUTTON_CAPTURE_CUT_START_F6, 0);
}

void JogYScreen::captureCutEnd() {
    // Get current position in machine coordinates
    _cutEndPoint = MotionController::Instance().getAxisPosition(AXIS_Y);

    // Update cut length when end point changes
    _cutLength = _cutEndPoint - _cutStartPoint;
    if (_cutLength < 0) _cutLength = 0;

    // Update displays
    genie.WriteObject(GENIE_OBJ_LEDDIGITS,
        LEDDIGITS_CUT_STOP_Y,
        static_cast<uint16_t>(_cutEndPoint * 1000));
    updateCutLengthDisplay();

    // Visual feedback
    showButtonSafe(WINBUTTON_CAPTURE_CUT_END_F6, 1);
    delay(200);
    showButtonSafe(WINBUTTON_CAPTURE_CUT_END_F6, 0);
}

void JogYScreen::captureRetractDistance() {
    // Calculate retract distance as distance from start point to current position
    float currentPos = MotionController::Instance().getAxisPosition(AXIS_Y);
    float distance = _cutStartPoint - currentPos;

    // Ensure retract distance is positive
    _retractDistance = (distance > 0) ? distance : 0.0f;

    // Update display
    genie.WriteObject(GENIE_OBJ_LEDDIGITS,
        LEDDIGITS_RETRACT_DISTANCE,
        static_cast<uint16_t>(_retractDistance * 1000));

    // Visual feedback - we don't have this button in the UI, so this is for future use
    // showButtonSafe(WINBUTTON_CAPTURE_RETRACT_F6, 1);
    // delay(200);
    // showButtonSafe(WINBUTTON_CAPTURE_RETRACT_F6, 0);
}

void JogYScreen::jogToStartPosition() {
    // Move to the start position
    MotionController::Instance().moveToWithRate(AXIS_Y, _cutStartPoint, 0.5f);

    // Illuminate LED to indicate at start position
    genie.WriteObject(GENIE_OBJ_LED, LED_AT_START_POSITION_Y, 1);

    // Visual feedback
    showButtonSafe(WINBUTTON_JOG_TO_START, 1);
    delay(200);
    showButtonSafe(WINBUTTON_JOG_TO_START, 0);
}

void JogYScreen::jogToEndPosition() {
    // Move to the end position
    MotionController::Instance().moveToWithRate(AXIS_Y, _cutEndPoint, 0.5f);

    // Visual feedback
    showButtonSafe(WINBUTTON_JOG_TO_END, 1);
    delay(200);
    showButtonSafe(WINBUTTON_JOG_TO_END, 0);
}

void JogYScreen::jogToRetractPosition() {
    // Get minimum Y position (home position)
    float yHomePos = 0.0f; // Default to 0 if we can't get a home position

    // Calculate retract position (always behind the start point)
    float desiredRetractPos = _cutStartPoint - _retractDistance;

    // Clamp to home position - don't go below machine minimum
    if (desiredRetractPos < yHomePos) {
        desiredRetractPos = yHomePos;
    }

    // Move to retract position
    MotionController::Instance().moveToWithRate(AXIS_Y, desiredRetractPos, 0.5f);

    // Visual feedback
    showButtonSafe(WINBUTTON_JOG_TO_RETRACT, 1);
    delay(200);
    showButtonSafe(WINBUTTON_JOG_TO_RETRACT, 0);
}

void JogYScreen::toggleJogMode() {
    auto& mpg = MPGJogManager::Instance();
    auto& ui = UIInputManager::Instance();

    // Toggle jog mode
    bool newState = !mpg.isEnabled();

    // If enabling jog mode, make sure we're not in MPG editing mode
    if (newState) {
        // Exit any active MPG editing modes
        if (_mpgSetLengthMode) {
            ui.unbindField();
            _mpgSetLengthMode = false;
            showButtonSafe(WINBUTTON_SET_WITH_MPG_F6, 0);
        }

        if (_mpgSetRetractMode) {
            ui.unbindField();
            _mpgSetRetractMode = false;
            showButtonSafe(WINBUTTON_SET_RETRACT_WITH_MPG_F6, 0);
        }

        // Enable jog mode for Y axis
        mpg.setEnabled(true);
        mpg.setAxis(AXIS_Y);  // Use setAxis instead of setActiveAxis
        showButtonSafe(WINBUTTON_ACTIVATE_JOG_Y_F6, 1);
    }
    else {
        // Disable jog mode
        mpg.setEnabled(false);
        showButtonSafe(WINBUTTON_ACTIVATE_JOG_Y_F6, 0);
    }
}


void JogYScreen::setLengthWithMPG() {
    auto& ui = UIInputManager::Instance();
    auto& mpg = MPGJogManager::Instance();

    // Toggle length MPG edit mode
    _mpgSetLengthMode = !_mpgSetLengthMode;

    if (_mpgSetLengthMode) {
        // Exit retract MPG mode if active
        if (_mpgSetRetractMode) {
            ui.unbindField();
            _mpgSetRetractMode = false;
            showButtonSafe(WINBUTTON_SET_RETRACT_WITH_MPG_F6, 0);
        }

        // Disable normal jog if enabled
        if (mpg.isEnabled()) {
            mpg.setEnabled(false);
            showButtonSafe(WINBUTTON_ACTIVATE_JOG_Y_F6, 0);
        }

        // Store current values
        _tempLength = _cutLength;

        // Show button as active
        showButtonSafe(WINBUTTON_SET_WITH_MPG_F6, 1);

        // Bind to edit field with fixed 5 thousandths increment
        ui.bindField(WINBUTTON_SET_WITH_MPG_F6, LEDDIGITS_CUT_LENGTH_Y,
            &_tempLength, 0.0f, 100.0f, MPG_FIXED_INCREMENT, 3);
    }
    else {
        // Exit MPG length edit mode
        if (ui.isFieldActive(WINBUTTON_SET_WITH_MPG_F6)) {
            // Update cut length and end point based on edited value
            _cutLength = _tempLength;
            _cutEndPoint = _cutStartPoint + _cutLength;

            // Unbind field
            ui.unbindField();

            // Update displays
            genie.WriteObject(GENIE_OBJ_LEDDIGITS,
                LEDDIGITS_CUT_STOP_Y,
                static_cast<uint16_t>(_cutEndPoint * 1000));
        }

        // Show button as inactive
        showButtonSafe(WINBUTTON_SET_WITH_MPG_F6, 0);
    }
}

void JogYScreen::setRetractWithMPG() {
    auto& ui = UIInputManager::Instance();
    auto& mpg = MPGJogManager::Instance();

    // Debug output to see what's happening
    ClearCore::ConnectorUsb.Send("[JogY] setRetractWithMPG button pressed, current mode: ");
    ClearCore::ConnectorUsb.SendLine(_mpgSetRetractMode ? "ON" : "OFF");

    // Toggle retract MPG edit mode
    _mpgSetRetractMode = !_mpgSetRetractMode;

    if (_mpgSetRetractMode) {
        ClearCore::ConnectorUsb.SendLine("[JogY] Entering retract MPG mode");

        // Exit length MPG mode if active
        if (_mpgSetLengthMode) {
            ui.unbindField();
            _mpgSetLengthMode = false;
            showButtonSafe(WINBUTTON_SET_WITH_MPG_F6, 0);
        }

        // Disable normal jog if enabled
        if (mpg.isEnabled()) {
            mpg.setEnabled(false);
            showButtonSafe(WINBUTTON_ACTIVATE_JOG_Y_F6, 0);
        }

        // Store current value
        _tempRetract = _retractDistance;

        // Show button as active
        showButtonSafe(WINBUTTON_SET_RETRACT_WITH_MPG_F6, 1);

        // Bind the field for editing
        ClearCore::ConnectorUsb.Send("[JogY] Binding field to button ID: ");
        ClearCore::ConnectorUsb.Send(WINBUTTON_SET_RETRACT_WITH_MPG_F6);
        ClearCore::ConnectorUsb.Send(", LED ID: ");
        ClearCore::ConnectorUsb.SendLine(LEDDIGITS_RETRACT_DISTANCE);

        ui.bindField(WINBUTTON_SET_RETRACT_WITH_MPG_F6, LEDDIGITS_RETRACT_DISTANCE,
            &_tempRetract, 0.0f, 100.0f, MPG_FIXED_INCREMENT, 3);

        // Verify binding was successful
        ClearCore::ConnectorUsb.Send("[JogY] Field active status: ");
        ClearCore::ConnectorUsb.SendLine(ui.isFieldActive(WINBUTTON_SET_RETRACT_WITH_MPG_F6) ? "ACTIVE" : "NOT ACTIVE");
    }
    else {
        ClearCore::ConnectorUsb.SendLine("[JogY] Exiting retract MPG mode");

        // Exit MPG retract edit mode
        if (ui.isFieldActive(WINBUTTON_SET_RETRACT_WITH_MPG_F6)) {
            // Update retract distance from edited value
            _retractDistance = _tempRetract;

            // Unbind field
            ui.unbindField();

            // Update display with new value
            genie.WriteObject(GENIE_OBJ_LEDDIGITS,
                LEDDIGITS_RETRACT_DISTANCE,
                static_cast<uint16_t>(_retractDistance * 1000));
        }

        // Show button as inactive
        showButtonSafe(WINBUTTON_SET_RETRACT_WITH_MPG_F6, 0);
    }
}

void JogYScreen::updateCutLengthDisplay() {
    _cutLength = _cutEndPoint - _cutStartPoint;
    if (_cutLength < 0) _cutLength = 0;

    genie.WriteObject(GENIE_OBJ_LEDDIGITS,
        LEDDIGITS_CUT_LENGTH_Y,
        static_cast<uint16_t>(_cutLength * 1000));
}

void JogYScreen::updateAllDisplays() {
    // Update all LED displays with current values
    genie.WriteObject(GENIE_OBJ_LEDDIGITS,
        LEDDIGITS_STOCK_END_Y,
        static_cast<uint16_t>(_cutStartPoint * 1000));

    genie.WriteObject(GENIE_OBJ_LEDDIGITS,
        LEDDIGITS_CUT_STOP_Y,
        static_cast<uint16_t>(_cutEndPoint * 1000));

    genie.WriteObject(GENIE_OBJ_LEDDIGITS,
        LEDDIGITS_RETRACT_DISTANCE,
        static_cast<uint16_t>(_retractDistance * 1000));

    updateCutLengthDisplay();

    float currentPos = MotionController::Instance().getAxisPosition(AXIS_Y);
    genie.WriteObject(GENIE_OBJ_LEDDIGITS,
        LEDDIGITS_TABLE_POSITION_Y,
        static_cast<uint16_t>(currentPos * 1000));
}

void JogYScreen::update() {
    // Live update of table position
    float pos = MotionController::Instance().getAxisPosition(AXIS_Y);
    genie.WriteObject(GENIE_OBJ_LEDDIGITS,
        LEDDIGITS_TABLE_POSITION_Y,
        static_cast<uint16_t>(pos * 1000));

    // Check if position is within 0.002 inches of the cut start point
    // and illuminate LED accordingly
    float distanceToStart = fabs(pos - _cutStartPoint);
    if (distanceToStart <= 0.002f) {
        // Table is at or very close to start position - illuminate LED
        genie.WriteObject(GENIE_OBJ_LED, LED_AT_START_POSITION_Y, 1);
    }
    else {
        // Table is not at start position - turn off LED
        genie.WriteObject(GENIE_OBJ_LED, LED_AT_START_POSITION_Y, 0);
    }

    // Make sure button state remains correct
    static uint32_t lastCheckTime = 0;
    uint32_t now = ClearCore::TimingMgr.Milliseconds();

    if (now - lastCheckTime > 500) {  // Check every 500ms
        lastCheckTime = now;

        // Ensure button states match their mode
        if (_mpgSetLengthMode) {
            genie.WriteObject(GENIE_OBJ_WINBUTTON, WINBUTTON_SET_WITH_MPG_F6, 1);
        }

        if (_mpgSetRetractMode) {
            genie.WriteObject(GENIE_OBJ_WINBUTTON, WINBUTTON_SET_RETRACT_WITH_MPG_F6, 1);
        }

        // Update jog button state based on MPG manager's enabled state
        auto& mpg = MPGJogManager::Instance();
        if (mpg.isEnabled()) {
            // We can't check if Y is the active axis since there's no getter,
            // so just reflect the enabled state
            genie.WriteObject(GENIE_OBJ_WINBUTTON, WINBUTTON_ACTIVATE_JOG_Y_F6, 1);
        }
    }
}
