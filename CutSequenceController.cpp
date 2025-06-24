// CutSequenceController.cpp - FINAL with spindle control integration
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
    _spindleAutoControlled = true;
    _spindleStartedBySequence = false;
    _manualSpindleControl = false;
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

    // Reset spindle control state
    _spindleStartedBySequence = false;
    _manualSpindleControl = false;

    clearPositionState();

    ClearCore::ConnectorUsb.SendLine("[CutSeq] Reset - job zero returned to position 0, next cut will be position 1");
}

bool CutSequenceController::advance() {
    if (_currentIndex + 1 < _xIncrements.size()) {
        ++_currentIndex;
        return true;
    }
    return false;
}

bool CutSequenceController::isComplete() const {
    return _lastCompletedPosition >= getTotalCuts();
}

int CutSequenceController::getCurrentIndex() const { return _currentIndex; }

// CORRECTED: Return number of cutting positions (excluding position 0) with debug
int CutSequenceController::getTotalCuts() const {
    int arraySize = static_cast<int>(_xIncrements.size());
    int totalCuts = arraySize - 1;  // Exclude position 0

    // Debug logging (can be removed after testing)
    ClearCore::ConnectorUsb.Send("[CutSeq] getTotalCuts: arraySize=");
    ClearCore::ConnectorUsb.Send(arraySize);
    ClearCore::ConnectorUsb.Send(", totalCuts=");
    ClearCore::ConnectorUsb.SendLine(totalCuts);

    return totalCuts;
}

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

