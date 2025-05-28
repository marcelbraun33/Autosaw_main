// CutCycleManager.h
#pragma once
#include "Config.h"
#include "MotionController.h"

// Enum to track the current cycle state
enum CutCycleState {
    CYCLE_IDLE,
    CYCLE_FEED,
    CYCLE_CUTTING,
    CYCLE_RETRACTING,
    CYCLE_POSITIONING,
    CYCLE_COMPLETE,
    CYCLE_PAUSED,
    CYCLE_ERROR
};

// Enum to define the cycle type
enum CutCycleType {
    CYCLE_SEMI_AUTO,
    CYCLE_FULLY_AUTO
};

class CutCycleManager {
public:
    // Singleton access
    static CutCycleManager& Instance();

    // Initialize the manager
    void setup();

    // Update function to be called in the main loop
    void update();

    // Cycle control functions
    bool startCycle(CutCycleType type);
    bool feedToPosition(int targetPosition);
    void pauseCycle();
    void resumeCycle();
    void cancelCycle();
    void exitFeedAndReturn();

    // Position control
    void advanceToNextPosition();
    void decrementToNextPosition();
    void rapidToPosition(int targetPosition);

    // Status information
    CutCycleState getCurrentState() const { return _currentState; }
    int getCurrentSlicePosition() const { return _currentPosition; }
    bool isCycleActive() const { return _currentState != CYCLE_IDLE && _currentState != CYCLE_COMPLETE; }
    bool isPaused() const { return _isPaused; }

    // Cut pressure and feed rate adjustment
    void setCutPressure(float pressure) { _cutPressure = pressure; }
    float getCutPressure() const { return _cutPressure; }

    void setFeedRate(float rate) { _feedRate = rate; }
    float getFeedRate() const { return _feedRate; }

private:
    CutCycleManager();

    // State management
    void setState(CutCycleState newState);
    void executeCurrentState();

    // Specific cycle execution steps
    void executeFeedCycle();
    void executeCuttingCycle();
    void executeRetractCycle();
    void executePositioningCycle();

    // Internal tracking variables
    CutCycleState _currentState;
    CutCycleType _cycleType;
    bool _isPaused;

    // Position tracking
    int _currentPosition;
    int _targetPosition;

    // Cut parameters
    float _cutPressure;
    float _feedRate;

    unsigned long _lastStateChangeTime;
    unsigned long _cycleStartTime;
};
