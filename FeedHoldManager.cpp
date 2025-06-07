#include "FeedHoldManager.h"
#include "MotionController.h"
#include "ClearCore.h"  // For ClearCore::ConnectorUsb
#include "Spindle.h"    // For controlling the spindle

FeedHoldManager::FeedHoldManager()
    : _paused(false), _feedStartPos(0.0f)
{
}

void FeedHoldManager::toggleFeedHold() {
    MotionController& motion = MotionController::Instance();
    if (!_paused) {
        // Note: We now expect the start position to be set externally before
        // feed begins, so we don't overwrite it here during pause

        // Log the stored start position for reference
        ClearCore::ConnectorUsb.Send("[FeedHoldManager] Feed paused. Stored start position: ");
        ClearCore::ConnectorUsb.SendLine(_feedStartPos);

        // Pause the feed (this calls the YAxis methods via MotionController)
        motion.pauseTorqueControlledFeed(AXIS_Y);
        _paused = true;
    }
    else {
        // Resume the feed without changing the stored start position
        motion.resumeTorqueControlledFeed(AXIS_Y);
        _paused = false;
        ClearCore::ConnectorUsb.SendLine("[FeedHoldManager] Feed resumed.");
    }
}

void FeedHoldManager::exitFeedHold() {
    if (_paused) {
        MotionController& motion = MotionController::Instance();

        // Log the operation with the stored position
        ClearCore::ConnectorUsb.Send("[FeedHoldManager] Exiting feed hold, returning to position: ");
        ClearCore::ConnectorUsb.SendLine(_feedStartPos);

        // Abort the torque-controlled feed
        motion.abortTorqueControlledFeed(AXIS_Y);

        // Return the Y-axis to the originally stored feed start position
        motion.moveTo(AXIS_Y, _feedStartPos, 1.0f);

        // Turn off the spindle if it is running
        if (motion.IsSpindleRunning()) {
            motion.StopSpindle();
            ClearCore::ConnectorUsb.SendLine("[FeedHoldManager] Spindle stopped.");
        }

        _paused = false;
    }
}

void FeedHoldManager::reset() {
    _paused = false;
    _feedStartPos = 0.0f;
}
