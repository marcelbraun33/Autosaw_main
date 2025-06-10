// Include screenmanager.h AFTER the ManualModeScreen.h to avoid circular dependencies
#include "ManualModeScreen.h" 
#include "UIInputManager.h"
#include "SettingsManager.h"
#include "PendantManager.h"
#include "MotionController.h"
#include <ClearCore.h>
#include "Config.h"
#include "screenmanager.h"  // Include this AFTER ManualModeScreen.h

ManualModeScreen::ManualModeScreen(ScreenManager& mgr) : _mgr(mgr) {}

extern Genie genie;

void ManualModeScreen::onShow() {
    // Always enable the pendant - no toggle button anymore
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
    // We always keep pendant enabled now - no need to disable it

    // Just unbind UI fields
    UIInputManager::Instance().unbindField();
}

void ManualModeScreen::handleEvent(const genieFrame& e) {
    // Now we handle button events specific to the Manual Mode screen
    if (e.reportObject.cmd != GENIE_REPORT_EVENT) {
        return;
    }

    if (e.reportObject.object == GENIE_OBJ_WINBUTTON) {
        switch (e.reportObject.index) {
        case WINBUTTON_SPINDLE_TOGGLE_F7:
            toggleSpindle();
            break;

        case WINBUTTON_ACTIVATE_HOMING:
            activateHoming();
            break;

        case WINBUTTON_SETTINGS_F7:
            goToSettings();
            break;
        }
    }
}

void ManualModeScreen::update() {
    auto& mc = MotionController::Instance();
    uint16_t displayVal = mc.IsSpindleRunning() ? (uint16_t)mc.CommandedRPM() : 0;
    genie.WriteObject(GENIE_OBJ_LED_DIGITS, LEDDIGITS_MANUAL_RPM, displayVal);
}

void ManualModeScreen::toggleSpindle() {
    ClearCore::ConnectorUsb.SendLine("[ManualMode] Spindle toggle button");

    auto& mc = MotionController::Instance();
    if (mc.IsSpindleRunning()) {
        mc.StopSpindle();
        genie.WriteObject(GENIE_OBJ_WINBUTTON, WINBUTTON_SPINDLE_TOGGLE_F7, 0);
    }
    else {
        auto& settings = SettingsManager::Instance().settings();
        mc.StartSpindle(settings.spindleRPM);
        genie.WriteObject(GENIE_OBJ_WINBUTTON, WINBUTTON_SPINDLE_TOGGLE_F7, 1);
    }
}

void ManualModeScreen::activateHoming() {
    ClearCore::ConnectorUsb.SendLine("[ManualMode] Home Axes button pressed");

    // Toggle enable on Y then Z to trigger MSP homing
    MOTOR_TABLE_Y.EnableRequest(false);
    MOTOR_TABLE_Y.EnableRequest(true);
    MOTOR_ROTARY_Z.EnableRequest(false);
    MOTOR_ROTARY_Z.EnableRequest(true);

    // Show the homing screen
    _mgr.ShowHoming();
}

void ManualModeScreen::goToSettings() {
    ClearCore::ConnectorUsb.SendLine("[ManualMode] Settings button pressed");
    _mgr.ShowSettings();
}
