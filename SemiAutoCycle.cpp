#include "SemiAutoCycle.h"
#include <cstring>
#include <cmath>
#include "Config.h"



SemiAutoCycle::SemiAutoCycle(CutData& cutData)
    : Cycle(cutData),
    _state(Idle),
    _cutIndex(0),
    _spindleOn(false),
    _feedHold(false),
    _feedRate(0.0f),
    _cutPressure(0.0f),
    _yAxis(nullptr),
    _settings(&SettingsManager::Instance())
{
    std::memset(_errorMsg, 0, sizeof(_errorMsg));
}

void SemiAutoCycle::setAxes(YAxis* y) {
    _yAxis = y;
}

void SemiAutoCycle::start() {
    _state = Ready;
    _feedHold = false;
    _spindleOn = false;
    _cutIndex = 0;
    _feedRate = _settings->settings().feedRate;
    _cutPressure = _settings->settings().cutPressure;
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
    // Check for alerts on Y axis
    static uint32_t lastAlertCheck = 0;
    uint32_t now = ClearCore::TimingMgr.Milliseconds();

    if (_yAxis && (now - lastAlertCheck > 500)) { // Check every 500ms
        lastAlertCheck = now;

        if (_yAxis->HasAlerts()) {
            ClearCore::ConnectorUsb.SendLine("[SemiAutoCycle] Y axis has alerts, clearing");
            _yAxis->ClearAlerts();

            // If in Error state for a while, try auto-recovery
            static uint32_t errorStartTime = 0;
            if (_state == Error) {
                if (errorStartTime == 0) {
                    errorStartTime = now;
                }
                else if (now - errorStartTime > 5000) { // 5 seconds in error state
                    ClearCore::ConnectorUsb.SendLine("[SemiAutoCycle] Auto-recovery attempt");
                    attemptRecovery();
                    errorStartTime = 0;
                }
            }
            else {
                errorStartTime = 0;
            }
        }
    }

    updateStateMachine();
}



bool SemiAutoCycle::isActive() const {
    return _state != Idle && _state != Complete && _state != Canceled && _state != Error;
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
    return _errorMsg;
}

// UI/Control methods
void SemiAutoCycle::setFeedRate(float rate) {
    _feedRate = rate;
    // Log the new feed rate for debugging
    ClearCore::ConnectorUsb.Send("[SemiAutoCycle] Feed rate set to: ");
    ClearCore::ConnectorUsb.Send(rate);
    ClearCore::ConnectorUsb.SendLine(" in/min");
}

void SemiAutoCycle::setCutPressure(float pressure) { _cutPressure = pressure; }
void SemiAutoCycle::setSpindleOn(bool on) { _spindleOn = on; /* Add hardware control here */ }
void SemiAutoCycle::moveTableToRetract() { transitionTo(MovingToRetract); }
void SemiAutoCycle::moveTableToStart() { transitionTo(MovingToStart); }
void SemiAutoCycle::incrementCutIndex(int delta) {
    int newIndex = _cutIndex + delta;
    if (newIndex >= 0 && newIndex < _cutData.totalSlices) {
        _cutIndex = newIndex;
    }
}
void SemiAutoCycle::feedToStop() {
    if (_state == Ready) {
        if (_yAxis) {
            // Make sure the axis has no alerts before starting a move
            _yAxis->ClearAlerts();

            // Capture the starting position
            _feedStartPos = _yAxis->GetPosition();

            ClearCore::ConnectorUsb.Send("[SemiAutoCycle] Starting feed from position: ");
            ClearCore::ConnectorUsb.SendLine(_feedStartPos);
            ClearCore::ConnectorUsb.Send("[SemiAutoCycle] Target end position: ");
            ClearCore::ConnectorUsb.SendLine(_cutData.cutEndPoint);

            transitionTo(FeedingToStop);
            _spindleOn = true;
        }
        else {
            ClearCore::ConnectorUsb.SendLine("[SemiAutoCycle] Cannot feed: Y axis not available");
            transitionTo(Error);
            snprintf(_errorMsg, sizeof(_errorMsg), "Y axis not available");
        }
    }
    else {
        ClearCore::ConnectorUsb.Send("[SemiAutoCycle] Cannot start feed in state: ");
        ClearCore::ConnectorUsb.SendLine(stateToString(_state));
    }
}
bool SemiAutoCycle::attemptRecovery() {
    ClearCore::ConnectorUsb.SendLine("[SemiAutoCycle] Attempting recovery...");

    if (_yAxis) {
        // Stop any current motion
        _yAxis->Stop();

        // Reset the motor
        if (!_yAxis->ResetAndPrepare()) {
            ClearCore::ConnectorUsb.SendLine("[SemiAutoCycle] Motor reset failed");
            return false;
        }

        // Return to a known good state
        _spindleOn = false;
        transitionTo(Ready);
        return true;
    }
    return false;
}

