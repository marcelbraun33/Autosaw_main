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
    // Only abort if actually running to avoid unnecessary work
    if (CutSequenceController::Instance().isActive()) {
        AutoCutCycleManager::Instance().abortCycle();
    }
    _torqueControlUI.onHide();  // Clean up torque control UI
}

void AutoCutScreen::handleEvent(const genieFrame& e) {
    if (e.reportObject.cmd != GENIE_REPORT_EVENT) return;

    switch (e.reportObject.object) {
    case GENIE_OBJ_WINBUTTON:
        switch (e.reportObject.index) {
        case WINBUTTON_START_AUTOFEED_F5:           // 25 - Start Auto Feed
            startCycle();
            break;
        case WINBUTTON_SLIDE_HOLD_F5:               // 23 - Pause/Resume toggle
            togglePauseResume();
            break;
        case WINBUTTON_END_CYCLE_F5:                // 22 - Stop/Cancel cycle
            cancelCycle();
            break;
        case WINBUTTON_ADJUST_CUT_PRESSURE_F5:      // 43 - Adjust cut pressure
            adjustCutPressure();
            break;
        case WINBUTTON_ADJUST_MAX_SPEED_F5:         // 45 - Adjust max speed
            adjustMaxSpeed();
            break;
        case WINBUTTON_MOVE_TO_START_POSITION:      // 44 - Rapid to Zero
            moveToStartPosition();
            break;
        case WINBUTTON_SPINDLE_F5:                  // 24 - Toggle spindle
            toggleSpindle();
            break;
        case WINBUTTON_SETTINGS_F5:                 // 26 - Open settings
            openSettings();
            break;
        case WINBUTTON_SETUP_AUTOCUT_F5:            // 46 - Setup Auto Cut
            openSetupAutocutScreen();
            break;
        }
        break;
    }
}

void AutoCutScreen::startCycle() {
    ClearCore::ConnectorUsb.SendLine("[AutoCut] Start Cycle requested");

    // Visual feedback
    updateButtonState(WINBUTTON_START_AUTOFEED_F5, true, nullptr, 0);

    auto& cutSeq = CutSequenceController::Instance();

    // Check if already running
    if (cutSeq.isActive()) {
        ClearCore::ConnectorUsb.SendLine("[AutoCut] Error: Cycle already running");
        flashButtonError(WINBUTTON_START_AUTOFEED_F5);
        return;
    }

    // Validate that setup has been done
    if (cutSeq.getTotalCuts() == 0) {
        ClearCore::ConnectorUsb.SendLine("[AutoCut] Error: No cuts configured - press Setup Auto Cut first");
        flashButtonError(WINBUTTON_START_AUTOFEED_F5);
        return;
    }

    // Check if all cuts are already complete
    if (cutSeq.getRemainingPositions() == 0) {
        ClearCore::ConnectorUsb.SendLine("[AutoCut] Error: All cuts already completed - reset from Jog X screen");
        flashButtonError(WINBUTTON_START_AUTOFEED_F5);
        return;
    }

    // Notify torque control UI that cutting is active
    _torqueControlUI.setCuttingActive(true);

    // Get the adjusted values to use for the cycle
    float cutPressure = _torqueControlUI.getCurrentCutPressure();
    float feedRate = _torqueControlUI.getCurrentFeedRate();

    // Apply these values before starting
    MotionController::Instance().setTorqueTarget(AXIS_Y, cutPressure);

    // Set a reasonable batch size (limit for safety)
    int remainingCuts = cutSeq.getRemainingPositions();
    int batchSize = remainingCuts > 5 ? 5 : remainingCuts; // Max 5 cuts per batch
    cutSeq.setBatchSize(batchSize);

    // Start the batch sequence
    if (cutSeq.startBatchSequence()) {
        ClearCore::ConnectorUsb.Send("[AutoCut] Batch started: ");
        ClearCore::ConnectorUsb.Send(static_cast<int>(batchSize));
        ClearCore::ConnectorUsb.Send(" cuts, ");
        ClearCore::ConnectorUsb.Send(static_cast<int>(remainingCuts));
        ClearCore::ConnectorUsb.SendLine(" total remaining");

        // Keep start button highlighted while running
        // (will be cleared in update() when cycle completes)
    }
    else {
        ClearCore::ConnectorUsb.SendLine("[AutoCut] Failed to start batch sequence");
        _torqueControlUI.setCuttingActive(false);
        updateButtonState(WINBUTTON_START_AUTOFEED_F5, false, nullptr, 0);
    }

    updateDisplay();
}

void AutoCutScreen::togglePauseResume() {
    auto& cutSeq = CutSequenceController::Instance();

    if (cutSeq.isActive()) {
        // Currently running - pause it
        cutSeq.pause();
        updateButtonState(WINBUTTON_SLIDE_HOLD_F5, true, "[AutoCut] Cycle paused", 0);
    }
    else {
        // Currently paused - resume it
        cutSeq.resume();
        updateButtonState(WINBUTTON_SLIDE_HOLD_F5, false, "[AutoCut] Cycle resumed", 0);
    }

    updateDisplay();
}

void AutoCutScreen::pauseCycle() {
    // Legacy method - redirect to toggle
    togglePauseResume();
}

void AutoCutScreen::resumeCycle() {
    auto& cutSeq = CutSequenceController::Instance();
    cutSeq.resume();
    updateButtonState(WINBUTTON_SLIDE_HOLD_F5, false, "[AutoCut] Cycle resumed", 0);
    updateDisplay();
}

