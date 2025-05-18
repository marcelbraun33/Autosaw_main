// === FeedCycle.cpp ===
#include "FeedCycle.h"

void FeedCycle::begin(MotionController* controller, AxisId axisId, float targetPos) {
    _controller = controller;
    _axisId = axisId;
    _target = targetPos;
}

void FeedCycle::update() {
    if (!_controller) return;
    float torque = readTorque();
    applyPressureCompensation(torque);
    float rate = _manualOverride * _autoAdjustment;
    _controller->moveToWithRate(_axisId, _target, rate);
}

float FeedCycle::readTorque() {
    return _controller ? _controller->getTorquePercent(_axisId) : 0.0f;
}

void FeedCycle::applyPressureCompensation(float torque) {
    if (torque > _pressureThreshold) {
        _autoAdjustment *= 0.9f;
    }
}

void FeedCycle::setManualOverride(float value) {
    _manualOverride = value;
}

float FeedCycle::getEffectiveOverride() const {
    return _manualOverride * _autoAdjustment;
}

bool FeedCycle::isFinished() const {
    // TODO: implement finish logic
    return false;
}

bool FeedCycle::hasError() const {
    // TODO: implement error detection
    return false;
}
