#include "DynamicFeed.h"
#include "YAxis.h"
#include "Config.h"
#include <ClearCore.h>

static constexpr float MAX_VELOCITY = 10000.0f;  // steps/s

DynamicFeed::DynamicFeed(YAxis* owner, float stepsPerInch, MotorDriver* motor)
    : _owner(owner)
    , _stepsPerInch(stepsPerInch)
    , _motor(motor)
{
}

DynamicFeed::~DynamicFeed() {
    // Nothing to clean up
}

bool DynamicFeed::start(float targetPosition, float initialVelocityScale) {
    if (!_motor) {
        ClearCore::ConnectorUsb.SendLine("[DynamicFeed] Cannot start: no motor available");
        return false;
    }

    float desired = targetPosition;
    if (desired < 0.0f || desired > MAX_Y_INCHES) {
        ClearCore::ConnectorUsb.SendLine("[DynamicFeed] Target out of bounds");
        return false;
    }

    _isActive = true;
    _targetPos = desired;
    _maxFeedRate = (initialVelocityScale > 0.01f && initialVelocityScale <= 1.0f) ?
        initialVelocityScale : 0.5f;
    _currentFeedRate = _maxFeedRate;
    _torqueErrorAccumulator = 0.0f;
    _lastTorqueUpdateTime = ClearCore::TimingMgr.Milliseconds();

    // Get current position from the owner
    float currentPos = static_cast<float>(_motor->PositionRefCommanded()) / _stepsPerInch;
    
    // Start velocity move in the correct direction
    float direction = (_targetPos > currentPos) ? 1.0f : -1.0f;
    int32_t velocitySteps = static_cast<int32_t>(direction * _currentFeedRate * _stepsPerInch);
    _motor->EnableRequest(true);
    _motor->VelMax(static_cast<uint32_t>(MAX_VELOCITY * _currentFeedRate));
    _motor->MoveVelocity(velocitySteps);

    ClearCore::ConnectorUsb.Send("[DynamicFeed] Starting torque-controlled feed to ");
    ClearCore::ConnectorUsb.Send(_targetPos);
    ClearCore::ConnectorUsb.Send(" inches at ");
    ClearCore::ConnectorUsb.Send(_currentFeedRate * 100.0f, 0);
    ClearCore::ConnectorUsb.Send("% speed, target torque: ");
    ClearCore::ConnectorUsb.Send(_torqueTarget, 1);
    ClearCore::ConnectorUsb.SendLine("%");

    return true;
}

void DynamicFeed::abort() {
    if (_isActive) {
        // Let YAxis handle the stop command
        _isActive = false;
        ClearCore::ConnectorUsb.SendLine("[DynamicFeed] Torque-controlled feed aborted");
    }
}

bool DynamicFeed::update(float currentPos) {
    if (!_isActive) {
        return false;
    }

    adjustFeedRateBasedOnTorque();
    
    float direction = (_targetPos > currentPos) ? 1.0f : -1.0f;
    
    // Check if we've reached or passed the target position
    if ((direction > 0 && currentPos >= _targetPos) ||
        (direction < 0 && currentPos <= _targetPos)) {
        _isActive = false;
        ClearCore::ConnectorUsb.SendLine("[DynamicFeed] Torque-controlled feed completed");
        return true;
    }
    
    return false;
}

bool DynamicFeed::isActive() const {
    return _isActive;
}

void DynamicFeed::setTorqueTarget(float targetPercent) {
    _torqueTarget = (targetPercent < 1.0f) ? 1.0f :
        (targetPercent > 95.0f) ? 95.0f : targetPercent;
    ClearCore::ConnectorUsb.Send("[DynamicFeed] Torque target set to ");
    ClearCore::ConnectorUsb.Send(_torqueTarget, 1);
    ClearCore::ConnectorUsb.SendLine("%");
}

float DynamicFeed::getTorqueTarget() const {
    return _torqueTarget;
}

void DynamicFeed::updateFeedRate(float newVelocityScale) {
    float velocityScale = (newVelocityScale < _minFeedRate) ? _minFeedRate :
        (newVelocityScale > 1.0f) ? 1.0f : newVelocityScale;
    _currentFeedRate = velocityScale;
    _motor->VelMax(static_cast<uint32_t>(MAX_VELOCITY * velocityScale));
}

float DynamicFeed::updateTorqueMeasurement() {
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

    // --- Moving average over last window period ---
    uint32_t now = ClearCore::TimingMgr.Milliseconds();
    _torqueBuffer[_torqueBufferHead] = newTorque;
    _torqueTimeBuffer[_torqueBufferHead] = now;
    _torqueBufferHead = (_torqueBufferHead + 1) % TORQUE_AVG_BUFFER_SIZE;
    if (_torqueBufferCount < TORQUE_AVG_BUFFER_SIZE) {
        _torqueBufferCount++;
    }

    // Remove samples older than window period and compute average
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

    // For current value measurement
    _torquePct = newTorque;
    
    return _smoothedTorque;
}

float DynamicFeed::getTorquePercent() const {
    return _smoothedTorque;
}

float DynamicFeed::getCurrentFeedRate() const {
    return _currentFeedRate;
}

void DynamicFeed::adjustFeedRateBasedOnTorque() {
    if (!_isActive) return;

    uint32_t now = ClearCore::TimingMgr.Milliseconds();
    float dt = static_cast<float>(now - _lastTorqueUpdateTime) / 1000.0f;
    _lastTorqueUpdateTime = now;
    if (dt < 0.005f || dt > 0.5f) dt = 0.01f;

    float torqueError = _torqueTarget - _torquePct;

    // Keep track of previous error for derivative term
    static float previousTorqueError = 0.0f;
    float torqueErrorDerivative = (torqueError - previousTorqueError) / dt;
    previousTorqueError = torqueError;

    // Update integral with anti-windup
    _torqueErrorAccumulator += torqueError * dt;
    _torqueErrorAccumulator = (_torqueErrorAccumulator > 5.0f) ? 5.0f :
        (_torqueErrorAccumulator < -5.0f) ? -5.0f : _torqueErrorAccumulator;

    // Full PID calculation
    float feedAdjustment = (TORQUE_Kp * torqueError) +
        (TORQUE_Ki * _torqueErrorAccumulator) +
        (TORQUE_Kd * torqueErrorDerivative);

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

        // Get current position from motor
        float currentPos = static_cast<float>(_motor->PositionRefCommanded()) / _stepsPerInch;
        float direction = (_targetPos > currentPos) ? 1.0f : -1.0f;
        int32_t velocitySteps = static_cast<int32_t>(direction * _currentFeedRate * MAX_VELOCITY);

        // Update velocity without stopping
        bool result = _motor->MoveVelocity(velocitySteps);

        ClearCore::ConnectorUsb.Send("[DynamicFeed] Feed rate updated: ");
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
        ClearCore::ConnectorUsb.Send("[DynamicFeed] Torque: ");
        ClearCore::ConnectorUsb.Send(_torquePct, 1);
        ClearCore::ConnectorUsb.Send("%, Target: ");
        ClearCore::ConnectorUsb.Send(_torqueTarget, 1);
        ClearCore::ConnectorUsb.Send("%, Feed rate: ");
        ClearCore::ConnectorUsb.Send(_currentFeedRate * 100.0f, 1);
        ClearCore::ConnectorUsb.SendLine("%");
    }
}