// CORRECTED: Build array with position 0 at index 0, positions 1-N at indices 1-N with enhanced debug
void CutSequenceController::buildXPositions(float stockZero, float increment, int totalSlices) {
    _xIncrements.clear();

    // Add position 0 (stock zero - not to be cut)
    _xIncrements.push_back(stockZero);

    // Add positions 1 through totalSlices (to be cut)
    for (int i = 1; i <= totalSlices; ++i) {
        _xIncrements.push_back(stockZero + i * increment);
    }

    // Load saved state to set proper starting position
    loadPositionState();
    _currentIndex = _lastCompletedPosition;

    // Enhanced debug logging
    ClearCore::ConnectorUsb.Send("[CutSeq] Built array for ");
    ClearCore::ConnectorUsb.Send(totalSlices);
    ClearCore::ConnectorUsb.Send(" cutting positions. Array contents:");
    for (size_t i = 0; i < _xIncrements.size() && i < 5; ++i) {
        ClearCore::ConnectorUsb.Send(" [");
        ClearCore::ConnectorUsb.Send(static_cast<int>(i));
        ClearCore::ConnectorUsb.Send("]=");
        ClearCore::ConnectorUsb.Send(_xIncrements[i]);
    }
    if (_xIncrements.size() > 5) {
        ClearCore::ConnectorUsb.Send(" ... [");
        ClearCore::ConnectorUsb.Send(static_cast<int>(_xIncrements.size() - 1));
        ClearCore::ConnectorUsb.Send("]=");
        ClearCore::ConnectorUsb.Send(_xIncrements.back());
    }
    ClearCore::ConnectorUsb.SendLine("");

    ClearCore::ConnectorUsb.Send("[CutSeq] Array size: ");
    ClearCore::ConnectorUsb.Send(static_cast<int>(_xIncrements.size()));
    ClearCore::ConnectorUsb.Send(", getTotalCuts() will return: ");
    ClearCore::ConnectorUsb.Send(static_cast<int>(_xIncrements.size()) - 1);
    ClearCore::ConnectorUsb.Send(", job zero at position ");
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

// CORRECTED: Use getTotalCuts() instead of _xIncrements.size()
int CutSequenceController::getRemainingPositions() const {
    int totalCuttingPositions = getTotalCuts();
    return totalCuttingPositions - _lastCompletedPosition;
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

// ENHANCED: startBatchSequence with spindle control
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

    // CORRECTED: Check against total cutting positions
    if (_lastCompletedPosition >= getTotalCuts()) {
        ClearCore::ConnectorUsb.Send("[CutSeq] Cannot start - all cutting positions completed (");
        ClearCore::ConnectorUsb.Send(_lastCompletedPosition);
        ClearCore::ConnectorUsb.Send("/");
        ClearCore::ConnectorUsb.Send(getTotalCuts());
        ClearCore::ConnectorUsb.SendLine(")");
        return false;
    }

    // Set up batch parameters correctly
    _batchStartPosition = _lastCompletedPosition;
    _batchCompletedCount = 0;
    _currentIndex = _lastCompletedPosition;

    // Log Y positions for verification
    ClearCore::ConnectorUsb.Send("[CutSeq] Y positions - Start: ");
    ClearCore::ConnectorUsb.Send(_yCutStart);
    ClearCore::ConnectorUsb.Send(", Stop: ");
    ClearCore::ConnectorUsb.Send(_yCutStop);
    ClearCore::ConnectorUsb.Send(", Retract: ");
    ClearCore::ConnectorUsb.SendLine(_yRetract);

    // Start with spindle control if enabled
    if (_spindleAutoControlled) {
        startSpindleIfNeeded();
        _state = SEQUENCE_STARTING_SPINDLE;
        _spindleStartTime = ClearCore::TimingMgr.Milliseconds();
        ClearCore::ConnectorUsb.SendLine("[CutSeq] Starting sequence with spindle startup");
    }
    else {
        // Skip spindle startup, go directly to retract
        _state = SEQUENCE_MOVING_TO_RETRACT;
        ClearCore::ConnectorUsb.SendLine("[CutSeq] Starting sequence without spindle control");
    }

    ClearCore::ConnectorUsb.Send("[CutSeq] Starting batch from job zero (position ");
    ClearCore::ConnectorUsb.Send(_lastCompletedPosition);
    ClearCore::ConnectorUsb.Send("), will cut positions ");
    ClearCore::ConnectorUsb.Send(_lastCompletedPosition + 1);
    ClearCore::ConnectorUsb.Send(" through ");
    int lastCutInBatch = _lastCompletedPosition + _batchSize;
    if (lastCutInBatch > getTotalCuts()) {
        lastCutInBatch = getTotalCuts();
    }
    ClearCore::ConnectorUsb.Send(lastCutInBatch);
    ClearCore::ConnectorUsb.SendLine("");

    return true;
}

// ENHANCED: update() with spindle states
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
    case SEQUENCE_STARTING_SPINDLE:
        updateStartingSpindle();
        break;
    case SEQUENCE_SPINDLE_READY:
        updateSpindleReady();
        break;
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

// NEW: Spindle control methods
void CutSequenceController::updateStartingSpindle() {
    // Wait for 1 second spin-up time
    unsigned long elapsed = ClearCore::TimingMgr.Milliseconds() - _spindleStartTime;
    if (elapsed >= 1000) { // 1 second spin-up
        if (isSpindleReady()) {
            ClearCore::ConnectorUsb.SendLine("[CutSeq] Spindle ready, proceeding to retract");
            _state = SEQUENCE_SPINDLE_READY;
        }
        else {
            ClearCore::ConnectorUsb.SendLine("[CutSeq] Warning: Spindle not ready after spin-up time, proceeding anyway");
            _state = SEQUENCE_SPINDLE_READY; // Proceed anyway
        }
    }
}

void CutSequenceController::updateSpindleReady() {
    // Spindle is ready, start the cutting sequence
    _state = SEQUENCE_MOVING_TO_RETRACT;
    ClearCore::ConnectorUsb.SendLine("[CutSeq] Spindle ready, starting cut sequence");
}

void CutSequenceController::startSpindleIfNeeded() {
    auto& motion = MotionController::Instance();

    if (!motion.IsSpindleRunning()) {
        float rpm = SettingsManager::Instance().settings().spindleRPM;
        ClearCore::ConnectorUsb.Send("[CutSeq] Starting spindle at ");
        ClearCore::ConnectorUsb.Send(rpm);
        ClearCore::ConnectorUsb.SendLine(" RPM");
        motion.StartSpindle(rpm);
        _spindleStartedBySequence = true;
        _manualSpindleControl = false;
    }
    else {
        ClearCore::ConnectorUsb.SendLine("[CutSeq] Spindle already running");
        _spindleStartedBySequence = false; // We didn't start it
    }
}

void CutSequenceController::stopSpindleIfControlled() {
    auto& motion = MotionController::Instance();

    // Only stop if we started it and auto-control is enabled
    if (_spindleAutoControlled && _spindleStartedBySequence && !_manualSpindleControl) {
        ClearCore::ConnectorUsb.SendLine("[CutSeq] Stopping spindle (sequence controlled)");
        motion.StopSpindle();
        _spindleStartedBySequence = false;
    }
    else {
        ClearCore::ConnectorUsb.SendLine("[CutSeq] Leaving spindle running (manual control or not started by sequence)");
    }
}

bool CutSequenceController::isSpindleReady() const {
    auto& motion = MotionController::Instance();
    return motion.IsSpindleRunning();
}

void CutSequenceController::manualSpindleStop() {
    auto& motion = MotionController::Instance();
    ClearCore::ConnectorUsb.SendLine("[CutSeq] Manual spindle stop during feed hold");
    motion.StopSpindle();
    _manualSpindleControl = true;
}

void CutSequenceController::manualSpindleStart() {
    auto& motion = MotionController::Instance();
    float rpm = SettingsManager::Instance().settings().spindleRPM;
    ClearCore::ConnectorUsb.Send("[CutSeq] Manual spindle start during feed hold at ");
    ClearCore::ConnectorUsb.Send(rpm);
    ClearCore::ConnectorUsb.SendLine(" RPM");
    motion.StartSpindle(rpm);
    _manualSpindleControl = true;
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

    // FINAL CORRECTED LOGIC:
    // Array structure: [pos0, pos1, pos2, ..., posN]
    // _currentIndex = number of completed positions (0-based)
    // We want to cut position (_currentIndex + 1) which is at array[_currentIndex + 1]

    int nextPositionToCut = _currentIndex + 1;        // 1-based position number (1, 2, 3...)
    int targetArrayIndex = _currentIndex + 1;         // Array index (1, 2, 3... skipping array[0])

    // CORRECTED: Check against total cutting positions
    if (nextPositionToCut > getTotalCuts()) {
        ClearCore::ConnectorUsb.Send("[CutSeq] ERROR: Next position to cut (");
        ClearCore::ConnectorUsb.Send(nextPositionToCut);
        ClearCore::ConnectorUsb.Send(") > total cutting positions (");
        ClearCore::ConnectorUsb.Send(getTotalCuts());
        ClearCore::ConnectorUsb.SendLine(")");
        _state = SEQUENCE_COMPLETED;
        return;
    }

    // Array bounds check
    if (targetArrayIndex >= static_cast<int>(_xIncrements.size())) {
        ClearCore::ConnectorUsb.Send("[CutSeq] ERROR: Array bounds - target index (");
        ClearCore::ConnectorUsb.Send(targetArrayIndex);
        ClearCore::ConnectorUsb.Send(") >= array size (");
        ClearCore::ConnectorUsb.Send(static_cast<int>(_xIncrements.size()));
        ClearCore::ConnectorUsb.SendLine(")");
        _state = SEQUENCE_COMPLETED;
        return;
    }

    float targetX = _xIncrements[targetArrayIndex];

    // Start move if not already moving
    if (!motion.isAxisMoving(AXIS_X)) {
        motion.moveTo(AXIS_X, targetX, 1.0f);
        _targetX = targetX;
        ClearCore::ConnectorUsb.Send("[CutSeq] Moving X to position ");
        ClearCore::ConnectorUsb.Send(nextPositionToCut);  // 1-based for display
        ClearCore::ConnectorUsb.Send(" (array[");
        ClearCore::ConnectorUsb.Send(targetArrayIndex);
        ClearCore::ConnectorUsb.Send("] = ");
        ClearCore::ConnectorUsb.Send(targetX);
        ClearCore::ConnectorUsb.SendLine(")");
    }

    // Check if at X position
    if (isAtPosition(targetX, _currentXPosition)) {
        _state = SEQUENCE_MOVING_TO_START;
        ClearCore::ConnectorUsb.Send("[CutSeq] At X position ");
        ClearCore::ConnectorUsb.Send(nextPositionToCut);
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

        int nextPositionToCut = _currentIndex + 1;  // 1-based position number
        ClearCore::ConnectorUsb.Send("[CutSeq] Starting cut at position ");
        ClearCore::ConnectorUsb.Send(nextPositionToCut);
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
        // Update position tracking correctly
        _currentIndex++;
        _batchCompletedCount++;
        _lastCompletedPosition = _currentIndex;
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

// ENHANCED: moveToNextBatchCut with spindle control
void CutSequenceController::moveToNextBatchCut() {
    ClearCore::ConnectorUsb.Send("[CutSeq] moveToNextBatchCut: completed=");
    ClearCore::ConnectorUsb.Send(_batchCompletedCount);
    ClearCore::ConnectorUsb.Send(", batchSize=");
    ClearCore::ConnectorUsb.Send(_batchSize);
    ClearCore::ConnectorUsb.Send(", currentIndex=");
    ClearCore::ConnectorUsb.Send(_currentIndex);
    ClearCore::ConnectorUsb.Send(", totalCuttingPositions=");
    ClearCore::ConnectorUsb.SendLine(getTotalCuts());

    // Check if batch is complete
    if (_batchCompletedCount >= _batchSize) {
        // Stop spindle if we control it
        stopSpindleIfControlled();

        _state = SEQUENCE_IDLE;
        ClearCore::ConnectorUsb.Send("[CutSeq] Batch completed! Cut ");
        ClearCore::ConnectorUsb.Send(_batchCompletedCount);
        ClearCore::ConnectorUsb.Send(" positions, job zero now at position ");
        ClearCore::ConnectorUsb.Send(_lastCompletedPosition);
        ClearCore::ConnectorUsb.SendLine(", ready for next batch");
        return;
    }

    // CORRECTED: Check against total cutting positions
    if (_currentIndex >= getTotalCuts()) {
        // Stop spindle if we control it
        stopSpindleIfControlled();

        _state = SEQUENCE_COMPLETED;
        ClearCore::ConnectorUsb.SendLine("[CutSeq] All cutting positions completed!");
        return;
    }

    // Continue to next cut in batch
    _state = SEQUENCE_MOVING_TO_X;

    int nextPositionToCut = _currentIndex + 1;  // Next position to cut (1-based)
    ClearCore::ConnectorUsb.Send("[CutSeq] Moving to next cut: position ");
    ClearCore::ConnectorUsb.Send(nextPositionToCut);
    ClearCore::ConnectorUsb.SendLine("");
}

// ENHANCED: pause with spindle handling
void CutSequenceController::pause() {
    if (_state != SEQUENCE_IDLE && _state != SEQUENCE_COMPLETED &&
        _state != SEQUENCE_PAUSED && _state != SEQUENCE_ABORTED) {
        _pausedState = _state;
        _state = SEQUENCE_PAUSED;

        auto& motion = MotionController::Instance();
        if (motion.isInTorqueControlledFeed(AXIS_Y)) {
            motion.pauseTorqueControlledFeed(AXIS_Y);
        }

        // Note: We deliberately DO NOT stop the spindle during pause
        // User can manually control it via the AutoCut screen button
        ClearCore::ConnectorUsb.SendLine("[CutSeq] Paused (spindle remains running for manual control)");
    }
}

// ENHANCED: resume with spindle handling
void CutSequenceController::resume() {
    if (_state == SEQUENCE_PAUSED) {
        _state = _pausedState;
        _pausedState = SEQUENCE_IDLE;

        auto& motion = MotionController::Instance();

        if (_state == SEQUENCE_CUTTING) {
            motion.resumeTorqueControlledFeed(AXIS_Y);
        }

        // If spindle auto-control is enabled and spindle is off, restart it
        if (_spindleAutoControlled && !motion.IsSpindleRunning()) {
            ClearCore::ConnectorUsb.SendLine("[CutSeq] Restarting spindle after resume");
            startSpindleIfNeeded();
            _state = SEQUENCE_STARTING_SPINDLE;
            _spindleStartTime = ClearCore::TimingMgr.Milliseconds();
        }

        ClearCore::ConnectorUsb.SendLine("[CutSeq] Resumed");
    }
}

// ENHANCED: abort with spindle control
void CutSequenceController::abort() {
    ClearCore::ConnectorUsb.SendLine("[CutSeq] Aborting sequence");

    // Stop spindle if we control it
    stopSpindleIfControlled();

    _state = SEQUENCE_ABORTED;

    auto& motion = MotionController::Instance();
    motion.abortTorqueControlledFeed(AXIS_Y);

    // Save current progress
    savePositionState();

    ClearCore::ConnectorUsb.SendLine("[CutSeq] Aborted, position saved");
}

// ENHANCED: isActive to include spindle states
bool CutSequenceController::isActive() const {
    return (_state != SEQUENCE_IDLE &&
        _state != SEQUENCE_COMPLETED &&
        _state != SEQUENCE_ABORTED &&
        _state != SEQUENCE_PAUSED);
}

bool CutSequenceController::isPaused() const {
    return _state == SEQUENCE_PAUSED;
}

float CutSequenceController::getBatchProgressPercent() const {
    if (_batchSize == 0) return 0.0f;
    return (float(_batchCompletedCount) / float(_batchSize)) * 100.0f;
}

float CutSequenceController::getBatchTargetX(int batchPosition) const {
    int targetIndex = _batchStartPosition + batchPosition;
    if (targetIndex >= 0 && targetIndex < static_cast<int>(_xIncrements.size())) {
        return _xIncrements[targetIndex];
    }
    return 0.0f;
}

bool CutSequenceController::isAtPosition(float target, float current, float tolerance) {
    return fabs(target - current) <= tolerance;
}

// Spindle control accessors
bool CutSequenceController::isSpindleAutoControlled() const {
    return _spindleAutoControlled;
}

void CutSequenceController::setSpindleAutoControlled(bool enabled) {
    _spindleAutoControlled = enabled;
    ClearCore::ConnectorUsb.Send("[CutSeq] Spindle auto-control set to: ");
    ClearCore::ConnectorUsb.SendLine(enabled ? "ENABLED" : "DISABLED");
}