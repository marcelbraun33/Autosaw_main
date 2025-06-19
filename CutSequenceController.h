// CutSequenceController.h - Enhanced with batch cutting and persistence
#pragma once
#include <vector>
#include <cmath>
#include <stdint.h>  // Add this include for uint32_t

class CutSequenceController {
public:
    // State machine for sequence execution
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

    // === NEW BATCH CUTTING METHODS ===
    // Batch control
    void setLastCompletedPosition(int position);
    int getLastCompletedPosition() const { return _lastCompletedPosition; }
    void setBatchSize(int size);
    int getBatchSize() const { return _batchSize; }
    int getRemainingPositions() const;
    int getMaxBatchSize() const;

    // Position persistence
    void savePositionState();
    void loadPositionState();
    void clearPositionState();

    // Batch sequence control
    bool startBatchSequence();
    void update();
    void pause();
    void resume();
    void abort();

    // State queries
    SequenceState getState() const { return _state; }
    bool isActive() const;
    bool isPaused() const { return _state == SEQUENCE_PAUSED; }

    // Progress tracking
    float getBatchProgressPercent() const;
    int getBatchCompletedCount() const { return _batchCompletedCount; }

    // Get target X for a given batch position (0-based within batch)
    float getBatchTargetX(int batchPosition) const;

private:
    CutSequenceController();

    // === EXISTING MEMBERS ===
    std::vector<float> _xIncrements;
    float _yCutStart = 0.0f;
    float _yCutStop = 0.0f;
    float _yRetract = 0.0f;
    int _currentIndex = 0;
    float _currentXPosition = 0.0f;

    // === NEW BATCH MEMBERS ===
    int _lastCompletedPosition = 0;  // Last position that was cut (1-based, 0 = none)
    int _batchSize = 1;              // Number of cuts in current batch
    int _batchStartPosition = 0;     // Starting position for current batch (0-based)
    int _batchCompletedCount = 0;    // Cuts completed in current batch

    // State machine
    SequenceState _state = SEQUENCE_IDLE;
    SequenceState _pausedState = SEQUENCE_IDLE;

    // Motion tracking
    float _targetX = 0.0f;
    float _targetY = 0.0f;

    // Persistence key for EEPROM - only declare once
    static constexpr uint32_t POSITION_STATE_KEY = 0x50534354; // "PSCT"

    // State machine methods
    void updateMovingToRetract();
    void updateMovingToX();
    void updateMovingToStart();
    void updateCutting();
    void updateRetracting();

    // Helper methods
    bool isAtPosition(float target, float current, float tolerance = 0.01f);
    void moveToNextBatchCut();
};