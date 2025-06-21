// SetupAutocutScreen.h - Form9 Setup Autocut Screen
#pragma once
#include "Screen.h"
#include "UIInputManager.h"

class ScreenManager;

class SetupAutocutScreen : public Screen {
public:
    SetupAutocutScreen(ScreenManager& mgr);

    void onShow() override;
    void onHide() override;
    void handleEvent(const genieFrame& e) override;
    void update() override;

    // Called when the encoder changes
    void onEncoderChanged(int deltaClicks);

    // Getter for checking if we're in editing mode
    bool isEditingSlices() const { return _editingSlices; }

private:
    void updateDisplay();
    void setSlicesToCut();
    void updateSlicesToCutButton();
    bool _needsDisplayUpdate = false;  // Flag for deferred display update

    ScreenManager& _mgr;
    float _tempSlices;  // Temporary value for editing slices
    bool _editingSlices;
    int _encoderDeltaAccum = 0;  // Accumulated encoder delta for tracking movement
};

// External reference to the global SetupAutocutScreen pointer
extern SetupAutocutScreen* g_setupAutocutScreen;
