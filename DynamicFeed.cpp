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

    // Save the current position for retract
    _startPos = static_cast<float>(_motor->PositionRefCommanded()) / _stepsPerInch;

    _state = State::Feeding;
    _targetPos = desired;
    _maxFeedRate = (initialVelocityScale > 0.01f && initialVelocityScale <= 1.0f) ?
        initialVelocityScale : 0.5f;
    _currentFeedRate = _maxFeedRate;
    _torqueErrorAccumulator = 0.0f;
    _lastTorqueUpdateTime = ClearCore::TimingMgr.Milliseconds();

    // Start velocity move in the correct direction
    float direction = (_targetPos > _startPos) ? 1.0f : -1.0f;
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
    stopAll();
    ClearCore::ConnectorUsb.SendLine("[DynamicFeed] Operation aborted");
}

bool DynamicFeed::update(float currentPos) {
    switch (_state) {
    case State::Idle:
        return false;

    case State::Feeding: {
        adjustFeedRateBasedOnTorque();
        float direction = (_targetPos > _startPos) ? 1.0f : -1.0f;
        // Check if we've reached or passed the target position
        if ((direction > 0 && currentPos >= _targetPos) ||
            (direction < 0 && currentPos <= _targetPos)) {
            ClearCore::ConnectorUsb.SendLine("[DynamicFeed] Feed complete, starting retract");
            startRetract();
        }
        break;
    }

    case State::Retracting: {
        float direction = (_startPos > _targetPos) ? 1.0f : -1.0f;
        // Check if we've reached or passed the start position
        if ((direction > 0 && currentPos >= _startPos) ||
            (direction < 0 && currentPos <= _startPos)) {
            stopAll();
            ClearCore::ConnectorUsb.SendLine("[DynamicFeed] Retract complete, ready");
            return true; // Entire cycle complete
        }
        break;
    }

    case State::Paused:
        // Do nothing while paused
        break;
    }
    return false;
}

bool DynamicFeed::isActive() const {
    return _state != State::Idle;
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
        }
        else {
            break;
        }
    }
    _smoothedTorque = (validCount > 0) ? (sum / validCount) : newTorque;
    _torquePct = newTorque;
    return _smoothedTorque;
}

float DynamicFeed::getTorquePercent() const {
    return _smoothedTorque;
}

float DynamicFeed::getCurrentFeedRate() const {
    return _currentFeedRate;
}

void DynamicFeed::setRapidVelocityScale(float scale) {
    _rapidFeedRate = (scale < _minFeedRate) ? _minFeedRate :
        (scale > 1.0f) ? 1.0f : scale;
}

float DynamicFeed::getRapidVelocityScale() const {
    return _rapidFeedRate;
}

