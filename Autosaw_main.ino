#include <ClearCore.h>
#include <genieArduinoDEV.h>
#include "Config.h"
#include "AutoSawController.h"
#include "SettingsManager.h"
#include "ScreenManager.h"
#include "UIInputManager.h"
#include "PendantManager.h"

// === Globals ===
Genie genie;

void myGenieEventHandler() {
    genieFrame evt;
    genie.DequeueEvent(&evt);

    if (evt.reportObject.cmd != GENIE_REPORT_EVENT) return;

    uint8_t object = evt.reportObject.object;
    uint8_t index = evt.reportObject.index;
   
    if (object == GENIE_OBJ_WINBUTTON) {
        switch (index) {
        case WINBUTTON_ACTIVATE_PENDANT:
            PendantManager::Instance().SetEnabled(!PendantManager::Instance().IsEnabled());
            ScreenManager::Instance().ShowManualMode();
            return;

        case WINBUTTON_SET_RPM_F7: {
            static bool rpmSetActive = false;
            rpmSetActive = !rpmSetActive;
            if (rpmSetActive) {
                UIInputManager::Instance().bindField(
                    WINBUTTON_SET_RPM_F7,
                    LEDDIGITS_MANUAL_RPM,
                    &SettingsManager::Instance().settings().defaultRPM,
                    100, 10000, 10, 0
                );
            }
            else {
                UIInputManager::Instance().unbindField();
                SettingsManager::Instance().save();
            }
            return;
        }

                                 // ✅ New additions for gear/settings buttons


        case WINBUTTON_SETTINGS_F5:
        case WINBUTTON_SETTINGS_SEMI:
        case WINBUTTON_SETTINGS_F7:
            if (ScreenManager::Instance().currentForm() != FORM_SETTINGS) {
                Serial.println("myGenieEventHandler: Opening Settings screen.");
                genie.WriteObject(GENIE_OBJ_WINBUTTON, index, 0);  // ⬅️ RESET BUTTON
                ScreenManager::Instance().ShowSettings();
            }
            return;



        }
    }


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
        Serial.print("Encoder Pos: ");
        Serial.println(pos);
        lastPos = pos;
    }
}
