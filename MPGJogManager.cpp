
// === MPGJogManager.cpp ===
#include "MPGJogManager.h"
#include "Config.h"
#include <ClearCore.h>

void MPGJogManager::begin(MotionController* controller, AxisId axisId) {
    _controller = controller;
    _axisId = axisId;
    _lastDetent = ClearCore::EncoderIn.Position() / ENCODER_COUNTS_PER_CLICK;
}

void MPGJogManager::update() {
    if (!_controller) return;

    int32_t position = ClearCore::EncoderIn.Position();
    int32_t detent = position / ENCODER_COUNTS_PER_CLICK;
    int32_t delta = detent - _lastDetent;
    if (delta == 0) return;

    _lastDetent = detent;
    float distance = delta * JOG_STEP;
    float current = _controller->getAxisPosition(_axisId);
    _controller->moveToWithRate(_axisId, current + distance, 1.0f);
}
