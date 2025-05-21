
// === MPGJogManager.cpp ===
#include "MPGJogManager.h"

MPGJogManager& MPGJogManager::Instance() {
    static MPGJogManager inst;
    return inst;
}

MPGJogManager::MPGJogManager() = default;

void MPGJogManager::setup() {
    _initialized = true;
    _enabled = false;
}

void MPGJogManager::update() {
    // Nothing to poll
}

void MPGJogManager::setEnabled(bool en) {
    _enabled = en;
}

bool MPGJogManager::isEnabled() const {
    return _initialized && _enabled;
}

void MPGJogManager::setAxis(AxisId axis) {
    _currentAxis = axis;
}

void MPGJogManager::setRangeMultiplier(int multiplier) {
    _range = multiplier;
}

void MPGJogManager::onEncoderDelta(int deltaClicks) {
    if (!isEnabled()) return;
    // deltaClicks * (0.001" * multiplier)
    float inches = deltaClicks * (0.001f * _range);
    MotionController::Instance().moveToWithRate(
        _currentAxis,
        MotionController::Instance().getAxisPosition(_currentAxis) + inches,
        0.5f
    );
}
