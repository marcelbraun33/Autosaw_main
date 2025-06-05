#include "YAxis.h"
#include "Config.h"
#include "ClearCore.h"
#include "EncoderPositionTracker.h"

static constexpr float MAX_VELOCITY = 10000.0f;      // steps/s
static constexpr float MAX_ACCELERATION = 100000.0f; // steps/s^2

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
    params.dwellMs = 100;
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

void YAxis::Update() {
    if (!_isSetup)
        return;

    // Update position and torque
    _currentPos = static_cast<float>(_motor->PositionRefCommanded()) / _stepsPerInch;
    _isMoving = !_motor->StepsComplete();
    UpdateTorqueMeasurement();

    // --- DEBUG: Log torque and feed state for troubleshooting ---
    static uint32_t lastDebugLog = 0;
    uint32_t now = ClearCore::TimingMgr.Milliseconds();
    if (now - lastDebugLog > 500) {
        lastDebugLog = now;
        ClearCore::ConnectorUsb.Send("[Y-Axis][DEBUG] inTorqueControlFeed: ");
        ClearCore::ConnectorUsb.Send(_inTorqueControlFeed ? "true" : "false");
        ClearCore::ConnectorUsb.Send(", isMoving: ");
        ClearCore::ConnectorUsb.Send(_isMoving ? "true" : "false");
        ClearCore::ConnectorUsb.Send(", torquePct: ");
        ClearCore::ConnectorUsb.Send(_torquePct, 1);
        ClearCore::ConnectorUsb.Send(", torqueTarget: ");
        ClearCore::ConnectorUsb.Send(_torqueTarget, 1);
        ClearCore::ConnectorUsb.Send(", feedRate: ");
        ClearCore::ConnectorUsb.Send(_currentFeedRate * 100.0f, 1);
        ClearCore::ConnectorUsb.SendLine("%");
    }
    // ------------------------------------------------------------

    // --- Torque-controlled feed logic ---
    if (_inTorqueControlFeed) {
        AdjustFeedRateBasedOnTorque();
        float direction = (_targetPos > _currentPos) ? 1.0f : -1.0f;
        // Check if we've reached or passed the target position
        if ((direction > 0 && _currentPos >= _targetPos) ||
            (direction < 0 && _currentPos <= _targetPos)) {
            Stop();
            _inTorqueControlFeed = false;
            ClearCore::ConnectorUsb.SendLine("[Y-Axis] Torque-controlled feed completed");
        }
        return;
    }

    // Homing state
    if (_homingHelper->isBusy()) {
        _homingHelper->process();
    }
    else if (!_isHomed && !_homingHelper->hasFailed()) {
        _isHomed = true;
        _hasBeenHomed = true;
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

float YAxis::GetTorquePercent() const {
    // Return the moving average torque value
    return _smoothedTorque;
}

void YAxis::SetTorqueTarget(float targetPercent) {
    _torqueTarget = (targetPercent < 1.0f) ? 1.0f :
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

    float desired = targetPosition;
    if (desired < 0.0f || desired > MAX_Y_INCHES) {
        ClearCore::ConnectorUsb.SendLine("[Y-Axis] Torque feed: target out of bounds");
        return false;
    }

    _inTorqueControlFeed = true;
    _targetPos = desired;
    _maxFeedRate = (initialVelocityScale > 0.01f && initialVelocityScale <= 1.0f) ?
        initialVelocityScale : 0.5f;
    _currentFeedRate = _maxFeedRate;
    _torqueErrorAccumulator = 0.0f;
    _lastTorqueUpdateTime = ClearCore::TimingMgr.Milliseconds();

    // Start velocity move in the correct direction
    float direction = (_targetPos > _currentPos) ? 1.0f : -1.0f;
    int32_t velocitySteps = static_cast<int32_t>(direction * _currentFeedRate * _stepsPerInch);
    _motor->EnableRequest(true);
    _motor->VelMax(static_cast<uint32_t>(MAX_VELOCITY * _currentFeedRate));
    _motor->MoveVelocity(velocitySteps);
    _isMoving = true;

    ClearCore::ConnectorUsb.Send("[Y-Axis] Starting torque-controlled feed to ");
    ClearCore::ConnectorUsb.Send(_targetPos);
    ClearCore::ConnectorUsb.Send(" inches at ");
    ClearCore::ConnectorUsb.Send(_currentFeedRate * 100.0f, 0);
    ClearCore::ConnectorUsb.Send("% speed, target torque: ");
    ClearCore::ConnectorUsb.Send(_torqueTarget, 1);
    ClearCore::ConnectorUsb.SendLine("%");

    return true;
}

void YAxis::UpdateFeedRate(float newVelocityScale) {
    if (!_isSetup || !_isMoving) return;
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
    float newTorque = 0.0f;
    if (_motor->HlfbState() == MotorDriver::HLFB_HAS_MEASUREMENT) {
        newTorque = _motor->HlfbPercent();
    }
    else if (_motor->HlfbState() == MotorDriver::HLFB_ASSERTED) {
        newTorque = 100.0f;
    }
    else {
        newTorque = 0.0f;
    }

    // --- Moving average over last 200ms ---
    uint32_t now = ClearCore::TimingMgr.Milliseconds();
    _torqueBuffer[_torqueBufferHead] = newTorque;
    _torqueTimeBuffer[_torqueBufferHead] = now;
    _torqueBufferHead = (_torqueBufferHead + 1) % TORQUE_AVG_BUFFER_SIZE;
    if (_torqueBufferCount < TORQUE_AVG_BUFFER_SIZE) {
        _torqueBufferCount++;
    }

    // Remove samples older than 200ms and compute average
    float sum = 0.0f;
    size_t validCount = 0;
    for (size_t i = 0; i < _torqueBufferCount; ++i) {
        size_t idx = (_torqueBufferHead + TORQUE_AVG_BUFFER_SIZE - i - 1) % TORQUE_AVG_BUFFER_SIZE;
        if (now - _torqueTimeBuffer[idx] <= TORQUE_AVG_WINDOW_MS) {
            sum += _torqueBuffer[idx];
            validCount++;
        } else {
            break; // Older than window, stop
        }
    }
    _smoothedTorque = (validCount > 0) ? (sum / validCount) : newTorque;

    // For legacy/debug, still update _torquePct with the latest value
    _torquePct = newTorque;
}

void YAxis::AdjustFeedRateBasedOnTorque() {
    if (!_inTorqueControlFeed || !_isMoving) return;

    uint32_t now = ClearCore::TimingMgr.Milliseconds();
    float dt = static_cast<float>(now - _lastTorqueUpdateTime) / 1000.0f;
    _lastTorqueUpdateTime = now;
    if (dt < 0.005f || dt > 0.5f) dt = 0.01f;

    float torqueError = _torqueTarget - _torquePct;

    // Enhanced PID control
    static constexpr float TORQUE_Kp_Override = 0.00004f;  // Faster P response
    static constexpr float TORQUE_Ki_Override = 0.01f;  // Stronger I for steady-state
    static constexpr float TORQUE_Kd_Override = 0.02f;  // D term for smoothing

    // Keep track of previous error for derivative term
    static float previousTorqueError = 0.0f;
    float torqueErrorDerivative = (torqueError - previousTorqueError) / dt;
    previousTorqueError = torqueError;

    // Update integral with anti-windup
    _torqueErrorAccumulator += torqueError * dt;
    _torqueErrorAccumulator = (_torqueErrorAccumulator > 5.0f) ? 5.0f :
        (_torqueErrorAccumulator < -5.0f) ? -5.0f : _torqueErrorAccumulator;

    // Full PID calculation
    float feedAdjustment = (TORQUE_Kp_Override * torqueError) +
        (TORQUE_Ki_Override * _torqueErrorAccumulator) +
        (TORQUE_Kd_Override * torqueErrorDerivative);

    // Calculate target feed rate
    float targetFeedRate = _currentFeedRate + feedAdjustment * dt;
    targetFeedRate = (targetFeedRate < _minFeedRate) ? _minFeedRate :
        (targetFeedRate > _maxFeedRate) ? _maxFeedRate : targetFeedRate;

    // Smoothly transition to target feed rate
    float rateOfChange = 15.0f; // Higher = faster response (adjust as needed)
    float previousFeedRate = _currentFeedRate;
    _currentFeedRate += (targetFeedRate - _currentFeedRate) * min(1.0f, dt * rateOfChange);

    // More frequent updates with smaller threshold
    static uint32_t lastVelocityUpdateTime = 0;
    bool significantChange = fabs(_currentFeedRate - previousFeedRate) > 0.002f; // 0.2% threshold
    bool timeToUpdate = (now - lastVelocityUpdateTime) > 20; // 20ms interval (50Hz)

    if (significantChange || timeToUpdate) {
        lastVelocityUpdateTime = now;

        float direction = (_targetPos > _currentPos) ? 1.0f : -1.0f;
        int32_t velocitySteps = static_cast<int32_t>(direction * _currentFeedRate * MAX_VELOCITY);

        // Update velocity without stopping
        bool result = _motor->MoveVelocity(velocitySteps);

        ClearCore::ConnectorUsb.Send("[Y-Axis] Feed rate updated: ");
        ClearCore::ConnectorUsb.Send(_currentFeedRate * 100.0f, 1);
        ClearCore::ConnectorUsb.Send("%, change: ");
        ClearCore::ConnectorUsb.Send((_currentFeedRate - previousFeedRate) * 100.0f, 2);
        ClearCore::ConnectorUsb.Send("%, error: ");
        ClearCore::ConnectorUsb.Send(torqueError, 2);
        ClearCore::ConnectorUsb.SendLine(result ? "" : " (FAILED)");
    }

    // Regular logging
    static uint32_t lastLogTime = 0;
    if (now - lastLogTime > 250) {
        lastLogTime = now;
        ClearCore::ConnectorUsb.Send("[Y-Axis] Torque: ");
        ClearCore::ConnectorUsb.Send(_torquePct, 1);
        ClearCore::ConnectorUsb.Send("%, Target: ");
        ClearCore::ConnectorUsb.Send(_torqueTarget, 1);
        ClearCore::ConnectorUsb.Send("%, Feed rate: ");
        ClearCore::ConnectorUsb.Send(_currentFeedRate * 100.0f, 1);
        ClearCore::ConnectorUsb.SendLine("%");
    }
}



bool YAxis::IsMoving() const { return !_motor->StepsComplete(); }
bool YAxis::IsHomed()  const { return _isHomed; }
bool YAxis::IsHoming() const { return _homingHelper->isBusy(); }
