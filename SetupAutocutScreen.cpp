// SetupAutocutScreen.cpp - Implements batch setup UI for auto cutting
#include "SetupAutocutScreen.h"
#include "screenmanager.h"
#include "CutSequenceController.h"
#include "MotionController.h"
#include "UIInputManager.h"
#include "MPGJogManager.h"
#include <ClearCore.h>
#include "Config.h"

extern Genie genie;

// Define fixed MPG increment for fine control
#define MPG_FIXED_INCREMENT 0.50f  // Half slice per increment, easier to get to round numbers

SetupAutocutScreen::SetupAutocutScreen(ScreenManager& mgr) 
    : _mgr(mgr), _tempSlices(1), _editingSlices(false) {}

void SetupAutocutScreen::onShow() {
    // Get current batch size from controller
    auto& seq = CutSequenceController::Instance();
    int currentBatchSize = seq.getBatchSize();

    // If not set, default to 1
    if (currentBatchSize <= 0) {
        currentBatchSize = 1;
    }

    _tempSlices = static_cast<float>(currentBatchSize);

    // Initialize MPG mode but disabled until user activates it
    MPGJogManager::Instance().setEnabled(false);

    // Calculate remaining positions
    int remainingPos = seq.getRemainingPositions();

    // Update displays
    updateDisplay();

    ClearCore::ConnectorUsb.Send("[SetupAutocut] Opened with batch size ");
    ClearCore::ConnectorUsb.Send(currentBatchSize);
    ClearCore::ConnectorUsb.Send(", remaining: ");
    ClearCore::ConnectorUsb.SendLine(remainingPos);
}

void SetupAutocutScreen::onHide() {
    // Clean up any active editing
    if (_editingSlices) {
        UIInputManager::Instance().unbindField();
        _editingSlices = false;
    }

    // Make sure MPG is disabled when leaving
    MPGJogManager::Instance().setEnabled(false);
}

void SetupAutocutScreen::handleEvent(const genieFrame& e) {
    if (e.reportObject.cmd != GENIE_REPORT_EVENT) return;

    auto& seq = CutSequenceController::Instance();
    auto& ui = UIInputManager::Instance();
    auto& mpg = MPGJogManager::Instance();

    switch (e.reportObject.object) {
    case GENIE_OBJ_WINBUTTON:
        switch (e.reportObject.index) {
        case WINBUTTON_SLICES_TO_CUT_F9:
            setSlicesToCut();
            break;

        case WINBUTTON_SETTINGS_F9:
            ScreenManager::Instance().ShowSettings();
            break;

        case WINBUTTON_BACK:
            // Return to auto cut screen
            ScreenManager::Instance().ShowAutoCut();
            break;
        }
        break;
    }
}

void SetupAutocutScreen::setSlicesToCut() {
    auto& ui = UIInputManager::Instance();
    auto& mpg = MPGJogManager::Instance();

    _editingSlices = !_editingSlices;

    if (_editingSlices) {
        // Start editing with MPG
        mpg.setEnabled(true);

        ClearCore::ConnectorUsb.SendLine("[SetupAutocut] Starting MPG slice adjustment");

        // Show active mode indicator
        showButtonSafe(WINBUTTON_SLICES_TO_CUT_F9, 1);
    }
    else {
        // Stop editing with MPG
        mpg.setEnabled(false);

        // Apply the value
        int maxBatch = CutSequenceController::Instance().getMaxBatchSize();
        int intSlices = static_cast<int>(round(_tempSlices));

        if (intSlices < 1) intSlices = 1;
        if (intSlices > maxBatch) intSlices = maxBatch;

        _tempSlices = static_cast<float>(intSlices);

        // Apply value to controller
        CutSequenceController::Instance().setBatchSize(intSlices);

        ClearCore::ConnectorUsb.Send("[SetupAutocut] Set batch size to: ");
        ClearCore::ConnectorUsb.SendLine(intSlices);

        // Show inactive mode indicator
        showButtonSafe(WINBUTTON_SLICES_TO_CUT_F9, 0);
    }
}

void SetupAutocutScreen::updateSlicesToCutButton() {
    genie.WriteObject(GENIE_OBJ_LED_DIGITS, LEDDIGITS_SLICES_TO_CUT_F9, static_cast<uint16_t>(_tempSlices));
}

