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
    // 1) Turn the pendant back on so the knob always works
    PendantManager::Instance().SetEnabled(true);

    // 2) Reset its last-known state so it won't immediately retrigger
    //    until the user actually moves the knob again.
    PendantManager::Instance().SetLastKnownSelector(
        PendantManager::Instance().ReadSelector()
    );

    // 3) Clean up any leftover field edits
    UIInputManager::Instance().unbindField();

    // 4) Sync the spindle toggle button visual state
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
