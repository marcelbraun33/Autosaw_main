// YAxis.cpp
#include "YAxis.h"
#include "Config.h"
#include "ClearCore.h"

static constexpr float MAX_VELOCITY = 10000.0f;   // steps/s
static constexpr float MAX_ACCELERATION = 100000.0f;  // steps/s^2

YAxis::YAxis()
    : _stepsPerInch(TABLE_STEPS_PER_INCH)
    , _motor(&MOTOR_TABLE_Y)
    , _isSetup(false)
    , _isHomed(false)
    , _homingState(HomingState::Idle)
    , _currentPos(0.0f)
    , _isMoving(false)
    , _torquePct(0.0f)
    , _backoffSteps(0)
{
}

void YAxis::Setup() {
    // Configure HLFB and motion limits
    _motor->HlfbMode(MotorDriver::HLFB_MODE_HAS_BIPOLAR_PWM);
    _motor->HlfbCarrier(MotorDriver::HLFB_CARRIER_482_HZ);

    // Ensure MAX_VELOCITY and MAX_ACCELERATION are the same as X-axis
    _motor->VelMax(MAX_VELOCITY);
    _motor->AccelMax(MAX_ACCELERATION);

    // Clear any existing alerts - use the same method as X-axis
    ClearAlerts();

    // Enable driver (only once) - match X-axis approach
    _motor->EnableRequest(true);

    // Wait for HLFB with timeout - add this to match X-axis
    uint32_t startTime = ClearCore::TimingMgr.Milliseconds();
    ClearCore::ConnectorUsb.Send("[Y-Axis] Waiting for HLFB...");
    while (_motor->HlfbState() != MotorDriver::HLFB_ASSERTED) {
        if (ClearCore::TimingMgr.Milliseconds() - startTime > 5000) {  // 5 second timeout
            ClearCore::ConnectorUsb.SendLine(" TIMEOUT");
            break;
        }
    }
    if (_motor->HlfbState() == MotorDriver::HLFB_ASSERTED) {
        ClearCore::ConnectorUsb.SendLine(" ASSERTED");
    }

    _isSetup = true;
    ClearCore::ConnectorUsb.SendLine("[Y-Axis] Setup complete");
}

void YAxis::ClearAlerts() {
    ClearCore::ConnectorUsb.SendLine("[Y-Axis] Checking for alerts");

    if (_motor->StatusReg().bit.AlertsPresent) {
        // Print which alerts are active
        ClearCore::ConnectorUsb.SendLine("[Y-Axis] Alerts present: ");

        if (_motor->AlertReg().bit.MotionCanceledInAlert) {
            ClearCore::ConnectorUsb.SendLine(" - MotionCanceledInAlert");
        }
        if (_motor->AlertReg().bit.MotionCanceledPositiveLimit) {
            ClearCore::ConnectorUsb.SendLine(" - MotionCanceledPositiveLimit");
        }
        if (_motor->AlertReg().bit.MotionCanceledNegativeLimit) {
            ClearCore::ConnectorUsb.SendLine(" - MotionCanceledNegativeLimit");
        }
        if (_motor->AlertReg().bit.MotionCanceledSensorEStop) {
            ClearCore::ConnectorUsb.SendLine(" - MotionCanceledSensorEStop");
        }
        if (_motor->AlertReg().bit.MotionCanceledMotorDisabled) {
            ClearCore::ConnectorUsb.SendLine(" - MotionCanceledMotorDisabled");
        }
        if (_motor->AlertReg().bit.MotorFaulted) {
            ClearCore::ConnectorUsb.SendLine(" - MotorFaulted");
            // If motor is faulted, cycle enable - ADD THIS SECTION
            _motor->EnableRequest(false);
            Delay_ms(100);
            _motor->EnableRequest(true);
            Delay_ms(100);
        }

        // Clear any remaining alerts
        _motor->ClearAlerts();
        ClearCore::ConnectorUsb.SendLine("[Y-Axis] Alerts cleared");
    }
    else {
        ClearCore::ConnectorUsb.SendLine("[Y-Axis] No alerts present");
    }
}