void AutoCutScreen::cancelCycle() {
    auto& cutSeq = CutSequenceController::Instance();
    cutSeq.abort();
    _torqueControlUI.setCuttingActive(false);
    updateButtonState(WINBUTTON_START_AUTOFEED_F5, false, "[AutoCut] Cycle cancelled", 0);
    updateButtonState(WINBUTTON_SLIDE_HOLD_F5, false, nullptr, 0);
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
    ClearCore::ConnectorUsb.SendLine("[AutoCut] Move to Start Position (Rapid to Job Zero)");

    // Visual feedback
    updateButtonState(WINBUTTON_MOVE_TO_START_POSITION, true, nullptr, 0);

    // Get Y job zero position using the same calculation as JogY screen
    auto& cutData = _mgr.GetCutData();
    float yHomePos = 0.0f;
    float desiredRetractPos = cutData.cutStartPoint - cutData.retractDistance;
    if (desiredRetractPos < yHomePos) {
        desiredRetractPos = yHomePos;
    }

    ClearCore::ConnectorUsb.Send("[AutoCut] Y Job Zero calculation: Cut Start ");
    ClearCore::ConnectorUsb.Send(cutData.cutStartPoint);
    ClearCore::ConnectorUsb.Send(" - Retract Distance ");
    ClearCore::ConnectorUsb.Send(cutData.retractDistance);
    ClearCore::ConnectorUsb.Send(" = ");
    ClearCore::ConnectorUsb.Send(desiredRetractPos);
    ClearCore::ConnectorUsb.SendLine(" (limited to >= 0.0)");

    // Start by moving Y to retract position at full speed
    MotionController::Instance().moveTo(AXIS_Y, desiredRetractPos, 1.0f);
    _rapidState = MovingYToRetract;

    delay(200);
    updateButtonState(WINBUTTON_MOVE_TO_START_POSITION, false, nullptr, 0);
}

void AutoCutScreen::openSetupAutocutScreen() {
    ScreenManager::Instance().ShowSetupAutocut();
}

void AutoCutScreen::flashButtonError(uint16_t buttonId) {
    // Flash button to indicate error
    for (int i = 0; i < 3; i++) {
        updateButtonState(buttonId, false, nullptr, 150);
        updateButtonState(buttonId, true, nullptr, 150);
    }
    updateButtonState(buttonId, false, nullptr, 0);
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

    // Job Remaining Cuts
    int remainingCuts = seq.getRemainingPositions();
    genie.WriteObject(GENIE_OBJ_LED_DIGITS, LEDDIGITS_JOB_REMAINING_F5, static_cast<uint16_t>(remainingCuts));

    // Show progress percentage
    float progress = seq.getBatchProgressPercent();
    // You could add a progress bar or percentage display

    // Show state-specific information
    switch (seq.getState()) {
    case CutSequenceController::SEQUENCE_CUTTING: {
        // Show distance to go in current cut
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
        // Calculate the same retract position as in moveToStartPosition()
        auto& cutData = _mgr.GetCutData();
        float yHomePos = 0.0f;
        float desiredRetractPos = cutData.cutStartPoint - cutData.retractDistance;
        if (desiredRetractPos < yHomePos) {
            desiredRetractPos = yHomePos;
        }

        float yCurrent = MotionController::Instance().getAbsoluteAxisPosition(AXIS_Y);
        if (fabs(yCurrent - desiredRetractPos) < 0.01f) { // Tolerance
            // Now move X to zero
            auto& cutDataForX = ScreenManager::Instance().GetCutData();
            float xZero = cutDataForX.useStockZero ? cutDataForX.positionZero : 0.0f;
            MotionController::Instance().moveTo(AXIS_X, xZero, 1.0f); // Full speed
            _rapidState = MovingXToZero;
        }
    }
    else if (_rapidState == MovingXToZero) {
        auto& cutData = ScreenManager::Instance().GetCutData();
        float xZero = cutData.useStockZero ? cutData.positionZero : 0.0f;
        float xCurrent = MotionController::Instance().getAbsoluteAxisPosition(AXIS_X);
        if (fabs(xCurrent - xZero) < 0.01f) { // Tolerance
            _rapidState = RapidIdle;
            ClearCore::ConnectorUsb.SendLine("[AutoCut] Rapid to start position complete");
            updateDisplay();
        }
    }

    // Update button states based on sequence status
    auto& cutSeq = CutSequenceController::Instance();
    static bool wasActive = false;
    bool isActive = cutSeq.isActive();

    // Update Start/Stop button states
    if (isActive != wasActive) {
        if (isActive) {
            // Sequence just started - keep start button lit
            updateButtonState(WINBUTTON_START_AUTOFEED_F5, true, nullptr, 0);
        }
        else {
            // Sequence just stopped - turn off start button
            updateButtonState(WINBUTTON_START_AUTOFEED_F5, false, nullptr, 0);
            updateButtonState(WINBUTTON_SLIDE_HOLD_F5, false, nullptr, 0);
            _torqueControlUI.setCuttingActive(false);

            // Check completion status
            if (cutSeq.getRemainingPositions() == 0) {
                ClearCore::ConnectorUsb.SendLine("[AutoCut] All cuts completed!");
            }
            else {
                ClearCore::ConnectorUsb.Send("[AutoCut] Batch completed. ");
                ClearCore::ConnectorUsb.Send(static_cast<int>(cutSeq.getRemainingPositions()));
                ClearCore::ConnectorUsb.SendLine(" cuts remaining.");
            }
        }
        wasActive = isActive;
    }

    // Check if cycle completed via AutoCutCycleManager (legacy support)
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