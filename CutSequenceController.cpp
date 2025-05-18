#include "CutSequenceController.h"

CutSequenceController& CutSequenceController::Instance() {
    static CutSequenceController instance;
    return instance;
}

void CutSequenceController::setup() {}

void CutSequenceController::update() {}

void CutSequenceController::startCycle() {
    _running = true;
}

void CutSequenceController::cancelCycle() {
    _running = false;
}

bool CutSequenceController::isRunning() const {
    return _running;
}
