// MotionController.cpp
#include "MotionController.h"
#include "Config.h"
#include "EStopManager.h"
#include <ClearCore.h>
#include "Spindle.h"
#include "XAxis.h"
#include "YAxis.h"
#include "ZAxis.h"
#include "EncoderPositionTracker.h"

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

    // Initialize encoder position tracker
    EncoderPositionTracker::Instance().setup(ENCODER_X_STEPS_PER_INCH, ENCODER_Y_STEPS_PER_INCH);
    ClearCore::ConnectorUsb.SendLine("[MotionController] Absolute position tracking initialized");
}

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

    // Update encoder position tracking
    EncoderPositionTracker::Instance().update();
}

bool MotionController::StartHomingAxis(AxisId a) {
    bool result = false;

    switch (a) {
    case AXIS_X: result = xAxis.StartHoming(); break;
    case AXIS_Y: result = yAxis.StartHoming(); break;
    case AXIS_Z: result = zAxis.StartHoming(); break;
    }

    return result;
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

// New method for getting absolute encoder-verified position
float MotionController::getAbsoluteAxisPosition(AxisId axis) const {
    switch (axis) {
    case AXIS_X:
        return EncoderPositionTracker::Instance().getAbsolutePositionX();
    case AXIS_Y:
        return EncoderPositionTracker::Instance().getAbsolutePositionY();
    case AXIS_Z:
        // Z doesn't have encoder tracking yet, fall back to motor position
        return zAxis.GetPosition();
    default:
        return 0.0f;
    }
}

// New method to verify position against encoder feedback
bool MotionController::verifyAxisPosition(AxisId axis, float expectedInches, float toleranceInches) {
    switch (axis) {
    case AXIS_X:
        return EncoderPositionTracker::Instance().verifyPositionX(expectedInches, toleranceInches);
    case AXIS_Y:
        return EncoderPositionTracker::Instance().verifyPositionY(expectedInches, toleranceInches);
    case AXIS_Z:
        // Z doesn't have encoder tracking yet
        return true;
    default:
        return false;
    }
}

// New method to check if encoder has detected position errors
bool MotionController::hasEncoderPositionError() const {
    return EncoderPositionTracker::Instance().hasPositionError();
}

// New method to clear encoder position errors
void MotionController::clearEncoderPositionErrors() {
    EncoderPositionTracker::Instance().clearPositionError();
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

bool MotionController::moveTo(AxisId axis, float pos, float scale) {
    switch (axis) {
    case AXIS_X: return xAxis.MoveTo(pos, scale);
    case AXIS_Y: return yAxis.MoveTo(pos, scale);
    case AXIS_Z: return zAxis.MoveTo(pos, scale);
    }
    return false;
}

bool MotionController::jogBy(AxisId axis, float deltaInches, float scale) {
    float cur = getAbsoluteAxisPosition(axis);  // Use absolute position
    return moveTo(axis, cur + deltaInches, scale);
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

    // Use absolute encoder positions for status reporting
    s.xPosition = getAbsoluteAxisPosition(AXIS_X);
    s.yPosition = getAbsoluteAxisPosition(AXIS_Y);
    s.zPosition = zAxis.GetPosition();  // Z axis still uses direct position

    s.xMoving = xAxis.IsMoving();
    s.yMoving = yAxis.IsMoving();
    s.zMoving = zAxis.IsMoving();
    s.xHomed = xAxis.IsHomed();
    s.yHomed = yAxis.IsHomed();
    s.zHomed = zAxis.IsHomed();
    return s;
}
// Add these methods to MotionController.cpp after existing code

bool MotionController::startTorqueControlledFeed(AxisId axis, float targetPosition, float initialVelocityScale) {
    if (axis == AXIS_Y) {
        return yAxis.StartTorqueControlledFeed(targetPosition, initialVelocityScale);
    }

    ClearCore::ConnectorUsb.SendLine("[MotionController] Torque controlled feed only supported for Y-axis");
    return false; // Only Y-axis supports torque control currently
}

void MotionController::setTorqueTarget(AxisId axis, float targetPercent) {
    if (axis == AXIS_Y) {
        yAxis.SetTorqueTarget(targetPercent);
    }
}

float MotionController::getTorqueTarget(AxisId axis) const {
    if (axis == AXIS_Y) {
        return yAxis.GetTorqueTarget();
    }
    return 0.0f;
}

bool MotionController::isInTorqueControlledFeed(AxisId axis) const {
    if (axis == AXIS_Y) {
        return yAxis.IsInTorqueControlledFeed();
    }
    return false;
}

void MotionController::abortTorqueControlledFeed(AxisId axis) {
    if (axis == AXIS_Y) {
        yAxis.AbortTorqueControlledFeed();
    }
}

YAxis &MotionController::YAxisInstance() {
    return yAxis;
}

