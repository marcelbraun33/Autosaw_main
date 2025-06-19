#include "AutoCutScreen.h"
#include "screenmanager.h"
#include "AutoCutCycleManager.h"
#include "CutSequenceController.h"
#include "JogXScreen.h"
#include "JogYScreen.h"
#include "MotionController.h"
#include "SettingsManager.h"
#include <ClearCore.h>
#include "Config.h"
#include "CutPositionData.h"

extern Genie genie;

AutoCutScreen::AutoCutScreen(ScreenManager& mgr) : _mgr(mgr) {}

void AutoCutScreen::onShow() {
    _feedHoldManager.reset();

    // Initialize the torque control UI with Form5 display IDs
    _torqueControlUI.init(
        LEDDIGITS_CUT_PRESSURE_F5,      // Cut pressure display
        LEDDIGITS_TARGET_FEEDRATE,       // Target feed rate display (if you have one on Form5)
        IGAUGE_AUTOCUT_FEED_PRESSURE,    // Torque gauge
        LEDDIGITS_FEED_OVERRIDE_F5       // Live feed rate display
    );

    _torqueControlUI.onShow();
    updateDisplay();
}

void AutoCutScreen::onHide() {
    AutoCutCycleManager::Instance().abortCycle();
    _torqueControlUI.onHide();  // Clean up torque control UI
}

void AutoCutScreen::handleEvent(const genieFrame& e) {
    if (e.reportObject.cmd != GENIE_REPORT_EVENT) return;

    switch (e.reportObject.object) {
    case GENIE_OBJ_WINBUTTON:
        switch (e.reportObject.index) {
        case WINBUTTON_START_AUTOFEED_F5:
            startCycle();
            break;
        case WINBUTTON_SLIDE_HOLD_F5:
            pauseCycle();
            break;
        case WINBUTTON_END_CYCLE_F5:
            cancelCycle();
            break;
        case WINBUTTON_ADJUST_CUT_PRESSURE_F5:
            adjustCutPressure();
            break;
        case WINBUTTON_ADJUST_MAX_SPEED_F5:
            adjustMaxSpeed();
            break;
        case WINBUTTON_MOVE_TO_START_POSITION:
            moveToStartPosition();
            break;
        case WINBUTTON_SPINDLE_F5:
            toggleSpindle();
            break;
        case WINBUTTON_SETTINGS_F5:
            openSettings();
            break;
        case WINBUTTON_SETUP_AUTOCUT_F5:  // NEW - Handle Setup Autocut button
            ScreenManager::Instance().ShowSetupAutocut();
            break;
        }
        break;
    }
}

void AutoCutScreen::startCycle() {
    // Access JogXScreen and JogYScreen via _mgr
    std::vector<float> xIncrements = _mgr.GetJogXScreen().getIncrementPositions();
    float yStart = _mgr.GetJogYScreen().getCutStart();
    float yStop = _mgr.GetJogYScreen().getCutStop();
    float yRetract = _mgr.GetJogYScreen().getRetractPosition();

    CutSequenceController::Instance().setXIncrements(xIncrements);
    CutSequenceController::Instance().setYCutStart(yStart);
    CutSequenceController::Instance().setYCutStop(yStop);
    CutSequenceController::Instance().setYRetract(yRetract);

    // Notify torque control UI that cutting is active
    _torqueControlUI.setCuttingActive(true);

    // Get the adjusted values to use for the cycle
    float cutPressure = _torqueControlUI.getCurrentCutPressure();
    float feedRate = _torqueControlUI.getCurrentFeedRate();

    // Apply these values before starting
    MotionController::Instance().setTorqueTarget(AXIS_Y, cutPressure);

    // TODO: You may need to pass feedRate to AutoCutCycleManager if it accepts it
    AutoCutCycleManager::Instance().startCycle();
    updateDisplay();
}

void AutoCutScreen::pauseCycle() {
    AutoCutCycleManager::Instance().feedHold();
    updateDisplay();
}

void AutoCutScreen::resumeCycle() {
    AutoCutCycleManager::Instance().resumeCycle();
    updateDisplay();
}

void AutoCutScreen::cancelCycle() {
    AutoCutCycleManager::Instance().abortCycle();
    _torqueControlUI.setCuttingActive(false);
    updateDisplay();
}

void AutoCutScreen::exitFeedHold() {
    AutoCutCycleManager::Instance().exitFeedHold();
    updateDisplay();
}

void AutoCutScreen::adjustCutPressure() {
    _torqueControlUI.toggleCutPressureAdjustment();

    // Update button states to reflect adjustment mode
    _torqueControlUI.updateButtonStates(
        WINBUTTON_ADJUST_CUT_PRESSURE_F5,
        WINBUTTON_ADJUST_MAX_SPEED_F5
    );

    updateDisplay();
}

void AutoCutScreen::adjustMaxSpeed() {
    _torqueControlUI.toggleFeedRateAdjustment();

    // Update button states to reflect adjustment mode
    _torqueControlUI.updateButtonStates(
        WINBUTTON_ADJUST_CUT_PRESSURE_F5,
        WINBUTTON_ADJUST_MAX_SPEED_F5
    );

    updateDisplay();
}

void AutoCutScreen::moveToStartPosition() {
    // Start by moving Y to retract
    float yRetract = CutSequenceController::Instance().getYRetract();
    MotionController::Instance().moveTo(AXIS_Y, yRetract, 1.0f); // 1.0f = full speed
    _rapidState = MovingYToRetract;
}