void SetupAutocutScreen::updateDisplay() {
    auto& seq = CutSequenceController::Instance();

    // Current batch size (slices to cut)
    genie.WriteObject(GENIE_OBJ_LED_DIGITS, LEDDIGITS_SLICES_TO_CUT_F9, static_cast<uint16_t>(_tempSlices));

    // Last completed position
    int lastCompleted = seq.getLastCompletedPosition();
    genie.WriteObject(GENIE_OBJ_LED_DIGITS, LEDDIGITS_START_POSITION_F9, static_cast<uint16_t>(lastCompleted));

    // Total slices
    int totalSlices = seq.getTotalCuts();
    genie.WriteObject(GENIE_OBJ_LED_DIGITS, LEDDIGITS_TOTAL_POSITIONS_F9, static_cast<uint16_t>(totalSlices));

    // Remaining positions
    int remainingPos = seq.getRemainingPositions();
    if (remainingPos < 0) remainingPos = 0;

    // Update thickness display from JogXScreen's global value
    float thickness = JogXScreen::GetCutThickness();
    genie.WriteObject(GENIE_OBJ_LED_DIGITS, LEDDIGITS_THICKNESS_F9, static_cast<uint16_t>(thickness * 1000));

    // Stock length display from CutData
    auto& cutData = ScreenManager::Instance().GetCutData();
    genie.WriteObject(GENIE_OBJ_LED_DIGITS, LEDDIGITS_STOCK_LENGTH_F9, static_cast<uint16_t>(cutData.stockLength * 1000));

    // Set batch size limits
    int maxBatch = seq.getMaxBatchSize();
    if (_tempSlices > maxBatch) {
        _tempSlices = maxBatch;
        genie.WriteObject(GENIE_OBJ_LED_DIGITS, LEDDIGITS_SLICES_TO_CUT_F9, static_cast<uint16_t>(_tempSlices));
    }
}

void SetupAutocutScreen::onEncoderChanged(int deltaClicks) {
    if (_editingSlices) {
        // Accumulate delta clicks - we need this because the encoder might generate
        // multiple clicks in one update cycle
        _encoderDeltaAccum += deltaClicks;
        
        // Only apply changes when they exceed a threshold value to avoid too-rapid changes
        if (abs(_encoderDeltaAccum) >= 1) {
            int delta = (_encoderDeltaAccum > 0) ? 1 : -1;
            _encoderDeltaAccum = 0; // Reset accumulator
            
            // Apply the delta using our fixed increment
            _tempSlices += delta * MPG_FIXED_INCREMENT;
            
            // Enforce limits
            int maxBatch = CutSequenceController::Instance().getMaxBatchSize();
            if (_tempSlices < 1) _tempSlices = 1;
            if (_tempSlices > maxBatch) _tempSlices = maxBatch;
            
            // Round to nearest integer since partial slices don't make sense
            _tempSlices = round(_tempSlices);
            
            // Update display
            genie.WriteObject(GENIE_OBJ_LED_DIGITS, LEDDIGITS_SLICES_TO_CUT_F9, 
                static_cast<uint16_t>(_tempSlices));
            
            ClearCore::ConnectorUsb.Send("[SetupAutocut] Encoder changed: ");
            ClearCore::ConnectorUsb.Send(delta);
            ClearCore::ConnectorUsb.Send(", new value: ");
            ClearCore::ConnectorUsb.SendLine(_tempSlices);
        }
    }
}

void SetupAutocutScreen::update() {
    // Only process encoder input when in editing mode
    if (_editingSlices) {
        auto& mpg = MPGJogManager::Instance();
        
        // Make sure button stays highlighted while in this mode
        static uint32_t lastUIRefresh = 0;
        uint32_t now = ClearCore::TimingMgr.Milliseconds();
        
        if (now - lastUIRefresh > 500) {
            showButtonSafe(WINBUTTON_SLICES_TO_CUT_F9, 1);
            lastUIRefresh = now;
        }
    }

    // Update all displays occasionally
    static uint32_t lastUpdate = 0;
    uint32_t now = ClearCore::TimingMgr.Milliseconds();
    if (now - lastUpdate > 1000) {
        updateDisplay();
        lastUpdate = now;
    }
}
