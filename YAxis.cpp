// YAxis.cpp
#include "YAxis.h"
#include "Config.h"
#include "ClearCore.h"
#include "EncoderPositionTracker.h"


static constexpr float MAX_VELOCITY = 10000.0f;   // steps/s
static constexpr float MAX_ACCELERATION = 100000.0f;  // steps/s^2

YAxis::YAxis()
    : _stepsPerInch(TABLE_STEPS_PER_INCH)
    , _motor(&MOTOR_TABLE_Y)
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
    params.dwellMs = 100;                  // shortened dwell
    params.backoffUnits = HOMING_BACKOFF_INCH;
    params.timeoutMs = HOMING_TIMEOUT_MS;

    _homingHelper = new HomingHelper(params);
}

YAxis::~YAxis() {
    delete _homingHelper;
}

void YAxis::Setup() {
    _motor->HlfbMode(MotorDriver::HLFB_MODE_HAS_BIPOLAR_PWM);
    _motor->HlfbCarrier(MotorDriver::HLFB_CARRIER_482_HZ);
    _motor->VelMax(MAX_VELOCITY);
    _motor->AccelMax(MAX_ACCELERATION);
    ClearAlerts();
    // don't enable yet—only on-demand in StartHoming()
    _isSetup = true;
    ClearCore::ConnectorUsb.SendLine("[Y-Axis] Setup complete");
}

void YAxis::ClearAlerts() {
    ClearCore::ConnectorUsb.SendLine("[Y-Axis] Checking for alerts");
    if (_motor->StatusReg().bit.AlertsPresent) {
        ClearCore::ConnectorUsb.SendLine("[Y-Axis] Alerts present:");
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
        ClearCore::ConnectorUsb.SendLine("[Y-Axis] Alerts cleared");
    }
    else {
        ClearCore::ConnectorUsb.SendLine("[Y-Axis] No alerts present");
    }
}

bool YAxis::StartHoming() {
    if (!_isSetup) {
        ClearCore::ConnectorUsb.SendLine("[Y-Axis] Cannot start homing: not setup");
        return false;
    }
    // enable on-demand to prevent MSP auto-homing
    _motor->EnableRequest(true);
    if (_motor->StatusReg().bit.AlertsPresent)
        ClearAlerts();

    bool ok = _homingHelper->start();
    if (ok) {
        _isHomed = false;
        ClearCore::ConnectorUsb.SendLine("[Y-Axis] Homing started");
    }
    return ok;
}


// Replace the existing Update method with this one that includes torque control
void YAxis::Update() {
    if (!_isSetup)
        return;

    // Update position feedback
    _currentPos = static_cast<float>(_motor->PositionRefCommanded()) / _stepsPerInch;
    _isMoving = !_motor->StepsComplete();

    // Update torque measurement
    UpdateTorqueMeasurement();

    // If in torque-controlled feed, adjust feed rate based on torque
    if (_inTorqueControlFeed && _isMoving) {
        AdjustFeedRateBasedOnTorque();
    }
    // Auto-exit torque control mode when move completes
    else if (_inTorqueControlFeed && !_isMoving) {
        _inTorqueControlFeed = false;
        ClearCore::ConnectorUsb.SendLine("[Y-Axis] Torque-controlled feed completed");
    }

    // homing state
    if (_homingHelper->isBusy()) {
        _homingHelper->process();
    }
    else if (!_isHomed && !_homingHelper->hasFailed()) {
        _isHomed = true;
        _hasBeenHomed = true;

        // Reset encoder position tracking when Y-axis homing completes
        EncoderPositionTracker::Instance().resetPositionAfterHoming();

        ClearCore::ConnectorUsb.SendLine("[Y-Axis] Homing complete, encoder position reset");
    }
}