void SemiAutoCycle::forceReset() {
    ClearCore::ConnectorUsb.SendLine("[SemiAutoCycle] Force resetting cycle state");

    if (_yAxis) {
        // Stop any ongoing motion
        _yAxis->Stop();
        _yAxis->ClearAlerts();
    }

    _spindleOn = false;
    _feedHold = false;
    transitionTo(Ready);
}


// Add this helper method to SemiAutoCycle class
float SemiAutoCycle::calculateVelocityScale(float feedRateInchesPerSec) const {
    if (!_yAxis) return 1.0f;

    float feedRateStepsPerSec = feedRateInchesPerSec * _yAxis->GetStepsPerInch();
    // Assuming MAX_VELOCITY_Y is accessible, otherwise use a reasonable value like 10000.0f
    float velocityScale = feedRateStepsPerSec / 10000.0f;

    // Clamp the velocity scale
    if (velocityScale < 0.01f) velocityScale = 0.01f;
    if (velocityScale > 1.0f) velocityScale = 1.0f;

    return velocityScale;
}

// Add this helper method to SemiAutoCycle class
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

// State queries for UI
bool SemiAutoCycle::isSpindleOn() const { return _spindleOn; }
bool SemiAutoCycle::isAtRetract() const {
    if (_yAxis) {
        float target = _cutData.cutStartPoint - _cutData.retractDistance;
        return std::abs(_yAxis->GetPosition() - target) < 0.01f;
    }
    return false;
}
bool SemiAutoCycle::isFeedRateOffsetActive() const {
    if (_settings) {
        return std::abs(_feedRate - _settings->settings().feedRate) > 0.001f;
    }
    return false;
}
bool SemiAutoCycle::isCutPressureOffsetActive() const {
    if (_settings) {
        return std::abs(_cutPressure - _settings->settings().cutPressure) > 0.001f;
    }
    return false;
}

float SemiAutoCycle::getCutPressure() const { return _cutPressure; }
float SemiAutoCycle::getFeedRate() const { return _feedRate; }

// Data for UI display
float SemiAutoCycle::commandedRPM() const {
    return _settings ? _settings->settings().defaultRPM : 0.0f;
}
float SemiAutoCycle::currentThickness() const {
    return _cutData.thickness;
}
float SemiAutoCycle::distanceToGo() const {
    if (!_yAxis) return 0.0f;
    switch (_state) {
    case FeedingToStop:
        return std::abs(_yAxis->GetPosition() - _cutData.cutEndPoint);
    case Returning:
        return std::abs(_yAxis->GetPosition() - _feedStartPos);
    default:
        return 0.0f;
    }
}

int SemiAutoCycle::currentCutIndex() const { return _cutIndex; }
int SemiAutoCycle::totalSlices() const { return _cutData.totalSlices; }

// Internal state machine logic
void SemiAutoCycle::transitionTo(State newState) {
    _state = newState;
}