bool YAxis::MoveTo(float positionInches, float velocityScale) {
    if (!_isSetup) {
        ClearCore::ConnectorUsb.SendLine("[Y-Axis] MoveTo failed: not setup");
        return false;
    }

    // Add alert check and clear before moving - like X-axis
    if (_motor->StatusReg().bit.AlertsPresent) {
        ClearCore::ConnectorUsb.SendLine("[Y-Axis] Alerts present before move, clearing...");
        ClearAlerts();
    }

    // Allow movement during certain homing states or when not homing
    if (_homingState != HomingState::Idle &&
        _homingState != HomingState::Complete &&
        _homingState != HomingState::Failed) {
        ClearCore::ConnectorUsb.Send("[Y-Axis] MoveTo failed: in homing state ");
        ClearCore::ConnectorUsb.SendLine(static_cast<int>(_homingState));
        return false;
    }

    _targetPos = positionInches;
    _isMoving = true;

    int32_t targetSteps = static_cast<int32_t>(_targetPos * _stepsPerInch);
    int32_t currentSteps = _motor->PositionRefCommanded();
    int32_t delta = targetSteps - currentSteps;

    // Try boosting velocity for Y-axis if needed
    float boostFactor = 1.5f; // Temporary boost to help diagnose speed issues
    _motor->VelMax(static_cast<uint32_t>(MAX_VELOCITY * velocityScale * boostFactor));

    ClearCore::ConnectorUsb.Send("[Y-Axis] MoveTo: ");
    ClearCore::ConnectorUsb.Send(positionInches);
    ClearCore::ConnectorUsb.Send(" inches, ");
    ClearCore::ConnectorUsb.Send(delta);
    ClearCore::ConnectorUsb.Send(" steps, VelMax=");
    ClearCore::ConnectorUsb.SendLine(static_cast<uint32_t>(MAX_VELOCITY * velocityScale * boostFactor));

    _motor->Move(delta);
    return true;
}
bool YAxis::StartHoming() {
    if (!_isSetup) {
        ClearCore::ConnectorUsb.SendLine("[Y-Axis] Cannot start homing: not setup");
        return false;
    }

    // Clear any alerts before starting
    if (_motor->StatusReg().bit.AlertsPresent) {
        ClearCore::ConnectorUsb.SendLine("[Y-Axis] Clearing alerts before homing");
        _motor->ClearAlerts();
    }

    // Only proceed if homing state is idle
    if (_homingState != HomingState::Idle) {
        ClearCore::ConnectorUsb.SendLine("[Y-Axis] Cannot start homing: already homing");
        return false;
    }

    _isHomed = false;
    _homingStartTime = ClearCore::TimingMgr.Milliseconds();
    _homingState = HomingState::ApproachFast;
    ClearCore::ConnectorUsb.SendLine("[Y-Axis] Homing sequence started");
    return true;
}

void YAxis::Update() {
    if (!_isSetup)
        return;

    // Update position feedback
    _currentPos = static_cast<float>(_motor->PositionRefCommanded()) / _stepsPerInch;
    _isMoving = !_motor->StepsComplete();
    _torquePct = GetTorquePercent();

    // Only run homing state machine when requested
    if (_homingState != HomingState::Idle) {
        processHoming();
    }
}

void YAxis::processHoming() {
    uint32_t now = ClearCore::TimingMgr.Milliseconds();

    // Log actual HLFB state for debugging
    ClearCore::ConnectorUsb.Send("[Y-Axis] HLFB State: ");
    ClearCore::ConnectorUsb.SendLine(_motor->HlfbState() == MotorDriver::HLFB_ASSERTED ? "ASSERTED" : "NOT ASSERTED");

    // YAxis.cpp - processHoming()
  // Handle alerts the same way X-axis does
    if (_motor->StatusReg().bit.AlertsPresent) {
        ClearCore::ConnectorUsb.SendLine("[Y-Axis] ALERT — homing canceled");
        _homingState = HomingState::Failed;
        return;
    }


    switch (_homingState) {
    case HomingState::ApproachFast:
        ClearCore::ConnectorUsb.SendLine("[Y-Axis] Starting fast approach");
        _motor->VelMax(HOME_VEL_FAST);
        _motor->MoveVelocity(static_cast<int32_t>(-HOME_VEL_FAST));
        _homingStartTime = now;
        _homingState = HomingState::ApproachSlow;
        break;

    case HomingState::ApproachSlow:
        if (now - _homingStartTime >= 2000) {
            ClearCore::ConnectorUsb.SendLine("[Y-Axis] Starting slow approach");
            _motor->VelMax(HOME_VEL_SLOW);
            _motor->MoveVelocity(static_cast<int32_t>(-HOME_VEL_SLOW));
            _homingStartTime = now;
            _homingState = HomingState::WaitingForHardStop;
        }
        break;

    case HomingState::WaitingForHardStop:
        // Wait for HLFB to assert (indicating contact with hardstop)
        if (_motor->HlfbState() == MotorDriver::HLFB_ASSERTED) {
            ClearCore::ConnectorUsb.SendLine("[Y-Axis] HLFB asserted, stopping move");
            _motor->MoveStopAbrupt();
            Delay_ms(20);

            // Initiate backoff move
            _backoffSteps = static_cast<int32_t>(HOMING_BACKOFF_INCH * _stepsPerInch);
            ClearCore::ConnectorUsb.Send("[Y-Axis] Starting backoff move of ");
            ClearCore::ConnectorUsb.Send(_backoffSteps);
            ClearCore::ConnectorUsb.SendLine(" steps");

            _motor->Move(_backoffSteps);
            _homingState = HomingState::Complete;
        }
        else if (now - _homingStartTime > HOMING_TIMEOUT_MS) {
            ClearCore::ConnectorUsb.SendLine("[Y-Axis] Homing timeout!");
            _homingState = HomingState::Failed;
        }
        break;

    case HomingState::Complete:
        if (_motor->StepsComplete()) {
            ClearCore::ConnectorUsb.SendLine("[Y-Axis] Backoff complete, setting zero position");
            _motor->PositionRefSet(0);
            _isHomed = true;
            _homingState = HomingState::Idle;
            ClearCore::ConnectorUsb.SendLine("[Y-Axis] Homing complete!");
        }
        break;

    case HomingState::Failed:
        ClearCore::ConnectorUsb.SendLine("[Y-Axis] Homing failed!");
        _homingState = HomingState::Idle;
        break;

    default:
        break;
    }
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
    return (_motor->HlfbState() == MotorDriver::HLFB_ASSERTED) ? 100.0f : 0.0f;
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
