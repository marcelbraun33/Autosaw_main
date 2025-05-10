// SettingsScreen.cpp
#include "SettingsScreen.h"
#include "ScreenManager.h"
#include "SettingsManager.h"
#include "UIInputManager.h"
#include <cmath>
#include "PendantManager.h"

extern Genie genie;

void SettingsScreen::onShow() {
    genie.WriteObject(GENIE_OBJ_WINBUTTON, WINBUTTON_BACK, 0);
    delay(50);
    genie.WriteObject(GENIE_OBJ_WINBUTTON, WINBUTTON_BACK, 0);

    auto& S = SettingsManager::Instance().settings();

    genie.WriteObject(GENIE_OBJ_LED_DIGITS, LEDDIGITS_DIAMETER_SETTINGS, (uint16_t)round(S.bladeDiameter * 10.0f));
    genie.WriteObject(GENIE_OBJ_LED_DIGITS, LEDDIGITS_THICKNESS_SETTINGS, (uint16_t)round(S.bladeThickness * 1000.0f));
    genie.WriteObject(GENIE_OBJ_LED_DIGITS, LEDDIGITS_RPM_SETTINGS, (uint16_t)S.defaultRPM);
    genie.WriteObject(GENIE_OBJ_LED_DIGITS, LEDDIGITS_FEEDRATE_SETTINGS, (uint16_t)round(S.feedRate * 10.0f));
    genie.WriteObject(GENIE_OBJ_LED_DIGITS, LEDDIGITS_RAPID_SETTINGS, (uint16_t)round(S.rapidRate * 10.0f));

    Serial.println("SettingsScreen: onShow() executed");
}

