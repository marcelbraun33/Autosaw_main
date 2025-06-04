// XAxis.cpp
#include "XAxis.h"
#include "Config.h"
#include "ClearCore.h"
#include "EncoderPositionTracker.h"

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
    , _hasBeenHomed(false)
{
    HomingParams params;
    params.motor = _motor;
    params.stepsPerUnit = _stepsPerInch;
    params.fastVel = HOME_VEL_FAST;
    params.slowVel = HOME_VEL_SLOW;
    params.dwellMs = 100;
    params.backoffUnits = HOMING_BACKOFF_INCH;
    params.timeoutMs = HOMING_TIMEOUT_MS;

    _homingHelper = new HomingHelper(params);
}

XAxis::~XAxis() {
    delete _homingHelper;
}

void XAxis::Setup() {
    _motor->HlfbMode(MotorDriver::HLFB_MODE_HAS_BIPOLAR_PWM);
    _motor->HlfbCarrier(MotorDriver::HLFB_CARRIER_482_HZ);
    _motor->VelMax(MAX_VELOCITY);
    _motor->AccelMax(MAX_ACCELERATION);
    ClearAlerts();
    _isSetup = true;
    ClearCore::ConnectorUsb.SendLine("[X-Axis] Setup complete");
}

void XAxis::ClearAlerts() {
    ClearCore::ConnectorUsb.SendLine("[X-Axis] Checking for alerts");
    if (_motor->StatusReg().bit.AlertsPresent) {
        ClearCore::ConnectorUsb.SendLine("[X-Axis] Alerts present:");
        if (_motor->AlertReg().bit.MotionCanceledInAlert)
            ClearCore::ConnectorUsb.SendLine(" - MotionCanceledInAlert");
        if (_motor->AlertReg().bit.MotionCanceledPositiveLimit)
            ClearCore::ConnectorUsb.SendLine(" - MotionCanceledPositiveLimit");
        if (_motor->AlertReg().bit.MotionCanceledNegativeLimit)
            ClearCore::ConnectorUsb.SendLine(" - MotionCanceledNegativeLimit");
        if (_motor->AlertReg().bit.MotionCanceledSensorEStop)
            ClearCore::ConnectorUsb.SendLine(" - MotionCanceledSensorEStop");
        if (_motor->AlertReg().bit.MotionCanceledMotorDisabled)
            ClearCore::ConnectorUsb.SendLine(" - MotionCanceledMotorDisabled");
        if (_motor->AlertReg().bit.MotorFaulted) {
            ClearCore::ConnectorUsb.SendLine(" - MotorFaulted");
            _motor->EnableRequest(false);
            Delay_ms(100);
            _motor->EnableRequest(true);
            Delay_ms(100);
        }
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
    // enable on-demand
    _motor->EnableRequest(true);
    if (_motor->StatusReg().bit.AlertsPresent) ClearAlerts();
    bool ok = _homingHelper->start();
    if (ok) {
        _isHomed = false;
        ClearCore::ConnectorUsb.SendLine("[X-Axis] Homing started");
    }
    return ok;
}

void XAxis::Update() {
    if (!_isSetup) return;
    _currentPos = static_cast<float>(_motor->PositionRefCommanded()) / _stepsPerInch;
    _isMoving = !_motor->StepsComplete();
    _torquePct = GetTorquePercent();

    if (_homingHelper->isBusy()) {
        _homingHelper->process();
    }
    else if (!_isHomed && !_homingHelper->hasFailed()) {
        _isHomed = true;
        _hasBeenHomed = true;

        // Reset encoder position tracking when X-axis homing completes
        EncoderPositionTracker::Instance().resetPositionAfterHoming();

        ClearCore::ConnectorUsb.SendLine("[X-Axis] Homing complete, encoder position reset");
    }
}

bool XAxis::MoveTo(float positionInches, float velocityScale) {
    if (!_isSetup) {
        ClearCore::ConnectorUsb.SendLine("[X-Axis] MoveTo failed: not setup");
        return false;
    }
    if (_homingHelper->isBusy()) {
        ClearCore::ConnectorUsb.SendLine("[X-Axis] MoveTo failed: homing in progress");
        return false;
    }
    if (!_hasBeenHomed) {
        ClearCore::ConnectorUsb.SendLine("[X-Axis] MoveTo failed: axis not homed yet");
        return false;
    }
    float desired = positionInches;
    if (desired < 0.0f || desired > MAX_X_INCHES) {
        ClearCore::ConnectorUsb.SendLine("[X-Axis] Soft limit reached, stopping");
        _motor->MoveStopAbrupt();
        _isMoving = false;
        return false;
    }
    _targetPos = desired;
    _isMoving = true;

    int32_t tgtSteps = static_cast<int32_t>(_targetPos * _stepsPerInch);
    int32_t curSteps = _motor->PositionRefCommanded();
    int32_t delta = tgtSteps - curSteps;
    if (delta == 0) return true;

    _motor->VelMax(static_cast<uint32_t>(MAX_VELOCITY * velocityScale));
    ClearCore::ConnectorUsb.Send("[X-Axis] MoveTo: ");
    ClearCore::ConnectorUsb.Send(_targetPos);
    ClearCore::ConnectorUsb.SendLine(" inches");
    return _motor->Move(delta);
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
    return _motor->HlfbState() == MotorDriver::HLFB_ASSERTED ? 100.0f : 0.0f;
}

bool XAxis::IsMoving() const { return !_motor->StepsComplete(); }
bool XAxis::IsHomed()  const { return _isHomed; }
bool XAxis::IsHoming() const { return _homingHelper->isBusy(); }
