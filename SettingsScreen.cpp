#include "SettingsScreen.h"
#include "ScreenManager.h"
#include "SettingsManager.h"
#include "UIInputManager.h"
#include "PendantManager.h"
#include <cmath>

extern Genie genie;

SettingsScreen::SettingsScreen(ScreenManager& mgr) : _mgr(mgr) {}

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
    if (e.reportObject.cmd != GENIE_REPORT_EVENT || e.reportObject.object != GENIE_OBJ_WINBUTTON)
        return;

    auto& ui = UIInputManager::Instance();
    auto& settings = SettingsManager::Instance().settings();
    // If you ever need job/cut data, use: auto& cutData = _mgr.GetCutData();

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

    case WINBUTTON_APPLY_OFFSET_PRESSURE: {
        auto& settings = SettingsManager::Instance().settings();
        auto& cutData = _mgr.GetCutData();

        // Update settings cut pressure value to current cutData cutPressure
        settings.cutPressure = cutData.cutPressure;

        // Clear the override flag
        cutData.cutPressureOverride = false;

        // Turn off the LED 4 indicator (for both screens)
        genie.WriteObject(GENIE_OBJ_LED, LED_CUT_PRESSURE_OFFSET_F2, 0);
        genie.WriteObject(GENIE_OBJ_LED, LED_CUT_PRESSURE_OFFSET_F5, 0);

        // Save the settings
        SettingsManager::Instance().save();

        // Visual feedback - flash the button
        genie.WriteObject(GENIE_OBJ_WINBUTTON, WINBUTTON_APPLY_OFFSET_PRESSURE, 1);
        delay(100);
        genie.WriteObject(GENIE_OBJ_WINBUTTON, WINBUTTON_APPLY_OFFSET_PRESSURE, 0);

        // Update the settings display to show the new value
        genie.WriteObject(GENIE_OBJ_LED_DIGITS, LEDDIGITS_CUT_PRESSURE_SETTINGS,
            static_cast<uint16_t>(settings.cutPressure * 100.0f));

        Serial.print("Applied cut pressure offset: ");
        Serial.println(settings.cutPressure);
    } break;
    }
}// Add this implementation to SettingsScreen.cpp

void SettingsScreen::onHide() {
    // Clean up any resources or state when the settings screen is hidden
    UIInputManager::Instance().unbindField();
    Serial.println("SettingsScreen: onHide() executed");
}
