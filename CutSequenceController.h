// CutSequenceController.h - Enhanced version building on existing code
#pragma once
#include <vector>
#include <cmath>

class CutSequenceController {
public:
    // Add state machine for sequence execution
    enum SequenceState {
        SEQUENCE_IDLE,
        SEQUENCE_MOVING_TO_RETRACT,
        SEQUENCE_MOVING_TO_X,
        SEQUENCE_MOVING_TO_START,
        SEQUENCE_CUTTING,
        SEQUENCE_RETRACTING,
        SEQUENCE_COMPLETED,
        SEQUENCE_PAUSED,
        SEQUENCE_ABORTED
    };

    // Add cutting patterns
    enum CutPattern {
        PATTERN_LEFT_TO_RIGHT,    // Your current default
        PATTERN_RIGHT_TO_LEFT,
        PATTERN_ALTERNATING,
        PATTERN_CENTER_OUT,
        PATTERN_OUTSIDE_IN
    };

    static CutSequenceController& Instance();

    // === EXISTING METHODS (keep all of these) ===
    void setXIncrements(const std::vector<float>& increments);
    void setYCutStart(float yStart);
    void setYCutStop(float yStop);
    void setYRetract(float yRetract);

    float getCurrentX() const;
    float getNextX() const;
    float getYCutStart() const;
    float getYCutStop() const;
    float getYRetract() const;

    void reset();
    bool advance();
    bool isComplete() const;
    int getCurrentIndex() const;
    int getTotalCuts() const;

    void buildXPositions(float stockZero, float increment, int totalSlices);
    int getClosestIndexForPosition(float x, float tolerance = 0.005f) const;

    // X position tracking and comparison
    void setCurrentXPosition(float x);
    float getCurrentXPosition() const;
    bool isAtCurrentIncrement(float tolerance = 0.001f) const;
    int findClosestIncrementIndex(float x, float tolerance = 0.001f) const;
    int getPositionIndexForX(float x, float tolerance = 0.001f) const;
    float getXForIndex(int idx) const;

    // === NEW METHODS FOR STATE MACHINE ===
    // Sequence control
    bool startSequence();
    void update();  // Call this from main loop
    void pause();
    void resume();
    void abort();

    // State queries
    SequenceState getState() const { return _state; }
    bool isActive() const;
    bool isPaused() const { return _state == SEQUENCE_PAUSED; }

    // Pattern control
    void setCutPattern(CutPattern pattern);
    CutPattern getCutPattern() const { return _cutPattern; }

    // Progress tracking
    float getProgressPercent() const;
    int getCompletedCount() const { return _completedCount; }
    void markCurrentAsCompleted();

    // Cut completion tracking
    bool isCutCompleted(int index) const;
    void resetCompletionStatus();

    // Get actual cut order based on pattern
    const std::vector<int>& getCutOrder() const { return _cutOrder; }
    int getCurrentOrderIndex() const { return _currentOrderIndex; }

private:
    CutSequenceController();

    // === EXISTING MEMBERS ===
    std::vector<float> _xIncrements;
    float _yCutStart = 0.0f;
    float _yCutStop = 0.0f;
    float _yRetract = 0.0f;
    int _currentIndex = 0;
    float _currentXPosition = 0.0f;

    // === NEW MEMBERS FOR STATE MACHINE ===
    SequenceState _state = SEQUENCE_IDLE;
    SequenceState _pausedState = SEQUENCE_IDLE;  // State to return to after pause
    CutPattern _cutPattern = PATTERN_LEFT_TO_RIGHT;

    // Cut order and completion tracking
    std::vector<int> _cutOrder;        // Indices in the order to cut
    std::vector<bool> _cutCompleted;   // Track which cuts are done
    int _currentOrderIndex = 0;        // Current position in _cutOrder
    int _completedCount = 0;

    // Motion tracking
    float _targetX = 0.0f;
    float _targetY = 0.0f;

    // State update methods
    void updateMovingToRetract();
    void updateMovingToX();
    void updateMovingToStart();
    void updateCutting();
    void updateRetracting();

    // Helper methods
    void buildCutOrder();
    bool isAtPosition(float target, float current, float tolerance = 0.01f);
    void moveToNextCut();
};