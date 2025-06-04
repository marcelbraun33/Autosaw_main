#include "JogYScreen.h"
#include "Config.h"
#include "MotionController.h"
#include "MPGJogManager.h"
#include "UIInputmanager.h"
#include "screenmanager.h"


#ifndef GENIE_OBJ_LEDDIGITS
#define GENIE_OBJ_LEDDIGITS 15
#endif

#ifndef GENIE_OBJ_WINBUTTON
#define GENIE_OBJ_WINBUTTON 10
#endif

#ifndef GENIE_OBJ_LED
#define GENIE_OBJ_LED 16
#endif

#define MPG_FIXED_INCREMENT 0.005f

extern Genie genie;

float JogYScreen::_tempLength = 0.0f;
float JogYScreen::_tempRetract = 0.0f;

JogYScreen::JogYScreen(ScreenManager& mgr) : _mgr(mgr) {}

void JogYScreen::onShow() {
    // Reset modes
    _mpgSetLengthMode = false;
    _mpgSetRetractMode = false;

    // MPG disabled at start
    MPGJogManager::Instance().setEnabled(false);

    // Let update() handle the rest
}

void JogYScreen::onHide() {
    // Clear LED indicators before leaving screen
    genie.WriteObject(GENIE_OBJ_LED, LED_AT_START_POSITION_Y, 0);


    auto& ui = UIInputManager::Instance();
    if (_mpgSetLengthMode || _mpgSetRetractMode) {
        ui.unbindField();
        _mpgSetLengthMode = false;
        _mpgSetRetractMode = false;
    }
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
    auto& cutData = _mgr.GetCutData();
    cutData.cutStartPoint = MotionController::Instance().getAxisPosition(AXIS_Y);
    cutData.cutLength = cutData.cutEndPoint - cutData.cutStartPoint;
    if (cutData.cutLength < 0) cutData.cutLength = 0;

    genie.WriteObject(GENIE_OBJ_LEDDIGITS,
        LEDDIGITS_STOCK_END_Y,
        static_cast<uint16_t>(cutData.cutStartPoint * 1000));
    updateCutLengthDisplay();

    genie.WriteObject(GENIE_OBJ_LED, LED_AT_START_POSITION_Y, 1);

    showButtonSafe(WINBUTTON_CAPTURE_CUT_START_F6, 1);
    delay(200);
    showButtonSafe(WINBUTTON_CAPTURE_CUT_START_F6, 0);
}

void JogYScreen::captureCutEnd() {
    auto& cutData = _mgr.GetCutData();
    cutData.cutEndPoint = MotionController::Instance().getAxisPosition(AXIS_Y);
    cutData.cutLength = cutData.cutEndPoint - cutData.cutStartPoint;
    if (cutData.cutLength < 0) cutData.cutLength = 0;

    genie.WriteObject(GENIE_OBJ_LEDDIGITS,
        LEDDIGITS_CUT_STOP_Y,
        static_cast<uint16_t>(cutData.cutEndPoint * 1000));
    updateCutLengthDisplay();

    showButtonSafe(WINBUTTON_CAPTURE_CUT_END_F6, 1);
    delay(200);
    showButtonSafe(WINBUTTON_CAPTURE_CUT_END_F6, 0);
}

void JogYScreen::captureRetractDistance() {
    auto& cutData = _mgr.GetCutData();
    float currentPos = MotionController::Instance().getAxisPosition(AXIS_Y);
    float distance = cutData.cutStartPoint - currentPos;
    cutData.retractDistance = (distance > 0) ? distance : 0.0f;

    genie.WriteObject(GENIE_OBJ_LEDDIGITS,
        LEDDIGITS_RETRACT_DISTANCE,
        static_cast<uint16_t>(cutData.retractDistance * 1000));
}

void JogYScreen::jogToStartPosition() {
    auto& cutData = _mgr.GetCutData();
    MotionController::Instance().moveToWithRate(AXIS_Y, cutData.cutStartPoint, 0.5f);
    genie.WriteObject(GENIE_OBJ_LED, LED_AT_START_POSITION_Y, 1);
    showButtonSafe(WINBUTTON_JOG_TO_START, 1);
    delay(200);
    showButtonSafe(WINBUTTON_JOG_TO_START, 0);
}

