
// ZAxis.cpp
#include "ZAxis.h"
#include "Config.h"
#include <ClearCore.h>

// Velocity/accel constants for Z axis

static constexpr float MAX_ACCEL_Z = 10000.0f;   // steps/s^2

ZAxis::ZAxis()
    : _stepsPerDeg(ROTARY_STEPS_PER_DEGREE)
    , _motor(&MOTOR_ROTARY_Z)
{
}

void ZAxis::Setup() {
    // Configure HLFB and motion limits
    _motor->HlfbMode(MotorDriver::HLFB_MODE_HAS_BIPOLAR_PWM);
    _motor->HlfbCarrier(MotorDriver::HLFB_CARRIER_482_HZ);
    _motor->VelMax(MAX_VELOCITY_Z);
    _motor->AccelMax(MAX_ACCEL_Z);

    // Enable motor
    _motor->EnableRequest(true);

    // Don’t block here—let the homing state machine handle HLFB when you actually home
    _isSetup = true;

}

bool ZAxis::StartHoming() {
    if (!_isSetup || _homingState != HomingState::Idle)
        return false;

    _isHomed = false;
    _homingStartTime = ClearCore::TimingMgr.Milliseconds();
    _homingState = HomingState::ApproachFast;
    return true;
}

void ZAxis::Update() {
    if (!_isSetup)
        return;

    // Update position and movement flag
    _currentPos = static_cast<float>(_motor->PositionRefCommanded()) / _stepsPerDeg;
    _isMoving = !_motor->StepsComplete();
    _torquePct = (_motor->HlfbState() == MotorDriver::HLFB_ASSERTED) ? 100.0f : 10.0f;

    // Process homing if active
    if (_homingState != HomingState::Idle) {
        processHoming();
    }
}

void ZAxis::processHoming() {
    uint32_t now = ClearCore::TimingMgr.Milliseconds();
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
        if (_motor->HlfbState() == MotorDriver::HLFB_ASSERTED) {
            _motor->MoveStopAbrupt();
            _backoffSteps = static_cast<int32_t>(HOMING_BACKOFF_DEG * _stepsPerDeg);
            _homingState = HomingState::Backoff;
        }
        else if (now - _homingStartTime >= HOMING_TIMEOUT_MS) {
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
        }
        break;

    case HomingState::Failed:
        // TODO: handle error (alarms, retries)
        _homingState = HomingState::Idle;
        break;

    default:
        break;
    }
}

bool ZAxis::MoveTo(float positionDeg, float velocityScale) {
    if (!_isSetup || _homingState != HomingState::Idle)
        return false;

    _targetPos = positionDeg;
    _isMoving = true;

    int32_t targetSteps = static_cast<int32_t>(_targetPos * _stepsPerDeg);
    int32_t currentSteps = _motor->PositionRefCommanded();
    int32_t delta = targetSteps - currentSteps;

    _motor->VelMax(static_cast<uint32_t>(MAX_VELOCITY_Z * velocityScale));
    _motor->Move(delta);
    return true;
}

void ZAxis::Stop() {
    if (_isMoving) {
        _motor->MoveStopDecel();
        _isMoving = false;
    }
}

void ZAxis::EmergencyStop() {
    _motor->MoveStopAbrupt();
    _isMoving = false;
    if (_homingState != HomingState::Idle) _homingState = HomingState::Idle;
}

float ZAxis::GetPosition() const {
    return static_cast<float>(_motor->PositionRefCommanded()) / _stepsPerDeg;
}

float ZAxis::GetTorquePercent() const {
    return (_motor->HlfbState() == MotorDriver::HLFB_ASSERTED) ? 100.0f : 10.0f;
}

bool ZAxis::IsMoving() const {
    return !_motor->StepsComplete();
}

bool ZAxis::IsHomed() const {
    return _isHomed;
}

bool ZAxis::IsHoming() const {
    return _homingState != HomingState::Idle;
}
