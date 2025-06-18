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
    // Validate parameters
    if (!CutPositionData::Instance().isReadyForSequence()) {
        ClearCore::ConnectorUsb.SendLine("[AutoCut] Parameters not ready");
        return;
    }
    
    // Build the sequence from current parameters
    CutPositionData::Instance().buildCutSequence();
    
    // Start the sequence
    if (CutSequenceController::Instance().startSequence()) {
        _state = STATE_IN_CYCLE;
    }
}

void AutoCutCycleManager::pauseCycle() {
    if (_state == AutoCutState::DynamicFeed) {
        _isFeedHold = true;
        setState(AutoCutState::Paused);
        // Optionally: MotionController::Instance().pauseFeed();
        updateUI();
    }
}

void AutoCutCycleManager::resumeCycle() {
    if (_state == AutoCutState::Paused) {
        _isFeedHold = false;
        setState(AutoCutState::DynamicFeed);
        // Optionally: MotionController::Instance().resumeFeed();
        updateUI();
    }
}

void AutoCutCycleManager::abortCycle() {
    stopSpindle();
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
        // Move Y back to _feedStartY (the position before feed started)
        moveToY(_feedStartY);
        updateUI();
    }
}

void AutoCutCycleManager::update() {
    // Let the sequence controller handle everything
    CutSequenceController::Instance().update();
    
    // Update position data for display
    auto& motion = MotionController::Instance();
    CutPositionData::Instance().updateCurrentPosition(
        motion.getAbsoluteAxisPosition(AXIS_X),
        motion.getAbsoluteAxisPosition(AXIS_Y)
    );
}

void AutoCutCycleManager::setState(AutoCutState newState) {
    _state = newState;
    _stateStartTime = ClearCore::TimingMgr.Milliseconds();
    updateUI();
}

void AutoCutCycleManager::executeState() {
    auto& seq = CutSequenceController::Instance();
    auto& motion = MotionController::Instance();

    switch (_state) {
    case AutoCutState::Idle:
        // Wait for start
        break;
    case AutoCutState::SpindleStart:
        startSpindle();
        _spindleStarted = true;
        setState(AutoCutState::WaitForSpindle);
        break;
    case AutoCutState::WaitForSpindle:
        if (ClearCore::TimingMgr.Milliseconds() - _stateStartTime > 500) {
            setState(AutoCutState::MoveToRetractY);
        }
        break;
    case AutoCutState::MoveToRetractY:
        moveToY(seq.getYRetract());
        if (motion.isAxisAtPosition(AXIS_Y, seq.getYRetract())) {
            setState(AutoCutState::MoveToX);
        }
        break;
    case AutoCutState::MoveToX:
        moveToX(seq.getCurrentX());
        if (motion.isAxisAtPosition(AXIS_X, seq.getCurrentX())) {
            setState(AutoCutState::MoveToCutStartY);
        }
        break;
    case AutoCutState::MoveToCutStartY:
        moveToY(seq.getYCutStart());
        if (motion.isAxisAtPosition(AXIS_Y, seq.getYCutStart())) {
            _feedStartY = seq.getYCutStart();
            setState(AutoCutState::DynamicFeed);
        }
        break;
    case AutoCutState::DynamicFeed:
        startDynamicFeed(seq.getYCutStop());
        // Wait for feed to complete or feed hold/abort
        if (_isFeedHold) {
            setState(AutoCutState::Paused);
        }
        else if (motion.isFeedComplete()) {
            setState(AutoCutState::RetractY);
        }
        break;
    case AutoCutState::RetractY:
        moveToY(seq.getYRetract());
        if (motion.isAxisAtPosition(AXIS_Y, seq.getYRetract())) {
            if (seq.advance()) {
                setState(AutoCutState::MoveToX);
            }
            else {
                setState(AutoCutState::Complete);
            }
        }
        break;
    case AutoCutState::Returning:
        // Wait for Y to reach _feedStartY
        if (motion.isAxisAtPosition(AXIS_Y, _feedStartY)) {
            _isReturningToStart = false;
            setState(AutoCutState::Idle);
        }
        break;
    case AutoCutState::Complete:
        stopSpindle();
        setState(AutoCutState::Idle);
        break;
    case AutoCutState::Paused:
        // Remain paused until resume or exitFeedHold
        break;
    case AutoCutState::Error:
        // Handle error
        break;
    }
}

void AutoCutCycleManager::startSpindle() {
    // Example: Start spindle at desired RPM
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
    // Start dynamic feed (torque/pressure controlled) to yStop
    // Provide a default initialVelocityScale (e.g., 1.0f)
    MotionController::Instance().startTorqueControlledFeed(AXIS_Y, yStop, 1.0f);
}

void AutoCutCycleManager::updateUI() {
    // Update UI elements/buttons/LEDs as needed
    // Example: show feed hold, exit, and return button states
    // This should be implemented to match your UI framework
}

bool AutoCutCycleManager::isInCycle() const {
    // Consider "in cycle" as any state except Idle, Complete, or Error
    return _state != AutoCutState::Idle &&
           _state != AutoCutState::Complete &&
           _state != AutoCutState::Error;
}
