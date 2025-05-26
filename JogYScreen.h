#pragma once
#include "Screen.h"
#include "Config.h"
#include "MotionController.h"
#include "MPGJogManager.h"

extern Genie genie;

class JogYScreen : public Screen {
public:
    void onShow() override;
    void onHide() override;
    void handleEvent(const genieFrame& e) override;
    void update() override;

private:
    void captureCutStart();
    void captureCutEnd();
    void captureRetractPosition();  // New method for capturing retract position
    void zeroToStartPosition();     // New method for zero to start position button
    void setWithMPG();
    void updateCutLengthDisplay();
    void updateAllDisplays();       // Helper to update all LED displays

    float _cutStartPoint = 0.0f;
    float _cutEndPoint = 0.0f;
    float _retractPosition = 0.0f;  // New variable for retract position
    bool _mpgSetMode = false;
    float _tempCutEndPoint = 0.0f;  // Temporary value while in "Set with MPG" mode
    static float _tempLength;       // For changing cut length directly in MPG mode
};