void JogYScreen::jogToEndPosition() {
    auto& cutData = _mgr.GetCutData();
    MotionController::Instance().moveToWithRate(AXIS_Y, cutData.cutEndPoint, 0.5f);
    showButtonSafe(WINBUTTON_JOG_TO_END, 1);
    delay(200);
    showButtonSafe(WINBUTTON_JOG_TO_END, 0);
}

void JogYScreen::jogToRetractPosition() {
    auto& cutData = _mgr.GetCutData();
    float yHomePos = 0.0f;
    float desiredRetractPos = cutData.cutStartPoint - cutData.retractDistance;
    if (desiredRetractPos < yHomePos) {
        desiredRetractPos = yHomePos;
    }
    MotionController::Instance().moveToWithRate(AXIS_Y, desiredRetractPos, 0.5f);
    showButtonSafe(WINBUTTON_JOG_TO_RETRACT, 1);
    delay(200);
    showButtonSafe(WINBUTTON_JOG_TO_RETRACT, 0);
}

void JogYScreen::toggleJogMode() {
    auto& mpg = MPGJogManager::Instance();
    auto& ui = UIInputManager::Instance();

    bool newState = !mpg.isEnabled();

    if (newState) {
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
        mpg.setEnabled(true);
        mpg.setAxis(AXIS_Y);
        showButtonSafe(WINBUTTON_ACTIVATE_JOG_Y_F6, 1);
    }
    else {
        mpg.setEnabled(false);
        showButtonSafe(WINBUTTON_ACTIVATE_JOG_Y_F6, 0);
    }
}

void JogYScreen::setLengthWithMPG() {
    auto& cutData = _mgr.GetCutData();
    auto& ui = UIInputManager::Instance();
    auto& mpg = MPGJogManager::Instance();

    _mpgSetLengthMode = !_mpgSetLengthMode;

    if (_mpgSetLengthMode) {
        if (_mpgSetRetractMode) {
            ui.unbindField();
            _mpgSetRetractMode = false;
            showButtonSafe(WINBUTTON_SET_RETRACT_WITH_MPG_F6, 0);
        }
        if (mpg.isEnabled()) {
            mpg.setEnabled(false);
            showButtonSafe(WINBUTTON_ACTIVATE_JOG_Y_F6, 0);
        }
        _tempLength = cutData.cutLength;
        showButtonSafe(WINBUTTON_SET_WITH_MPG_F6, 1);
        ui.bindField(WINBUTTON_SET_WITH_MPG_F6, LEDDIGITS_CUT_LENGTH_Y,
            &_tempLength, 0.0f, 100.0f, MPG_FIXED_INCREMENT, 3);
    }
    else {
        if (ui.isFieldActive(WINBUTTON_SET_WITH_MPG_F6)) {
            cutData.cutLength = _tempLength;
            cutData.cutEndPoint = cutData.cutStartPoint + cutData.cutLength;
            ui.unbindField();
            genie.WriteObject(GENIE_OBJ_LEDDIGITS,
                LEDDIGITS_CUT_STOP_Y,
                static_cast<uint16_t>(cutData.cutEndPoint * 1000));
        }
        showButtonSafe(WINBUTTON_SET_WITH_MPG_F6, 0);
    }
}

