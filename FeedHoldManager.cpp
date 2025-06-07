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

        // First make sure motion has fully stopped - use correct ClearCore delay function
        Delay_ms(150);

        // Abort the torque-controlled feed
        motion.abortTorqueControlledFeed(AXIS_Y);

        // Small delay to ensure complete abort before starting new motion
        Delay_ms(100);

        // Use smoother return motion with reduced velocity scale (0.7f instead of 1.0f)
        // for gentler movement back to start position
        motion.moveTo(AXIS_Y, _feedStartPos, 0.7f);

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