void SemiAutoCycle::updateStateMachine() {
    // Debug state transitions
    static State lastState = Idle;
    if (_state != lastState) {
        ClearCore::ConnectorUsb.Send("[SemiAutoCycle] State change: ");
        ClearCore::ConnectorUsb.Send(stateToString(lastState));
        ClearCore::ConnectorUsb.Send(" -> ");
        ClearCore::ConnectorUsb.SendLine(stateToString(_state));
        lastState = _state;
    }

    switch (_state) {
    case Ready:
        // Wait for user to press "Feed to Stop"
        break;

    case FeedingToStop:
        if (_yAxis) {
            float stopPos = _cutData.cutEndPoint;

            // Check if we're already at the stop position
            if (std::abs(_yAxis->GetPosition() - stopPos) < 0.01f) {
                ClearCore::ConnectorUsb.SendLine("[FeedingToStop] Already at stop position, transitioning to Returning");
                transitionTo(Returning);
                return;
            }

            // Reset and prepare the motor if not moving
            if (!_yAxis->IsMoving()) {
                // Make sure the motor is ready for motion
                if (!_yAxis->ResetAndPrepare()) {
                    ClearCore::ConnectorUsb.SendLine("[FeedingToStop] Motor reset failed, retrying...");
                    return; // Will retry on next update
                }

                // Calculate velocity scale correctly
                float feedRateInchesPerSec = _feedRate / 60.0f;
                float velocityScale = feedRateInchesPerSec / 10.0f; // Assuming 10 in/sec max

                // Clamp velocity scale
                if (velocityScale < 0.01f) velocityScale = 0.01f;
                if (velocityScale > 1.0f) velocityScale = 1.0f;

                ClearCore::ConnectorUsb.Send("[FeedingToStop] Moving to: ");
                ClearCore::ConnectorUsb.Send(stopPos);
                ClearCore::ConnectorUsb.Send(" at velocity scale: ");
                ClearCore::ConnectorUsb.SendLine(velocityScale);

                if (!_yAxis->MoveTo(stopPos, velocityScale)) {
                    ClearCore::ConnectorUsb.SendLine("[FeedingToStop] Failed to start move! Retrying");
                    return; // Will retry on next update
                }
            }

            // Rest of existing code...
        }
        break;


    case Returning:
        if (_yAxis) {
            // Debug info
            ClearCore::ConnectorUsb.Send("[DEBUG] Returning to position: ");
            ClearCore::ConnectorUsb.SendLine(_feedStartPos);

            // Check if already at return position
            if (std::abs(_yAxis->GetPosition() - _feedStartPos) < 0.01f) {
                ClearCore::ConnectorUsb.SendLine("[Returning] Already at start position, cycle complete");
                _spindleOn = false;
                transitionTo(Ready);
                return;
            }

            // Reset and prepare the motor if not moving
            if (!_yAxis->IsMoving()) {
                // Reset and prepare the motor before attempting move
                if (!_yAxis->ResetAndPrepare()) {
                    ClearCore::ConnectorUsb.SendLine("[Returning] Motor reset failed, retrying...");
                    static uint32_t lastResetAttempt = 0;
                    uint32_t now = ClearCore::TimingMgr.Milliseconds();

                    if (now - lastResetAttempt > 1000) {  // Retry every second
                        lastResetAttempt = now;
                        // No state transition - will retry on next update
                        return;
                    }
                }
                else {
                    ClearCore::ConnectorUsb.SendLine("[DEBUG] Starting return move");
                    if (!_yAxis->MoveTo(_feedStartPos, 1.0f)) {
                        ClearCore::ConnectorUsb.SendLine("[Returning] Failed to start return move! Retrying...");
                        return; // Will retry on next update
                    }
                }
            }

            // Rest of existing code...
            // Check completion and stall detection...
        }
        break;

    case Paused:
        // Hold all motion
        break;

    case Complete:
    case Canceled:
    case Error:
    case Idle:
    default:
        break;
    }
}
