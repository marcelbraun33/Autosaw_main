
// === MotionController.cpp ===
#include "MotionController.h"
#include "Config.h"
#include <ClearCore.h>

MotionController& MotionController::Instance() {
    static MotionController instance;
    return instance;
}

MotionController::MotionController() = default;

void MotionController::setup() {
    spindle.Setup();
    xAxis.Setup();
    yAxis.Setup();
    zAxis.Setup();
}

void MotionController::update() {
    xAxis.Update();
    yAxis.Update();
    zAxis.Update();

    if (spindle.IsRunning() && !SAFETY_RELAY_OUTPUT.State()) {
        SAFETY_RELAY_OUTPUT.State(true);
    }
}

void MotionController::StartSpindle(float rpm) {
    if (!SAFETY_RELAY_OUTPUT.State()) return;
    spindle.Start(rpm);
}

void MotionController::StopSpindle() {
    spindle.Stop();
}

bool MotionController::IsSpindleRunning() const {
    return spindle.IsRunning();
}

float MotionController::CommandedRPM() const {
    return spindle.CommandedRPM();
}

bool MotionController::moveToWithRate(AxisId axis, float target, float rate) {
    switch (axis) {
    case AXIS_X: return xAxis.MoveTo(target, rate);
    case AXIS_Y: return yAxis.MoveTo(target, rate);
    case AXIS_Z: return zAxis.MoveTo(target, rate);
    }
    return false;
}

float MotionController::getAxisPosition(AxisId axis) const {
    switch (axis) {
    case AXIS_X: return xAxis.GetPosition();
    case AXIS_Y: return yAxis.GetPosition();
    case AXIS_Z: return zAxis.GetPosition();
    }
    return 0.0f;
}

bool MotionController::isAxisMoving(AxisId axis) const {
    switch (axis) {
    case AXIS_X: return xAxis.IsMoving();
    case AXIS_Y: return yAxis.IsMoving();
    case AXIS_Z: return zAxis.IsMoving();
    }
    return false;
}

float MotionController::getTorquePercent(AxisId axis) const {
    switch (axis) {
    case AXIS_X: return xAxis.GetTorquePercent();
    case AXIS_Y: return yAxis.GetTorquePercent();
    case AXIS_Z: return zAxis.GetTorquePercent();
    }
    return 0.0f;
}

void MotionController::EmergencyStop() {
    spindle.EmergencyStop();
    xAxis.EmergencyStop();
    yAxis.EmergencyStop();
    zAxis.EmergencyStop();
}

MotionController::MotionStatus MotionController::getStatus() const {
    MotionStatus s;
    s.spindleRunning = spindle.IsRunning();
    s.spindleRPM = spindle.CommandedRPM();
    s.xPosition = xAxis.GetPosition();
    s.yPosition = yAxis.GetPosition();
    s.zPosition = zAxis.GetPosition();
    s.xMoving = xAxis.IsMoving();
    s.yMoving = yAxis.IsMoving();
    s.zMoving = zAxis.IsMoving();
    s.xHomed = xAxis.IsHomed();
    s.yHomed = yAxis.IsHomed();
    s.zHomed = zAxis.IsHomed();
    return s;
}