void AutoCutScreen::toggleSpindle() {
    auto& motion = MotionController::Instance();

    if (motion.IsSpindleRunning()) {
        motion.StopSpindle();
        updateButtonState(WINBUTTON_SPINDLE_F5, false, "[AutoCut] Spindle stopped", 0);
    }
    else {
        // Get RPM from settings
        float rpm = SettingsManager::Instance().settings().spindleRPM;

        ClearCore::ConnectorUsb.Send("[AutoCut] Starting spindle at rpm: ");
        ClearCore::ConnectorUsb.SendLine(rpm);

        motion.StartSpindle(rpm);
        updateButtonState(WINBUTTON_SPINDLE_F5, true, "[AutoCut] Spindle started", 0);
    }

    updateDisplay();
}

void AutoCutScreen::openSettings() {
    ScreenManager::Instance().ShowSettings();
}

void AutoCutScreen::updateDisplay() {
    auto& seq = CutSequenceController::Instance();
    auto& posData = CutPositionData::Instance();

    // Stock Length (inches, scaled to 0.001)
    float stockLength = ScreenManager::Instance().GetCutData().stockLength;
    int32_t scaledStockLength = static_cast<int32_t>(stockLength * 1000.0f);
    genie.WriteObject(GENIE_OBJ_LED_DIGITS, LEDDIGITS_STOCK_LENGTH_F5, static_cast<uint16_t>(scaledStockLength));

    // Cutting Position (1-based)
    int currentCut = seq.getCurrentIndex() + 1;
    genie.WriteObject(GENIE_OBJ_LED_DIGITS, LEDDIGITS_CUTTING_POSITION_F5, static_cast<uint16_t>(currentCut));

    // Total Slices/Positions
    int totalSlices = seq.getTotalCuts();
    genie.WriteObject(GENIE_OBJ_LED_DIGITS, LEDDIGITS_TOTAL_SLICES_F5, static_cast<uint16_t>(totalSlices));

    // Show progress percentage
    float progress = seq.getBatchProgressPercent();
    // You could add a progress bar or percentage display

    // Show state-specific information
    switch (seq.getState()) {
        case CutSequenceController::SEQUENCE_CUTTING: {
            // Show distance to go in current cut - access private members through proper methods
            float yCurrentPos = MotionController::Instance().getAbsoluteAxisPosition(AXIS_Y);
            float yCutStop = seq.getYCutStop();
            float distanceToGo = yCutStop - yCurrentPos;
            if (distanceToGo < 0) distanceToGo = 0;
            genie.WriteObject(GENIE_OBJ_LED_DIGITS, LEDDIGITS_DISTANCE_TO_GO_F5, 
                static_cast<uint16_t>(distanceToGo * 1000));
            break;
        }
        case CutSequenceController::SEQUENCE_MOVING_TO_X:
            // Could show "Moving to position X"
            break;
        case CutSequenceController::SEQUENCE_COMPLETED:
            // Show completion message or flash indicator
            break;
    }

    // Spindle RPM
    uint16_t rpm = MotionController::Instance().IsSpindleRunning() ? 
        static_cast<uint16_t>(MotionController::Instance().CommandedRPM()) : 0;
    genie.WriteObject(GENIE_OBJ_LED_DIGITS, LEDDIGITS_RPM_F5, rpm);

    // Thickness
    float thickness = ScreenManager::Instance().GetCutData().thickness;
    int32_t scaledThickness = static_cast<int32_t>(thickness * 1000.0f);
    genie.WriteObject(GENIE_OBJ_LED_DIGITS, LEDDIGITS_THICKNESS_F5, static_cast<uint16_t>(scaledThickness));
}

void AutoCutScreen::updateButtonState(uint16_t buttonId, bool state, const char* logMessage, uint16_t delayMs) {
    genie.WriteObject(GENIE_OBJ_WINBUTTON, buttonId, state ? 1 : 0);
    if (logMessage) {
        ClearCore::ConnectorUsb.SendLine(logMessage);
    }
    if (delayMs > 0) {
        delay(delayMs);
    }
}

void AutoCutScreen::update() {
    // Update torque control UI (handles gauge updates, encoder input, etc.)
    _torqueControlUI.update();

    // Handle rapid movement state machine
    if (_rapidState == MovingYToRetract) {
        float yRetract = CutSequenceController::Instance().getYRetract();
        float yCurrent = MotionController::Instance().getAbsoluteAxisPosition(AXIS_Y);
        if (fabs(yCurrent - yRetract) < 0.01f) { // Tolerance
            // Now move X to zero
            auto& cutData = ScreenManager::Instance().GetCutData();
            float xZero = cutData.useStockZero ? cutData.positionZero : 0.0f;
            MotionController::Instance().moveTo(AXIS_X, xZero, 1.0f);
            _rapidState = MovingXToZero;
        }
    }
    else if (_rapidState == MovingXToZero) {
        auto& cutData = ScreenManager::Instance().GetCutData();
        float xZero = cutData.useStockZero ? cutData.positionZero : 0.0f;
        float xCurrent = MotionController::Instance().getAbsoluteAxisPosition(AXIS_X);
        if (fabs(xCurrent - xZero) < 0.01f) { // Tolerance
            _rapidState = RapidIdle;
            updateDisplay();
        }
    }

    // Check if cycle completed
    auto& mgr = AutoCutCycleManager::Instance();
    static bool wasInCycle = false;
    bool isInCycle = mgr.isInCycle();

    if (wasInCycle && !isInCycle) {
        // Cycle just completed
        _torqueControlUI.setCuttingActive(false);

        // Exit any adjustment mode
        if (_torqueControlUI.isAdjusting()) {
            _torqueControlUI.exitAdjustmentMode();
            _torqueControlUI.updateButtonStates(
                WINBUTTON_ADJUST_CUT_PRESSURE_F5,
                WINBUTTON_ADJUST_MAX_SPEED_F5
            );
        }
    }
    wasInCycle = isInCycle;

    // Update display regularly
    updateDisplay();
}