void JogYScreen::setRetractWithMPG() {
    auto& cutData = _mgr.GetCutData();
    auto& ui = UIInputManager::Instance();
    auto& mpg = MPGJogManager::Instance();

    ClearCore::ConnectorUsb.Send("[JogY] setRetractWithMPG button pressed, current mode: ");
    ClearCore::ConnectorUsb.SendLine(_mpgSetRetractMode ? "ON" : "OFF");

    _mpgSetRetractMode = !_mpgSetRetractMode;

    if (_mpgSetRetractMode) {
        ClearCore::ConnectorUsb.SendLine("[JogY] Entering retract MPG mode");
        if (_mpgSetLengthMode) {
            ui.unbindField();
            _mpgSetLengthMode = false;
            showButtonSafe(WINBUTTON_SET_WITH_MPG_F6, 0);
        }
        if (mpg.isEnabled()) {
            mpg.setEnabled(false);
            showButtonSafe(WINBUTTON_ACTIVATE_JOG_Y_F6, 0);
        }
        _tempRetract = cutData.retractDistance;
        showButtonSafe(WINBUTTON_SET_RETRACT_WITH_MPG_F6, 1);
        ClearCore::ConnectorUsb.Send("[JogY] Binding field to button ID: ");
        ClearCore::ConnectorUsb.Send(WINBUTTON_SET_RETRACT_WITH_MPG_F6);
        ClearCore::ConnectorUsb.Send(", LED ID: ");
        ClearCore::ConnectorUsb.SendLine(LEDDIGITS_RETRACT_DISTANCE);

        ui.bindField(WINBUTTON_SET_RETRACT_WITH_MPG_F6, LEDDIGITS_RETRACT_DISTANCE,
            &_tempRetract, 0.0f, 100.0f, MPG_FIXED_INCREMENT, 3);

        ClearCore::ConnectorUsb.Send("[JogY] Field active status: ");
        ClearCore::ConnectorUsb.SendLine(ui.isFieldActive(WINBUTTON_SET_RETRACT_WITH_MPG_F6) ? "ACTIVE" : "NOT ACTIVE");
    }
    else {
        ClearCore::ConnectorUsb.SendLine("[JogY] Exiting retract MPG mode");
        if (ui.isFieldActive(WINBUTTON_SET_RETRACT_WITH_MPG_F6)) {
            cutData.retractDistance = _tempRetract;
            ui.unbindField();
            genie.WriteObject(GENIE_OBJ_LEDDIGITS,
                LEDDIGITS_RETRACT_DISTANCE,
                static_cast<uint16_t>(cutData.retractDistance * 1000));
        }
        showButtonSafe(WINBUTTON_SET_RETRACT_WITH_MPG_F6, 0);
    }
}

void JogYScreen::updateCutLengthDisplay() {
    auto& cutData = _mgr.GetCutData();
    cutData.cutLength = cutData.cutEndPoint - cutData.cutStartPoint;
    if (cutData.cutLength < 0) cutData.cutLength = 0;

    genie.WriteObject(GENIE_OBJ_LEDDIGITS,
        LEDDIGITS_CUT_LENGTH_Y,
        static_cast<uint16_t>(cutData.cutLength * 1000));
}

void JogYScreen::updateAllDisplays() {
    auto& cutData = _mgr.GetCutData();
    genie.WriteObject(GENIE_OBJ_LEDDIGITS,
        LEDDIGITS_STOCK_END_Y,
        static_cast<uint16_t>(cutData.cutStartPoint * 1000));
    genie.WriteObject(GENIE_OBJ_LEDDIGITS,
        LEDDIGITS_CUT_STOP_Y,
        static_cast<uint16_t>(cutData.cutEndPoint * 1000));
    genie.WriteObject(GENIE_OBJ_LEDDIGITS,
        LEDDIGITS_RETRACT_DISTANCE,
        static_cast<uint16_t>(cutData.retractDistance * 1000));
    updateCutLengthDisplay();

    float currentPos = MotionController::Instance().getAxisPosition(AXIS_Y);
    genie.WriteObject(GENIE_OBJ_LEDDIGITS,
        LEDDIGITS_TABLE_POSITION_Y,
        static_cast<uint16_t>(currentPos * 1000));
}

void JogYScreen::update() {
    auto& cutData = _mgr.GetCutData();
    float pos = MotionController::Instance().getAxisPosition(AXIS_Y);
    genie.WriteObject(GENIE_OBJ_LEDDIGITS,
        LEDDIGITS_TABLE_POSITION_Y,
        static_cast<uint16_t>(pos * 1000));

    float distanceToStart = fabs(pos - cutData.cutStartPoint);
    if (distanceToStart <= 0.002f) {
        genie.WriteObject(GENIE_OBJ_LED, LED_AT_START_POSITION_Y, 1);
    }
    else {
        genie.WriteObject(GENIE_OBJ_LED, LED_AT_START_POSITION_Y, 0);
    }

    static uint32_t lastCheckTime = 0;
    uint32_t now = ClearCore::TimingMgr.Milliseconds();

    if (now - lastCheckTime > 500) {
        lastCheckTime = now;
        if (_mpgSetLengthMode) {
            genie.WriteObject(GENIE_OBJ_WINBUTTON, WINBUTTON_SET_WITH_MPG_F6, 1);
        }
        if (_mpgSetRetractMode) {
            genie.WriteObject(GENIE_OBJ_WINBUTTON, WINBUTTON_SET_RETRACT_WITH_MPG_F6, 1);
        }
        auto& mpg = MPGJogManager::Instance();
        if (mpg.isEnabled()) {
            genie.WriteObject(GENIE_OBJ_WINBUTTON, WINBUTTON_ACTIVATE_JOG_Y_F6, 1);
        }
    }
}
