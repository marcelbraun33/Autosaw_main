
// YAxis.cpp
#include "YAxis.h"
#include "Config.h"
#include <ClearCore.h>

static constexpr float MAX_VELOCITY = 10000.0f;   // steps/s
static constexpr float MAX_ACCELERATION = 100000.0f;  // steps/s^2

YAxis::YAxis()
    : _stepsPerInch(TABLE_STEPS_PER_INCH)
    , _motor(&MOTOR_TABLE_Y)
{
}

void YAxis::Setup() {
    // Configure HLFB and motion limits
    _motor->HlfbMode(MotorDriver::HLFB_MODE_HAS_BIPOLAR_PWM);
    _motor->HlfbCarrier(MotorDriver::HLFB_CARRIER_482_HZ);
    _motor->VelMax(MAX_VELOCITY);
    _motor->AccelMax(MAX_ACCELERATION);

    // Enable motor
    _motor->EnableRequest(true);

    // Don’t block here—let the homing state machine handle HLFB when you actually home
    _isSetup = true;

}

bool YAxis::StartHoming() {
    if (!_isSetup || _homingState != HomingState::Idle)
        return false;

    _isHomed = false;
    _homingStartTime = ClearCore::TimingMgr.Milliseconds();
    _homingState = HomingState::ApproachFast;
    return true;
}

void YAxis::Update() {
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

void YAxis::processHoming() {
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
            _backoffSteps = static_cast<int32_t>(HOMING_BACKOFF_INCH * _stepsPerInch);
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

bool YAxis::MoveTo(float positionInches, float velocityScale) {
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

void YAxis::Stop() {
    if (_isMoving) {
        _motor->MoveStopDecel();
        _isMoving = false;
    }
}

void YAxis::EmergencyStop() {
    _motor->MoveStopAbrupt();
    _isMoving = false;
    if (_homingState != HomingState::Idle) {
        _homingState = HomingState::Idle;
    }
}

float YAxis::GetPosition() const {
    return static_cast<float>(_motor->PositionRefCommanded()) / _stepsPerInch;
}

float YAxis::GetTorquePercent() const {
    return (_motor->HlfbState() == MotorDriver::HLFB_ASSERTED) ? 100.0f : 10.0f;
}

bool YAxis::IsMoving() const {
    return !_motor->StepsComplete();
}

bool YAxis::IsHomed() const {
    return _isHomed;
}

bool YAxis::IsHoming() const {
    return _homingState != HomingState::Idle;
}

