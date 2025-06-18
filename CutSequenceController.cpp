// CutSequenceController.cpp - Enhanced to include existing code plus new features
#include "CutSequenceController.h"
#include "MotionController.h"
#include "CutPositionData.h"
#include <ClearCore.h>
#include <algorithm>

CutSequenceController& CutSequenceController::Instance() {
    static CutSequenceController instance;
    return instance;
}

CutSequenceController::CutSequenceController() {}

// === EXISTING METHODS (unchanged) ===
void CutSequenceController::setXIncrements(const std::vector<float>& increments) {
    _xIncrements = increments;
    _currentIndex = 0;

    // NEW: Initialize completion tracking
    _cutCompleted.resize(increments.size(), false);
    _completedCount = 0;
    buildCutOrder();  // Build cut order based on pattern
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
    _currentOrderIndex = 0;
    _completedCount = 0;
    _state = SEQUENCE_IDLE;
    resetCompletionStatus();
}

bool CutSequenceController::advance() {
    if (_currentIndex + 1 < _xIncrements.size()) {
        ++_currentIndex;
        return true;
    }
    return false;
}

bool CutSequenceController::isComplete() const {
    return _completedCount >= _xIncrements.size();
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

    // Initialize completion tracking
    _cutCompleted.resize(totalSlices, false);
    _completedCount = 0;
    buildCutOrder();
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

// === NEW STATE MACHINE METHODS ===

bool CutSequenceController::startSequence() {
    if (_state != SEQUENCE_IDLE) {
        ClearCore::ConnectorUsb.SendLine("[CutSeq] Cannot start - already active");
        return false;
    }

    if (_xIncrements.empty()) {
        ClearCore::ConnectorUsb.SendLine("[CutSeq] Cannot start - no positions defined");
        return false;
    }

    // Reset state
    _currentOrderIndex = 0;
    _state = SEQUENCE_MOVING_TO_RETRACT;

    // Set first cut index based on pattern
    if (!_cutOrder.empty()) {
        _currentIndex = _cutOrder[0];
    }

    ClearCore::ConnectorUsb.Send("[CutSeq] Starting sequence with ");
    ClearCore::ConnectorUsb.Send(getTotalCuts());
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

    // Start move if not already moving
    if (!motion.isAxisMoving(AXIS_Y)) {
        motion.moveTo(AXIS_Y, _yRetract, 1.0f);  // Full speed
        _targetY = _yRetract;
    }

    // Check if at retract position
    if (isAtPosition(_yRetract, currentY)) {
        _state = SEQUENCE_MOVING_TO_X;
        ClearCore::ConnectorUsb.SendLine("[CutSeq] At retract height");
    }
}

void CutSequenceController::updateMovingToX() {
    auto& motion = MotionController::Instance();

    // Get target X for current cut
    if (_currentOrderIndex >= _cutOrder.size()) return;
    int cutIndex = _cutOrder[_currentOrderIndex];
    float targetX = _xIncrements[cutIndex];

    // Start move if not already moving
    if (!motion.isAxisMoving(AXIS_X)) {
        motion.moveTo(AXIS_X, targetX, 1.0f);  // Full speed
        _targetX = targetX;
    }

    // Check if at X position
    if (isAtPosition(targetX, _currentXPosition)) {
        _state = SEQUENCE_MOVING_TO_START;
        _currentIndex = cutIndex;  // Update current index
        ClearCore::ConnectorUsb.Send("[CutSeq] At X position ");
        ClearCore::ConnectorUsb.SendLine(targetX);
    }
}

void CutSequenceController::updateMovingToStart() {
    auto& motion = MotionController::Instance();
    float currentY = motion.getAbsoluteAxisPosition(AXIS_Y);

    // Start move if not already moving
    if (!motion.isAxisMoving(AXIS_Y)) {
        motion.moveTo(AXIS_Y, _yCutStart, 1.0f);  // Full speed
        _targetY = _yCutStart;
    }

    // Check if at start position
    if (isAtPosition(_yCutStart, currentY)) {
        _state = SEQUENCE_CUTTING;

        // Start torque-controlled feed
        auto& posData = CutPositionData::Instance();
        float cutPressure = 70.0f;  // Default, should get from settings
        float feedRate = 0.5f;      // Default, should get from settings

        motion.setTorqueTarget(AXIS_Y, cutPressure);
        motion.startTorqueControlledFeed(AXIS_Y, _yCutStop, feedRate);

        ClearCore::ConnectorUsb.Send("[CutSeq] Starting cut ");
        ClearCore::ConnectorUsb.Send(_currentOrderIndex + 1);
        ClearCore::ConnectorUsb.Send(" of ");
        ClearCore::ConnectorUsb.SendLine(getTotalCuts());
    }
}

void CutSequenceController::updateCutting() {
    auto& motion = MotionController::Instance();
    float currentY = motion.getAbsoluteAxisPosition(AXIS_Y);

    // Check if cut is complete
    if (!motion.isInTorqueControlledFeed(AXIS_Y) && isAtPosition(_yCutStop, currentY)) {
        // Mark this cut as completed
        markCurrentAsCompleted();

        _state = SEQUENCE_RETRACTING;
        ClearCore::ConnectorUsb.SendLine("[CutSeq] Cut completed");
    }
}

void CutSequenceController::updateRetracting() {
    auto& motion = MotionController::Instance();
    float currentY = motion.getAbsoluteAxisPosition(AXIS_Y);

    // Start move if not already moving
    if (!motion.isAxisMoving(AXIS_Y)) {
        motion.moveTo(AXIS_Y, _yRetract, 1.0f);  // Full speed
        _targetY = _yRetract;
    }

    // Check if at retract position
    if (isAtPosition(_yRetract, currentY)) {
        moveToNextCut();
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

void CutSequenceController::setCutPattern(CutPattern pattern) {
    _cutPattern = pattern;
    buildCutOrder();
}

void CutSequenceController::buildCutOrder() {
    _cutOrder.clear();
    int total = _xIncrements.size();

    switch (_cutPattern) {
    case PATTERN_LEFT_TO_RIGHT:
        for (int i = 0; i < total; i++) {
            _cutOrder.push_back(i);
        }
        break;

    case PATTERN_RIGHT_TO_LEFT:
        for (int i = total - 1; i >= 0; i--) {
            _cutOrder.push_back(i);
        }
        break;

    case PATTERN_ALTERNATING:
        for (int i = 0; i < total; i++) {
            if (i % 2 == 0) {
                _cutOrder.push_back(i / 2);
            }
            else {
                _cutOrder.push_back(total - 1 - (i / 2));
            }
        }
        break;

    case PATTERN_CENTER_OUT:
    {
        int center = total / 2;
        _cutOrder.push_back(center);
        for (int offset = 1; offset <= center; offset++) {
            if (center - offset >= 0) _cutOrder.push_back(center - offset);
            if (center + offset < total) _cutOrder.push_back(center + offset);
        }
    }
    break;

    case PATTERN_OUTSIDE_IN:
    {
        int left = 0;
        int right = total - 1;
        bool fromLeft = true;
        while (left <= right) {
            if (fromLeft) {
                _cutOrder.push_back(left++);
            }
            else {
                _cutOrder.push_back(right--);
            }
            fromLeft = !fromLeft;
        }
    }
    break;
    }
}

bool CutSequenceController::isActive() const {
    return (_state != SEQUENCE_IDLE &&
        _state != SEQUENCE_COMPLETED &&
        _state != SEQUENCE_ABORTED);
}

float CutSequenceController::getProgressPercent() const {
    if (_xIncrements.empty()) return 0.0f;
    return (float(_completedCount) / float(_xIncrements.size())) * 100.0f;
}

void CutSequenceController::markCurrentAsCompleted() {
    if (_currentOrderIndex < _cutOrder.size()) {
        int cutIndex = _cutOrder[_currentOrderIndex];
        if (cutIndex < _cutCompleted.size() && !_cutCompleted[cutIndex]) {
            _cutCompleted[cutIndex] = true;
            _completedCount++;
        }
    }
}

bool CutSequenceController::isCutCompleted(int index) const {
    if (index >= 0 && index < _cutCompleted.size()) {
        return _cutCompleted[index];
    }
    return false;
}

void CutSequenceController::resetCompletionStatus() {
    std::fill(_cutCompleted.begin(), _cutCompleted.end(), false);
    _completedCount = 0;
}

void CutSequenceController::moveToNextCut() {
    _currentOrderIndex++;

    if (_currentOrderIndex >= _cutOrder.size() || isComplete()) {
        _state = SEQUENCE_COMPLETED;
        ClearCore::ConnectorUsb.SendLine("[CutSeq] All cuts completed!");
    }
    else {
        _state = SEQUENCE_MOVING_TO_X;
        ClearCore::ConnectorUsb.Send("[CutSeq] Moving to cut ");
        ClearCore::ConnectorUsb.Send(_currentOrderIndex + 1);
        ClearCore::ConnectorUsb.Send(" of ");
        ClearCore::ConnectorUsb.SendLine(getTotalCuts());
    }
}

bool CutSequenceController::isAtPosition(float target, float current, float tolerance) {
    return fabs(target - current) <= tolerance;
}