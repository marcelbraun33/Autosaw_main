#include "SemiAutoCycle.h"
#include <cstring>
#include <cmath>
#include "Config.h"

// ===== Core cycle operations =====

void SemiAutoCycle::setAxes(YAxis* y) {
    _yAxis = y;
}

void SemiAutoCycle::pause() {
    if (_state != Paused && isActive()) {
        _state = Paused;
        _feedHold = true;
        if (_yAxis) _yAxis->Stop();
    }
}

void SemiAutoCycle::resume() {
    if (_state == Paused) {
        _state = WaitingAtStart;
        _feedHold = false;
    }
}

void SemiAutoCycle::cancel() {
    _state = Canceled;
    _feedHold = false;
    _spindleOn = false;
    if (_yAxis) _yAxis->EmergencyStop();
}

void SemiAutoCycle::update() {
    static uint32_t lastAlertCheck = 0;
    uint32_t now = ClearCore::TimingMgr.Milliseconds();

    if (_yAxis && (now - lastAlertCheck > 500)) {
        lastAlertCheck = now;
        if (_yAxis->HasAlerts()) {
            _yAxis->ClearAlerts();
            static uint32_t errorStartTime = 0;
            if (_state == Error) {
                if (errorStartTime == 0) {
                    errorStartTime = now;
                }
                else if (now - errorStartTime > 5000) {
                    attemptRecovery();
                    errorStartTime = 0;
                }
            }
            else {
                errorStartTime = 0;
            }
        }
    }

    // Ensure cut pressure is always being used from cutData
    if (_cutData.cutPressureOverride) {
        _cutPressure = _cutData.cutPressure;
    }

    // Always propagate the current cut pressure to the Y-axis for torque control
    if (_yAxis) {
        // Set the torque limit based on the current cut pressure
        _yAxis->SetTorqueLimit(_cutData.cutPressure);
    }

    updateStateMachine();
}

// ===== Constructor and Start method =====

SemiAutoCycle::SemiAutoCycle(CutData& cutData)
    : Cycle(cutData)
    , _state(Idle)
    , _cutIndex(0)
    , _spindleOn(false)
    , _feedHold(false)
    , _feedRate(0.0f)
    , _cutPressure(0.0f)
    , _yAxis(nullptr)
    , _settings(&SettingsManager::Instance())
    , _cutData(cutData)
{
    std::memset(_errorMsg, 0, sizeof(_errorMsg));

    // Initialize cutPressure from settings or from cutData if override is active
    if (cutData.cutPressureOverride) {
        _cutPressure = cutData.cutPressure;
    }
    else {
        _cutPressure = _settings->settings().cutPressure;
        cutData.cutPressure = _cutPressure;
    }
}

void SemiAutoCycle::start() {
    _state = Ready;
    _feedHold = false;
    _spindleOn = false;
    _cutIndex = 0;
    _feedRate = _settings->settings().feedRate;

    // Use cutData pressure if override is active, otherwise use from settings
    if (_cutData.cutPressureOverride) {
        _cutPressure = _cutData.cutPressure;
    }
    else {
        _cutPressure = _settings->settings().cutPressure;
        _cutData.cutPressure = _cutPressure;
    }
}

// ===== Cycle status methods =====

bool SemiAutoCycle::isActive() const {
    return _state != Idle &&
        _state != Ready &&  // Consider Ready as not "active" for button latching
        _state != Complete &&
        _state != Canceled &&
        _state != Error;
}

bool SemiAutoCycle::isComplete() const {
    return _state == Complete;
}

bool SemiAutoCycle::isPaused() const {
    return _state == Paused;
}

bool SemiAutoCycle::hasError() const {
    return _state == Error;
}

const char* SemiAutoCycle::errorMessage() const {
    return (_state == Error) ? _errorMsg : "";
}

// ===== Recovery and Reset =====

bool SemiAutoCycle::attemptRecovery() {
    if (_state != Error)
        return true;

    if (_yAxis) {
        // Try to recover the Y axis
        if (_yAxis->ResetAndPrepare()) {
            // Recovery successful
            _state = Idle;  // Reset state to idle
            std::memset(_errorMsg, 0, sizeof(_errorMsg));
            return true;
        }
    }
    return false;
}

