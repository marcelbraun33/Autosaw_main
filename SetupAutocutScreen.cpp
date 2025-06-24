// SetupAutocutScreen.cpp - Enhanced with zero prevention fixes
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
#define MPG_FIXED_INCREMENT 1.0f  // One slice per increment

SetupAutocutScreen::SetupAutocutScreen(ScreenManager& mgr)
    : _mgr(mgr), _tempSlices(1), _editingSlices(false), _needsDisplayUpdate(false) {
    ClearCore::ConnectorUsb.SendLine("[SetupAutocut] Constructor called - initialized _tempSlices to 1");
}

void SetupAutocutScreen::onShow() {
    ClearCore::ConnectorUsb.SendLine("[SetupAutocut] onShow() - START");

    // Get current batch size from controller
    auto& seq = CutSequenceController::Instance();
    int currentBatchSize = seq.getBatchSize();

    ClearCore::ConnectorUsb.Send("[SetupAutocut] Current batch size from controller: ");
    ClearCore::ConnectorUsb.SendLine(currentBatchSize);

    // CRITICAL FIX: Never allow zero batch size - force minimum of 1
    if (currentBatchSize <= 0) {
        ClearCore::ConnectorUsb.SendLine("[SetupAutocut] FIXING: Zero batch size detected, forcing to 1");
        currentBatchSize = 1;
        seq.setBatchSize(1);  // Immediately fix it in the controller
    }

    // Set the display value, ensuring it's never zero
    _tempSlices = static_cast<float>(currentBatchSize);
    if (_tempSlices <= 0.0f) {
        ClearCore::ConnectorUsb.SendLine("[SetupAutocut] FIXING: Zero _tempSlices detected, forcing to 1");
        _tempSlices = 1.0f;
    }

    _editingSlices = false;  // Ensure we start in non-editing mode

    ClearCore::ConnectorUsb.SendLine("[SetupAutocut] Initializing MPG...");
    // Initialize MPG mode but disabled until user activates it
    MPGJogManager::Instance().setEnabled(false);

    ClearCore::ConnectorUsb.SendLine("[SetupAutocut] About to call performAutomaticSetup()...");

    // AUTOMATICALLY CONFIGURE CUT POSITIONS AND Y VALUES
    performAutomaticSetup();

    ClearCore::ConnectorUsb.SendLine("[SetupAutocut] performAutomaticSetup() completed");

    // Defer the expensive display updates to first update() call
    _needsDisplayUpdate = true;

    // Enhanced logging with final verification
    ClearCore::ConnectorUsb.Send("[SetupAutocut] onShow() completed with batch size ");
    ClearCore::ConnectorUsb.Send(currentBatchSize);
    ClearCore::ConnectorUsb.Send(", _tempSlices = ");
    ClearCore::ConnectorUsb.SendLine(static_cast<int>(_tempSlices));

    // VERIFICATION: Double-check that we never have zero
    if (seq.getBatchSize() <= 0 || _tempSlices <= 0.0f) {
        ClearCore::ConnectorUsb.SendLine("[SetupAutocut] CRITICAL ERROR: Still have zero values after fix!");
    }
    else {
        ClearCore::ConnectorUsb.SendLine("[SetupAutocut] VERIFIED: No zero values detected");
    }
}

