// AutoCutCycleManager.h
#pragma once
#include "CutSequenceController.h"
#include "MotionController.h"

// State machine for the auto cut cycle
enum class AutoCutState {
    Idle,
    SpindleStart,
    WaitForSpindle,
    MoveToRetractY,
    MoveToX,
    MoveToCutStartY,
    DynamicFeed,
    RetractY,
    Returning,
    Complete,
    Paused,
    Error
};

class AutoCutCycleManager {
public:
    static AutoCutCycleManager& Instance();

    void setup();
    void update();

    // Cycle control
    void startCycle();
    void pauseCycle();
    void resumeCycle();
    void abortCycle();
    void feedHold();
    void exitFeedHold();

    // State info
    AutoCutState getState() const { return _state; }
    bool isActive() const { return _state != AutoCutState::Idle && _state != AutoCutState::Complete; }
    bool isPaused() const { return _state == AutoCutState::Paused; }
    bool isReturning() const { return _state == AutoCutState::Returning; }
    bool isInCycle() const;

private:
    AutoCutCycleManager();

    void setState(AutoCutState newState);
    void executeState();

    // Internal helpers
    void startSpindle();
    void stopSpindle();
    void moveToY(float pos);
    void moveToX(float pos);
    void startDynamicFeed(float yStop);
    void updateUI();

    // State
    AutoCutState _state = AutoCutState::Idle;
    unsigned long _stateStartTime = 0;
    bool _spindleStarted = false;
    bool _isFeedHold = false;
    bool _isReturningToStart = false;
    float _feedStartY = 0.0f;
};
