// Update to Autosaw_main/SettingsScreen.cpp

#include "SettingsScreen.h"
#include "ScreenManager.h"
#include "SettingsManager.h"
#include "UIInputManager.h"
#include "MPGJogManager.h"  // Added for MPG functionality
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
    genie.WriteObject(GENIE_OBJ_LED_DIGITS, LEDDIGITS_RPM_SETTINGS, (uint16_t)S.spindleRPM);
    genie.WriteObject(GENIE_OBJ_LED_DIGITS, LEDDIGITS_FEEDRATE_SETTINGS, (uint16_t)round(S.feedRate * 10.0f));
    genie.WriteObject(GENIE_OBJ_LED_DIGITS, LEDDIGITS_RAPID_SETTINGS, (uint16_t)round(S.rapidRate * 10.0f));

    // Add display for cut pressure setting
#ifdef SETTINGS_HAS_CUT_PRESSURE
    genie.WriteObject(GENIE_OBJ_LED_DIGITS, LEDDIGITS_CUT_PRESSURE_SETTINGS, (uint16_t)S.cutPressure);
#endif

    // Reset the MPG encoder mode if previously active
    if (_adjustingCutPressure) {
        _adjustingCutPressure = false;
        MPGJogManager::Instance().setEnabled(false);
    }

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

        // In SettingsScreen.cpp - in the WINBUTTON_SET_RPM_SETTINGS case
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
            // Bind directly to spindleRPM
            ui.bindField(WINBUTTON_SET_RPM_SETTINGS, LEDDIGITS_RPM_SETTINGS,
                &settings.spindleRPM, 100, 4000, 10, 0);
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

        // Add handler for cut pressure setting
    case WINBUTTON_SET_CUT_PRESSURE_F3:
#ifdef SETTINGS_HAS_CUT_PRESSURE
        // Toggle MPG adjustment mode
        _adjustingCutPressure = !_adjustingCutPressure;

        if (_adjustingCutPressure) {
            // Enter cut pressure adjustment mode with MPG
            ui.unbindField(); // Unbind any active field first

            // Highlight the button
            genie.WriteObject(GENIE_OBJ_WINBUTTON, WINBUTTON_SET_CUT_PRESSURE_F3, 1);

            // Setup MPG for adjustment
            _tempCutPressure = settings.cutPressure;
            _lastEncoderPos = ClearCore::EncoderIn.Position();

            // Enable MPG motion
            MPGJogManager::Instance().setEnabled(true);

            // Reset encoder to avoid phantom movements
            UIInputManager::Instance().resetRaw();

            // Display current value
            genie.WriteObject(GENIE_OBJ_LED_DIGITS, LEDDIGITS_CUT_PRESSURE_SETTINGS,
                static_cast<uint16_t>(_tempCutPressure));

            Serial.print("Adjusting cut pressure: ");
            Serial.println(_tempCutPressure);
        }
        else {
            // Exit cut pressure adjustment mode
            MPGJogManager::Instance().setEnabled(false);

            // Save the adjusted value
            settings.cutPressure = _tempCutPressure;
            SettingsManager::Instance().save();

            // Reset button state
            genie.WriteObject(GENIE_OBJ_WINBUTTON, WINBUTTON_SET_CUT_PRESSURE_F3, 0);

            Serial.print("Cut pressure set to: ");
            Serial.println(settings.cutPressure);
        }
#endif
        break;

    case WINBUTTON_BACK:
        Serial.println("SettingsScreen: BACK pressed");
        ui.unbindField();

        // Make sure to disable MPG and save settings if we were adjusting cut pressure
        if (_adjustingCutPressure) {
            _adjustingCutPressure = false;
            MPGJogManager::Instance().setEnabled(false);
            settings.cutPressure = _tempCutPressure;
        }

        SettingsManager::Instance().save();
        showButtonSafe(WINBUTTON_BACK, 0, 0);

        int selector = PendantManager::Instance().LastKnownSelector();
        Serial.print("Selector value: ");
        Serial.println(selector, HEX);

        switch (selector) {
        case 0x01:
            // Reserved for future use
            break;
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

    // Clean up MPG mode if active
    if (_adjustingCutPressure) {
        _adjustingCutPressure = false;
        MPGJogManager::Instance().setEnabled(false);

        // Save any pending changes
#ifdef SETTINGS_HAS_CUT_PRESSURE
        SettingsManager::Instance().settings().cutPressure = _tempCutPressure;
        SettingsManager::Instance().save();
#endif
    }

    showButtonSafe(WINBUTTON_BACK, 0, 0);
}

void SettingsScreen::update() {
    // Handle MPG encoder for cut pressure adjustment
    if (_adjustingCutPressure) {
        // Get current encoder position
        int32_t currentEncoderPos = ClearCore::EncoderIn.Position();

        // Calculate delta in encoder counts
        int32_t delta = currentEncoderPos - _lastEncoderPos;

        // Only process significant changes to avoid noise
        if (abs(delta) >= ENCODER_COUNTS_PER_CLICK) {
            _lastEncoderPos = currentEncoderPos;

            // Determine direction and change amount
            float changeAmount = (delta > 0) ? 1.0f : -1.0f;

            // Update the cut pressure value
            _tempCutPressure += changeAmount;

            // Clamp to valid range (min is now 1.0)
            if (_tempCutPressure < 1.0f) _tempCutPressure = 1.0f;
            if (_tempCutPressure > 100.0f) _tempCutPressure = 100.0f;

            // Debug output
            Serial.print("Cut pressure adjusted to: ");
            Serial.println(_tempCutPressure);

            // Update display
            genie.WriteObject(GENIE_OBJ_LED_DIGITS, LEDDIGITS_CUT_PRESSURE_SETTINGS,
                static_cast<uint16_t>(_tempCutPressure));
        }
    }
}
