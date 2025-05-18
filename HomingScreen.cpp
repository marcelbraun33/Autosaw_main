
// === HomingScreen.cpp ===
#include "HomingScreen.h"
#include "Config.h"
#include "ScreenManager.h"
#include <ClearCore.h>

void HomingScreen::onShow() {
    _xDone = false;
    _yDone = false;
    ClearCore::ConnectorUsb.SendLine("[HOMING] Waiting for X and Y to assert HLFB");
}

void HomingScreen::update() {
    // Check X and Y HLFB status
    bool xAssert = (MOTOR_FENCE_X.HlfbState() == MotorDriver::HLFB_ASSERTED);
    bool yAssert = (MOTOR_TABLE_Y.HlfbState() == MotorDriver::HLFB_ASSERTED);

    // Log each axis completion once
    if (xAssert && !_xDone) {
        _xDone = true;
        ClearCore::ConnectorUsb.SendLine("[HOMING] X axis homing complete");
    }
    if (yAssert && !_yDone) {
        _yDone = true;
        ClearCore::ConnectorUsb.SendLine("[HOMING] Y axis homing complete");
    }

    // When both are done, transition back to manual mode
    if (_xDone && _yDone) {
        ClearCore::ConnectorUsb.SendLine("[HOMING] Both axes homed — returning to Manual Mode");
        ScreenManager::Instance().ShowManualMode();
    }
}
