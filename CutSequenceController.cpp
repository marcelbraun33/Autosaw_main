// CutSequenceController.cpp - Enhanced with batch cutting and persistence
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
    // Load saved position state on startup
    loadPositionState();
}

// === EXISTING METHODS (unchanged) ===
void CutSequenceController::setXIncrements(const std::vector<float>& increments) {
    _xIncrements = increments;
    _currentIndex = 0;
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
    _lastCompletedPosition = 0;
    _batchCompletedCount = 0;
    _state = SEQUENCE_IDLE;
    clearPositionState();
}

bool CutSequenceController::advance() {
    if (_currentIndex + 1 < _xIncrements.size()) {
        ++_currentIndex;
        return true;
    }
    return false;
}

bool CutSequenceController::isComplete() const {
    return _currentIndex >= _xIncrements.size();
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
    _currentIndex = 0;

    // When positions are rebuilt, load saved state
    loadPositionState();

    ClearCore::ConnectorUsb.Send("[CutSeq] Built ");
    ClearCore::ConnectorUsb.Send(totalSlices);
    ClearCore::ConnectorUsb.Send(" positions, last completed: ");
    ClearCore::ConnectorUsb.SendLine(_lastCompletedPosition);
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

// === NEW BATCH CUTTING METHODS ===

void CutSequenceController::setLastCompletedPosition(int position) {
    _lastCompletedPosition = position;
    savePositionState();
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
    return totalPositions - _lastCompletedPosition;
}

int CutSequenceController::getMaxBatchSize() const {
    return getRemainingPositions();
}

void CutSequenceController::savePositionState() {
    // Save to EEPROM or persistent storage
    // For now, just log - implement actual EEPROM saving based on your hardware
    ClearCore::ConnectorUsb.Send("[CutSeq] Saving position state: ");
    ClearCore::ConnectorUsb.SendLine(_lastCompletedPosition);

    // Example EEPROM save (pseudo-code):
    // EEPROM.write(POSITION_STATE_ADDR, POSITION_STATE_KEY);
    // EEPROM.write(POSITION_STATE_ADDR + 4, _lastCompletedPosition);
}

void CutSequenceController::loadPositionState() {
    // Load from EEPROM or persistent storage
    // For now, just initialize - implement actual EEPROM loading

    // Example EEPROM load (pseudo-code):
    // uint32_t key = EEPROM.read(POSITION_STATE_ADDR);
    // if (key == POSITION_STATE_KEY) {
    //     _lastCompletedPosition = EEPROM.read(POSITION_STATE_ADDR + 4);
    // }

    ClearCore::ConnectorUsb.Send("[CutSeq] Loaded position state: ");
    ClearCore::ConnectorUsb.SendLine(_lastCompletedPosition);
}

void CutSequenceController::clearPositionState() {
    _lastCompletedPosition = 0;
    savePositionState();
}

bool CutSequenceController::startBatchSequence() {
    if (_state != SEQUENCE_IDLE) {
        ClearCore::ConnectorUsb.SendLine("[CutSeq] Cannot start - already active");
        return false;
    }

    if (_xIncrements.empty() || _batchSize <= 0) {
        ClearCore::ConnectorUsb.SendLine("[CutSeq] Cannot start - no positions or invalid batch size");
        return false;
    }

    if (_lastCompletedPosition >= _xIncrements.size()) {
        ClearCore::ConnectorUsb.SendLine("[CutSeq] Cannot start - all positions completed");
        return false;
    }

    // Set up batch parameters
    _batchStartPosition = _lastCompletedPosition;
    _batchCompletedCount = 0;
    _currentIndex = _batchStartPosition;

    // Start sequence
    _state = SEQUENCE_MOVING_TO_RETRACT;

    ClearCore::ConnectorUsb.Send("[CutSeq] Starting batch from position ");
    ClearCore::ConnectorUsb.Send(_batchStartPosition + 1); // 1-based for display
    ClearCore::ConnectorUsb.Send(" with ");
    ClearCore::ConnectorUsb.Send(_batchSize);
    ClearCore::ConnectorUsb.SendLine(" cuts");

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
        ClearCore::ConnectorUsb.SendLine("[CutSeq] Already at retract height");
        return;
    }

    // Start move if not already moving
    if (!motion.isAxisMoving(AXIS_Y)) {
        motion.moveTo(AXIS_Y, _yRetract, 1.0f);
        _targetY = _yRetract;
    }

    // Check if reached retract position
    if (isAtPosition(_yRetract, currentY)) {
        _state = SEQUENCE_MOVING_TO_X;
        ClearCore::ConnectorUsb.SendLine("[CutSeq] At retract height");
    }
}

