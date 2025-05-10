
// ManualModeScreen.cpp
#include "ManualModeScreen.h"
#include "UIInputManager.h"
#include "SettingsManager.h"
#include "PendantManager.h"
#include "MotionController.h"
#include <ClearCore.h>
#include "Config.h"

extern Genie genie;

void ManualModeScreen::onShow() {
    PendantManager::Instance().SetEnabled(true);
    UIInputManager::Instance().unbindField();

    // Sync the spindle toggle button visual state
    bool spindleActive = MotionController::Instance().IsSpindleRunning();
    genie.WriteObject(GENIE_OBJ_WINBUTTON, WINBUTTON_SPINDLE_TOGGLE_F7, spindleActive ? 1 : 0);

    // Reset Set RPM button visual state
    genie.WriteObject(GENIE_OBJ_WINBUTTON, WINBUTTON_SET_RPM_F7, 0);
    delay(50);
    genie.WriteObject(GENIE_OBJ_WINBUTTON, WINBUTTON_SET_RPM_F7, 0);

    update();
}

void ManualModeScreen::onHide() {
    UIInputManager::Instance().unbindField();
    PendantManager::Instance().SetEnabled(true);
    genie.WriteObject(GENIE_OBJ_WINBUTTON, WINBUTTON_SET_RPM_F7, 0);
}

void ManualModeScreen::handleEvent(const genieFrame&) {
    // No editable fields — ignore inputs
}

void ManualModeScreen::update() {
    auto& mc = MotionController::Instance();
    uint16_t displayVal = mc.IsSpindleRunning() ? (uint16_t)mc.CommandedRPM() : 0;
    genie.WriteObject(GENIE_OBJ_LED_DIGITS, LEDDIGITS_MANUAL_RPM, displayVal);
}
