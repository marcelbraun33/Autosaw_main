#include "DynamicFeed.h"
#include "YAxis.h"
#include "Config.h"
#include <ClearCore.h>

static constexpr float MAX_VELOCITY = 10000.0f;  // steps/s
static constexpr float MAX_ACCELERATION = 100000.0f; // steps/s^2 - Define it here since it's not found

DynamicFeed::DynamicFeed(YAxis* owner, float stepsPerInch, MotorDriver* motor)
    : _owner(owner)
    , _stepsPerInch(stepsPerInch)
    , _motor(motor)
{
    // Store original acceleration value - use our defined constant
    _originalAccelValue = MAX_ACCELERATION;
}

DynamicFeed::~DynamicFeed() {
    // Nothing to clean up
}

void DynamicFeed::setAccelerationFactor(float factor) {
    _accelFactor = (factor < 0.2f) ? 0.2f :
        (factor > 2.0f) ? 2.0f : factor;

    ClearCore::ConnectorUsb.Send("[DynamicFeed] Acceleration factor set to: ");
    ClearCore::ConnectorUsb.SendLine(_accelFactor);
}

float DynamicFeed::getAccelerationFactor() const {
    return _accelFactor;
}

void DynamicFeed::setRampingParameters(float startRampTime, float endRampTime) {
    _startRampDuration = (startRampTime < 0.1f) ? 0.1f :
        (startRampTime > 2.0f) ? 2.0f : startRampTime;
    _endRampDuration = (endRampTime < 0.1f) ? 0.1f :
        (endRampTime > 1.0f) ? 1.0f : endRampTime;

    ClearCore::ConnectorUsb.Send("[DynamicFeed] Ramping parameters set: start=");
    ClearCore::ConnectorUsb.Send(_startRampDuration);
    ClearCore::ConnectorUsb.Send("s, end=");
    ClearCore::ConnectorUsb.Send(_endRampDuration);
    ClearCore::ConnectorUsb.SendLine("s");
}

void DynamicFeed::configureAccelerationProfile(float startRatio, float endRatio) {
    _startAccelRatio = (startRatio < 0.1f) ? 0.1f :
        (startRatio > 1.0f) ? 1.0f : startRatio;
    _endAccelRatio = (endRatio < 0.1f) ? 0.1f :
        (endRatio > 1.0f) ? 1.0f : endRatio;
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

    // Start with lower initial feed rate for gradual ramp-up
    _currentFeedRate = min(0.15f * _maxFeedRate, _maxFeedRate);
    _torqueErrorAccumulator = 0.0f;
    _lastTorqueUpdateTime = ClearCore::TimingMgr.Milliseconds();
    _rampStartTime = _lastTorqueUpdateTime;

    // Store original acceleration value
    _originalAccelValue = MAX_ACCELERATION; // Using our defined constant

    // Start velocity move in the correct direction
    float direction = (_targetPos > _startPos) ? 1.0f : -1.0f;
    _feedDirection = direction;

    // Configure more gentle acceleration for startup
    _motor->EnableRequest(true);
    _motor->VelMax(static_cast<uint32_t>(MAX_VELOCITY * _maxFeedRate));

    // Reduce acceleration rate for smoother starts
    _motor->AccelMax(static_cast<uint32_t>(_originalAccelValue * _accelFactor * _startAccelRatio));

    // Start with reduced speed for initial ramp-up
    _motor->MoveVelocity(static_cast<int32_t>(direction * _currentFeedRate * _stepsPerInch));

    ClearCore::ConnectorUsb.Send("[DynamicFeed] Starting torque-controlled feed to ");
    ClearCore::ConnectorUsb.Send(_targetPos);
    ClearCore::ConnectorUsb.Send(" inches with gentle ramp-up at ");
    ClearCore::ConnectorUsb.Send(_currentFeedRate * 100.0f, 0);
    ClearCore::ConnectorUsb.Send("% initial speed, target torque: ");
    ClearCore::ConnectorUsb.Send(_torqueTarget, 1);
    ClearCore::ConnectorUsb.SendLine("%");

    return true;
}

void DynamicFeed::abort() {
    // First reduce acceleration for gentler stop
    _motor->AccelMax(static_cast<uint32_t>(_originalAccelValue * _accelFactor * _endAccelRatio));

    // Then use decelerated stop
    _motor->MoveStopDecel();

    // Restore original acceleration after a brief delay
    Delay_ms(100);
    _motor->AccelMax(_originalAccelValue);

    _state = State::Idle;
    ClearCore::ConnectorUsb.SendLine("[DynamicFeed] Operation aborted with controlled deceleration");
}

