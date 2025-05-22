// XAxis.cpp
#include "XAxis.h"
#include "Config.h"
#include "ClearCore.h"

static constexpr float MAX_VELOCITY = 10000.0f;   // steps/s
static constexpr float MAX_ACCELERATION = 100000.0f;  // steps/s^2

XAxis::XAxis()
    : _stepsPerInch(FENCE_STEPS_PER_INCH)
    , _motor(&MOTOR_FENCE_X)
    , _isSetup(false)
    , _isHomed(false)
    , _isMoving(false)
    , _currentPos(0.0f)
    , _torquePct(0.0f)
{
    // Set up homing parameters
    HomingParams params;
    params.motor = _motor;
    params.stepsPerUnit = _stepsPerInch;
    params.fastVel = HOME_VEL_FAST;
    params.slowVel = HOME_VEL_SLOW;
    params.dwellMs = 2000;
    params.backoffUnits = HOMING_BACKOFF_INCH;
    params.timeoutMs = HOMING_TIMEOUT_MS;

    // Create the HomingHelper with our parameters
    _homingHelper = new HomingHelper(params);
}

XAxis::~XAxis() {
    if (_homingHelper) {
        delete _homingHelper;
        _homingHelper = nullptr;
    }
}

void XAxis::Setup() {
    // Configure HLFB and motion limits
    _motor->HlfbMode(MotorDriver::HLFB_MODE_HAS_BIPOLAR_PWM);
    _motor->HlfbCarrier(MotorDriver::HLFB_CARRIER_482_HZ);
    _motor->VelMax(MAX_VELOCITY);
    _motor->AccelMax(MAX_ACCELERATION);

    // Clear any existing alerts
    ClearAlerts();

    // Enable driver (only once)
    _motor->EnableRequest(true);

    // Wait for HLFB with timeout
    uint32_t startTime = ClearCore::TimingMgr.Milliseconds();
    ClearCore::ConnectorUsb.Send("[X-Axis] Waiting for HLFB...");
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
    ClearCore::ConnectorUsb.SendLine("[X-Axis] Setup complete");
}

void XAxis::ClearAlerts() {
    ClearCore::ConnectorUsb.SendLine("[X-Axis] Checking for alerts");

    if (_motor->StatusReg().bit.AlertsPresent) {
        // Print which alerts are active
        ClearCore::ConnectorUsb.SendLine("[X-Axis] Alerts present: ");

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
            // If motor is faulted, cycle enable
            _motor->EnableRequest(false);
            Delay_ms(100);
            _motor->EnableRequest(true);
            Delay_ms(100);
        }

        // Clear any remaining alerts
        _motor->ClearAlerts();
        ClearCore::ConnectorUsb.SendLine("[X-Axis] Alerts cleared");
    }
    else {
        ClearCore::ConnectorUsb.SendLine("[X-Axis] No alerts present");
    }
}

bool XAxis::StartHoming() {
    if (!_isSetup) {
        ClearCore::ConnectorUsb.SendLine("[X-Axis] Cannot start homing: not setup");
        return false;
    }

    // Check for alerts and clear them before homing
    if (_motor->StatusReg().bit.AlertsPresent) {
        ClearCore::ConnectorUsb.SendLine("[X-Axis] Alerts present before homing, clearing...");
        ClearAlerts();
    }

    // Start homing with HomingHelper
    bool success = _homingHelper->start();
    if (success) {
        _isHomed = false; // Will be set to true when homing completes
        ClearCore::ConnectorUsb.SendLine("[X-Axis] Homing sequence started");
    }
    else {
        ClearCore::ConnectorUsb.SendLine("[X-Axis] Cannot start homing: already busy");
    }
    return success;
}

void XAxis::Update() {
    if (!_isSetup)
        return;

    // Update position feedback
    _currentPos = static_cast<float>(_motor->PositionRefCommanded()) / _stepsPerInch;
    _isMoving = !_motor->StepsComplete();
    _torquePct = GetTorquePercent();

    // Process homing if active
    if (_homingHelper->isBusy()) {
        _homingHelper->process();
    }
    else if (!_isHomed && !_homingHelper->hasFailed() && !_homingHelper->isBusy()) {
        // Homing completed successfully
        _isHomed = true;
        ClearCore::ConnectorUsb.SendLine("[X-Axis] Homing complete!");
    }
}

bool XAxis::MoveTo(float positionInches, float velocityScale) {
    if (!_isSetup) {
        ClearCore::ConnectorUsb.SendLine("[X-Axis] MoveTo failed: not setup");
        return false;
    }

    // Don't allow moves while homing
    if (_homingHelper->isBusy()) {
        ClearCore::ConnectorUsb.SendLine("[X-Axis] MoveTo failed: homing in progress");
        return false;
    }

    _targetPos = positionInches;
    _isMoving = true;

    int32_t targetSteps = static_cast<int32_t>(_targetPos * _stepsPerInch);
    int32_t currentSteps = _motor->PositionRefCommanded();
    int32_t delta = targetSteps - currentSteps;

    _motor->VelMax(static_cast<uint32_t>(MAX_VELOCITY * velocityScale));
    ClearCore::ConnectorUsb.Send("[X-Axis] MoveTo: ");
    ClearCore::ConnectorUsb.Send(positionInches);
    ClearCore::ConnectorUsb.Send(" inches, ");
    ClearCore::ConnectorUsb.Send(delta);
    ClearCore::ConnectorUsb.SendLine(" steps");

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
    return (_motor->HlfbState() == MotorDriver::HLFB_ASSERTED) ? 100.0f : 0.0f;
}

bool XAxis::IsMoving() const {
    return !_motor->StepsComplete();
}

bool XAxis::IsHomed() const {
    return _isHomed;
}

bool XAxis::IsHoming() const {
    return _homingHelper->isBusy();
}
