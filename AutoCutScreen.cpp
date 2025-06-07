#include "AutoCutScreen.h"
#include "screenmanager.h" // Required for _mgr.GetCutData()

AutoCutScreen::AutoCutScreen(ScreenManager& mgr) : _mgr(mgr) {}

void AutoCutScreen::onShow() {
    // Example: auto& cutData = _mgr.GetCutData();
    // Initialize auto-cut state here
    
    // Reset the feed hold manager when showing the screen
    _feedHoldManager.reset();
}

void AutoCutScreen::onHide() {
    // Cleanup or disable auto-cut state here
}

void AutoCutScreen::handleEvent(const genieFrame& e) {
    // Handle auto-cut events here
}

void AutoCutScreen::pauseCycle() {
    // Delegate to FeedHoldManager for pause/resume logic
    _feedHoldManager.toggleFeedHold();
}

void AutoCutScreen::resumeCycle() {
    // Call toggleFeedHold() again to resume
    _feedHoldManager.toggleFeedHold();
}

void AutoCutScreen::cancelCycle() {
    // Delegate to FeedHoldManager for abort/return logic
    _feedHoldManager.exitFeedHold();
}
