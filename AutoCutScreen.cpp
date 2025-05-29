#include "AutoCutScreen.h"
#include "screenmanager.h" // Required for _mgr.GetCutData()

AutoCutScreen::AutoCutScreen(ScreenManager& mgr) : _mgr(mgr) {}

void AutoCutScreen::onShow() {
    // Example: auto& cutData = _mgr.GetCutData();
    // Initialize auto-cut state here
}

void AutoCutScreen::onHide() {
    // Cleanup or disable auto-cut state here
}

void AutoCutScreen::handleEvent(const genieFrame& e) {
    // Handle auto-cut events here
}

void AutoCutScreen::pauseCycle() {
    // Use auto& cutData = _mgr.GetCutData(); if you need shared job/cut data
}

void AutoCutScreen::resumeCycle() {
    // Use auto& cutData = _mgr.GetCutData(); if you need shared job/cut data
}

void AutoCutScreen::cancelCycle() {
    // Use auto& cutData = _mgr.GetCutData(); if you need shared job/cut data
}