void SetupAutocutScreen::performAutomaticSetup() {
    ClearCore::ConnectorUsb.SendLine("[SetupAutocut] performAutomaticSetup() - ENTRY");

    auto& cutData = _mgr.GetCutData();
    auto& cutSeq = CutSequenceController::Instance();

    // Debug: Print all cut data values
    ClearCore::ConnectorUsb.Send("[SetupAutocut] Cut data - increment: ");
    ClearCore::ConnectorUsb.SendLine(cutData.increment);
    ClearCore::ConnectorUsb.Send("[SetupAutocut] Cut data - totalSlices: ");
    ClearCore::ConnectorUsb.SendLine(cutData.totalSlices);
    ClearCore::ConnectorUsb.Send("[SetupAutocut] Cut data - cutStartPoint: ");
    ClearCore::ConnectorUsb.SendLine(cutData.cutStartPoint);
    ClearCore::ConnectorUsb.Send("[SetupAutocut] Cut data - cutEndPoint: ");
    ClearCore::ConnectorUsb.SendLine(cutData.cutEndPoint);
    ClearCore::ConnectorUsb.Send("[SetupAutocut] Cut data - retractDistance: ");
    ClearCore::ConnectorUsb.SendLine(cutData.retractDistance);

    // Validate setup parameters
    if (cutData.increment <= 0.0f) {
        ClearCore::ConnectorUsb.Send("[SetupAutocut] ERROR: Invalid increment (");
        ClearCore::ConnectorUsb.Send(cutData.increment);
        ClearCore::ConnectorUsb.SendLine(") - configure on Jog X screen");
        return;
    }

    if (cutData.totalSlices <= 0) {
        ClearCore::ConnectorUsb.Send("[SetupAutocut] ERROR: Invalid total slices (");
        ClearCore::ConnectorUsb.Send(cutData.totalSlices);
        ClearCore::ConnectorUsb.SendLine(") - configure on Jog X screen");
        return;
    }

    ClearCore::ConnectorUsb.SendLine("[SetupAutocut] Cut data validation passed");

    // Get Y positions from cut data
    float yStart = cutData.cutStartPoint;     // 1.81 - where cutting begins
    float yStop = cutData.cutEndPoint;        // 4.61 - where cutting ends (deeper)
    float yRetract = cutData.cutStartPoint - cutData.retractDistance;  // 1.52 - retract position

    ClearCore::ConnectorUsb.Send("[SetupAutocut] Y calculations - Raw Start: ");
    ClearCore::ConnectorUsb.Send(yStart);
    ClearCore::ConnectorUsb.Send(", Raw Stop: ");
    ClearCore::ConnectorUsb.Send(yStop);
    ClearCore::ConnectorUsb.Send(", Raw Retract calc: ");
    ClearCore::ConnectorUsb.SendLine(yRetract);

    // Ensure retract position doesn't go below zero
    if (yRetract < 0.0f) {
        ClearCore::ConnectorUsb.Send("[SetupAutocut] WARNING: Retract position ");
        ClearCore::ConnectorUsb.Send(yRetract);
        ClearCore::ConnectorUsb.SendLine(" adjusted to 0.0");
        yRetract = 0.0f;
    }

    ClearCore::ConnectorUsb.Send("[SetupAutocut] Final Y positions - Start: ");
    ClearCore::ConnectorUsb.Send(yStart);
    ClearCore::ConnectorUsb.Send(", Stop: ");
    ClearCore::ConnectorUsb.Send(yStop);
    ClearCore::ConnectorUsb.Send(", Retract: ");
    ClearCore::ConnectorUsb.SendLine(yRetract);

    // CORRECTED VALIDATION: For downward cutting, start should be less than stop
    if (yStart >= yStop) {
        ClearCore::ConnectorUsb.Send("[SetupAutocut] ERROR: Cut start (");
        ClearCore::ConnectorUsb.Send(yStart);
        ClearCore::ConnectorUsb.Send(") must be above (less than) cut stop (");
        ClearCore::ConnectorUsb.Send(yStop);
        ClearCore::ConnectorUsb.SendLine(") for downward cutting - configure on Jog Y screen");
        return;
    }

    if (yRetract > yStart) {
        ClearCore::ConnectorUsb.Send("[SetupAutocut] WARNING: Retract position (");
        ClearCore::ConnectorUsb.Send(yRetract);
        ClearCore::ConnectorUsb.Send(") should be above (less than) cut start (");
        ClearCore::ConnectorUsb.Send(yStart);
        ClearCore::ConnectorUsb.SendLine(")");
    }

    ClearCore::ConnectorUsb.SendLine("[SetupAutocut] Y position validation passed");

    // Get X positions
    ClearCore::ConnectorUsb.SendLine("[SetupAutocut] Getting X increment positions...");

    auto& jogXScreen = _mgr.GetJogXScreen();
    std::vector<float> xIncrements = jogXScreen.getIncrementPositions();

    ClearCore::ConnectorUsb.Send("[SetupAutocut] Retrieved ");
    ClearCore::ConnectorUsb.Send(static_cast<int>(xIncrements.size()));
    ClearCore::ConnectorUsb.SendLine(" X positions");

    if (xIncrements.empty()) {
        ClearCore::ConnectorUsb.SendLine("[SetupAutocut] ERROR: No X positions available from JogX screen");
        return;
    }

    // Log first few X positions for verification
    ClearCore::ConnectorUsb.SendLine("[SetupAutocut] First few X positions:");
    for (size_t i = 0; i < std::min(xIncrements.size(), size_t(3)); ++i) {
        ClearCore::ConnectorUsb.Send("  [");
        ClearCore::ConnectorUsb.Send(static_cast<int>(i));
        ClearCore::ConnectorUsb.Send("]: ");
        ClearCore::ConnectorUsb.SendLine(xIncrements[i]);
    }

    // Configure the CutSequenceController
    ClearCore::ConnectorUsb.SendLine("[SetupAutocut] Configuring CutSequenceController...");

    cutSeq.setXIncrements(xIncrements);
    cutSeq.setYCutStart(yStart);      // 1.81
    cutSeq.setYCutStop(yStop);        // 4.61  
    cutSeq.setYRetract(yRetract);     // 1.52

    // Verify the values were set correctly
    ClearCore::ConnectorUsb.SendLine("[SetupAutocut] Verifying controller values...");
    ClearCore::ConnectorUsb.Send("[SetupAutocut] Controller Y Start: ");
    ClearCore::ConnectorUsb.SendLine(cutSeq.getYCutStart());
    ClearCore::ConnectorUsb.Send("[SetupAutocut] Controller Y Stop: ");
    ClearCore::ConnectorUsb.SendLine(cutSeq.getYCutStop());
    ClearCore::ConnectorUsb.Send("[SetupAutocut] Controller Y Retract: ");
    ClearCore::ConnectorUsb.SendLine(cutSeq.getYRetract());
    ClearCore::ConnectorUsb.Send("[SetupAutocut] Controller Total Cuts: ");
    ClearCore::ConnectorUsb.SendLine(cutSeq.getTotalCuts());

    ClearCore::ConnectorUsb.Send("[SetupAutocut] SUCCESS: Automatic setup complete with ");
    ClearCore::ConnectorUsb.Send(static_cast<int>(xIncrements.size()));
    ClearCore::ConnectorUsb.SendLine(" X positions configured");

    ClearCore::ConnectorUsb.SendLine("[SetupAutocut] performAutomaticSetup() - EXIT SUCCESS");
}

