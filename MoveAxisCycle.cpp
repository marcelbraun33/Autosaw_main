#include "MoveAxisCycle.h"

MoveAxisCycle::MoveAxisCycle(MotionController &mc, AxisId axis, float target, float speed)
    : _mc(mc), _axis(axis), _target(target), _speed(speed) {}

void MoveAxisCycle::start() {
    _started = true;
    _stopped = false;
    _error = false;
    _errorCode = CycleError::None;
    _mc.moveToWithRate(_axis, _target, _speed);
}

void MoveAxisCycle::stop() {
    _stopped = true;
    _mc.stopAxis(_axis);
}

bool MoveAxisCycle::isDone() const {
    if (_stopped) return true;
    return !_mc.isAxisMoving(_axis);
}

bool MoveAxisCycle::hasError() const {
    return _error;
}

CycleError MoveAxisCycle::error() const {
    return _errorCode;
}
