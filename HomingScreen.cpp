// core/HomingScreen.cpp
#include "HomingScreen.h"
#include "Config.h"
#include "ScreenManager.h"
#include <ClearCore.h>
#include "MotionController.h"
#include "EncoderPositionTracker.h"

// Global timing for initial HLFB delay
static uint32_t gHomingStartTime = 0;
static constexpr uint32_t gHomingDelayMs = 2000;  // 2-second delay
static bool gHomingDelayDone = false;
static bool gHomingSequenceStarted = false;
static bool gXHomingComplete = false;
static bool gYHomingComplete = false;

void HomingScreen::onShow() {
    _xDone = false;
    _yDone = false;
    gHomingDelayDone = false;
    gHomingSequenceStarted = false;
    gXHomingComplete = false;
    gYHomingComplete = false;
    gHomingStartTime = ClearCore::TimingMgr.Milliseconds();
    ClearCore::ConnectorUsb.SendLine("[HOMING] Waiting 2 seconds before homing move");
}

void HomingScreen::update() {
    uint32_t now = ClearCore::TimingMgr.Milliseconds();
    auto& mc = MotionController::Instance();

    // Initial delay before commanding homing
    if (!gHomingDelayDone) {
        if (now - gHomingStartTime >= gHomingDelayMs) {
            gHomingDelayDone = true;
            ClearCore::ConnectorUsb.SendLine("[HOMING] Delay elapsed, starting sequential homing");
            gHomingSequenceStarted = true;
        }
        return;
    }

    // Sequential homing
    if (gHomingSequenceStarted) {
        // Home X first
        if (!gXHomingComplete) {
            if (!_xDone) {
                ClearCore::ConnectorUsb.SendLine("[HOMING] Starting X-axis homing");
                mc.StartHomingAxis(AXIS_X);
                _xDone = true;  // Mark as started
            }

            // Check if X homing has completed
            MotionController::MotionStatus status = mc.getStatus();
            if (status.xHomed) {
                gXHomingComplete = true;
                ClearCore::ConnectorUsb.SendLine("[HOMING] X axis homing complete");
                Delay_ms(500);  // Give a brief pause between axes
            }
        }
        // Once X is done, start Y
        else if (!gYHomingComplete) {
            if (!_yDone) {
                ClearCore::ConnectorUsb.SendLine("[HOMING] Starting Y-axis homing");
                mc.StartHomingAxis(AXIS_Y);
                _yDone = true;  // Mark as started
            }

            // Check if Y homing has completed
            MotionController::MotionStatus status = mc.getStatus();
            if (status.yHomed) {
                gYHomingComplete = true;
                ClearCore::ConnectorUsb.SendLine("[HOMING] Y axis homing complete");
            }
        }

        // If both are complete, continue to manual mode
        if (gXHomingComplete && gYHomingComplete) {
            ClearCore::ConnectorUsb.SendLine("[HOMING] Both axes homed — returning to Manual Mode");
            
            // Reset encoder position tracking after all axes complete homing
            EncoderPositionTracker::Instance().resetPositionAfterHoming();
            ClearCore::ConnectorUsb.SendLine("[HomingScreen] All axes homed, encoder positions reset");

            ScreenManager::Instance().ShowManualMode();
        }
    }
}
