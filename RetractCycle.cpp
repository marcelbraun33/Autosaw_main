
// === RetractCycle.cpp ===
#include "RetractCycle.h"

void RetractCycle::begin(MotionController* controller, AxisId axisId, float retractDistance) {
    _controller = controller;
    _axisId = axisId;
    _retractDistance = retractDistance;
    _started = false;
}

void RetractCycle::update() {
    if (!_controller || _started) return;
    _started = true;

    float currentPos = _controller->getAxisPosition(_axisId);
    float targetPos = currentPos - _retractDistance;

    // Rapid retract at full speed
    _controller->moveToWithRate(_axisId, targetPos, 1.0f);
}

bool RetractCycle::isFinished() const {
    if (!_controller) return true;
    // Finished when move is complete
    return !_controller->isAxisMoving(_axisId);
}
