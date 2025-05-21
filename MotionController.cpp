// MotionController.cpp
#include "MotionController.h"
#include "Config.h"
#include "EStopManager.h"
#include <ClearCore.h>
#include "Spindle.h"
#include "XAxis.h"
#include "YAxis.h"
#include "ZAxis.h"

MotionController& MotionController::Instance() {
    static MotionController instance;
    return instance;
}

MotionController::MotionController() = default;



void MotionController::setup() {
    // Clock rate and mode setup for all motors
    MotorMgr.MotorInputClocking(MotorManager::CLOCK_RATE_NORMAL);
    MotorMgr.MotorModeSet(MotorManager::MOTOR_M2M3, Connector::CPM_MODE_STEP_AND_DIR);
    MotorMgr.MotorModeSet(MotorManager::MOTOR_M0M1, Connector::CPM_MODE_A_DIRECT_B_PWM);

    // Then set up individual axes
    spindle.Setup();
    xAxis.Setup();
    yAxis.Setup();
    zAxis.Setup();
}
// Add this to MotionController.cpp
void MotionController::ClearAxisAlerts() {
    ClearCore::ConnectorUsb.SendLine("[MotionController] Clearing all axis alerts");

    // Clear any motor faults by cycling enable
    xAxis.ClearAlerts();
    yAxis.ClearAlerts();
    zAxis.ClearAlerts();
}
void MotionController::update() {
    xAxis.Update();
    yAxis.Update();
    zAxis.Update();
}

bool MotionController::StartHomingAxis(AxisId a) {
    switch (a) {
    case AXIS_X: return xAxis.StartHoming();
    case AXIS_Y: return yAxis.StartHoming();
    case AXIS_Z: return zAxis.StartHoming();
    }
    return false;
}

bool MotionController::StartHomingAll() {
    bool okX = xAxis.StartHoming();
    bool okY = yAxis.StartHoming();
    bool okZ = zAxis.StartHoming();
    return okX && okY && okZ;
}

void MotionController::StartSpindle(float rpm) {
    // Manual clamp in place of constrain()
    if (rpm < 0.0f)                rpm = 0.0f;
    else if (rpm > SPINDLE_MAX_RPM) rpm = SPINDLE_MAX_RPM;

    // Respect E-Stop status
    if (!EStopManager::Instance().isSafetyRelayEnabled()) {
        ClearCore::ConnectorUsb.SendLine("Cannot start spindle: E-STOP ACTIVE");
        return;
    }

    spindle.Start(rpm);

    // Log via ConnectorUsb
    ClearCore::ConnectorUsb.Send("Spindle ON @ ");
    ClearCore::ConnectorUsb.Send(static_cast<int>(rpm));
    ClearCore::ConnectorUsb.SendLine(" RPM");
}

void MotionController::StopSpindle() {
    spindle.Stop();
    ClearCore::ConnectorUsb.SendLine("Spindle OFF");
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
