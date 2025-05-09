#include "SettingsScreen.h"
#include "ScreenManager.h"
#include "SettingsManager.h"
#include "UIInputManager.h"
#include <cmath>
#include "PendantManager.h"

extern Genie genie;

void SettingsScreen::onShow() {
    // First reset the button so Genie doesn't show last-known state
    genie.WriteObject(GENIE_OBJ_WINBUTTON, WINBUTTON_BACK, 0);
    delay(50); // Let Genie finish drawing the form
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
    Serial.print("SettingsScreen::handleEvent - object: ");
    Serial.print(e.reportObject.object);
    Serial.print(", index: ");
    Serial.println(e.reportObject.index);

    if (e.reportObject.cmd != GENIE_REPORT_EVENT || e.reportObject.object != GENIE_OBJ_WINBUTTON)
        return;

    switch (e.reportObject.index) {
    case WINBUTTON_SET_DIAMETER_SETTINGS:
        UIInputManager::Instance().bindField(WINBUTTON_SET_DIAMETER_SETTINGS, LEDDIGITS_DIAMETER_SETTINGS,
            &SettingsManager::Instance().settings().bladeDiameter, 0.1f, 5.0f, 0.1f, 1);
        break;
    case WINBUTTON_SET_THICKNESS_SETTINGS:
        UIInputManager::Instance().bindField(WINBUTTON_SET_THICKNESS_SETTINGS, LEDDIGITS_THICKNESS_SETTINGS,
            &SettingsManager::Instance().settings().bladeThickness, 0.001f, 0.5f, 0.001f, 3);
        break;
    case WINBUTTON_SET_RPM_SETTINGS:
        Serial.println("Entering RPM edit mode from Settings screen");
        UIInputManager::Instance().bindField(WINBUTTON_SET_RPM_SETTINGS, LEDDIGITS_RPM_SETTINGS,
            &SettingsManager::Instance().settings().defaultRPM, 100, 10000, 100, 0);
        break;
    case WINBUTTON_SET_FEEDRATE_SETTINGS:
        UIInputManager::Instance().bindField(WINBUTTON_SET_FEEDRATE_SETTINGS, LEDDIGITS_FEEDRATE_SETTINGS,
            &SettingsManager::Instance().settings().feedRate, 1.0f, 50.0f, 0.1f, 1);
        break;
    case WINBUTTON_SET_RAPID_SETTINGS:
        UIInputManager::Instance().bindField(WINBUTTON_SET_RAPID_SETTINGS, LEDDIGITS_RAPID_SETTINGS,
            &SettingsManager::Instance().settings().rapidRate, 10.0f, 200.0f, 1.0f, 1);
        break;
    case WINBUTTON_BACK:
        Serial.println("SettingsScreen: BACK pressed");

        UIInputManager::Instance().unbindField();
        SettingsManager::Instance().save();

        // Reset button visually to allow re-press
        showButtonSafe(WINBUTTON_BACK, 0, 0);

        int selector = PendantManager::Instance().LastKnownSelector();
        Serial.print("Selector value: ");
        Serial.println(selector, HEX);

        Serial.println("Navigating from Settings back to selector-defined screen");
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
    showButtonSafe(WINBUTTON_BACK, 0, 0);  // Clear back button on exit
}
