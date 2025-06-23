// CutSequenceController.cpp - Fixed with proper 1-based position tracking and state management
#include "CutSequenceController.h"
#include "MotionController.h"
#include "CutPositionData.h"
#include "SettingsManager.h"

// Avoid min/max macro conflicts with std:: functions
#undef min
#undef max

#include <ClearCore.h>
#include <algorithm>
#include <cmath>

CutSequenceController& CutSequenceController::Instance() {
    static CutSequenceController instance;
    return instance;
}

CutSequenceController::CutSequenceController() {
    // Initialize with proper starting values
    _lastCompletedPosition = 0;  // 0 means no positions completed yet
    _currentIndex = 0;           // Internal 0-based index
    loadPositionState();
}

// === EXISTING METHODS (updated for proper position tracking) ===
void CutSequenceController::setXIncrements(const std::vector<float>& increments) {
    _xIncrements = increments;
    _currentIndex = _lastCompletedPosition;  // Start from last completed position
}

void CutSequenceController::setYCutStart(float yStart) { _yCutStart = yStart; }
void CutSequenceController::setYCutStop(float yStop) { _yCutStop = yStop; }
void CutSequenceController::setYRetract(float yRetract) { _yRetract = yRetract; }

float CutSequenceController::getCurrentX() const {
    if (_currentIndex < _xIncrements.size()) return _xIncrements[_currentIndex];
    return 0.0f;
}

float CutSequenceController::getNextX() const {
    if (_currentIndex + 1 < _xIncrements.size()) return _xIncrements[_currentIndex + 1];
    return 0.0f;
}

float CutSequenceController::getYCutStart() const { return _yCutStart; }
float CutSequenceController::getYCutStop() const { return _yCutStop; }
float CutSequenceController::getYRetract() const { return _yRetract; }

void CutSequenceController::reset() {
    _currentIndex = 0;
    _lastCompletedPosition = 0;  // Reset to beginning
    _batchCompletedCount = 0;
    _state = SEQUENCE_IDLE;
    clearPositionState();

    ClearCore::ConnectorUsb.SendLine("[CutSeq] Reset - job zero returned to position 1");
}

bool CutSequenceController::advance() {
    if (_currentIndex + 1 < _xIncrements.size()) {
        ++_currentIndex;
        return true;
    }
    return false;
}

bool CutSequenceController::isComplete() const {
    return _lastCompletedPosition >= _xIncrements.size();
}

int CutSequenceController::getCurrentIndex() const { return _currentIndex; }
int CutSequenceController::getTotalCuts() const { return static_cast<int>(_xIncrements.size()); }

void CutSequenceController::setCurrentXPosition(float x) {
    _currentXPosition = x;
}

float CutSequenceController::getCurrentXPosition() const {
    return _currentXPosition;
}

bool CutSequenceController::isAtCurrentIncrement(float tolerance) const {
    if (_currentIndex < _xIncrements.size()) {
        return std::fabs(_currentXPosition - _xIncrements[_currentIndex]) < tolerance;
    }
    return false;
}

