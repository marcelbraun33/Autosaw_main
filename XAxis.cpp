// XAxis.cpp
#include "XAxis.h"
#include "Config.h"
#include <ClearCore.h>

static constexpr float MAX_VELOCITY = 10000.0f;   // steps/s
static constexpr float MAX_ACCELERATION = 100000.0f;  // steps/s^2

XAxis::XAxis()
    : _stepsPerInch(FENCE_STEPS_PER_INCH)
    , _motor(&MOTOR_FENCE_X)
{
}

void XAxis::Setup() {
    // Configure HLFB and motion limits
    _motor->HlfbMode(MotorDriver::HLFB_MODE_HAS_BIPOLAR_PWM);
    _motor->HlfbCarrier(MotorDriver::HLFB_CARRIER_482_HZ);
    _motor->VelMax(MAX_VELOCITY);
    _motor->AccelMax(MAX_ACCELERATION);

    // Configure motor for Step/Direction mode
    // The ClearCore motor driver (ConnectorM1) should already be in step/dir mode by default

    // Enable motor
    _motor->EnableRequest(true);

    // Don't block here—let the homing state machine handle HLFB when you actually home
    _isSetup = true;
}

bool XAxis::StartHoming() {
    if (!_isSetup || _homingState != HomingState::Idle)
        return false;

    _isHomed = false;
    _homingStartTime = ClearCore::TimingMgr.Milliseconds();
    _homingState = HomingState::ApproachFast;
    return true;
}

void XAxis::Update() {
    if (!_isSetup)
        return;

    // Update position and move status
    _currentPos = static_cast<float>(_motor->PositionRefCommanded()) / _stepsPerInch;
    _isMoving = !_motor->StepsComplete();
    _torquePct = GetTorquePercent();

    // Drive homing state machine
    if (_homingState != HomingState::Idle) {
        processHoming();
    }
}

void XAxis::processHoming() {
    uint32_t now = ClearCore::TimingMgr.Milliseconds();

    // Add debug logging for homing state changes
    static HomingState lastState = _homingState;
    if (lastState != _homingState) {
        ClearCore::ConnectorUsb.Send("[X-Axis] Homing state: ");
        ClearCore::ConnectorUsb.Send(static_cast<int>(lastState));
        ClearCore::ConnectorUsb.Send(" -> ");
        ClearCore::ConnectorUsb.SendLine(static_cast<int>(_homingState));
        lastState = _homingState;
    }

    switch (_homingState) {
    case HomingState::ApproachFast:
        _motor->VelMax(HOME_VEL_FAST);
        _motor->MoveVelocity(static_cast<int32_t>(-HOME_VEL_FAST));
        _homingStartTime = now;
        _homingState = HomingState::ApproachSlow;
        break;

    case HomingState::ApproachSlow:
        if (now - _homingStartTime >= 2000) {
            _motor->VelMax(HOME_VEL_SLOW);
            _motor->MoveVelocity(static_cast<int32_t>(-HOME_VEL_SLOW));
            _homingStartTime = now;
            _homingState = HomingState::WaitingForHardStop;
        }
        break;

    case HomingState::WaitingForHardStop:
        // Add debug for HLFB state
        static uint32_t lastHlfbLogTime = 0;
        if (now - lastHlfbLogTime > 1000) {  // Log every second
            ClearCore::ConnectorUsb.Send("[X-Axis] HLFB State: ");
            ClearCore::ConnectorUsb.SendLine(_motor->HlfbState() == MotorDriver::HLFB_ASSERTED ? "ASSERTED" : "NOT ASSERTED");
            lastHlfbLogTime = now;
        }

        if (_motor->HlfbState() == MotorDriver::HLFB_ASSERTED) {
            _motor->MoveStopAbrupt();
            _backoffSteps = static_cast<int32_t>(HOMING_BACKOFF_INCH * _stepsPerInch);
            _homingState = HomingState::Backoff;
        }
        else if (now - _homingStartTime >= HOMING_TIMEOUT_MS) {
            ClearCore::ConnectorUsb.SendLine("[X-Axis] Homing timeout!");
            _homingState = HomingState::Failed;
        }
        break;

    case HomingState::Backoff:
        _motor->Move(_backoffSteps);
        _homingState = HomingState::Complete;
        break;

    case HomingState::Complete:
        if (_motor->StepsComplete()) {
            _motor->PositionRefSet(0);
            _isHomed = true;
            _homingState = HomingState::Idle;
            ClearCore::ConnectorUsb.SendLine("[X-Axis] Homing complete!");
        }
        break;

    case HomingState::Failed:
        ClearCore::ConnectorUsb.SendLine("[X-Axis] Homing failed!");
        _homingState = HomingState::Idle;
        break;

    default:
        break;
    }
}

bool XAxis::MoveTo(float positionInches, float velocityScale) {
    if (!_isSetup || _homingState != HomingState::Idle)
        return false;

    _targetPos = positionInches;
    _isMoving = true;

    int32_t targetSteps = static_cast<int32_t>(_targetPos * _stepsPerInch);
    int32_t currentSteps = _motor->PositionRefCommanded();
    int32_t delta = targetSteps - currentSteps;

    _motor->VelMax(static_cast<uint32_t>(MAX_VELOCITY * velocityScale));
    _motor->Move(delta);
    return true;
}

void XAxis::Stop() {
    if (_isMoving) {
        _motor->MoveStopDecel();
        _isMoving = false;
    }
}

void XAxis::EmergencyStop() {
    _motor->MoveStopAbrupt();
    _isMoving = false;
}

float XAxis::GetPosition() const {
    return static_cast<float>(_motor->PositionRefCommanded()) / _stepsPerInch;
}

float XAxis::GetTorquePercent() const {
    return (_motor->HlfbState() == MotorDriver::HLFB_ASSERTED) ? 100.0f : 10.0f;
}

bool XAxis::IsMoving() const {
    return !_motor->StepsComplete();
}

bool XAxis::IsHomed() const {
    return _isHomed;
}

bool XAxis::IsHoming() const {
    return _homingState != HomingState::Idle;
}