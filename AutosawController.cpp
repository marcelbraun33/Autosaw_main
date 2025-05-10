#include "AutoSawController.h"
#include "EStopManager.h"
#include "MotionController.h"
#include "PendantManager.h"
#include "UIInputManager.h"
#include "ScreenManager.h"
#include "SettingsManager.h"
#include "FileManager.h"
#include "Config.h"
#include <genieArduinoDEV.h>
#include <ClearCore.h>

// Pull in shared Genie instance and event handler
extern Genie genie;
extern void myGenieEventHandler();

AutoSawController& AutoSawController::Instance() {
    static AutoSawController instance;
    return instance;
}

void AutoSawController::setup() {
    Serial.begin(USB_BAUD);
    while (!Serial);

    GENIE_SERIAL_PORT.begin(GENIE_BAUD);
    genie.Begin(GENIE_SERIAL_PORT);
    genie.AttachEventHandler(myGenieEventHandler);

    ClearCore::EncoderIn.Enable(true);
    ClearCore::EncoderIn.Position(0);

    EStopManager::Instance().setup();
    MotionController::Instance().setup();
    PendantManager::Instance().Init();
    UIInputManager::Instance().init(ENCODER_COUNTS_PER_CLICK);
    SettingsManager::Instance().load();
    FileManager::Instance(); // if constructor handles init
    ScreenManager::Instance().Init();
    ScreenManager::Instance().ShowManualMode();
}

void AutoSawController::update() {
    genie.DoEvents();
    EStopManager::Instance().update();
    PendantManager::Instance().Update();
    UIInputManager::Instance().update();
    if (ScreenManager::Instance().currentScreen()) {
        ScreenManager::Instance().currentScreen()->update();
    }

}
