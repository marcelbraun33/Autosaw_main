#pragma once

#include "Screen.h"
#include "Config.h"
#include "MotionController.h"
#include "MPGJogManager.h"
#include "CutData.h"
class ScreenManager;

// Define LED ID for Offset Applied indication
#ifndef LED_AT_START_POSITION_Y
#define LED_AT_START_POSITION_Y 1  // LED indicator for table at start position
#endif

extern Genie genie;

class JogYScreen : public Screen {
public:

    float getCutStart() const;
    float getCutStop() const;
    float getRetractPosition() const;

    void onShow() override;
    void onHide() override;
    void handleEvent(const genieFrame& e) override;
    void update() override;
    JogYScreen(ScreenManager& mgr);

private:
    // Input capture methods
    void captureCutStart();
    void captureCutEnd();
    void captureRetractDistance();

    // Navigation methods
    void jogToStartPosition();
    void jogToEndPosition();
    void jogToRetractPosition();

    // MPG editing methods
    void setLengthWithMPG();
    void setRetractWithMPG();

    // Jog activation
    void toggleJogMode();

    // UI update helpers
    void updateCutLengthDisplay();
    void updateAllDisplays();

    // MPG editing state
    bool _mpgSetLengthMode = false;
    bool _mpgSetRetractMode = false;

    // Temporary values during editing
    static float _tempLength;
    static float _tempRetract;
    ScreenManager& _mgr;
};
