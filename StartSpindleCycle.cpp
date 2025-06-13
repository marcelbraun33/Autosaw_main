#include "StartSpindleCycle.h"

StartSpindleCycle::StartSpindleCycle(Spindle &spindle, bool state)
    : _spindle(spindle), _targetState(state) {}

void StartSpindleCycle::start() {
    _started = true;
    _done = false;
    _error = false;
    _errorCode = CycleError::None;

    if (_targetState) {
        _spindle.start();
    } else {
        _spindle.stop();
    }
    _done = true; // Assume instant for now
}

void StartSpindleCycle::stop() {
    // No-op for spindle, could force stop if needed
    _done = true;
}

bool StartSpindleCycle::isDone() const {
    return _done;
}

bool StartSpindleCycle::hasError() const {
    return _error;
}

CycleError StartSpindleCycle::error() const {
    return _errorCode;
}