bool YAxis::MoveTo(float positionInches, float velocityScale) {
    if (!_isSetup) {
        ClearCore::ConnectorUsb.SendLine("[Y-Axis] MoveTo failed: not setup");
        return false;
    }
    if (_homingHelper->isBusy()) {
        ClearCore::ConnectorUsb.SendLine("[Y-Axis] MoveTo failed: homing in progress");
        return false;
    }
    if (!_hasBeenHomed) {
        ClearCore::ConnectorUsb.SendLine("[Y-Axis] MoveTo failed: axis not homed yet");
        return false;
    }

    float desired = positionInches;
    // soft-limit clamp and stop
    if (desired < 0.0f || desired > MAX_Y_INCHES) {
        ClearCore::ConnectorUsb.SendLine("[Y-Axis] Soft limit reached, stopping");
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
    ClearCore::ConnectorUsb.Send("[Y-Axis] MoveTo: ");
    ClearCore::ConnectorUsb.Send(_targetPos);
    ClearCore::ConnectorUsb.SendLine(" inches");
    return _motor->Move(delta);
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
}

float YAxis::GetPosition() const {
    return static_cast<float>(_motor->PositionRefCommanded()) / _stepsPerInch;
}

// Add to YAxis.cpp - Implementation of torque control

// Replace the existing GetTorquePercent method with this improved version
float YAxis::GetTorquePercent() const {
    // Read torque from HLFB if available (more accurate than just checking ASSERTED state)
    if (_motor->HlfbState() == MotorDriver::HLFB_HAS_MEASUREMENT) {
        return _motor->HlfbPercent();
    }
    // Fallback to binary torque reading
    return (_motor->HlfbState() == MotorDriver::HLFB_ASSERTED) ? 100.0f : 0.0f;
}

void YAxis::SetTorqueTarget(float targetPercent) {
    // Clamp target to valid range
    _torqueTarget = (targetPercent < 20.0f) ? 20.0f :
        (targetPercent > 95.0f) ? 95.0f : targetPercent;

    ClearCore::ConnectorUsb.Send("[Y-Axis] Torque target set to ");
    ClearCore::ConnectorUsb.Send(_torqueTarget, 1);
    ClearCore::ConnectorUsb.SendLine("%");
}

float YAxis::GetTorqueTarget() const {
    return _torqueTarget;
}

bool YAxis::StartTorqueControlledFeed(float targetPosition, float initialVelocityScale) {
    if (!_isSetup) {
        ClearCore::ConnectorUsb.SendLine("[Y-Axis] Cannot start torque feed: not setup");
        return false;
    }
    if (_homingHelper->isBusy()) {
        ClearCore::ConnectorUsb.SendLine("[Y-Axis] Cannot start torque feed: homing in progress");
        return false;
    }
    if (!_hasBeenHomed) {
        ClearCore::ConnectorUsb.SendLine("[Y-Axis] Cannot start torque feed: axis not homed");
        return false;
    }

    // Validate position and velocity
    float desired = targetPosition;
    if (desired < 0.0f || desired > MAX_Y_INCHES) {
        ClearCore::ConnectorUsb.SendLine("[Y-Axis] Torque feed: target out of bounds");
        return false;
    }

    // Set up torque control parameters
    _inTorqueControlFeed = true;
    _targetPos = desired;
    _maxFeedRate = (initialVelocityScale > 0.01f && initialVelocityScale <= 1.0f) ?
        initialVelocityScale : 0.5f;
    _currentFeedRate = _maxFeedRate;  // Start at max feed rate
    _torqueErrorAccumulator = 0.0f;   // Reset integral term
    _lastTorqueUpdateTime = ClearCore::TimingMgr.Milliseconds();

    // Start the move
    _isMoving = true;

    int32_t tgtSteps = static_cast<int32_t>(_targetPos * _stepsPerInch);
    int32_t curSteps = _motor->PositionRefCommanded();
    int32_t delta = tgtSteps - curSteps;
    if (delta == 0) {
        _inTorqueControlFeed = false;
        return true;  // Already at position
    }

    // Make sure the motor is enabled
    _motor->EnableRequest(true);

    // Set initial velocity
    _motor->VelMax(static_cast<uint32_t>(MAX_VELOCITY * _currentFeedRate));

    ClearCore::ConnectorUsb.Send("[Y-Axis] Starting torque-controlled feed to ");
    ClearCore::ConnectorUsb.Send(_targetPos);
    ClearCore::ConnectorUsb.Send(" inches at ");
    ClearCore::ConnectorUsb.Send(_currentFeedRate * 100.0f, 0);
    ClearCore::ConnectorUsb.Send("% speed, target torque: ");
    ClearCore::ConnectorUsb.Send(_torqueTarget, 1);
    ClearCore::ConnectorUsb.SendLine("%");

    return _motor->Move(delta);
}

void YAxis::UpdateFeedRate(float newVelocityScale) {
    if (!_isSetup || !_isMoving) return;

    // Clamp to valid range
    float velocityScale = (newVelocityScale < _minFeedRate) ? _minFeedRate :
        (newVelocityScale > 1.0f) ? 1.0f : newVelocityScale;

    _currentFeedRate = velocityScale;
    _motor->VelMax(static_cast<uint32_t>(MAX_VELOCITY * velocityScale));
}

bool YAxis::IsInTorqueControlledFeed() const {
    return _inTorqueControlFeed;
}

void YAxis::AbortTorqueControlledFeed() {
    if (_inTorqueControlFeed) {
        Stop();
        _inTorqueControlFeed = false;
        ClearCore::ConnectorUsb.SendLine("[Y-Axis] Torque-controlled feed aborted");
    }
}

void YAxis::UpdateTorqueMeasurement() {
    // Read torque from HLFB
    if (_motor->HlfbState() == MotorDriver::HLFB_HAS_MEASUREMENT) {
        _torquePct = _motor->HlfbPercent();
    }
    else if (_motor->HlfbState() == MotorDriver::HLFB_ASSERTED) {
        _torquePct = 100.0f;  // Binary fallback
    }
    else {
        _torquePct = 0.0f;    // Not asserted
    }
}

void YAxis::AdjustFeedRateBasedOnTorque() {
    if (!_inTorqueControlFeed || !_isMoving) return;

    // Calculate time delta for PID calculations
    uint32_t now = ClearCore::TimingMgr.Milliseconds();
    float dt = static_cast<float>(now - _lastTorqueUpdateTime) / 1000.0f; // seconds
    _lastTorqueUpdateTime = now;

    // Only process if we have a valid time delta
    if (dt < 0.01f || dt > 0.5f) {
        dt = 0.01f;  // Default to 10ms if time measurement is off
    }

    // Calculate torque error (negative when torque exceeds target)
    float torqueError = _torqueTarget - _torquePct;

    // Update integral term with anti-windup
    _torqueErrorAccumulator += torqueError * dt;
    _torqueErrorAccumulator = (_torqueErrorAccumulator > 10.0f) ? 10.0f :
        (_torqueErrorAccumulator < -10.0f) ? -10.0f : _torqueErrorAccumulator;

    // Calculate PID-like feed adjustment
    float feedAdjustment = (TORQUE_Kp * torqueError) +
        (TORQUE_Ki * _torqueErrorAccumulator);

    // Apply the feed adjustment
    _currentFeedRate += feedAdjustment * dt;

    // Clamp to configured min/max rates
    _currentFeedRate = (_currentFeedRate < _minFeedRate) ? _minFeedRate :
        (_currentFeedRate > _maxFeedRate) ? _maxFeedRate : _currentFeedRate;

    // Apply the new feed rate to the motor
    _motor->VelMax(static_cast<uint32_t>(MAX_VELOCITY * _currentFeedRate));

    // Periodically log feed and torque data
    static uint32_t lastLogTime = 0;
    if (now - lastLogTime > 500) { // Log every 500ms
        lastLogTime = now;
        ClearCore::ConnectorUsb.Send("[Y-Axis] Torque: ");
        ClearCore::ConnectorUsb.Send(_torquePct, 1);
        ClearCore::ConnectorUsb.Send("%, Feed rate: ");
        ClearCore::ConnectorUsb.Send(_currentFeedRate * 100.0f, 1);
        ClearCore::ConnectorUsb.Send("%, Error: ");
        ClearCore::ConnectorUsb.Send(torqueError, 1);
        ClearCore::ConnectorUsb.SendLine("%");
    }
}

bool YAxis::IsMoving() const { return !_motor->StepsComplete(); }
bool YAxis::IsHomed()  const { return _isHomed; }
bool YAxis::IsHoming() const { return _homingHelper->isBusy(); }
