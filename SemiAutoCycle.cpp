#include "SemiAutoCycle.h"
#include <cstring>
#include <cmath>
#include "Config.h"

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

    updateStateMachine();
}

bool SemiAutoCycle::isActive() const { return _state != Idle && _state != Complete && _state != Canceled && _state != Error; }
bool SemiAutoCycle::isComplete() const { return _state == Complete; }
bool SemiAutoCycle::isPaused() const { return _state == Paused; }
bool SemiAutoCycle::hasError() const { return _state == Error; }
const char* SemiAutoCycle::errorMessage() const { return _errorMsg; }

void SemiAutoCycle::setFeedRate(float rate) {
    _feedRate = rate;

    // Compute new velocity scale
    if (_state == FeedingToStop && _yAxis) {
        float feedRateIPS = _feedRate / 60.0f;
        float scale = calculateVelocityScale(feedRateIPS);

        // First, attempt to update velocity directly
        _yAxis->UpdateVelocity(scale);

        // Then, if motor is not moving (e.g. at start/after a jitter), reissue MoveTo
        float currentPos = _yAxis->GetActualPosition();
        float stopPos = _cutData.cutEndPoint;
        float delta = stopPos - currentPos;
        if (std::abs(delta) > 0.01f && !_yAxis->IsMoving()) {
            // Reset & prepare so that MoveTo takes new scale
            _yAxis->ResetAndPrepare();
            _yAxis->MoveTo(stopPos, scale);
        }
    }
}

void SemiAutoCycle::setCutPressure(float pressure) { _cutPressure = pressure; }
void SemiAutoCycle::setSpindleOn(bool on) { _spindleOn = on; }
void SemiAutoCycle::moveTableToRetract() { transitionTo(MovingToRetract); }
void SemiAutoCycle::moveTableToStart() { transitionTo(MovingToStart); }
void SemiAutoCycle::incrementCutIndex(int delta) {
    int newIndex = _cutIndex + delta;
    if (newIndex >= 0 && newIndex < _cutData.totalSlices) {
        _cutIndex = newIndex;
    }
}

void SemiAutoCycle::feedToStop() {
    if (_state != Ready) {
        return;
    }
    if (!_yAxis) {
        transitionTo(Error);
        snprintf(_errorMsg, sizeof(_errorMsg), "Y axis not available");
        return;
    }
    _yAxis->ClearAlerts();
    _feedStartPos = _yAxis->GetPosition();
    transitionTo(FeedingToStop);
    _spindleOn = true;
}

bool SemiAutoCycle::attemptRecovery() {
    if (_yAxis) {
        _yAxis->Stop();
        if (!_yAxis->ResetAndPrepare()) {
            return false;
        }
        _spindleOn = false;
        transitionTo(Ready);
        return true;
    }
    return false;
}

void SemiAutoCycle::forceReset() {
    if (_yAxis) {
        _yAxis->Stop();
        _yAxis->ClearAlerts();
    }
    _spindleOn = false;
    _feedHold = false;
    transitionTo(Ready);
}

void SemiAutoCycle::updateFeedRateVelocity() {
    if (_state == FeedingToStop && _yAxis) {
        float scale = calculateVelocityScale(_feedRate / 60.0f);
        _yAxis->UpdateVelocity(scale);
    }
}

float SemiAutoCycle::calculateVelocityScale(float feedIPS) const {
    if (!_yAxis) return 1.0f;
    float s = feedIPS * _yAxis->GetStepsPerInch();
    s /= MAX_VELOCITY_Y;
    if (s < 0.01f) s = 0.01f;
    if (s > 1.0f)  s = 1.0f;
    return s;
}

const char* SemiAutoCycle::stateToString(State state) const {
    switch (state) {
    case Idle:            return "Idle";
    case Ready:           return "Ready";
    case Returning:       return "Returning";
    case MovingToRetract: return "MovingToRetract";
    case MovingToStart:   return "MovingToStart";
    case WaitingAtStart:  return "WaitingAtStart";
    case FeedingToStop:   return "FeedingToStop";
    case Retracting:      return "Retracting";
    case Paused:          return "Paused";
    case Complete:        return "Complete";
    case Canceled:        return "Canceled";
    case Error:           return "Error";
    }
    return "Unknown";
}

float SemiAutoCycle::commandedRPM()    const { return _settings ? _settings->settings().defaultRPM : 0.0f; }
float SemiAutoCycle::currentThickness()const { return _cutData.thickness; }
float SemiAutoCycle::distanceToGo()    const { return _yAxis ? std::abs(_yAxis->GetActualPosition() - _cutData.cutEndPoint) : 0.0f; }

bool SemiAutoCycle::isSpindleOn()               const { return _spindleOn; }
bool SemiAutoCycle::isAtRetract()               const { if (_yAxis) { float t = _cutData.cutStartPoint - _cutData.retractDistance; return std::abs(_yAxis->GetPosition() - t) < 0.01f; } return false; }
bool SemiAutoCycle::isFeedRateOffsetActive()    const { return _settings && std::abs(_feedRate - _settings->settings().feedRate) > 0.001f; }
bool SemiAutoCycle::isCutPressureOffsetActive() const { return _settings && std::abs(_cutPressure - _settings->settings().cutPressure) > 0.001f; }

void SemiAutoCycle::transitionTo(State newState) { _state = newState; }

void SemiAutoCycle::updateStateMachine() {
    float raw = distanceToGo();
    _displayDist = _displayDist * 0.9f + raw * 0.1f;

    // Always update velocity mid‐move without stopping
    if (_state == FeedingToStop) {
        updateFeedRateVelocity();
    }

    static State lastState = Idle;
    if (_state != lastState) {
        lastState = _state;
    }

    switch (_state) {
    case Ready: break;

    case FeedingToStop: {
        if (_yAxis) {
            float stopPos = _cutData.cutEndPoint;
            if (std::abs(_yAxis->GetPosition() - stopPos) < 0.01f) {
                transitionTo(Returning);
                return;
            }
            if (!_yAxis->IsMoving() && _yAxis->ResetAndPrepare()) {
                _yAxis->MoveTo(stopPos, calculateVelocityScale(_feedRate / 60.0f));
            }
        }
    } break;

    case Returning: {
        if (_yAxis) {
            if (std::abs(_yAxis->GetPosition() - _feedStartPos) < 0.01f) {
                _spindleOn = false;
                transitionTo(Ready);
                return;
            }
            if (!_yAxis->IsMoving() && _yAxis->ResetAndPrepare()) {
                _yAxis->MoveTo(_feedStartPos, 1.0f);
            }
        }
    } break;

    default: break;
    }
}
