#ifndef FEED_HOLD_MANAGER_H
#define FEED_HOLD_MANAGER_H

// FeedHoldManager centralizes feed hold (pause/resume/abort) logic for all screens.
// UI screens should delegate feed hold button events to this class.
class FeedHoldManager {
public:
    FeedHoldManager();

    // Toggle between pausing and resuming the feed.
    void toggleFeedHold();

    // Exit feed hold: abort feed and move Y axis to the original start position.
    void exitFeedHold();

    // Reset the manager state (e.g. when feed finishes).
    void reset();

    // Check if feed is currently paused
    bool isPaused() const { return _paused; }

    // Set the start position explicitly
    void setStartPosition(float position) { _feedStartPos = position; }

    // Get the stored start position
    float getStartPosition() const { return _feedStartPos; }

private:
    bool _paused;
    float _feedStartPos;
};

#endif // FEED_HOLD_MANAGER_H