void DynamicFeed::adjustFeedRateBasedOnTorque() {
    if (_state != State::Feeding) return;

    uint32_t now = ClearCore::TimingMgr.Milliseconds();
    float dt = static_cast<float>(now - _lastTorqueUpdateTime) / 1000.0f;
    _lastTorqueUpdateTime = now;
    if (dt < 0.005f || dt > 0.5f) dt = 0.01f;

    float torqueError = _torqueTarget - _torquePct;

    static float previousTorqueError = 0.0f;
    float torqueErrorDerivative = (torqueError - previousTorqueError) / dt;
    previousTorqueError = torqueError;

    _torqueErrorAccumulator += torqueError * dt;
    _torqueErrorAccumulator = (_torqueErrorAccumulator > 5.0f) ? 5.0f :
        (_torqueErrorAccumulator < -5.0f) ? -5.0f : _torqueErrorAccumulator;

    float feedAdjustment = (TORQUE_Kp * torqueError) +
        (TORQUE_Ki * _torqueErrorAccumulator) +
        (TORQUE_Kd * torqueErrorDerivative);

    float targetFeedRate = _currentFeedRate + feedAdjustment * dt;
    targetFeedRate = (targetFeedRate < _minFeedRate) ? _minFeedRate :
        (targetFeedRate > _maxFeedRate) ? _maxFeedRate : targetFeedRate;

    float rateOfChange = 15.0f;
    float previousFeedRate = _currentFeedRate;
    _currentFeedRate += (targetFeedRate - _currentFeedRate) * min(1.0f, dt * rateOfChange);

    static uint32_t lastVelocityUpdateTime = 0;
    bool significantChange = fabs(_currentFeedRate - previousFeedRate) > 0.002f;
    bool timeToUpdate = (now - lastVelocityUpdateTime) > 20;

    if (significantChange || timeToUpdate) {
        lastVelocityUpdateTime = now;
        float currentPos = static_cast<float>(_motor->PositionRefCommanded()) / _stepsPerInch;
        float direction = (_targetPos > _startPos) ? 1.0f : -1.0f;
        int32_t velocitySteps = static_cast<int32_t>(direction * _currentFeedRate * MAX_VELOCITY);
        _motor->MoveVelocity(velocitySteps);

        ClearCore::ConnectorUsb.Send("[DynamicFeed] Feed rate updated: ");
        ClearCore::ConnectorUsb.Send(_currentFeedRate * 100.0f, 1);
        ClearCore::ConnectorUsb.Send("%, change: ");
        ClearCore::ConnectorUsb.Send((_currentFeedRate - previousFeedRate) * 100.0f, 2);
        ClearCore::ConnectorUsb.Send("%, error: ");
        ClearCore::ConnectorUsb.Send(torqueError, 2);
        ClearCore::ConnectorUsb.SendLine("");
    }
}

void DynamicFeed::startRetract() {
    _state = State::Retracting;
    // Use rapid feed rate for retract
    float direction = (_startPos > _targetPos) ? 1.0f : -1.0f;
    int32_t velocitySteps = static_cast<int32_t>(direction * _rapidFeedRate * _stepsPerInch);
    _motor->VelMax(static_cast<uint32_t>(MAX_VELOCITY * _rapidFeedRate));
    _motor->MoveVelocity(velocitySteps);
    ClearCore::ConnectorUsb.Send("[DynamicFeed] Retracting to ");
    ClearCore::ConnectorUsb.Send(_startPos);
    ClearCore::ConnectorUsb.Send(" inches at ");
    ClearCore::ConnectorUsb.Send(_rapidFeedRate * 100.0f, 0);
    ClearCore::ConnectorUsb.SendLine("% rapid speed");
}

void DynamicFeed::stopAll() {
    _motor->MoveStopDecel();
    _state = State::Idle;
}

void DynamicFeed::pause() {
    if (_state == State::Feeding) {
        // Store feed direction before pausing (1 for forward, -1 for reverse)
        _feedDirection = (_targetPos > _startPos) ? 1.0f : -1.0f;
        
        // Save the previous state and enter paused state
        _state = State::Paused;
        
        ClearCore::ConnectorUsb.SendLine("[DynamicFeed] Feed paused");
    }
}

void DynamicFeed::resume() {
    if (_state == State::Paused) {
        // Return to feeding state
        _state = State::Feeding;
        
        // Resume velocity using stored direction and feed rate
        int32_t velocitySteps = static_cast<int32_t>(_feedDirection * _currentFeedRate * MAX_VELOCITY);
        _motor->VelMax(static_cast<uint32_t>(MAX_VELOCITY * _currentFeedRate));
        _motor->MoveVelocity(velocitySteps);
        
        _lastTorqueUpdateTime = ClearCore::TimingMgr.Milliseconds();
        
        ClearCore::ConnectorUsb.Send("[DynamicFeed] Feed resumed at ");
        ClearCore::ConnectorUsb.Send(_currentFeedRate * 100.0f, 1);
        ClearCore::ConnectorUsb.SendLine("% feed rate");
    }
}

bool DynamicFeed::isPaused() const {
    return _state == State::Paused;
}