void SemiAutoCycle::forceReset() {
    _state = Idle;
    _feedHold = false;
    _spindleOn = false;
    std::memset(_errorMsg, 0, sizeof(_errorMsg));
}

// ===== Feed rate and cut pressure control =====

void SemiAutoCycle::setFeedRate(float rate) {
    if (rate < 0.1f) rate = 0.1f;
    if (rate > 25.0f) rate = 25.0f;

    _feedRate = rate;

    // Update the feed velocity if we're currently moving
    updateFeedRateVelocity();
}

void SemiAutoCycle::updateFeedRateVelocity() {
    if (_yAxis && _state == FeedingToStop) {
        float velocityScale = calculateVelocityScale(_feedRate);
        _yAxis->UpdateVelocity(velocityScale);
    }
}

bool SemiAutoCycle::isFeedRateOffsetActive() const {
    return std::abs(_feedRate - _settings->settings().feedRate) > 0.001f;
}

void SemiAutoCycle::setCutPressure(float pressure) {
    _cutPressure = pressure;
    _cutData.cutPressure = pressure;

    // Check if the cut pressure has been overridden
    if (std::abs(_cutPressure - _settings->settings().cutPressure) > 0.001f) {
        _cutData.cutPressureOverride = true;
    }

    // Immediately propagate to Y-axis if available
    if (_yAxis) {
        _yAxis->SetTorqueLimit(_cutData.cutPressure);
    }
}

bool SemiAutoCycle::isCutPressureOffsetActive() const {
    return _cutData.cutPressureOverride;
}

// ===== Spindle control =====

void SemiAutoCycle::setSpindleOn(bool on) {
    _spindleOn = on;
    // Implement spindle control logic here if needed
}

bool SemiAutoCycle::isSpindleOn() const {
    return _spindleOn;
}

// ===== Table movement commands =====

bool SemiAutoCycle::isAtRetract() const {
    if (!_yAxis) return false;

    // Calculate the retract position as cutStartPoint minus retractDistance
    float retractPos = _cutData.cutStartPoint - _cutData.retractDistance;
    float currentPos = _yAxis->GetPosition();

    // Allow small tolerance for floating point precision
    return std::abs(currentPos - retractPos) < 0.001f;
}

void SemiAutoCycle::moveTableToRetract() {
    if (_yAxis && _yAxis->IsHomed()) {
        _state = MovingToRetract;
        // Calculate retract point as cutStartPoint minus retractDistance
        float retractPoint = _cutData.cutStartPoint - _cutData.retractDistance;
        _yAxis->MoveTo(retractPoint, 1.0f);
    }
}

void SemiAutoCycle::moveTableToStart() {
    if (_yAxis && _yAxis->IsHomed()) {
        _state = MovingToStart;
        _yAxis->MoveTo(_cutData.cutStartPoint, 1.0f);
    }
}

void SemiAutoCycle::incrementCutIndex(int delta) {
    int newIndex = _cutIndex + delta;
    if (newIndex >= 0 && newIndex < _cutData.totalSlices) {
        _cutIndex = newIndex;
    }
}

void SemiAutoCycle::feedToStop() {
    if (_state != Ready && _state != WaitingAtStart) {
        return;
    }

    // Start feeding the Y axis to the end point
    if (_yAxis && _yAxis->IsHomed()) {
        _state = FeedingToStop;
        _feedStartPos = _yAxis->GetPosition();
        float velocityScale = calculateVelocityScale(_feedRate);

        // Move to the end point
        _yAxis->MoveTo(_cutData.cutEndPoint, velocityScale);
        _hasStartedMove = true;
    }
}

// ===== Data accessors and calculations =====

