
// ManualModeScreen.cpp
#include "ManualModeScreen.h"
#include "UIInputManager.h"
#include "SettingsManager.h"
#include "PendantManager.h"
#include <ClearCore.h>
#include "Config.h"

extern Genie genie;

void ManualModeScreen::onShow() {
    auto& S = SettingsManager::Instance().settings();
    UIInputManager::Instance().unbindField();
    PendantManager::Instance().SetEnabled(true);

    // Force the button to visually reset to OFF (twice for reliability)
    genie.WriteObject(GENIE_OBJ_WINBUTTON, WINBUTTON_SET_RPM_F7, 0);
    delay(50);
    genie.WriteObject(GENIE_OBJ_WINBUTTON, WINBUTTON_SET_RPM_F7, 0);

    genie.WriteObject(GENIE_OBJ_LED_DIGITS, LEDDIGITS_MANUAL_RPM, (uint16_t)S.defaultRPM);

    Serial.println("ManualModeScreen: onShow() - reset RPM edit state.");
}


void ManualModeScreen::onHide() {
    UIInputManager::Instance().unbindField();
    PendantManager::Instance().SetEnabled(true);
    genie.WriteObject(GENIE_OBJ_WINBUTTON, WINBUTTON_SET_RPM_F7, 0);
}

void ManualModeScreen::handleEvent(const genieFrame& e) {
    if (e.reportObject.cmd != GENIE_REPORT_EVENT || e.reportObject.object != GENIE_OBJ_WINBUTTON)
        return;

    switch (e.reportObject.index) {
    case WINBUTTON_SET_RPM_F7:
        handleRPMEditButton();
        break;
    default:
        break;
    }
}

void ManualModeScreen::handleRPMEditButton() {
    auto& ui = UIInputManager::Instance();
    auto& S = SettingsManager::Instance().settings();

    if (ui.isEditing()) {
        if (ui.isFieldActive(WINBUTTON_SET_RPM_F7)) {
            ui.unbindField();
            genie.WriteObject(GENIE_OBJ_WINBUTTON, WINBUTTON_SET_RPM_F7, 0);
            SettingsManager::Instance().save();
            PendantManager::Instance().SetEnabled(true);
        }
    }
    else {
        ui.bindField(WINBUTTON_SET_RPM_F7, LEDDIGITS_MANUAL_RPM, &S.defaultRPM, 100, 10000, 10, 0);
        genie.WriteObject(GENIE_OBJ_WINBUTTON, WINBUTTON_SET_RPM_F7, 1);
        PendantManager::Instance().SetEnabled(false);
    }
}
