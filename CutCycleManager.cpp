
// CutCycleManager.cpp
#include "CutCycleManager.h"
#include "CutPositionData.h"
#include "SettingsManager.h"

CutCycleManager& CutCycleManager::Instance() {
    static CutCycleManager instance;
    return instance;
}

CutCycleManager::CutCycleManager()
    : _currentState(CYCLE_IDLE)
    , _cycleType(CYCLE_SEMI_AUTO)
    , _isPaused(false)
    , _currentPosition(0)
    , _targetPosition(0)
    , _cutPressure(0.0f)
    , _feedRate(0.0f)
    , _lastStateChangeTime(0)
    , _cycleStartTime(0)
{
    // Load settings
    auto& settings = SettingsManager::Instance().settings();
    _feedRate = settings.feedRate;
    // Cut pressure would be loaded from settings if implemented
}

void CutCycleManager::setup() {
    // Any additional setup needed
}

void CutCycleManager::update() {
    if (_isPaused || _currentState == CYCLE_IDLE || _currentState == CYCLE_COMPLETE) {
        return;
    }

    executeCurrentState();
}

bool CutCycleManager::startCycle(CutCycleType type) {
    if (_currentState != CYCLE_IDLE && _currentState != CYCLE_COMPLETE) {
        return false; // Already in a cycle
    }

    _cycleType = type;
    _isPaused = false;
    _cycleStartTime = ClearCore::TimingMgr.Milliseconds();
    _currentPosition = CutPositionData::Instance().getCurrentSlicePosition();

    // Set initial state to feed
    setState(CYCLE_FEED);
    return true;
}

bool CutCycleManager::feedToPosition(int targetPosition) {
    if (_currentState != CYCLE_IDLE && _currentState != CYCLE_COMPLETE) {
        return false; // Already in a cycle
    }

    auto& posData = CutPositionData::Instance();

    // Validate position
    if (targetPosition < 0 || targetPosition > posData.getTotalSlices()) {
        return false;
    }

    _targetPosition = targetPosition;
    _cycleType = CYCLE_SEMI_AUTO; // Force semi-auto mode
    _isPaused = false;
    _cycleStartTime = ClearCore::TimingMgr.Milliseconds();

    // Set initial state to feed
    setState(CYCLE_POSITIONING);
    return true;
}

void CutCycleManager::pauseCycle() {
    if (_currentState != CYCLE_IDLE && _currentState != CYCLE_COMPLETE) {
        _isPaused = true;

        // TODO: Issue motion stop commands if needed
        // MotionController::Instance().stopMotion(AXIS_X);
        // MotionController::Instance().stopMotion(AXIS_Y);
    }
}

void CutCycleManager::resumeCycle() {
    _isPaused = false;
}

void CutCycleManager::cancelCycle() {
    // Stop all movement
    auto& motion = MotionController::Instance();

    // Issue stop commands if needed
    // motion.stopMotion(AXIS_X);
    // motion.stopMotion(AXIS_Y);

    setState(CYCLE_IDLE);
    _isPaused = false;
}

void CutCycleManager::exitFeedAndReturn() {
    if (_isPaused || _currentState == CYCLE_CUTTING) {
        auto& posData = CutPositionData::Instance();

        // Return Y to start + retract position
        float retractPos = posData.getCutStartPosition() - posData.getRetractDistance();
        MotionController::Instance().moveTo(AXIS_Y, retractPos, 1.0f); // Use rapid speed

        // Cancel the cycle
        setState(CYCLE_IDLE);
        _isPaused = false;
    }
}

void CutCycleManager::advanceToNextPosition() {
    auto& posData = CutPositionData::Instance();
    int nextPos = posData.getCurrentSlicePosition() + 1;

    // Don't go beyond total slices
    if (nextPos > posData.getTotalSlices()) {
        nextPos = posData.getTotalSlices();
    }

    posData.setCurrentSlicePosition(nextPos);
    _currentPosition = nextPos;

    // Calculate X position and move
    float xPos = posData.getXZeroPosition() + (nextPos * posData.getIncrement());
    MotionController::Instance().moveTo(AXIS_X, xPos, 1.0f);
}

void CutCycleManager::decrementToNextPosition() {
    auto& posData = CutPositionData::Instance();
    int nextPos = posData.getCurrentSlicePosition() - 1;

    // Don't go below zero
    if (nextPos < 0) {
        nextPos = 0;
    }

    posData.setCurrentSlicePosition(nextPos);
    _currentPosition = nextPos;

    // Calculate X position and move
    float xPos = posData.getXZeroPosition() + (nextPos * posData.getIncrement());
    MotionController::Instance().moveTo(AXIS_X, xPos, 1.0f);
}

