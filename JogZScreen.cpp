#include "JogZScreen.h"
#include "screenmanager.h" // Required for _mgr.GetCutData()

JogZScreen::JogZScreen(ScreenManager& mgr) : _mgr(mgr) {}

void JogZScreen::onShow() {
    // Example: auto& cutData = _mgr.GetCutData();
    // Initialize Z screen state here
}

void JogZScreen::onHide() {
    // Cleanup or disable Z screen state here
}

void JogZScreen::handleEvent(const genieFrame& e) {
    // Handle Z axis events here
}

void JogZScreen::setRPM(float value) {
    // Set spindle or tool RPM, using cutData if needed
}

void JogZScreen::toggleEnable() {
    // Enable/disable Z axis or spindle, using cutData if needed
}
