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
        if (_yAxis) _feedStartPos = _yAxis->GetPosition();
        transitionTo(FeedingToStop);
        _spindleOn = true;
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
    switch (_state) {
    case Ready:
        // Wait for user to press "Feed to Stop"
        break;
    case FeedingToStop:
        if (_yAxis) {
            float stopPos = _cutData.cutEndPoint;

            // Calculate the feed rate as a velocity scale
            float feedRateInchesPerSec = _feedRate / 60.0f;
            float feedRateStepsPerSec = feedRateInchesPerSec * _yAxis->GetStepsPerInch();
            float velocityScale = feedRateStepsPerSec / MAX_VELOCITY_Y;

            // Clamp the velocity scale
            if (velocityScale < 0.01f) velocityScale = 0.01f;
            if (velocityScale > 1.0f) velocityScale = 1.0f;

            if (!_yAxis->IsMoving()) {
                _yAxis->MoveTo(stopPos, velocityScale);
            }
            if (std::abs(_yAxis->GetPosition() - stopPos) < 0.01f) {
                transitionTo(Returning);
            }
        }
        break;



    case Returning:
        if (_yAxis) {
            if (!_yAxis->IsMoving()) {
                _yAxis->MoveTo(_feedStartPos, 1.0f); // Return at full speed
            }
            if (std::abs(_yAxis->GetPosition() - _feedStartPos) < 0.01f) {
                // Unlatch the button and go back to ready
                _spindleOn = false;
                transitionTo(Ready);
            }
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