bool DynamicFeed::update(float currentPos) {
    switch (_state) {
    case State::Idle:
        return false;

    case State::Feeding: {
        // Gradually restore normal acceleration after startup ramp
        uint32_t now = ClearCore::TimingMgr.Milliseconds();
        if (now - _rampStartTime < _startRampDuration * 1000) {
            // Still in startup ramp, maintain reduced acceleration
            float progressRatio = static_cast<float>(now - _rampStartTime) / (_startRampDuration * 1000);
            float accelRatio = _startAccelRatio + progressRatio * (1.0f - _startAccelRatio);
            uint32_t targetAccel = static_cast<uint32_t>(_originalAccelValue * _accelFactor * accelRatio);

            // Don't update too frequently - only when there's a meaningful change
            static uint32_t lastAccelUpdate = 0;
            if (now - lastAccelUpdate > 100) {
                lastAccelUpdate = now;
                _motor->AccelMax(targetAccel);
            }
        }
        else {
            // Once ramp time has passed, restore to normal acceleration with factor applied
            _motor->AccelMax(static_cast<uint32_t>(_originalAccelValue * _accelFactor));
        }

        // Continue with normal feed rate adjustment
        adjustFeedRateBasedOnTorque();
        float direction = (_targetPos > _startPos) ? 1.0f : -1.0f;

        // Check if we've reached or passed the target position
        if ((direction > 0 && currentPos >= _targetPos) ||
            (direction < 0 && currentPos <= _targetPos)) {
            ClearCore::ConnectorUsb.SendLine("[DynamicFeed] Feed complete, preparing for smooth retract");

            // Smoothly transition to stop before retracting
            _motor->MoveStopDecel();
            Delay_ms(200); // Wait for deceleration to complete

            startRetract();
        }
        break;
    }

    case State::Retracting: {
        float direction = (_startPos > _targetPos) ? 1.0f : -1.0f;
        // Check if we've reached or passed the start position
        if ((direction > 0 && currentPos >= _startPos) ||
            (direction < 0 && currentPos <= _startPos)) {

            // Reduce acceleration for final approach
            _motor->AccelMax(static_cast<uint32_t>(_originalAccelValue * _accelFactor * _endAccelRatio));

            // Slow deceleration stop
            _motor->MoveStopDecel();

            // Restore original acceleration
            Delay_ms(100);
            _motor->AccelMax(_originalAccelValue);

            _state = State::Idle;
            ClearCore::ConnectorUsb.SendLine("[DynamicFeed] Retract complete with smooth stop, ready");
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

    // Reduced rate of change for smoother transitions (5.0 instead of 15.0)
    float rateOfChange = 5.0f;
    float previousFeedRate = _currentFeedRate;
    _currentFeedRate += (targetFeedRate - _currentFeedRate) * min(1.0f, dt * rateOfChange);

    static uint32_t lastVelocityUpdateTime = 0;
    bool significantChange = fabs(_currentFeedRate - previousFeedRate) > 0.002f;
    bool timeToUpdate = (now - lastVelocityUpdateTime) > 20;

    if (significantChange || timeToUpdate) {
        lastVelocityUpdateTime = now;
        float currentPos = static_cast<float>(_motor->PositionRefCommanded()) / _stepsPerInch;
        float direction = (_targetPos > _startPos) ? 1.0f : -1.0f;
        int32_t velocitySteps = static_cast<int32_t>(direction * _currentFeedRate * _stepsPerInch);
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

    // Configure gentle acceleration for retract to avoid jerk
    _motor->AccelMax(static_cast<uint32_t>(_originalAccelValue * _accelFactor * _startAccelRatio));

    // Execute multi-step velocity ramp to avoid jerky starts
    float direction = (_startPos > _targetPos) ? 1.0f : -1.0f;
    _motor->VelMax(static_cast<uint32_t>(MAX_VELOCITY * _rapidFeedRate));

    // Start at 30% speed
    _motor->MoveVelocity(static_cast<int32_t>(direction * 0.3f * _rapidFeedRate * _stepsPerInch));
    Delay_ms(100);

    // Ramp to 60% speed 
    _motor->MoveVelocity(static_cast<int32_t>(direction * 0.6f * _rapidFeedRate * _stepsPerInch));
    Delay_ms(100);

    // Final ramp to full retract speed
    _motor->MoveVelocity(static_cast<int32_t>(direction * _rapidFeedRate * _stepsPerInch));

    // Restore normal acceleration after starting the retract
    Delay_ms(100);
    _motor->AccelMax(static_cast<uint32_t>(_originalAccelValue * _accelFactor));

    ClearCore::ConnectorUsb.Send("[DynamicFeed] Retracting to ");
    ClearCore::ConnectorUsb.Send(_startPos);
    ClearCore::ConnectorUsb.Send(" inches with smooth acceleration at ");
    ClearCore::ConnectorUsb.Send(_rapidFeedRate * 100.0f, 0);
    ClearCore::ConnectorUsb.SendLine("% rapid speed");
}

void DynamicFeed::stopAll() {
    // Use controlled deceleration instead of abrupt stop
    _motor->AccelMax(static_cast<uint32_t>(_originalAccelValue * _accelFactor * _endAccelRatio));
    _motor->MoveStopDecel();

    // Delay to allow for deceleration before state change
    Delay_ms(100);

    // Restore original acceleration
    _motor->AccelMax(_originalAccelValue);
    _state = State::Idle;
}

void DynamicFeed::pause() {
    if (_state == State::Feeding) {
        // Store feed direction before pausing (1 for forward, -1 for reverse)
        _feedDirection = (_targetPos > _startPos) ? 1.0f : -1.0f;

        // Reduce acceleration for smoother stop
        _motor->AccelMax(static_cast<uint32_t>(_originalAccelValue * _accelFactor * _endAccelRatio));

        // Stop with controlled deceleration
        _motor->MoveStopDecel();

        // Save the previous state and enter paused state
        _state = State::Paused;

        ClearCore::ConnectorUsb.SendLine("[DynamicFeed] Feed paused with gentle deceleration");
    }
}

void DynamicFeed::resume() {
    if (_state == State::Paused) {
        // Return to feeding state
        _state = State::Feeding;
        _rampStartTime = ClearCore::TimingMgr.Milliseconds();

        // Configure gentler acceleration for resumption
        _motor->AccelMax(static_cast<uint32_t>(_originalAccelValue * _accelFactor * _startAccelRatio));

        // Start with lower speed then ramp up
        float initialResumeRate = _currentFeedRate * 0.5f;

        // Resume velocity using stored direction and gradual feed rate ramp
        _motor->MoveVelocity(static_cast<int32_t>(_feedDirection * initialResumeRate * _stepsPerInch));
        Delay_ms(100);

        // Gradually increase to target speed
        _motor->MoveVelocity(static_cast<int32_t>(_feedDirection * _currentFeedRate * 0.7f * _stepsPerInch));
        Delay_ms(100);

        // Full speed
        _motor->MoveVelocity(static_cast<int32_t>(_feedDirection * _currentFeedRate * _stepsPerInch));

        _lastTorqueUpdateTime = ClearCore::TimingMgr.Milliseconds();

        ClearCore::ConnectorUsb.Send("[DynamicFeed] Feed resumed with smooth acceleration at ");
        ClearCore::ConnectorUsb.Send(_currentFeedRate * 100.0f, 1);
        ClearCore::ConnectorUsb.SendLine("% feed rate");
    }
}

bool DynamicFeed::isPaused() const {
    return _state == State::Paused;
}

void DynamicFeed::executeRampToVelocity(float targetVelocityScale, float rampTime) {
    float direction = _feedDirection;

    uint32_t startTime = ClearCore::TimingMgr.Milliseconds();
    float startVelocity = 0.2f * targetVelocityScale; // Start at 20% of target

    // Start with lower speed
    _motor->MoveVelocity(static_cast<int32_t>(direction * startVelocity * _stepsPerInch));

    // Ramp up in small increments
    uint32_t steps = 5; // Number of steps in ramp
    uint32_t stepTime = static_cast<uint32_t>((rampTime * 1000.0f) / steps);

    for (uint32_t i = 1; i <= steps; i++) {
        float ratio = static_cast<float>(i) / steps;
        float velocity = startVelocity + (targetVelocityScale - startVelocity) * ratio;

        Delay_ms(stepTime);
        _motor->MoveVelocity(static_cast<int32_t>(direction * velocity * _stepsPerInch));
    }
}
