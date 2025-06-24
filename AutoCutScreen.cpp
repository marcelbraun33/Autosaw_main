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

// ENHANCED: onHide with spindle shutdown
void AutoCutScreen::onHide() {
    auto& cutSeq = CutSequenceController::Instance();
    auto& motion = MotionController::Instance();

    // Only abort if actually running to avoid unnecessary work
    if (cutSeq.isActive()) {
        ClearCore::ConnectorUsb.SendLine("[AutoCut] Aborting active sequence due to screen exit");
        cutSeq.abort();
    }

    // CRITICAL: Always stop spindle when exiting AutoCut screen
    if (motion.IsSpindleRunning()) {
        ClearCore::ConnectorUsb.SendLine("[AutoCut] Stopping spindle due to screen exit");
        motion.StopSpindle();
        updateButtonState(WINBUTTON_SPINDLE_F5, false, nullptr, 0);
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

    // Check if setup has been properly configured
    if (cutSeq.getTotalCuts() == 0) {
        ClearCore::ConnectorUsb.SendLine("[AutoCut] Error: No cuts configured - press Setup Auto Cut first");
        flashSetupAutocutButton();
        updateButtonState(WINBUTTON_START_AUTOFEED_F5, false, nullptr, 0);
        return;
    }

    // Check if all cuts are already complete
    if (cutSeq.getRemainingPositions() == 0) {
        ClearCore::ConnectorUsb.SendLine("[AutoCut] Error: All cuts already completed - reset from Jog X screen");
        flashButtonError(WINBUTTON_START_AUTOFEED_F5);
        return;
    }

    // Validate batch size
    int configuredBatchSize = cutSeq.getBatchSize();
    int remainingCuts = cutSeq.getRemainingPositions();

    if (configuredBatchSize <= 0) {
        ClearCore::ConnectorUsb.SendLine("[AutoCut] Error: Invalid batch size - please configure in Setup Auto Cut");
        flashSetupAutocutButton();
        updateButtonState(WINBUTTON_START_AUTOFEED_F5, false, nullptr, 0);
        return;
    }

    ClearCore::ConnectorUsb.Send("[AutoCut] Configured batch size from Setup: ");
    ClearCore::ConnectorUsb.SendLine(configuredBatchSize);
    ClearCore::ConnectorUsb.Send("[AutoCut] Remaining cuts available: ");
    ClearCore::ConnectorUsb.SendLine(remainingCuts);

    // Validate batch size doesn't exceed remaining cuts
    if (configuredBatchSize > remainingCuts) {
        ClearCore::ConnectorUsb.Send("[AutoCut] Warning: Batch size (");
        ClearCore::ConnectorUsb.Send(configuredBatchSize);
        ClearCore::ConnectorUsb.Send(") exceeds remaining cuts (");
        ClearCore::ConnectorUsb.Send(remainingCuts);
        ClearCore::ConnectorUsb.SendLine("), adjusting to remaining cuts");
        cutSeq.setBatchSize(remainingCuts);
        configuredBatchSize = remainingCuts;
    }

    // Notify torque control UI that cutting is active
    _torqueControlUI.setCuttingActive(true);

    // Get the adjusted values to use for the cycle
    float cutPressure = _torqueControlUI.getCurrentCutPressure();
    float feedRate = _torqueControlUI.getCurrentFeedRate();

    // Apply these values before starting
    MotionController::Instance().setTorqueTarget(AXIS_Y, cutPressure);

    // Start the batch sequence using the configured batch size
    if (cutSeq.startBatchSequence()) {
        ClearCore::ConnectorUsb.Send("[AutoCut] Batch started: ");
        ClearCore::ConnectorUsb.Send(configuredBatchSize);
        ClearCore::ConnectorUsb.Send(" cuts configured, ");
        ClearCore::ConnectorUsb.Send(remainingCuts);
        ClearCore::ConnectorUsb.SendLine(" total remaining (spindle will start automatically)");
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
    else if (cutSeq.isPaused()) {
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

// ENHANCED: cancelCycle with spindle control
void AutoCutScreen::cancelCycle() {
    auto& cutSeq = CutSequenceController::Instance();
    auto& motion = MotionController::Instance();

    ClearCore::ConnectorUsb.SendLine("[AutoCut] Cancel cycle requested");

    // Abort the sequence (this will stop spindle if auto-controlled)
    cutSeq.abort();

    // CRITICAL: Reset torque control properly
    motion.abortTorqueControlledFeed(AXIS_Y);
    motion.setTorqueTarget(AXIS_Y, 0.0f);  // Clear torque target

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

// ENHANCED: moveToStartPosition with spindle shutdown
void AutoCutScreen::moveToStartPosition() {
    auto& cutSeq = CutSequenceController::Instance();
    auto& motion = MotionController::Instance();

    ClearCore::ConnectorUsb.SendLine("[AutoCut] Move to Start Position (Rapid to Job Zero) - Exit and Return");

    // CRITICAL: Always stop spindle before exit and return
    if (motion.IsSpindleRunning()) {
        ClearCore::ConnectorUsb.SendLine("[AutoCut] Stopping spindle for Exit and Return");
        motion.StopSpindle();
        updateButtonState(WINBUTTON_SPINDLE_F5, false, nullptr, 0);
    }

    // If sequence is active, abort it first
    if (cutSeq.isActive()) {
        ClearCore::ConnectorUsb.SendLine("[AutoCut] Aborting active sequence for Exit and Return");
        cutSeq.abort();
        updateButtonState(WINBUTTON_START_AUTOFEED_F5, false, nullptr, 0);
        updateButtonState(WINBUTTON_SLIDE_HOLD_F5, false, nullptr, 0);
        _torqueControlUI.setCuttingActive(false);
    }

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
    motion.moveTo(AXIS_Y, desiredRetractPos, 1.0f);
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

// Simple helper method for Setup Auto Cut button flashing
void AutoCutScreen::flashSetupAutocutButton() {
    ClearCore::ConnectorUsb.SendLine("[AutoCut] Please configure cutting parameters in Setup Auto Cut first");

    // Flash the Setup Auto Cut button to draw attention
    for (int i = 0; i < 5; i++) {
        updateButtonState(WINBUTTON_SETUP_AUTOCUT_F5, true, nullptr, 200);
        updateButtonState(WINBUTTON_SETUP_AUTOCUT_F5, false, nullptr, 200);
    }
    updateButtonState(WINBUTTON_SETUP_AUTOCUT_F5, false, nullptr, 0);
}

// ENHANCED: toggleSpindle with sequence awareness
void AutoCutScreen::toggleSpindle() {
    auto& motion = MotionController::Instance();
    auto& cutSeq = CutSequenceController::Instance();

    if (motion.IsSpindleRunning()) {
        motion.StopSpindle();
        updateButtonState(WINBUTTON_SPINDLE_F5, false, "[AutoCut] Spindle stopped manually", 0);

        // If sequence is running, mark as manual control
        if (cutSeq.isActive() || cutSeq.isPaused()) {
            cutSeq.manualSpindleStop();
        }
    }
    else {
        // Get RPM from settings
        float rpm = SettingsManager::Instance().settings().spindleRPM;

        ClearCore::ConnectorUsb.Send("[AutoCut] Starting spindle manually at ");
        ClearCore::ConnectorUsb.Send(rpm);
        ClearCore::ConnectorUsb.SendLine(" RPM");

        motion.StartSpindle(rpm);
        updateButtonState(WINBUTTON_SPINDLE_F5, true, "[AutoCut] Spindle started manually", 0);

        // If sequence is running, mark as manual control
        if (cutSeq.isActive() || cutSeq.isPaused()) {
            cutSeq.manualSpindleStart();
        }
    }

    updateDisplay();
}

void AutoCutScreen::openSettings() {
    ScreenManager::Instance().ShowSettings();
}

// ENHANCED: updateDisplay with spindle state information
void AutoCutScreen::updateDisplay() {
    auto& seq = CutSequenceController::Instance();
    auto& posData = CutPositionData::Instance();
    auto& cutData = ScreenManager::Instance().GetCutData();
    auto& motion = MotionController::Instance();

    // Stock Length (inches, scaled to 0.001)
    float stockLength = cutData.stockLength;
    int32_t scaledStockLength = static_cast<int32_t>(stockLength * 1000.0f);
    genie.WriteObject(GENIE_OBJ_LED_DIGITS, LEDDIGITS_STOCK_LENGTH_F5, static_cast<uint16_t>(scaledStockLength));

    // Cutting Position - Calculate real-time position relative to stock zero
    int currentCutPosition = 0;
    if (cutData.useStockZero && cutData.increment > 0.0f) {
        // Get current absolute X position
        float currentX = motion.getAbsoluteAxisPosition(AXIS_X);

        // Calculate position relative to stock zero
        float relativeX = currentX - cutData.positionZero;

        // Calculate which "slice position" this represents
        currentCutPosition = static_cast<int>(round(relativeX / cutData.increment));

        // Ensure it's not negative
        if (currentCutPosition < 0) currentCutPosition = 0;
    }
    else if (!cutData.useStockZero && cutData.increment > 0.0f) {
        // If not using stock zero, calculate from absolute zero
        float currentX = motion.getAbsoluteAxisPosition(AXIS_X);
        currentCutPosition = static_cast<int>(round(currentX / cutData.increment));
        if (currentCutPosition < 0) currentCutPosition = 0;
    }

    genie.WriteObject(GENIE_OBJ_LED_DIGITS, LEDDIGITS_CUTTING_POSITION_F5, static_cast<uint16_t>(currentCutPosition));

    // Total Slices/Positions
    int totalSlices = seq.getTotalCuts();
    genie.WriteObject(GENIE_OBJ_LED_DIGITS, LEDDIGITS_TOTAL_SLICES_F5, static_cast<uint16_t>(totalSlices));

    // Job Remaining Cuts
    int remainingCuts = seq.getRemainingPositions();
    genie.WriteObject(GENIE_OBJ_LED_DIGITS, LEDDIGITS_JOB_REMAINING_F5, static_cast<uint16_t>(remainingCuts));

    // Show progress percentage
    float progress = seq.getBatchProgressPercent();

    // Enhanced state-specific information with spindle states
    switch (seq.getState()) {
    case CutSequenceController::SEQUENCE_STARTING_SPINDLE:
        ClearCore::ConnectorUsb.SendLine("[AutoCut] Display: Starting spindle...");
        break;
    case CutSequenceController::SEQUENCE_SPINDLE_READY:
        ClearCore::ConnectorUsb.SendLine("[AutoCut] Display: Spindle ready");
        break;
    case CutSequenceController::SEQUENCE_CUTTING: {
        // Show distance to go in current cut
        float yCurrentPos = motion.getAbsoluteAxisPosition(AXIS_Y);
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
        ClearCore::ConnectorUsb.SendLine("[AutoCut] Display: Sequence completed");
        break;
    }

    // Enhanced Spindle RPM display
    uint16_t rpm = motion.IsSpindleRunning() ?
        static_cast<uint16_t>(motion.CommandedRPM()) : 0;
    genie.WriteObject(GENIE_OBJ_LED_DIGITS, LEDDIGITS_RPM_F5, rpm);

    // Update spindle button state to match actual spindle status
    bool spindleRunning = motion.IsSpindleRunning();
    static bool lastSpindleState = false;
    if (spindleRunning != lastSpindleState) {
        updateButtonState(WINBUTTON_SPINDLE_F5, spindleRunning, nullptr, 0);
        lastSpindleState = spindleRunning;
    }

    // Thickness
    float thickness = cutData.thickness;
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

// ENHANCED: update with spindle state monitoring
void AutoCutScreen::update() {
    // CRITICAL: Update the cut sequence controller state machine
    CutSequenceController::Instance().update();

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

    // Update display regularly
    updateDisplay();
}