// Add this new method to verify controller state
void SetupAutocutScreen::debugControllerState() {
    ClearCore::ConnectorUsb.SendLine("[SetupAutocut] === CONTROLLER STATE DEBUG ===");

    auto& cutSeq = CutSequenceController::Instance();

    ClearCore::ConnectorUsb.Send("Total cuts: ");
    ClearCore::ConnectorUsb.SendLine(cutSeq.getTotalCuts());

    ClearCore::ConnectorUsb.Send("Y Cut Start: ");
    ClearCore::ConnectorUsb.SendLine(cutSeq.getYCutStart());

    ClearCore::ConnectorUsb.Send("Y Cut Stop: ");
    ClearCore::ConnectorUsb.SendLine(cutSeq.getYCutStop());

    ClearCore::ConnectorUsb.Send("Y Retract: ");
    ClearCore::ConnectorUsb.SendLine(cutSeq.getYRetract());

    ClearCore::ConnectorUsb.Send("Current Index: ");
    ClearCore::ConnectorUsb.SendLine(cutSeq.getCurrentIndex());

    ClearCore::ConnectorUsb.Send("Last Completed Position: ");
    ClearCore::ConnectorUsb.SendLine(cutSeq.getLastCompletedPosition());

    ClearCore::ConnectorUsb.Send("Remaining Positions: ");
    ClearCore::ConnectorUsb.SendLine(cutSeq.getRemainingPositions());

    ClearCore::ConnectorUsb.Send("Max Batch Size: ");
    ClearCore::ConnectorUsb.SendLine(cutSeq.getMaxBatchSize());

    ClearCore::ConnectorUsb.Send("Current Batch Size: ");
    ClearCore::ConnectorUsb.SendLine(cutSeq.getBatchSize());

    ClearCore::ConnectorUsb.Send("Is Active: ");
    ClearCore::ConnectorUsb.SendLine(cutSeq.isActive() ? "YES" : "NO");

    ClearCore::ConnectorUsb.SendLine("[SetupAutocut] === END CONTROLLER DEBUG ===");
}

