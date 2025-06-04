#include "ManualModeScreen.h"
#include "UIInputManager.h"
#include "SettingsManager.h"
#include "PendantManager.h"
#include "MotionController.h"
#include <ClearCore.h>
#include "Config.h"

ManualModeScreen::ManualModeScreen(ScreenManager& mgr) : _mgr(mgr) {}

extern Genie genie;

void ManualModeScreen::onShow() {
    // Setup pendant
    PendantManager::Instance().SetEnabled(true);
    PendantManager::Instance().SetLastKnownSelector(
        PendantManager::Instance().ReadSelector()
    );

    // Clean up fields
    UIInputManager::Instance().unbindField();

    // Just set spindle button state
    bool spindleActive = MotionController::Instance().IsSpindleRunning();
    genie.WriteObject(
        GENIE_OBJ_WINBUTTON,
        WINBUTTON_SPINDLE_TOGGLE_F7,
        spindleActive ? 1 : 0
    );

    // Finally, run the first update
    update();
}

void ManualModeScreen::onHide() {
    // We no longer disable the pendant here — leave it on
    // (so turning the knob still works on Jog screens, etc.)

    // Just unbind UI fields
    UIInputManager::Instance().unbindField();
}

void ManualModeScreen::handleEvent(const genieFrame&) {
    // No editable fields — ignore
}

void ManualModeScreen::update() {
    auto& mc = MotionController::Instance();
    uint16_t displayVal = mc.IsSpindleRunning() ? (uint16_t)mc.CommandedRPM() : 0;
    genie.WriteObject(GENIE_OBJ_LED_DIGITS, LEDDIGITS_MANUAL_RPM, displayVal);
}
