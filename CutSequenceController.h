// CutSequenceController.h - Enhanced with spindle control
#pragma once
#include <vector>
#include <cmath>
#include <stdint.h>

class CutSequenceController {
public:
    // State machine for sequence execution
    enum SequenceState {
        SEQUENCE_IDLE,
        SEQUENCE_STARTING_SPINDLE,      // NEW: Turning on spindle, waiting for spin-up
        SEQUENCE_SPINDLE_READY,         // NEW: Spindle running, ready to begin cuts
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

    // === BATCH CUTTING METHODS ===
    // Batch control
    void setLastCompletedPosition(int position);
    int getLastCompletedPosition() const;
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
    bool isPaused() const;

    // Progress tracking
    float getBatchProgressPercent() const;
    int getBatchCompletedCount() const { return _batchCompletedCount; }

    // Get target X for a given batch position (0-based within batch)
    float getBatchTargetX(int batchPosition) const;

    // === NEW SPINDLE CONTROL METHODS ===
    bool isSpindleAutoControlled() const;
    void setSpindleAutoControlled(bool enabled);

    // Manual spindle control during feed hold
    void manualSpindleStop();
    void manualSpindleStart();

private:
    CutSequenceController();

    // === EXISTING MEMBERS ===
    std::vector<float> _xIncrements;
    float _yCutStart = 0.0f;
    float _yCutStop = 0.0f;
    float _yRetract = 0.0f;
    int _currentIndex = 0;
    float _currentXPosition = 0.0f;

    // === BATCH MEMBERS ===
    int _lastCompletedPosition = 0;
    int _batchSize = 1;
    int _batchStartPosition = 0;
    int _batchCompletedCount = 0;

    // State machine
    SequenceState _state = SEQUENCE_IDLE;
    SequenceState _pausedState = SEQUENCE_IDLE;

    // Motion tracking
    float _targetX = 0.0f;
    float _targetY = 0.0f;

    // === NEW SPINDLE CONTROL MEMBERS ===
    bool _spindleAutoControlled = true;     // Whether sequence controls spindle automatically
    bool _spindleStartedBySequence = false; // Track if we started the spindle (for cleanup)
    unsigned long _spindleStartTime = 0;    // When spindle was started (for spin-up delay)
    bool _manualSpindleControl = false;     // User manually controlled spindle during feed hold

    // Persistence key for EEPROM
    static constexpr uint32_t POSITION_STATE_KEY = 0x50534354; // "PSCT"

    // State machine methods
    void updateStartingSpindle();           // NEW: Handle spindle startup
    void updateSpindleReady();              // NEW: Handle spindle ready state
    void updateMovingToRetract();
    void updateMovingToX();
    void updateMovingToStart();
    void updateCutting();
    void updateRetracting();

    // Helper methods
    bool isAtPosition(float target, float current, float tolerance = 0.01f);
    void moveToNextBatchCut();

    // NEW: Spindle helper methods
    void startSpindleIfNeeded();
    void stopSpindleIfControlled();
    bool isSpindleReady() const;
};