#include "SemiAutoScreen.h"
#include "screenmanager.h" // Required for _mgr.GetCutData()

SemiAutoScreen::SemiAutoScreen(ScreenManager& mgr) : _mgr(mgr) {}

void SemiAutoScreen::onShow() {
    // Example: auto& cutData = _mgr.GetCutData();
    // Initialize semi-auto state here
}

void SemiAutoScreen::onHide() {
    // Cleanup or disable semi-auto state here
}

void SemiAutoScreen::handleEvent(const genieFrame& e) {
    // Handle semi-auto events here
}

void SemiAutoScreen::startFeedToStop() {
    // Use auto& cutData = _mgr.GetCutData(); if you need shared job/cut data
}

void SemiAutoScreen::advanceIncrement() {
    // Use auto& cutData = _mgr.GetCutData(); if you need shared job/cut data
}

void SemiAutoScreen::feedHold() {
    // Use auto& cutData = _mgr.GetCutData(); if you need shared job/cut data
}

void SemiAutoScreen::exitFeedHold() {
    // Use auto& cutData = _mgr.GetCutData(); if you need shared job/cut data
}