void SetupAutocutScreen::onHide() {
    ClearCore::ConnectorUsb.SendLine("[SetupAutocut] onHide() called");

    // Clean up any active editing
    if (_editingSlices) {
        UIInputManager::Instance().unbindField();
        _editingSlices = false;
    }

    // Make sure MPG is disabled when leaving
    MPGJogManager::Instance().setEnabled(false);

    ClearCore::ConnectorUsb.SendLine("[SetupAutocut] onHide() completed");
}

void SetupAutocutScreen::handleEvent(const genieFrame& e) {
    if (e.reportObject.cmd != GENIE_REPORT_EVENT) return;

    auto& seq = CutSequenceController::Instance();
    auto& ui = UIInputManager::Instance();
    auto& mpg = MPGJogManager::Instance();

    switch (e.reportObject.object) {
    case GENIE_OBJ_WINBUTTON:
        switch (e.reportObject.index) {
        case WINBUTTON_SLICES_TO_CUT_F9:            // 53
            setSlicesToCut();
            break;

        case WINBUTTON_SETTINGS_F9:                 // 52
            ScreenManager::Instance().ShowSettings();
            break;

        case WINBUTTON_RETURN_TO_AUTOCUT_F9:        // 54 - Return To Autocut
            // Debug controller state before returning
            debugControllerState();
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

        // Apply the value with enhanced safety
        int maxBatch = CutSequenceController::Instance().getMaxBatchSize();
        int intSlices = static_cast<int>(round(_tempSlices));

        // ENHANCED SAFETY: Triple-check bounds
        if (intSlices < 1) {
            ClearCore::ConnectorUsb.SendLine("[SetupAutocut] setSlicesToCut: Forcing minimum 1");
            intSlices = 1;
        }
        if (intSlices > maxBatch) {
            ClearCore::ConnectorUsb.Send("[SetupAutocut] setSlicesToCut: Capping at ");
            ClearCore::ConnectorUsb.SendLine(maxBatch);
            intSlices = maxBatch;
        }

        _tempSlices = static_cast<float>(intSlices);

        // Apply value to controller
        CutSequenceController::Instance().setBatchSize(intSlices);

        ClearCore::ConnectorUsb.Send("[SetupAutocut] Set batch size to: ");
        ClearCore::ConnectorUsb.SendLine(intSlices);

        // VERIFICATION: Make sure controller actually has the right value
        int verifyBatch = CutSequenceController::Instance().getBatchSize();
        if (verifyBatch != intSlices) {
            ClearCore::ConnectorUsb.Send("[SetupAutocut] WARNING: Controller batch size mismatch! Expected ");
            ClearCore::ConnectorUsb.Send(intSlices);
            ClearCore::ConnectorUsb.Send(", got ");
            ClearCore::ConnectorUsb.SendLine(verifyBatch);
        }

        // Show inactive mode indicator
        showButtonSafe(WINBUTTON_SLICES_TO_CUT_F9, 0);
    }
}

void SetupAutocutScreen::updateSlicesToCutButton() {
    genie.WriteObject(GENIE_OBJ_LED_DIGITS, LEDDIGITS_SLICES_TO_CUT_F9, static_cast<uint16_t>(_tempSlices));
}

void SetupAutocutScreen::updateDisplay() {
    auto& seq = CutSequenceController::Instance();

    // ENHANCED: Ensure _tempSlices is never zero before displaying
    if (_tempSlices <= 0.0f) {
        ClearCore::ConnectorUsb.SendLine("[SetupAutocut] updateDisplay: Fixing zero _tempSlices");
        _tempSlices = 1.0f;
    }

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

    // ENHANCED: Set batch size limits with zero prevention
    int maxBatch = seq.getMaxBatchSize();
    if (_tempSlices > maxBatch) {
        _tempSlices = static_cast<float>(maxBatch);
        genie.WriteObject(GENIE_OBJ_LED_DIGITS, LEDDIGITS_SLICES_TO_CUT_F9, static_cast<uint16_t>(_tempSlices));
    }
    if (_tempSlices < 1.0f) {
        ClearCore::ConnectorUsb.SendLine("[SetupAutocut] updateDisplay: Preventing _tempSlices below 1");
        _tempSlices = 1.0f;
        genie.WriteObject(GENIE_OBJ_LED_DIGITS, LEDDIGITS_SLICES_TO_CUT_F9, static_cast<uint16_t>(_tempSlices));
    }
}

void SetupAutocutScreen::onEncoderChanged(int deltaClicks) {
    if (_editingSlices) {
        ClearCore::ConnectorUsb.Send("[SetupAutocut] Encoder delta: ");
        ClearCore::ConnectorUsb.SendLine(deltaClicks);

        // Accumulate delta clicks
        _encoderDeltaAccum += deltaClicks;

        // Only apply changes when they exceed a threshold value
        if (abs(_encoderDeltaAccum) >= 1) {
            int delta = (_encoderDeltaAccum > 0) ? 1 : -1;
            _encoderDeltaAccum = 0; // Reset accumulator

            // Apply the delta using our fixed increment
            _tempSlices += delta * MPG_FIXED_INCREMENT;

            // ENHANCED BOUNDS CHECKING: Absolutely never allow zero or negative
            int maxBatch = CutSequenceController::Instance().getMaxBatchSize();
            if (_tempSlices < 1.0f) {
                ClearCore::ConnectorUsb.SendLine("[SetupAutocut] Encoder: Preventing below 1, setting to 1");
                _tempSlices = 1.0f;
            }
            if (_tempSlices > maxBatch) {
                ClearCore::ConnectorUsb.Send("[SetupAutocut] Encoder: Capping at max batch size ");
                ClearCore::ConnectorUsb.SendLine(maxBatch);
                _tempSlices = static_cast<float>(maxBatch);
            }

            // Round to nearest integer since partial slices don't make sense
            _tempSlices = round(_tempSlices);

            // SAFETY CHECK: Ensure rounding didn't create zero
            if (_tempSlices < 1.0f) {
                ClearCore::ConnectorUsb.SendLine("[SetupAutocut] Encoder: Post-round safety fix to 1");
                _tempSlices = 1.0f;
            }

            // Update display
            genie.WriteObject(GENIE_OBJ_LED_DIGITS, LEDDIGITS_SLICES_TO_CUT_F9,
                static_cast<uint16_t>(_tempSlices));

            ClearCore::ConnectorUsb.Send("[SetupAutocut] Encoder changed: ");
            ClearCore::ConnectorUsb.Send(delta);
            ClearCore::ConnectorUsb.Send(", new value: ");
            ClearCore::ConnectorUsb.SendLine(static_cast<int>(_tempSlices));
        }
    }
}

void SetupAutocutScreen::update() {
    // Handle deferred display update from onShow() - happens only once
    if (_needsDisplayUpdate) {
        ClearCore::ConnectorUsb.SendLine("[SetupAutocut] Performing deferred display update");
        updateDisplay();
        _needsDisplayUpdate = false;

        // Debug controller state after initial display update
        static bool debugPrinted = false;
        if (!debugPrinted) {
            debugControllerState();
            debugPrinted = true;
        }
    }

    // Handle MPG encoder input when in editing mode
    if (_editingSlices) {
        static int32_t lastEncoderPosition = 0;
        int32_t currentPosition = ClearCore::EncoderIn.Position();

        if (currentPosition != lastEncoderPosition) {
            int deltaClicks = (currentPosition - lastEncoderPosition) / ENCODER_COUNTS_PER_CLICK;
            if (deltaClicks != 0) {
                onEncoderChanged(deltaClicks);
                lastEncoderPosition = currentPosition;
            }
        }

        // Make sure button stays highlighted while in this mode
        static uint32_t lastUIRefresh = 0;
        uint32_t now = ClearCore::TimingMgr.Milliseconds();

        if (now - lastUIRefresh > 500) {
            showButtonSafe(WINBUTTON_SLICES_TO_CUT_F9, 1);
            lastUIRefresh = now;
        }
    }

    // Update all displays occasionally (less frequently to reduce overhead)
    static uint32_t lastUpdate = 0;
    uint32_t now = ClearCore::TimingMgr.Milliseconds();
    if (now - lastUpdate > 2000) {  // Every 2 seconds
        updateDisplay();
        lastUpdate = now;
    }
}