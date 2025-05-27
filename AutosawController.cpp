#include <ClearCore.h>
#include <genieArduinoDEV.h>
#include "Config.h"
#include "AutosawController.h"
#include "EStopManager.h"
#include "PendantManager.h"
#include "UIInputManager.h"
#include "SettingsManager.h"
#include "FileManager.h"
#include "ScreenManager.h"
#include "MotionController.h"
#include "MPGJogManager.h"

extern Genie genie;                     // main sketch defines this
extern void myGenieEventHandler();      // forward-declare event handler

AutoSawController& AutoSawController::Instance() {
    static AutoSawController inst;
    return inst;
}

void AutoSawController::setup() {
    // Serial and Genie UI
    Serial.begin(USB_BAUD);
    while (!Serial);
    GENIE_SERIAL_PORT.begin(GENIE_BAUD);
    genie.Begin(GENIE_SERIAL_PORT);
    genie.AttachEventHandler(myGenieEventHandler);

    // Input and safety
    ClearCore::EncoderIn.Enable(true);
    EStopManager::Instance().setup();

    // Motion hardware
    MotionController::Instance().setup();

    // Pendant and UI input
    PendantManager::Instance().Init();
    UIInputManager::Instance().init(ENCODER_COUNTS_PER_CLICK);

    // Settings and filesystem
    SettingsManager::Instance().load();
    FileManager::Instance();

    // UI startup
    ScreenManager::Instance().Init();

    // Jog manager
    MPGJogManager::Instance().setup();
}

void AutoSawController::update() {
    // Process touch and button events
    genie.DoEvents();

    // E-stop and pendant
    EStopManager::Instance().update();
    PendantManager::Instance().Update();
    UIInputManager::Instance().update();

    // Drive updates
    MotionController::Instance().update();

    // UI screen logic
    if (ScreenManager::Instance().currentScreen()) {
        ScreenManager::Instance().currentScreen()->update();
    }
}
