// === AutoSawController.cpp ===
#include <ClearCore.h>
#include <genieArduinoDEV.h>
#include "Config.h"
#include "AutoSawController.h"
#include "EStopManager.h"
#include "PendantManager.h"
#include "UIInputManager.h"
#include "SettingsManager.h"
#include "FileManager.h"
#include "ScreenManager.h"
#include "MotionController.h"
#include "MPGJogManager.h"


extern Genie genie;  // main sketch defines this
extern void myGenieEventHandler();  // forward-declare the event handler from the sketch

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
    // --- AXIS TEST — drive +1 inch at 50% speed ---
    Serial.println("AXIS TEST: Move X +1\" at 50%");
    MotionController::Instance().moveToWithRate(AXIS_X, 1.0f, 0.5f);
    // wait for it...
    while (MotionController::Instance().isAxisMoving(AXIS_X)) {
        // let your Update() call in loop() tick the state machines
        delay(10);
    }
    Serial.println("AXIS TEST COMPLETE");


    // Pendant and UI input
    PendantManager::Instance().Init();
    UIInputManager::Instance().init(ENCODER_COUNTS_PER_CLICK);

    // Settings and filesystem
    SettingsManager::Instance().load();
    FileManager::Instance();

  
    ScreenManager::Instance().Init();
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