void CutCycleManager::rapidToPosition(int targetPosition) {
    auto& posData = CutPositionData::Instance();

    // Validate position
    if (targetPosition < 0) {
        targetPosition = 0;
    }
    else if (targetPosition > posData.getTotalSlices()) {
        targetPosition = posData.getTotalSlices();
    }

    posData.setCurrentSlicePosition(targetPosition);
    _currentPosition = targetPosition;

    // Calculate X position and move
    float xPos = posData.getXZeroPosition() + (targetPosition * posData.getIncrement());
    MotionController::Instance().moveTo(AXIS_X, xPos, 1.0f);
}

void CutCycleManager::setState(CutCycleState newState) {
    if (_currentState != newState) {
        _currentState = newState;
        _lastStateChangeTime = ClearCore::TimingMgr.Milliseconds();

        // Debug output
        ClearCore::ConnectorUsb.Send("[CutCycle] State changed to: ");
        ClearCore::ConnectorUsb.SendLine(static_cast<int>(_currentState));
    }
}

void CutCycleManager::executeCurrentState() {
    switch (_currentState) {
    case CYCLE_FEED:
        executeFeedCycle();
        break;
    case CYCLE_CUTTING:
        executeCuttingCycle();
        break;
    case CYCLE_RETRACTING:
        executeRetractCycle();
        break;
    case CYCLE_POSITIONING:
        executePositioningCycle();
        break;
    default:
        // No action for other states
        break;
    }
}

void CutCycleManager::executeCuttingCycle() {
    auto& motion = MotionController::Instance();
    auto& posData = CutPositionData::Instance();

    if (!motion.isAxisMoving(AXIS_Y)) {
        float currentY = motion.getAxisPosition(AXIS_Y);
        float targetY = posData.getCutEndPosition();

        if (fabs(currentY - targetY) > 0.001f) {
            // Move Y to cut end position
            motion.moveToWithRate(AXIS_Y, targetY, _feedRate);
        }
        else {
            // We've reached the end position, begin retracting
            setState(CYCLE_RETRACTING);
        }
    }
}
void CutCycleManager::executeFeedCycle() {
    auto& motion = MotionController::Instance();
    auto& posData = CutPositionData::Instance();

    // Check if we need to move the Y axis 
    float currentY = motion.getAxisPosition(AXIS_Y);
    float targetY = posData.getCutEndPosition();

    // Log the current and target positions
    ClearCore::ConnectorUsb.Send("[CutCycle] Y position: current=");
    ClearCore::ConnectorUsb.Send(currentY);
    ClearCore::ConnectorUsb.Send(", target=");
    ClearCore::ConnectorUsb.SendLine(targetY);

    if (fabs(currentY - targetY) > 0.001f) {
        // Move Y to cut end position at feed rate
        ClearCore::ConnectorUsb.Send("[CutCycle] Moving Y to ");
        ClearCore::ConnectorUsb.Send(targetY);
        ClearCore::ConnectorUsb.Send(" at feed rate ");
        ClearCore::ConnectorUsb.SendLine(_feedRate);

        motion.moveToWithRate(AXIS_Y, targetY, _feedRate);

        // Transition to cutting state to monitor the move
        setState(CYCLE_CUTTING);
    }
    else {
        // We're already at the target position, skip to retracting
        setState(CYCLE_RETRACTING);
    }
}

void CutCycleManager::executeRetractCycle() {
    auto& motion = MotionController::Instance();
    auto& posData = CutPositionData::Instance();

    if (!motion.isAxisMoving(AXIS_Y)) {
        float currentY = motion.getAxisPosition(AXIS_Y);
        float retractPos = posData.getCutStartPosition() - posData.getRetractDistance();

        if (fabs(currentY - retractPos) > 0.001f) {
            // Move to retract position
            motion.moveTo(AXIS_Y, retractPos, 1.0f); // Use rapid speed for retract
        }
        else {
            // Retract complete
            if (_cycleType == CYCLE_FULLY_AUTO) {
                // If we're in auto mode and have more slices, continue
                int currentPos = posData.getCurrentSlicePosition();
                if (currentPos < posData.getTotalSlices()) {
                    // Move to next position
                    advanceToNextPosition();
                    setState(CYCLE_FEED);
                }
                else {
                    // All slices complete
                    setState(CYCLE_COMPLETE);
                }
            }
            else {
                // Semi-auto mode, stop after one cycle
                setState(CYCLE_COMPLETE);
            }
        }
    }
}

void CutCycleManager::executePositioningCycle() {
    auto& motion = MotionController::Instance();

    if (!motion.isAxisMoving(AXIS_X)) {
        // Ready for next action - either complete or cut
        if (_targetPosition == CutPositionData::Instance().getCurrentSlicePosition()) {
            // We're at the target, ready to cut
            setState(CYCLE_FEED);
        }
        else {
            // Something went wrong, exit cycle
            setState(CYCLE_IDLE);
        }
    }
}