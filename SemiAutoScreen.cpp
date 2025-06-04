#include "SemiAutoScreen.h"
#include "screenmanager.h" // Required for _mgr.GetCutData()
#include "MotionController.h"
#include "UIInputManager.h"
#include <ClearCore.h>

extern Genie genie;

SemiAutoScreen::SemiAutoScreen(ScreenManager& mgr) : _mgr(mgr) {}

void SemiAutoScreen::onShow() {
    // Just clean up fields
    UIInputManager::Instance().unbindField();

    // Only set spindle button state
    auto& mc = MotionController::Instance();
    bool spindleActive = mc.IsSpindleRunning();
    genie.WriteObject(GENIE_OBJ_WINBUTTON, WINBUTTON_SPINDLE_ON, spindleActive ? 1 : 0);
}

void SemiAutoScreen::onHide() {
    // Stop any ongoing operations
    UIInputManager::Instance().unbindField();

    // Clear all visual indicators
    genie.WriteObject(GENIE_OBJ_LED, LED_READY, 0);
}

void SemiAutoScreen::handleEvent(const genieFrame& e) {
    // Minimal handling to prevent crashes
    if (e.reportObject.cmd != GENIE_REPORT_EVENT) {
        return;
    }
}

void SemiAutoScreen::update() {
    // Basic updates for display values
    auto& mc = MotionController::Instance();

    // Update RPM display
    uint16_t rpm = mc.IsSpindleRunning() ? (uint16_t)mc.CommandedRPM() : 0;
    genie.WriteObject(GENIE_OBJ_LED_DIGITS, LEDDIGITS_RPM_DISPLAY, rpm);

    // You can add additional updates for other displays as needed
}