int CutSequenceController::findClosestIncrementIndex(float x, float tolerance) const {
    for (size_t i = 0; i < _xIncrements.size(); ++i) {
        if (std::fabs(x - _xIncrements[i]) < tolerance) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

void CutSequenceController::buildXPositions(float stockZero, float increment, int totalSlices) {
    _xIncrements.clear();
    for (int i = 0; i < totalSlices; ++i) {
        _xIncrements.push_back(stockZero + i * increment);
    }

    // Load saved state to set proper starting position
    loadPositionState();
    _currentIndex = _lastCompletedPosition;  // Start from last completed

    ClearCore::ConnectorUsb.Send("[CutSeq] Built ");
    ClearCore::ConnectorUsb.Send(totalSlices);
    ClearCore::ConnectorUsb.Send(" positions, job zero at position ");
    ClearCore::ConnectorUsb.Send(_lastCompletedPosition);
    ClearCore::ConnectorUsb.Send(" (next cut will be position ");
    ClearCore::ConnectorUsb.Send(_lastCompletedPosition + 1);
    ClearCore::ConnectorUsb.SendLine(")");
}

int CutSequenceController::getClosestIndexForPosition(float x, float tolerance) const {
    int closest = -1;
    float minDiff = tolerance;
    for (size_t i = 0; i < _xIncrements.size(); ++i) {
        float diff = std::fabs(x - _xIncrements[i]);
        if (diff < minDiff) {
            minDiff = diff;
            closest = static_cast<int>(i);
        }
    }
    return closest;
}

int CutSequenceController::getPositionIndexForX(float x, float tolerance) const {
    int closest = -1;
    float minDiff = tolerance;
    for (size_t i = 0; i < _xIncrements.size(); ++i) {
        float diff = std::fabs(x - _xIncrements[i]);
        if (diff < minDiff) {
            minDiff = diff;
            closest = static_cast<int>(i);
        }
    }
    return (closest >= 0) ? closest : 0;
}

float CutSequenceController::getXForIndex(int idx) const {
    if (idx >= 0 && idx < static_cast<int>(_xIncrements.size())) {
        return _xIncrements[idx];
    }
    return 0.0f;
}

// === CORRECTED BATCH CUTTING METHODS ===

void CutSequenceController::setLastCompletedPosition(int position) {
    _lastCompletedPosition = position;  // This is 1-based position number
    savePositionState();

    ClearCore::ConnectorUsb.Send("[CutSeq] Job zero updated to position ");
    ClearCore::ConnectorUsb.SendLine(_lastCompletedPosition);
}

void CutSequenceController::setBatchSize(int size) {
    int maxSize = getMaxBatchSize();
    _batchSize = (size > maxSize) ? maxSize : size;
    _batchSize = (_batchSize < 1) ? 1 : _batchSize;

    ClearCore::ConnectorUsb.Send("[CutSeq] Batch size set to: ");
    ClearCore::ConnectorUsb.SendLine(_batchSize);
}

int CutSequenceController::getRemainingPositions() const {
    int totalPositions = _xIncrements.size();
    return totalPositions - _lastCompletedPosition;  // Remaining cuts from current job zero
}

int CutSequenceController::getMaxBatchSize() const {
    return getRemainingPositions();
}

int CutSequenceController::getLastCompletedPosition() const {
    return _lastCompletedPosition;  // Returns 1-based position number
}

void CutSequenceController::savePositionState() {
    ClearCore::ConnectorUsb.Send("[CutSeq] Saving job zero state: position ");
    ClearCore::ConnectorUsb.SendLine(_lastCompletedPosition);
    // TODO: Implement actual EEPROM saving
}

void CutSequenceController::loadPositionState() {
    // TODO: Implement actual EEPROM loading
    // For now, _lastCompletedPosition remains at its initialized value
    ClearCore::ConnectorUsb.Send("[CutSeq] Loaded job zero state: position ");
    ClearCore::ConnectorUsb.SendLine(_lastCompletedPosition);
}

void CutSequenceController::clearPositionState() {
    _lastCompletedPosition = 0;  // Reset to beginning
    savePositionState();
}

bool CutSequenceController::startBatchSequence() {
    ClearCore::ConnectorUsb.Send("[CutSeq] startBatchSequence called - State: ");
    ClearCore::ConnectorUsb.SendLine(static_cast<int>(_state));

    if (_state != SEQUENCE_IDLE) {
        ClearCore::ConnectorUsb.SendLine("[CutSeq] Cannot start - already active");
        return false;
    }

    if (_xIncrements.empty() || _batchSize <= 0) {
        ClearCore::ConnectorUsb.Send("[CutSeq] Cannot start - no positions (");
        ClearCore::ConnectorUsb.Send(static_cast<int>(_xIncrements.size()));
        ClearCore::ConnectorUsb.Send(") or invalid batch size (");
        ClearCore::ConnectorUsb.Send(_batchSize);
        ClearCore::ConnectorUsb.SendLine(")");
        return false;
    }

    if (_lastCompletedPosition >= _xIncrements.size()) {
        ClearCore::ConnectorUsb.Send("[CutSeq] Cannot start - all positions completed (");
        ClearCore::ConnectorUsb.Send(_lastCompletedPosition);
        ClearCore::ConnectorUsb.Send("/");
        ClearCore::ConnectorUsb.Send(static_cast<int>(_xIncrements.size()));
        ClearCore::ConnectorUsb.SendLine(")");
        return false;
    }

    // CRITICAL FIX: Set up batch parameters correctly
    _batchStartPosition = _lastCompletedPosition;  // Where we're starting from (0-based for completed)
    _batchCompletedCount = 0;

    // IMPORTANT: _currentIndex should point to the NEXT position to cut
    _currentIndex = _lastCompletedPosition;  // This will be incremented before cutting

    // Log Y positions for verification
    ClearCore::ConnectorUsb.Send("[CutSeq] Y positions - Start: ");
    ClearCore::ConnectorUsb.Send(_yCutStart);
    ClearCore::ConnectorUsb.Send(", Stop: ");
    ClearCore::ConnectorUsb.Send(_yCutStop);
    ClearCore::ConnectorUsb.Send(", Retract: ");
    ClearCore::ConnectorUsb.SendLine(_yRetract);

    // Start sequence - ALWAYS begin with retract position
    _state = SEQUENCE_MOVING_TO_RETRACT;

    ClearCore::ConnectorUsb.Send("[CutSeq] Starting batch from job zero (position ");
    ClearCore::ConnectorUsb.Send(_lastCompletedPosition);
    ClearCore::ConnectorUsb.Send("), will cut positions ");
    ClearCore::ConnectorUsb.Send(_lastCompletedPosition + 1);
    ClearCore::ConnectorUsb.Send(" through ");
    int lastCutInBatch = _lastCompletedPosition + _batchSize;
    if (lastCutInBatch > _xIncrements.size()) {
        lastCutInBatch = _xIncrements.size();
    }
    ClearCore::ConnectorUsb.Send(lastCutInBatch);
    ClearCore::ConnectorUsb.SendLine("");

    return true;
}

void CutSequenceController::update() {
    if (_state == SEQUENCE_IDLE || _state == SEQUENCE_PAUSED ||
        _state == SEQUENCE_COMPLETED || _state == SEQUENCE_ABORTED) {
        return;
    }

    // Update current position from motion controller
    auto& motion = MotionController::Instance();
    _currentXPosition = motion.getAbsoluteAxisPosition(AXIS_X);
    float currentY = motion.getAbsoluteAxisPosition(AXIS_Y);

    // State machine
    switch (_state) {
    case SEQUENCE_MOVING_TO_RETRACT:
        updateMovingToRetract();
        break;
    case SEQUENCE_MOVING_TO_X:
        updateMovingToX();
        break;
    case SEQUENCE_MOVING_TO_START:
        updateMovingToStart();
        break;
    case SEQUENCE_CUTTING:
        updateCutting();
        break;
    case SEQUENCE_RETRACTING:
        updateRetracting();
        break;
    default:
        break;
    }
}

void CutSequenceController::updateMovingToRetract() {
    auto& motion = MotionController::Instance();
    float currentY = motion.getAbsoluteAxisPosition(AXIS_Y);

    // Check if already at retract position
    if (isAtPosition(_yRetract, currentY)) {
        _state = SEQUENCE_MOVING_TO_X;
        ClearCore::ConnectorUsb.SendLine("[CutSeq] Already at retract height, moving to X position");
        return;
    }

    // Start move if not already moving
    if (!motion.isAxisMoving(AXIS_Y)) {
        motion.moveTo(AXIS_Y, _yRetract, 1.0f);
        _targetY = _yRetract;
        ClearCore::ConnectorUsb.Send("[CutSeq] Moving Y to retract position: ");
        ClearCore::ConnectorUsb.SendLine(_yRetract);
    }

    // Check if reached retract position
    if (isAtPosition(_yRetract, currentY)) {
        _state = SEQUENCE_MOVING_TO_X;
        ClearCore::ConnectorUsb.SendLine("[CutSeq] At retract height, proceeding to X move");
    }
}

void CutSequenceController::updateMovingToX() {
    auto& motion = MotionController::Instance();

    // CRITICAL FIX: Calculate next position to cut correctly
    int nextCutPosition = _currentIndex + 1;  // 1-based position number
    int arrayIndex = nextCutPosition - 1;     // 0-based array index

    if (nextCutPosition > _xIncrements.size()) {
        ClearCore::ConnectorUsb.Send("[CutSeq] ERROR: Next cut position (");
        ClearCore::ConnectorUsb.Send(nextCutPosition);
        ClearCore::ConnectorUsb.Send(") > total cuts (");
        ClearCore::ConnectorUsb.Send(static_cast<int>(_xIncrements.size()));
        ClearCore::ConnectorUsb.SendLine(")");
        _state = SEQUENCE_COMPLETED;
        return;
    }

    float targetX = _xIncrements[arrayIndex];

    // Start move if not already moving
    if (!motion.isAxisMoving(AXIS_X)) {
        motion.moveTo(AXIS_X, targetX, 1.0f);
        _targetX = targetX;
        ClearCore::ConnectorUsb.Send("[CutSeq] Moving X to position ");
        ClearCore::ConnectorUsb.Send(nextCutPosition);  // 1-based for display
        ClearCore::ConnectorUsb.Send(" (array[");
        ClearCore::ConnectorUsb.Send(arrayIndex);
        ClearCore::ConnectorUsb.Send("] = ");
        ClearCore::ConnectorUsb.Send(targetX);
        ClearCore::ConnectorUsb.SendLine(")");
    }

    // Check if at X position
    if (isAtPosition(targetX, _currentXPosition)) {
        _state = SEQUENCE_MOVING_TO_START;
        ClearCore::ConnectorUsb.Send("[CutSeq] At X position ");
        ClearCore::ConnectorUsb.Send(nextCutPosition);
        ClearCore::ConnectorUsb.SendLine("");
    }
}

void CutSequenceController::updateMovingToStart() {
    auto& motion = MotionController::Instance();
    float currentY = motion.getAbsoluteAxisPosition(AXIS_Y);

    // Start move if not already moving
    if (!motion.isAxisMoving(AXIS_Y)) {
        motion.moveTo(AXIS_Y, _yCutStart, 1.0f);
        _targetY = _yCutStart;
        ClearCore::ConnectorUsb.Send("[CutSeq] Moving Y to cut start: ");
        ClearCore::ConnectorUsb.SendLine(_yCutStart);
    }

    // Check if at start position
    if (isAtPosition(_yCutStart, currentY)) {
        _state = SEQUENCE_CUTTING;

        // Start torque-controlled feed
        float cutPressure = 70.0f;  // Should get from settings
        float feedRate = 0.5f;      // Should get from settings

        motion.setTorqueTarget(AXIS_Y, cutPressure);
        motion.startTorqueControlledFeed(AXIS_Y, _yCutStop, feedRate);

        int nextCutPosition = _currentIndex + 1;  // 1-based position number
        ClearCore::ConnectorUsb.Send("[CutSeq] Starting cut at position ");
        ClearCore::ConnectorUsb.Send(nextCutPosition);
        ClearCore::ConnectorUsb.Send(" from Y=");
        ClearCore::ConnectorUsb.Send(_yCutStart);
        ClearCore::ConnectorUsb.Send(" to Y=");
        ClearCore::ConnectorUsb.SendLine(_yCutStop);
    }
}

void CutSequenceController::updateCutting() {
    auto& motion = MotionController::Instance();
    float currentY = motion.getAbsoluteAxisPosition(AXIS_Y);

    // Check if cut is complete
    bool feedComplete = !motion.isInTorqueControlledFeed(AXIS_Y);
    bool atCutStop = isAtPosition(_yCutStop, currentY, 0.05f);

    if (feedComplete && atCutStop) {
        // CRITICAL FIX: Update position tracking correctly
        _currentIndex++;  // Advance to the position we just completed
        _batchCompletedCount++;
        _lastCompletedPosition = _currentIndex;  // Update job zero (1-based)
        savePositionState();

        _state = SEQUENCE_RETRACTING;
        ClearCore::ConnectorUsb.Send("[CutSeq] Cut completed at position ");
        ClearCore::ConnectorUsb.Send(_lastCompletedPosition);
        ClearCore::ConnectorUsb.Send(" (");
        ClearCore::ConnectorUsb.Send(_batchCompletedCount);
        ClearCore::ConnectorUsb.Send("/");
        ClearCore::ConnectorUsb.Send(_batchSize);
        ClearCore::ConnectorUsb.Send(" in batch), job zero now at position ");
        ClearCore::ConnectorUsb.SendLine(_lastCompletedPosition);
    }
    else if (feedComplete && !atCutStop) {
        // Feed completed but not at target
        ClearCore::ConnectorUsb.Send("[CutSeq] WARNING: Feed complete but not at cut stop. Y=");
        ClearCore::ConnectorUsb.Send(currentY);
        ClearCore::ConnectorUsb.Send(", Target=");
        ClearCore::ConnectorUsb.SendLine(_yCutStop);

        // Still mark as complete and continue
        _currentIndex++;
        _batchCompletedCount++;
        _lastCompletedPosition = _currentIndex;
        savePositionState();
        _state = SEQUENCE_RETRACTING;
    }
}

void CutSequenceController::updateRetracting() {
    auto& motion = MotionController::Instance();
    float currentY = motion.getAbsoluteAxisPosition(AXIS_Y);

    // Start move if not already moving
    if (!motion.isAxisMoving(AXIS_Y)) {
        motion.moveTo(AXIS_Y, _yRetract, 1.0f);
        _targetY = _yRetract;
        ClearCore::ConnectorUsb.SendLine("[CutSeq] Started retract move");
    }

    // Check if at retract position
    if (isAtPosition(_yRetract, currentY)) {
        ClearCore::ConnectorUsb.SendLine("[CutSeq] Retract complete, checking for next cut");
        moveToNextBatchCut();
    }
}

void CutSequenceController::moveToNextBatchCut() {
    ClearCore::ConnectorUsb.Send("[CutSeq] moveToNextBatchCut: completed=");
    ClearCore::ConnectorUsb.Send(_batchCompletedCount);
    ClearCore::ConnectorUsb.Send(", batchSize=");
    ClearCore::ConnectorUsb.Send(_batchSize);
    ClearCore::ConnectorUsb.Send(", currentIndex=");
    ClearCore::ConnectorUsb.Send(_currentIndex);
    ClearCore::ConnectorUsb.Send(", totalCuts=");
    ClearCore::ConnectorUsb.SendLine(static_cast<int>(_xIncrements.size()));

    // Check if batch is complete
    if (_batchCompletedCount >= _batchSize) {
        // CRITICAL FIX: Reset to IDLE so next batch can start
        _state = SEQUENCE_IDLE;  // Changed from SEQUENCE_COMPLETED
        ClearCore::ConnectorUsb.Send("[CutSeq] Batch completed! Cut ");
        ClearCore::ConnectorUsb.Send(_batchCompletedCount);
        ClearCore::ConnectorUsb.Send(" positions, job zero now at position ");
        ClearCore::ConnectorUsb.Send(_lastCompletedPosition);
        ClearCore::ConnectorUsb.SendLine(", ready for next batch");
        return;
    }

    // Check if we've reached the end of all positions  
    if (_currentIndex >= _xIncrements.size()) {
        _state = SEQUENCE_COMPLETED;  // Only use COMPLETED when ALL cuts done
        ClearCore::ConnectorUsb.SendLine("[CutSeq] All positions completed!");
        return;
    }

    // Continue to next cut in batch
    _state = SEQUENCE_MOVING_TO_X;

    int nextPosition = _currentIndex + 1;  // Next position to cut (1-based)
    ClearCore::ConnectorUsb.Send("[CutSeq] Moving to next cut: position ");
    ClearCore::ConnectorUsb.Send(nextPosition);
    ClearCore::ConnectorUsb.SendLine("");
}

void CutSequenceController::pause() {
    if (_state != SEQUENCE_IDLE && _state != SEQUENCE_COMPLETED &&
        _state != SEQUENCE_PAUSED && _state != SEQUENCE_ABORTED) {
        _pausedState = _state;
        _state = SEQUENCE_PAUSED;

        auto& motion = MotionController::Instance();
        if (motion.isInTorqueControlledFeed(AXIS_Y)) {
            motion.pauseTorqueControlledFeed(AXIS_Y);
        }

        ClearCore::ConnectorUsb.SendLine("[CutSeq] Paused");
    }
}

void CutSequenceController::resume() {
    if (_state == SEQUENCE_PAUSED) {
        _state = _pausedState;

        auto& motion = MotionController::Instance();
        if (_state == SEQUENCE_CUTTING) {
            motion.resumeTorqueControlledFeed(AXIS_Y);
        }

        ClearCore::ConnectorUsb.SendLine("[CutSeq] Resumed");
    }
}

void CutSequenceController::abort() {
    _state = SEQUENCE_ABORTED;

    auto& motion = MotionController::Instance();
    motion.abortTorqueControlledFeed(AXIS_Y);

    ClearCore::ConnectorUsb.SendLine("[CutSeq] Aborted");
}

// IMPORTANT: Override isActive to properly handle COMPLETED state
bool CutSequenceController::isActive() const {
    return (_state != SEQUENCE_IDLE &&
        _state != SEQUENCE_COMPLETED &&
        _state != SEQUENCE_ABORTED);
}

float CutSequenceController::getBatchProgressPercent() const {
    if (_batchSize == 0) return 0.0f;
    return (float(_batchCompletedCount) / float(_batchSize)) * 100.0f;
}

float CutSequenceController::getBatchTargetX(int batchPosition) const {
    int targetIndex = _batchStartPosition + batchPosition;
    if (targetIndex >= 0 && targetIndex < _xIncrements.size()) {
        return _xIncrements[targetIndex];
    }
    return 0.0f;
}

bool CutSequenceController::isAtPosition(float target, float current, float tolerance) {
    return fabs(target - current) <= tolerance;
}