void SettingsScreen::handleEvent(const genieFrame& e) {
   // Serial.print("SettingsScreen::handleEvent - object: ");
   // Serial.print(e.reportObject.object);
   // Serial.print(", index: ");
   // Serial.println(e.reportObject.index);

    if (e.reportObject.cmd != GENIE_REPORT_EVENT || e.reportObject.object != GENIE_OBJ_WINBUTTON)
        return;

    auto& ui = UIInputManager::Instance();
    auto& settings = SettingsManager::Instance().settings();

    switch (e.reportObject.index) {
    case WINBUTTON_SET_DIAMETER_SETTINGS:
        if (ui.isEditing()) {
            if (ui.isFieldActive(WINBUTTON_SET_DIAMETER_SETTINGS)) {
                ui.unbindField();
                genie.WriteObject(GENIE_OBJ_WINBUTTON, WINBUTTON_SET_DIAMETER_SETTINGS, 0);
                SettingsManager::Instance().save();
            }
            else {
                genie.WriteObject(GENIE_OBJ_WINBUTTON, WINBUTTON_SET_DIAMETER_SETTINGS, 0);
            }
        }
        else {
            ui.bindField(WINBUTTON_SET_DIAMETER_SETTINGS, LEDDIGITS_DIAMETER_SETTINGS,
                &settings.bladeDiameter, 0.1f, 10.0f, 0.1f, 1);
            genie.WriteObject(GENIE_OBJ_WINBUTTON, WINBUTTON_SET_DIAMETER_SETTINGS, 1);
        }
        break;

    case WINBUTTON_SET_THICKNESS_SETTINGS:
        if (ui.isEditing()) {
            if (ui.isFieldActive(WINBUTTON_SET_THICKNESS_SETTINGS)) {
                ui.unbindField();
                genie.WriteObject(GENIE_OBJ_WINBUTTON, WINBUTTON_SET_THICKNESS_SETTINGS, 0);
                SettingsManager::Instance().save();
            }
            else {
                genie.WriteObject(GENIE_OBJ_WINBUTTON, WINBUTTON_SET_THICKNESS_SETTINGS, 0);
            }
        }
        else {
            ui.bindField(WINBUTTON_SET_THICKNESS_SETTINGS, LEDDIGITS_THICKNESS_SETTINGS,
                &settings.bladeThickness, 0.001f, 0.5f, 0.001f, 3);
            genie.WriteObject(GENIE_OBJ_WINBUTTON, WINBUTTON_SET_THICKNESS_SETTINGS, 1);
        }
        break;

    case WINBUTTON_SET_RPM_SETTINGS:
        if (ui.isEditing()) {
            if (ui.isFieldActive(WINBUTTON_SET_RPM_SETTINGS)) {
                ui.unbindField();
                genie.WriteObject(GENIE_OBJ_WINBUTTON, WINBUTTON_SET_RPM_SETTINGS, 0);
                SettingsManager::Instance().save();
            }
            else {
                genie.WriteObject(GENIE_OBJ_WINBUTTON, WINBUTTON_SET_RPM_SETTINGS, 0);
            }
        }
        else {
            ui.bindField(WINBUTTON_SET_RPM_SETTINGS, LEDDIGITS_RPM_SETTINGS,
                &settings.defaultRPM, 100, 4000, 10, 0);
            genie.WriteObject(GENIE_OBJ_WINBUTTON, WINBUTTON_SET_RPM_SETTINGS, 1);
        }
        break;

    case WINBUTTON_SET_FEEDRATE_SETTINGS:
        if (ui.isEditing()) {
            if (ui.isFieldActive(WINBUTTON_SET_FEEDRATE_SETTINGS)) {
                ui.unbindField();
                genie.WriteObject(GENIE_OBJ_WINBUTTON, WINBUTTON_SET_FEEDRATE_SETTINGS, 0);
                SettingsManager::Instance().save();
            }
            else {
                genie.WriteObject(GENIE_OBJ_WINBUTTON, WINBUTTON_SET_FEEDRATE_SETTINGS, 0);
            }
        }
        else {
            ui.bindField(WINBUTTON_SET_FEEDRATE_SETTINGS, LEDDIGITS_FEEDRATE_SETTINGS,
                &settings.feedRate, 0.0f, 25.0f, 0.1f, 1);
            genie.WriteObject(GENIE_OBJ_WINBUTTON, WINBUTTON_SET_FEEDRATE_SETTINGS, 1);
        }
        break;

    case WINBUTTON_SET_RAPID_SETTINGS:
        if (ui.isEditing()) {
            if (ui.isFieldActive(WINBUTTON_SET_RAPID_SETTINGS)) {
                ui.unbindField();
                genie.WriteObject(GENIE_OBJ_WINBUTTON, WINBUTTON_SET_RAPID_SETTINGS, 0);
                SettingsManager::Instance().save();
            }
            else {
                genie.WriteObject(GENIE_OBJ_WINBUTTON, WINBUTTON_SET_RAPID_SETTINGS, 0);
            }
        }
        else {
            ui.bindField(WINBUTTON_SET_RAPID_SETTINGS, LEDDIGITS_RAPID_SETTINGS,
                &settings.rapidRate, 0.0f, 300.0f, 1.0f, 0);
            genie.WriteObject(GENIE_OBJ_WINBUTTON, WINBUTTON_SET_RAPID_SETTINGS, 1);
        }
        break;


    case WINBUTTON_BACK:
        Serial.println("SettingsScreen: BACK pressed");
        ui.unbindField();
        SettingsManager::Instance().save();
        showButtonSafe(WINBUTTON_BACK, 0, 0);

        int selector = PendantManager::Instance().LastKnownSelector();
        Serial.print("Selector value: ");
        Serial.println(selector, HEX);

        switch (selector) {
        case 0x01: ScreenManager::Instance().ShowJogX(); break;
        case 0x02: ScreenManager::Instance().ShowJogY(); break;
        case 0x04: ScreenManager::Instance().ShowJogZ(); break;
        case 0x08: ScreenManager::Instance().ShowSemiAuto(); break;
        case 0x10: ScreenManager::Instance().ShowAutoCut(); break;
        default:   ScreenManager::Instance().ShowManualMode(); break;
        }
        break;
    }
}

void SettingsScreen::onHide() {
    Serial.println("SettingsScreen: onHide()");
    showButtonSafe(WINBUTTON_BACK, 0, 0);
}
