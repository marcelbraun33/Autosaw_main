// AutoCutCycleManager.cpp - Updated for batch cutting
#include "AutoCutCycleManager.h"
#include "ClearCore.h"

AutoCutCycleManager& AutoCutCycleManager::Instance() {
    static AutoCutCycleManager instance;
    return instance;
}

AutoCutCycleManager::AutoCutCycleManager() {}

void AutoCutCycleManager::setup() {
    _state = AutoCutState::Idle;
    _spindleStarted = false;
    _isFeedHold = false;
    _isReturningToStart = false;
}

void AutoCutCycleManager::startCycle() {
    // Use batch sequence instead of reset
    if (!CutSequenceController::Instance().startBatchSequence()) {
        ClearCore::ConnectorUsb.SendLine("[AutoCutCycle] Failed to start batch sequence");
        return;
    }

    setState(AutoCutState::SpindleStart);
}

void AutoCutCycleManager::pauseCycle() {
    if (_state == AutoCutState::DynamicFeed) {
        _isFeedHold = true;
        setState(AutoCutState::Paused);
        CutSequenceController::Instance().pause();
        updateUI();
    }
}

void AutoCutCycleManager::resumeCycle() {
    if (_state == AutoCutState::Paused) {
        _isFeedHold = false;
        setState(AutoCutState::DynamicFeed);
        CutSequenceController::Instance().resume();
        updateUI();
    }
}

void AutoCutCycleManager::abortCycle() {
    stopSpindle();
    CutSequenceController::Instance().abort();
    setState(AutoCutState::Idle);
    _isFeedHold = false;
    _isReturningToStart = false;
    updateUI();
}

void AutoCutCycleManager::feedHold() {
    if (_state == AutoCutState::DynamicFeed || _state == AutoCutState::Paused) {
        if (!_isFeedHold) {
            pauseCycle();
        }
        else {
            resumeCycle();
        }
    }
}

void AutoCutCycleManager::exitFeedHold() {
    if (_state == AutoCutState::Paused && !_isReturningToStart) {
        _isReturningToStart = true;
        setState(AutoCutState::Returning);

        // Abort the sequence controller
        CutSequenceController::Instance().abort();

        // Move Y back to retract position
        float yRetract = CutSequenceController::Instance().getYRetract();
        moveToY(yRetract);
        updateUI();
    }
}

void AutoCutCycleManager::update() {
    // Let CutSequenceController handle its own state machine
    CutSequenceController::Instance().update();

    // Monitor sequence state and update our state accordingly
    auto seqState = CutSequenceController::Instance().getState();

    switch (seqState) {
    case CutSequenceController::SEQUENCE_IDLE:
        if (_state != AutoCutState::Idle && _state != AutoCutState::SpindleStart) {
            setState(AutoCutState::Idle);
        }
        break;

    case CutSequenceController::SEQUENCE_MOVING_TO_RETRACT:
    case CutSequenceController::SEQUENCE_MOVING_TO_X:
    case CutSequenceController::SEQUENCE_MOVING_TO_START:
        if (_state != AutoCutState::MoveToX && _state != AutoCutState::MoveToCutStartY) {
            setState(AutoCutState::MoveToX);
        }
        break;

    case CutSequenceController::SEQUENCE_CUTTING:
        if (_state != AutoCutState::DynamicFeed && _state != AutoCutState::Paused) {
            setState(AutoCutState::DynamicFeed);
        }
        break;

    case CutSequenceController::SEQUENCE_RETRACTING:
        if (_state != AutoCutState::RetractY) {
            setState(AutoCutState::RetractY);
        }
        break;

    case CutSequenceController::SEQUENCE_COMPLETED:
        setState(AutoCutState::Complete);
        stopSpindle();
        break;

    case CutSequenceController::SEQUENCE_PAUSED:
        if (_state != AutoCutState::Paused) {
            setState(AutoCutState::Paused);
        }
        break;

    case CutSequenceController::SEQUENCE_ABORTED:
        setState(AutoCutState::Idle);
        stopSpindle();
        break;
    }

    // Handle spindle start separately
    if (_state == AutoCutState::SpindleStart) {
        startSpindle();
        _spindleStarted = true;
        setState(AutoCutState::WaitForSpindle);
    }
    else if (_state == AutoCutState::WaitForSpindle) {
        if (ClearCore::TimingMgr.Milliseconds() - _stateStartTime > 500) {
            // Let sequence controller take over
            setState(AutoCutState::MoveToRetractY);
        }
    }
    else if (_state == AutoCutState::Returning && _isReturningToStart) {
        // Check if at retract position
        float yRetract = CutSequenceController::Instance().getYRetract();
        if (MotionController::Instance().isAxisAtPosition(AXIS_Y, yRetract)) {
            _isReturningToStart = false;
            setState(AutoCutState::Idle);
        }
    }
}

void AutoCutCycleManager::setState(AutoCutState newState) {
    _state = newState;
    _stateStartTime = ClearCore::TimingMgr.Milliseconds();
    updateUI();
}

void AutoCutCycleManager::executeState() {
    // This method is now mostly replaced by monitoring CutSequenceController
    // Keep for compatibility but most logic moved to update()
}

void AutoCutCycleManager::startSpindle() {
    // Start spindle at desired RPM
    MotionController::Instance().StartSpindle(MotionController::Instance().getSpindleRPM());
}

void AutoCutCycleManager::stopSpindle() {
    MotionController::Instance().StopSpindle();
}

void AutoCutCycleManager::moveToY(float pos) {
    MotionController::Instance().MoveAxisTo(AXIS_Y, pos);
}

void AutoCutCycleManager::moveToX(float pos) {
    MotionController::Instance().MoveAxisTo(AXIS_X, pos);
}

void AutoCutCycleManager::startDynamicFeed(float yStop) {
    // This is now handled by CutSequenceController
    // Keep for compatibility
    MotionController::Instance().startTorqueControlledFeed(AXIS_Y, yStop, 1.0f);
}

void AutoCutCycleManager::updateUI() {
    // Update UI elements/buttons/LEDs as needed
    // This should be implemented to match your UI framework
}

bool AutoCutCycleManager::isInCycle() const {
    // Check both our state and sequence controller state
    bool ourState = (_state != AutoCutState::Idle &&
        _state != AutoCutState::Complete &&
        _state != AutoCutState::Error);

    bool seqState = CutSequenceController::Instance().isActive();

    return ourState || seqState;
}