void CutSequenceController::updateMovingToX() {
    auto& motion = MotionController::Instance();

    // Get target X for current cut
    if (_currentIndex >= _xIncrements.size()) return;
    float targetX = _xIncrements[_currentIndex];

    // Start move if not already moving
    if (!motion.isAxisMoving(AXIS_X)) {
        motion.moveTo(AXIS_X, targetX, 1.0f);
        _targetX = targetX;
    }

    // Check if at X position
    if (isAtPosition(targetX, _currentXPosition)) {
        _state = SEQUENCE_MOVING_TO_START;
        ClearCore::ConnectorUsb.Send("[CutSeq] At X position ");
        ClearCore::ConnectorUsb.SendLine(targetX);
    }
}

void CutSequenceController::updateMovingToStart() {
    auto& motion = MotionController::Instance();
    float currentY = motion.getAbsoluteAxisPosition(AXIS_Y);

    // Start move if not already moving
    if (!motion.isAxisMoving(AXIS_Y)) {
        motion.moveTo(AXIS_Y, _yCutStart, 1.0f);
        _targetY = _yCutStart;
    }

    // Check if at start position
    if (isAtPosition(_yCutStart, currentY)) {
        _state = SEQUENCE_CUTTING;

        // Start torque-controlled feed
        auto& posData = CutPositionData::Instance();
        float cutPressure = 70.0f;  // Should get from settings
        float feedRate = 0.5f;      // Should get from settings

        motion.setTorqueTarget(AXIS_Y, cutPressure);
        motion.startTorqueControlledFeed(AXIS_Y, _yCutStop, feedRate);

        ClearCore::ConnectorUsb.Send("[CutSeq] Starting cut at position ");
        ClearCore::ConnectorUsb.SendLine(_currentIndex + 1); // 1-based for display
    }
}

void CutSequenceController::updateCutting() {
    auto& motion = MotionController::Instance();
    float currentY = motion.getAbsoluteAxisPosition(AXIS_Y);

    // Check if cut is complete
    if (!motion.isInTorqueControlledFeed(AXIS_Y) && isAtPosition(_yCutStop, currentY)) {
        // Mark this position as completed
        _batchCompletedCount++;
        _lastCompletedPosition = _currentIndex + 1; // Store as 1-based
        savePositionState();

        _state = SEQUENCE_RETRACTING;
        ClearCore::ConnectorUsb.Send("[CutSeq] Cut completed at position ");
        ClearCore::ConnectorUsb.SendLine(_lastCompletedPosition);
    }
}

void CutSequenceController::updateRetracting() {
    auto& motion = MotionController::Instance();
    float currentY = motion.getAbsoluteAxisPosition(AXIS_Y);

    // Start move if not already moving
    if (!motion.isAxisMoving(AXIS_Y)) {
        motion.moveTo(AXIS_Y, _yRetract, 1.0f);
        _targetY = _yRetract;
    }

    // Check if at retract position
    if (isAtPosition(_yRetract, currentY)) {
        moveToNextBatchCut();
    }
}

void CutSequenceController::moveToNextBatchCut() {
    // Check if batch is complete
    if (_batchCompletedCount >= _batchSize || _currentIndex + 1 >= _xIncrements.size()) {
        _state = SEQUENCE_COMPLETED;
        ClearCore::ConnectorUsb.Send("[CutSeq] Batch completed! Cut ");
        ClearCore::ConnectorUsb.Send(_batchCompletedCount);
        ClearCore::ConnectorUsb.SendLine(" positions");
    }
    else {
        // Move to next cut in batch
        _currentIndex++;
        _state = SEQUENCE_MOVING_TO_X;
        ClearCore::ConnectorUsb.Send("[CutSeq] Moving to next cut: position ");
        ClearCore::ConnectorUsb.SendLine(_currentIndex + 1);
    }
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