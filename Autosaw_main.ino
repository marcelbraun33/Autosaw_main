#include <ClearCore.h>
#include <genieArduinoDEV.h>
#include "Config.h"
#include "AutoSawController.h"
#include "SettingsManager.h"
#include "ScreenManager.h"
#include "UIInputManager.h"
#include "PendantManager.h"
#include "MotionController.h"

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
        case WINBUTTON_ACTIVATE_PENDANT:
            ClearCore::ConnectorUsb.SendLine("[EV] Pendant toggle button");
            PendantManager::Instance().SetEnabled(!PendantManager::Instance().IsEnabled());
            ScreenManager::Instance().ShowManualMode();
            return;

        case WINBUTTON_SETTINGS_F5:
        case WINBUTTON_SETTINGS_SEMI:
        case WINBUTTON_SETTINGS_F7:
            ClearCore::ConnectorUsb.SendLine("[EV] Settings button");
            if (ScreenManager::Instance().currentForm() != FORM_SETTINGS) {
                genie.WriteObject(GENIE_OBJ_WINBUTTON, index, 0);
                ScreenManager::Instance().ShowSettings();
            }
            return;

        case WINBUTTON_SPINDLE_TOGGLE_F7:
            ClearCore::ConnectorUsb.SendLine("[EV] Spindle toggle button");
            if (MotionController::Instance().IsSpindleRunning()) {
                MotionController::Instance().StopSpindle();
            }
            else {
                auto& settings = SettingsManager::Instance().settings();
                MotionController::Instance().StartSpindle(settings.defaultRPM);
            }
            return;

        case WINBUTTON_ACTIVATE_HOMING:
            ClearCore::ConnectorUsb.SendLine("[EV] Home Axes button pressed");
            // Trigger MSP auto-homing via enable toggle on X (M2) and Y (M3) connectors
            // X axis on M2
            MOTOR_TABLE_Y.EnableRequest(false);
            MOTOR_TABLE_Y.EnableRequest(true);
            // Y axis on M3
            MOTOR_ROTARY_Z.EnableRequest(false);
            MOTOR_ROTARY_Z.EnableRequest(true);
            ScreenManager::Instance().ShowHoming();
            return;

        default:
            ClearCore::ConnectorUsb.Send("[EV] Other WINBUTTON idx=");
            ClearCore::ConnectorUsb.Send(index);
            ClearCore::ConnectorUsb.SendLine();
            break;
        }
    }

    // Dispatch other events to current screen
    if (ScreenManager::Instance().currentScreen()) {
        ScreenManager::Instance().currentScreen()->handleEvent(evt);
    }
}


void setup() {
    AutoSawController::Instance().setup();
}

void loop() {
    AutoSawController::Instance().update();

    // Optional: debug the encoder position
    static int32_t lastPos = 0;
    int32_t pos = ClearCore::EncoderIn.Position();
    if (pos != lastPos) {
        lastPos = pos;
    }
}