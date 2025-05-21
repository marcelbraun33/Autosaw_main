// core/HomingScreen.cpp
#include "HomingScreen.h"
#include "Config.h"
#include "ScreenManager.h"
#include <ClearCore.h>
#include "MotionController.h"

// Global timing for initial HLFB delay
static uint32_t gHomingStartTime = 0;
static constexpr uint32_t gHomingDelayMs = 2000;  // 2-second delay
static bool gHomingDelayDone = false;
static bool gHomingCommanded = false;

void HomingScreen::onShow() {
    _xDone = false;
    _yDone = false;
    gHomingDelayDone = false;
    gHomingCommanded = false;
    gHomingStartTime = ClearCore::TimingMgr.Milliseconds();
    ClearCore::ConnectorUsb.SendLine("[HOMING] Waiting 2 seconds before homing move");
}

void HomingScreen::update() {
    uint32_t now = ClearCore::TimingMgr.Milliseconds();

    // Initial delay before commanding homing move
    if (!gHomingDelayDone) {
        if (now - gHomingStartTime >= gHomingDelayMs) {
            gHomingDelayDone = true;
            ClearCore::ConnectorUsb.SendLine("[HOMING] Delay elapsed, starting homing");

            // Start the homing state machine in each axis
            MotionController::Instance().StartHomingAxis(AXIS_X);
            MotionController::Instance().StartHomingAxis(AXIS_Y);
        }
        return;
    }

    // Check homing progress using the existing methods
    auto& mc = MotionController::Instance();

    // Use getStatus() which already contains the homed state
    MotionController::MotionStatus status = mc.getStatus();
    bool xHomed = status.xHomed;
    bool yHomed = status.yHomed;

    if (xHomed && !_xDone) {
        _xDone = true;
        ClearCore::ConnectorUsb.SendLine("[HOMING] X axis homing complete");
    }

    if (yHomed && !_yDone) {
        _yDone = true;
        ClearCore::ConnectorUsb.SendLine("[HOMING] Y axis homing complete");
    }

    if (_xDone && _yDone) {
        ClearCore::ConnectorUsb.SendLine("[HOMING] Both axes homed — returning to Manual Mode");
        ScreenManager::Instance().ShowManualMode();
    }
}