float SemiAutoCycle::calculateVelocityScale(float feedRateInchesPerSec) const {
    // Convert feed rate to velocity scale
    const float MAX_FEED_RATE = 25.0f;  // inches/min

    // Clamp feed rate to valid range
    float clampedRate = feedRateInchesPerSec;
    if (clampedRate < 0.1f) clampedRate = 0.1f;
    if (clampedRate > MAX_FEED_RATE) clampedRate = MAX_FEED_RATE;

    // Convert to a scale from 0.0 to 1.0
    return clampedRate / MAX_FEED_RATE;
}

float SemiAutoCycle::commandedRPM() const {
    // Implement based on your spindle control logic
    return _spindleOn ? 3000.0f : 0.0f;  // Default to 3000 RPM when on
}

float SemiAutoCycle::currentThickness() const {
    if (!_yAxis) return 0.0f;

    // Calculate current thickness based on Y-axis position
    float startPos = _cutData.cutStartPoint;
    float currentPos = _yAxis->GetPosition();

    return std::abs(currentPos - startPos);
}

float SemiAutoCycle::distanceToGo() const {
    if (!_yAxis || !_hasStartedMove) return 0.0f;

    // Calculate distance from current position to target
    float currentPos = _yAxis->GetPosition();
    float endPos = _cutData.cutEndPoint;

    return std::abs(endPos - currentPos);
}

// ===== State machine management =====

void SemiAutoCycle::updateStateMachine() {
    if (_feedHold && _state != Paused) {
        // Store the previous state for resuming later
        transitionTo(Paused);
        return;
    }

    switch (_state) {
    case Idle:
        // Wait for external command to change state
        break;

    case Ready:
        // Wait for feedToStop() call
        break;

    case MovingToRetract:
        if (!_yAxis->IsMoving()) {
            transitionTo(Ready);
        }
        break;

    case MovingToStart:
        if (!_yAxis->IsMoving()) {
            transitionTo(WaitingAtStart);
        }
        break;

    case WaitingAtStart:
        // Wait for feedToStop() call
        break;

    case FeedingToStop:
        // Update distance display
        _displayDist = distanceToGo();

        if (!_yAxis->IsMoving()) {
            // Movement complete - immediately transition to retracting
            transitionTo(Retracting);

            // Start the retract move
            if (_yAxis && _yAxis->IsHomed()) {
                float retractPoint = _cutData.cutStartPoint - _cutData.retractDistance;
                _yAxis->MoveTo(retractPoint, 1.0f);
            }
        }
        break;

    case Retracting:
        if (!_yAxis->IsMoving()) {
            transitionTo(Ready);
            _hasStartedMove = false; // Reset the move flag when cycle is complete
        }
        break;

    case Paused:
        // Wait for resume() call
        break;

    case Complete:
        // Auto-transition to Ready state after a brief delay
        // This ensures the "Complete" state is visible briefly
        static uint32_t completeTime = 0;
        if (completeTime == 0) {
            completeTime = ClearCore::TimingMgr.Milliseconds();
        }
        else if ((ClearCore::TimingMgr.Milliseconds() - completeTime) > 1000) {
            transitionTo(Ready);
            completeTime = 0;
        }
        break;

    case Canceled:
    case Error:
        // Terminal states that require external reset
        break;
    }
}


void SemiAutoCycle::transitionTo(State newState) {
    if (_state == newState) return;

    ClearCore::ConnectorUsb.Send("[SemiAutoCycle] State change: ");
    ClearCore::ConnectorUsb.Send(stateToString(_state));
    ClearCore::ConnectorUsb.Send(" -> ");
    ClearCore::ConnectorUsb.SendLine(stateToString(newState));

    _state = newState;
}

const char* SemiAutoCycle::stateToString(State state) const {
    switch (state) {
    case Idle: return "Idle";
    case Ready: return "Ready";
    case Returning: return "Returning";
    case MovingToRetract: return "MovingToRetract";
    case MovingToStart: return "MovingToStart";
    case WaitingAtStart: return "WaitingAtStart";
    case FeedingToStop: return "FeedingToStop";
    case Retracting: return "Retracting";
    case Paused: return "Paused";
    case Complete: return "Complete";
    case Canceled: return "Canceled";
    case Error: return "Error";
    default: return "Unknown";
    }
}