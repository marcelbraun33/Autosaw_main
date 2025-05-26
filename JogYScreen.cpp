#include "JogYScreen.h"
#include "Config.h"
#include "MotionController.h"
#include "MPGJogManager.h"
#include "UIInputmanager.h"  // Add this include for UIInputManager

// Define Genie object types if not already defined
#ifndef GENIE_OBJ_LEDDIGITS
#define GENIE_OBJ_LEDDIGITS 15
#endif

// Define button IDs for new buttons if not already defined
#ifndef WINBUTTON_CAPTURE_RETRACT_F6
#define WINBUTTON_CAPTURE_RETRACT_F6 39  // Button ID for "Capture position as Safe Retract"
#endif

#ifndef WINBUTTON_ZERO_TO_START_F6
#define WINBUTTON_ZERO_TO_START_F6 34    // Button ID for "Zero to Start Position" 
#endif

#ifndef LEDDIGITS_RETRACT_POSITION_Y
#define LEDDIGITS_RETRACT_POSITION_Y 25  // LEDDigits ID for retract position
#endif

extern Genie genie;

// Static member initialization
float JogYScreen::_tempLength = 0.0f;

void JogYScreen::onShow() {
    // Initialize LEDs for start & stop
    genie.WriteObject(GENIE_OBJ_LEDDIGITS,
        LEDDIGITS_STOCK_END_Y,
        static_cast<uint16_t>(_cutStartPoint * 1000));
    genie.WriteObject(GENIE_OBJ_LEDDIGITS,
        LEDDIGITS_CUT_STOP_Y,
        static_cast<uint16_t>(_cutEndPoint * 1000));
    genie.WriteObject(GENIE_OBJ_LEDDIGITS,
        LEDDIGITS_RETRACT_POSITION_Y,
        static_cast<uint16_t>(_retractPosition * 1000));
    updateCutLengthDisplay();
}

void JogYScreen::onHide() {
    // If we're in MPG set mode, exit it before hiding the screen
    if (_mpgSetMode) {
        UIInputManager::Instance().unbindField();
        _mpgSetMode = false;
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

    case WINBUTTON_CAPTURE_RETRACT_F6:  // New handler
        captureRetractPosition();
        break;

    case WINBUTTON_ZERO_TO_START_F6:    // New handler
        zeroToStartPosition();
        break;

    case WINBUTTON_SET_WITH_MPG_F6:
        setWithMPG();
        break;

    default:
        break;
    }
}

void JogYScreen::update() {
    // live update of table position
    float pos = MotionController::Instance().getAxisPosition(AXIS_Y);
    genie.WriteObject(GENIE_OBJ_LEDDIGITS,
        LEDDIGITS_TABLE_POSITION_Y,
        static_cast<uint16_t>(pos * 1000));
}

void JogYScreen::captureCutStart() {
    _cutStartPoint = MotionController::Instance().getAxisPosition(AXIS_Y);
    genie.WriteObject(GENIE_OBJ_LEDDIGITS,
        LEDDIGITS_STOCK_END_Y,
        static_cast<uint16_t>(_cutStartPoint * 1000));
    updateCutLengthDisplay();
}

void JogYScreen::captureCutEnd() {
    _cutEndPoint = MotionController::Instance().getAxisPosition(AXIS_Y);
    genie.WriteObject(GENIE_OBJ_LEDDIGITS,
        LEDDIGITS_CUT_STOP_Y,
        static_cast<uint16_t>(_cutEndPoint * 1000));
    updateCutLengthDisplay();
}

void JogYScreen::captureRetractPosition() {
    _retractPosition = MotionController::Instance().getAxisPosition(AXIS_Y);
    genie.WriteObject(GENIE_OBJ_LEDDIGITS,
        LEDDIGITS_RETRACT_POSITION_Y,
        static_cast<uint16_t>(_retractPosition * 1000));

    // Flash button as feedback
    showButtonSafe(WINBUTTON_CAPTURE_RETRACT_F6, 1);
    delay(200);
    showButtonSafe(WINBUTTON_CAPTURE_RETRACT_F6, 0);
}

void JogYScreen::zeroToStartPosition() {
    // Move the table to the start position
    if (_cutStartPoint != 0.0f) {
        MotionController::Instance().moveToWithRate(AXIS_Y, _cutStartPoint, 0.5f);

        // Flash button as feedback
        showButtonSafe(WINBUTTON_ZERO_TO_START_F6, 1);
        delay(200);
        showButtonSafe(WINBUTTON_ZERO_TO_START_F6, 0);
    }
}

void JogYScreen::setWithMPG() {
    auto& mpg = MPGJogManager::Instance();
    auto& ui = UIInputManager::Instance();

    // Toggle the MPG set mode
    _mpgSetMode = !_mpgSetMode;

    if (_mpgSetMode) {
        // Entering MPG set mode

        // Disable normal jog if it's enabled
        if (mpg.isEnabled()) {
            mpg.setEnabled(false);
            showButtonSafe(WINBUTTON_ACTIVATE_JOG, 0); // Turn off the jog button if it exists
        }

        // Calculate the current cut length
        float currentLength = _cutEndPoint - _cutStartPoint;

        // Store temporary values for potential cancel
        _tempCutEndPoint = _cutEndPoint;
        _tempLength = currentLength;

        // Show the button as active
        showButtonSafe(WINBUTTON_SET_WITH_MPG_F6, 1);

        // Bind the CUT_LENGTH field for editing
        ui.bindField(WINBUTTON_SET_WITH_MPG_F6, LEDDIGITS_CUT_LENGTH_Y,
            &_tempLength, 0.0f, 100.0f, mpg.getAxisIncrement(), 3);
    }
    else {
        // Exiting MPG set mode

        // If the field is active, get the final value and calculate new end point
        if (ui.isFieldActive(WINBUTTON_SET_WITH_MPG_F6)) {
            // New end point = start point + edited length
            _cutEndPoint = _cutStartPoint + _tempLength;

            // Unbind the field
            ui.unbindField();

            // Update the cut stop display with the new calculated endpoint
            genie.WriteObject(GENIE_OBJ_LEDDIGITS,
                LEDDIGITS_CUT_STOP_Y,
                static_cast<uint16_t>(_cutEndPoint * 1000));
        }

        // Show the button as inactive
        showButtonSafe(WINBUTTON_SET_WITH_MPG_F6, 0);
    }
}

void JogYScreen::updateCutLengthDisplay() {
    float length = _cutEndPoint - _cutStartPoint;
    genie.WriteObject(GENIE_OBJ_LEDDIGITS,
        LEDDIGITS_CUT_LENGTH_Y,
        static_cast<uint16_t>(length * 1000));
}

void JogYScreen::updateAllDisplays() {
    // Update all LED displays
    genie.WriteObject(GENIE_OBJ_LEDDIGITS,
        LEDDIGITS_STOCK_END_Y,
        static_cast<uint16_t>(_cutStartPoint * 1000));

    genie.WriteObject(GENIE_OBJ_LEDDIGITS,
        LEDDIGITS_CUT_STOP_Y,
        static_cast<uint16_t>(_cutEndPoint * 1000));

    genie.WriteObject(GENIE_OBJ_LEDDIGITS,
        LEDDIGITS_RETRACT_POSITION_Y,
        static_cast<uint16_t>(_retractPosition * 1000));

    updateCutLengthDisplay();
}
