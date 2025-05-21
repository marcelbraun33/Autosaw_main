#include <SD.h>
#include <ClearCore.h>
#include <genieArduinoDEV.h>
#include "Config.h"
#include "AutoSawController.h"
#include "SettingsManager.h"
#include "ScreenManager.h"
#include "UIInputManager.h"
#include "PendantManager.h"
#include "MotionController.h"
#include "MPGJogManager.h"


// Reference the single global Genie instance defined in the main sketch
Genie genie;

void myGenieEventHandler() {
    // Debug: handler entry
    ClearCore::ConnectorUsb.SendLine("[EV] myGenieEventHandler called");

    genieFrame evt;
    genie.DequeueEvent(&evt);

    if (evt.reportObject.cmd != GENIE_REPORT_EVENT) {
        ClearCore::ConnectorUsb.SendLine("[EV] Not GENIE_REPORT_EVENT");
        return;
    }
    ClearCore::ConnectorUsb.SendLine("[EV] REPORT_EVENT received");

    uint8_t object = evt.reportObject.object;
    uint8_t index = evt.reportObject.index;

    if (object == GENIE_OBJ_WINBUTTON) {
        ClearCore::ConnectorUsb.Send("[EV] WINBUTTON idx=");
        ClearCore::ConnectorUsb.Send(index);
        ClearCore::ConnectorUsb.SendLine();

        switch (index) {
            // ─── Jog Enable ──────────────────────────────────────────
        case WINBUTTON_ACTIVATE_JOG: {
            ClearCore::ConnectorUsb.SendLine("[EV] Activate Jog button");
            if (ScreenManager::Instance().currentForm() == FORM_JOG_X) {
                bool enabled = !MPGJogManager::Instance().isEnabled();
                MPGJogManager::Instance().setEnabled(enabled);
                MPGJogManager::Instance().setAxis(AXIS_X);

                // capture current range knobs
                int range = JOG_MULTIPLIER_X1;
                if (RANGE_PIN_X10.State())  range = JOG_MULTIPLIER_X10;
                if (RANGE_PIN_X100.State()) range = JOG_MULTIPLIER_X100;
                MPGJogManager::Instance().setRangeMultiplier(range);

                // ** reset the encoder baseline so no phantom jump **
                UIInputManager::Instance().resetRaw();

                // reflect new state on the UI button
                genie.WriteObject(
                    GENIE_OBJ_WINBUTTON,
                    WINBUTTON_ACTIVATE_JOG,
                    enabled ? 1 : 0
                );
            }
            return;
        }

                                   // ─── Pendant Toggle ─────────────────────────────────────
        case WINBUTTON_ACTIVATE_PENDANT:
            ClearCore::ConnectorUsb.SendLine("[EV] Pendant toggle button");
            PendantManager::Instance().SetEnabled(
                !PendantManager::Instance().IsEnabled()
            );
            ScreenManager::Instance().ShowManualMode();
            return;

            // ─── Settings Screen ────────────────────────────────────
        case WINBUTTON_SETTINGS_F5:
        case WINBUTTON_SETTINGS_SEMI:
        case WINBUTTON_SETTINGS_F7:
            ClearCore::ConnectorUsb.SendLine("[EV] Settings button");
            if (ScreenManager::Instance().currentForm() != FORM_SETTINGS) {
                genie.WriteObject(GENIE_OBJ_WINBUTTON, index, 0);
                ScreenManager::Instance().ShowSettings();
            }
            return;

            // ─── Spindle On/Off ─────────────────────────────────────
        case WINBUTTON_SPINDLE_TOGGLE_F7:
            ClearCore::ConnectorUsb.SendLine("[EV] Spindle toggle button");
            if (MotionController::Instance().IsSpindleRunning()) {
                MotionController::Instance().StopSpindle();
            }
            else {
                auto& S = SettingsManager::Instance().settings();
                MotionController::Instance().StartSpindle(S.defaultRPM);
            }
            return;

            // ─── Homing ──────────────────────────────────────────────
        case WINBUTTON_ACTIVATE_HOMING:
            ClearCore::ConnectorUsb.SendLine("[EV] Home Axes button pressed");
            // toggle enable on Y then Z to trigger MSP homing
            MOTOR_TABLE_Y.EnableRequest(false);
            MOTOR_TABLE_Y.EnableRequest(true);
            MOTOR_ROTARY_Z.EnableRequest(false);
            MOTOR_ROTARY_Z.EnableRequest(true);
            ScreenManager::Instance().ShowHoming();
            return;

            // ─── Fallback ───────────────────────────────────────────
        default:
            ClearCore::ConnectorUsb.Send("[EV] Other WINBUTTON idx=");
            ClearCore::ConnectorUsb.Send(index);
            ClearCore::ConnectorUsb.SendLine();
            break;
        }
    }

    // Dispatch anything else to the current screen
    if (ScreenManager::Instance().currentScreen()) {
        ScreenManager::Instance().currentScreen()->handleEvent(evt);
    }
}

void setup() {
    AutoSawController::Instance().setup();
}

void loop() {
    AutoSawController::Instance().update();
}

