#include "AutoCutScreen.h"
#include "screenmanager.h"
#include "AutoCutCycleManager.h"
#include "CutSequenceController.h"
#include "JogXScreen.h"
#include "JogYScreen.h"
#include <ClearCore.h>
#include "Config.h"

AutoCutScreen::AutoCutScreen(ScreenManager& mgr) : _mgr(mgr) {}

void AutoCutScreen::onShow() {
    _feedHoldManager.reset();
    updateDisplay();
}

void AutoCutScreen::onHide() {
    AutoCutCycleManager::Instance().abortCycle();
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
    updateDisplay();
}

void AutoCutScreen::exitFeedHold() {
    AutoCutCycleManager::Instance().exitFeedHold();
    updateDisplay();
}

void AutoCutScreen::adjustCutPressure() {
    // Implement cut pressure adjustment logic here
    updateDisplay();
}

void AutoCutScreen::adjustMaxSpeed() {
    // Implement max speed adjustment logic here
    updateDisplay();
}

void AutoCutScreen::moveToStartPosition() {
    // Start by moving Y to retract
    float yRetract = CutSequenceController::Instance().getYRetract();
    MotionController::Instance().moveTo(AXIS_Y, yRetract, 1.0f); // 1.0f = full speed
    _rapidState = MovingYToRetract;
}


void AutoCutScreen::toggleSpindle() {
    // Implement spindle on/off logic here
    updateDisplay();
}

void AutoCutScreen::openSettings() {
    // Implement settings screen navigation here
    // Example: _mgr.ShowSettings();
    updateDisplay();
}

void AutoCutScreen::updateDisplay() {
    auto& seq = CutSequenceController::Instance();
    auto& mgr = AutoCutCycleManager::Instance();

    // Stock Length (inches, scaled to 0.001)
    float stockLength = ScreenManager::Instance().GetCutData().stockLength;
    int32_t scaledStockLength = static_cast<int32_t>(stockLength * 1000.0f);
    genie.WriteObject(GENIE_OBJ_LED_DIGITS, LEDDIGITS_STOCK_LENGTH_F5, static_cast<uint16_t>(scaledStockLength));

    // Cutting Position (0-based: 0 at stock zero, increments as X moves)
    float xCurrent = MotionController::Instance().getAbsoluteAxisPosition(AXIS_X);
    int positionIndex = seq.getPositionIndexForX(xCurrent, 0.01f); // 0.01" tolerance
    genie.WriteObject(GENIE_OBJ_LED_DIGITS, LEDDIGITS_CUTTING_POSITION_F5, static_cast<uint16_t>(positionIndex));

    // Total Slices/Positions (from CutSequenceController)
    int totalSlices = seq.getTotalCuts();
    genie.WriteObject(GENIE_OBJ_LED_DIGITS, LEDDIGITS_TOTAL_SLICES_F5, static_cast<uint16_t>(totalSlices));

    // Current X Position (scaled to 0.001, if you want to display it)
    float xPos = seq.getCurrentX();
    int32_t scaledXPos = static_cast<int32_t>(xPos * 1000.0f);
    // Example: genie.WriteObject(GENIE_OBJ_LED_DIGITS, LEDDIGITS_X_POSITION_F5, static_cast<uint16_t>(scaledXPos));

    // Distance to Go (Y axis: cut stop - current Y, scaled to 0.001)
    float yStop = seq.getYCutStop();
    float yCurrent = MotionController::Instance().getAbsoluteAxisPosition(AXIS_Y);
    float distanceToGo = yStop - yCurrent;
    if (distanceToGo < 0.0f) distanceToGo = 0.0f;
    int32_t scaledDistanceToGo = static_cast<int32_t>(distanceToGo * 1000.0f);
    genie.WriteObject(GENIE_OBJ_LED_DIGITS, LEDDIGITS_DISTANCE_TO_GO_F5, static_cast<uint16_t>(scaledDistanceToGo));

    // (Add more as needed, e.g., thickness, feedrate, etc.)
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
    // ... existing update logic ...
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
            updateDisplay(); // <-- Add this line
            // Optionally: notify user
        }
    }

}
