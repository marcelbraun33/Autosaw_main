#pragma once
#include "Screen.h"
#include "Config.h"
#include "MotionController.h"
#include "MPGJogManager.h"

// Define LED ID for Offset Applied indication
#ifndef LED_AT_START_POSITION_Y
#define LED_AT_START_POSITION_Y 1  // LED indicator for table at start position
#endif

extern Genie genie;

class JogYScreen : public Screen {
public:
    void onShow() override;
    void onHide() override;
    void handleEvent(const genieFrame& e) override;
    void update() override;

private:
    // Input capture methods
    void captureCutStart();
    void captureCutEnd();
    void captureRetractDistance();  // Changed from captureRetractPosition

    // Navigation methods
    void jogToStartPosition();      // Updated name from zeroToStartPosition
    void jogToEndPosition();        // New method for "Jog To End" button
    void jogToRetractPosition();    // Updated name from moveToRetractPosition

    // MPG editing methods
    void setLengthWithMPG();        // For cut length editing
    void setRetractWithMPG();       // For retract distance editing 

    // Jog activation
    void toggleJogMode();           // For the "Enable Jog" button

    // UI update helpers
    void updateCutLengthDisplay();
    void updateAllDisplays();

    // Motion coordinate values (absolute machine coordinates)
    float _cutStartPoint = 0.0f;   // Absolute machine Y coordinate for start
    float _cutEndPoint = 0.0f;     // Absolute machine Y coordinate for end

    // UI-facing values (always positive)
    float _retractDistance = 0.5f; // Distance to retract, always positive
    float _cutLength = 0.0f;       // Cut length display value, always positive

    // MPG editing state
    bool _mpgSetLengthMode = false;  // True when editing cut length with MPG
    bool _mpgSetRetractMode = false; // True when editing retract with MPG

    // Temporary values during editing
    static float _tempLength;       // For editing cut length with MPG
    static float _tempRetract;      // For editing retract distance with MPG
};
