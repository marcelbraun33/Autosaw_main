// JogXScreen.cpp
#include "JogXScreen.h"
#include "MotionController.h"
#include "ScreenManager.h"
#include "UIInputManager.h"
#include "MPGJogManager.h"
#include "SettingsManager.h"
#include <cmath>
#include "Config.h"

// Define LED indicator constants if not already defined in Config.h
#define LED_NEGATIVE_INDICATOR 0   // Form1: LED indicator for negative value

// GenieArduino standard object types
#define GENIE_OBJ_DIPSW                 0
#define GENIE_OBJ_KNOB                  1
#define GENIE_OBJ_ROCKERSW              2
#define GENIE_OBJ_ROTARYSW              3
#define GENIE_OBJ_SLIDER                4
#define GENIE_OBJ_TRACKBAR              5
#define GENIE_OBJ_WINBUTTON             6
#define GENIE_OBJ_ANGULAR_METER         7
#define GENIE_OBJ_COOL_GAUGE            8
#define GENIE_OBJ_CUSTOM_DIGITS         9
#define GENIE_OBJ_FORM                  10
#define GENIE_OBJ_GAUGE                 11
#define GENIE_OBJ_IMAGE                 12
#define GENIE_OBJ_KEYBOARD              13
#define GENIE_OBJ_LED                   14
#define GENIE_OBJ_LED_DIGITS            15
#define GENIE_OBJ_METER                 16
#define GENIE_OBJ_STRINGS               17
#define GENIE_OBJ_THERMOMETER           18
#define GENIE_OBJ_USER_LED              19
#define GENIE_OBJ_VIDEO                 20
#define GENIE_OBJ_STATIC_TEXT           21
#define GENIE_OBJ_SOUND                 22
#define GENIE_OBJ_TIMER                 23

extern Genie genie;

// Storage for position values
struct {
    float positionZero = 0.0f;        // Home/zero position reference
    float stockLength = 0.0f;         // Total stock length
    float increment = 0.0f;           // Distance between cuts
    float thickness = 0.0f;           // Cut thickness (increment - blade)
    int totalSlices = 0;              // Number of total slices
    int currentSlice = 0;             // Current slice position
    bool useStockZero = false;        // Whether to reference position from stock zero
} jogXData;

void JogXScreen::onShow() {
    // Enable the MPG Jog Manager for X axis control
    MPGJogManager::Instance().setEnabled(true);
    MPGJogManager::Instance().setAxis(AXIS_X);

    // Reset all button states
    for (int i = 0; i <= WINBUTTON_SET_TOTAL_SLICES; i++) {
        showButtonSafe(i, 0);
    }

    // Initialize the negative indicator LED to off - try both LED types
    genie.WriteObject(GENIE_OBJ_LED, LED_NEGATIVE_INDICATOR, 0);
    genie.WriteObject(GENIE_OBJ_USER_LED, LED_NEGATIVE_INDICATOR, 0);

    // Update all displays with current values
    updatePositionDisplay();
    updateStockLengthDisplay();
    updateIncrementDisplay();
    updateThicknessDisplay();
    updateTotalSlicesDisplay();
    updateSliceCounterDisplay();
}

void JogXScreen::onHide() {
    // Disable MPG jogging when leaving screen
    MPGJogManager::Instance().setEnabled(false);

    // Ensure all buttons are reset
    for (int i = 0; i <= WINBUTTON_SET_TOTAL_SLICES; i++) {
        showButtonSafe(i, 0);
    }

    // Turn off the negative indicator LED - try both LED types
    genie.WriteObject(GENIE_OBJ_LED, LED_NEGATIVE_INDICATOR, 0);
    genie.WriteObject(GENIE_OBJ_USER_LED, LED_NEGATIVE_INDICATOR, 0);

    // Unbind any active fields
    UIInputManager::Instance().unbindField();
}

void JogXScreen::handleEvent(const genieFrame& e) {
    // Only handle winbutton events
    if (e.reportObject.cmd != GENIE_REPORT_EVENT || e.reportObject.object != GENIE_OBJ_WINBUTTON)
        return;

    auto& ui = UIInputManager::Instance();
    auto& mpg = MPGJogManager::Instance();

    // Handle button presses based on index
    switch (e.reportObject.index) {
    case WINBUTTON_CAPTURE_ZERO:
        captureZero();
        break;

    case WINBUTTON_CAPTURE_STOCK_LENGTH:
        captureStockLength();
        break;

    case WINBUTTON_CAPTURE_INCREMENT:
        captureIncrement();
        break;

    case WINBUTTON_DIVIDE_SET:
        calculateAndSetIncrement();
        break;

    case WINBUTTON_ACTIVATE_JOG:
        // Toggle jog mode
        if (mpg.isEnabled()) {
            mpg.setEnabled(false);
            showButtonSafe(WINBUTTON_ACTIVATE_JOG, 0);
        }
        else {
            mpg.setEnabled(true);
            mpg.setAxis(AXIS_X);
            showButtonSafe(WINBUTTON_ACTIVATE_JOG, 1);
        }
        break;

    case WINBUTTON_SET_STOCK_LENGTH:
        // Toggle editing of stock length
        if (ui.isEditing()) {
            if (ui.isFieldActive(WINBUTTON_SET_STOCK_LENGTH)) {
                ui.unbindField();
                showButtonSafe(WINBUTTON_SET_STOCK_LENGTH, 0);
                // Recalculate dependent values
                calculateTotalSlices();
                updateTotalSlicesDisplay();
                updateSliceCounterDisplay();
            }
        }
        else {
            // Disable jog if active
            if (mpg.isEnabled()) {
                mpg.setEnabled(false);
                showButtonSafe(WINBUTTON_ACTIVATE_JOG, 0);
                ClearCore::ConnectorUsb.SendLine("Disabling jog for stock length editing");
            }

            // Bind to field editor
            ui.bindField(WINBUTTON_SET_STOCK_LENGTH, LEDDIGITS_STOCK_LENGTH,
                &jogXData.stockLength, 0.0f, 100.0f, 0.125f, 3);
            showButtonSafe(WINBUTTON_SET_STOCK_LENGTH, 1);
        }
        break;

    case WINBUTTON_SET_CUT_THICKNESS:
        // Toggle editing of cut thickness
        if (ui.isEditing()) {
            if (ui.isFieldActive(WINBUTTON_SET_CUT_THICKNESS)) {
                ui.unbindField();
                showButtonSafe(WINBUTTON_SET_CUT_THICKNESS, 0);
                // Update increment based on thickness + blade
                float bladeThickness = SettingsManager::Instance().settings().bladeThickness;
                jogXData.increment = jogXData.thickness + bladeThickness;
                updateIncrementDisplay();
                calculateTotalSlices();
                updateTotalSlicesDisplay();
                updateSliceCounterDisplay();
            }
        }
        else {
            // Disable jog if active
            if (mpg.isEnabled()) {
                mpg.setEnabled(false);
                showButtonSafe(WINBUTTON_ACTIVATE_JOG, 0);
                ClearCore::ConnectorUsb.SendLine("Disabling jog for thickness editing");
            }

            // Bind to field editor
            ui.bindField(WINBUTTON_SET_CUT_THICKNESS, LEDDIGITS_CUT_THICKNESS,
                &jogXData.thickness, 0.0f, 10.0f, 0.001f, 3);
            showButtonSafe(WINBUTTON_SET_CUT_THICKNESS, 1);
        }
        break;

    case WINBUTTON_SET_TOTAL_SLICES:
        // Toggle editing of total slices
        if (ui.isEditing()) {
            if (ui.isFieldActive(WINBUTTON_SET_TOTAL_SLICES)) {
                ui.unbindField();
                showButtonSafe(WINBUTTON_SET_TOTAL_SLICES, 0);
                // Recalculate increment based on slices
                if (jogXData.totalSlices > 0) {
                    jogXData.increment = jogXData.stockLength / jogXData.totalSlices;
                    float bladeThickness = SettingsManager::Instance().settings().bladeThickness;
                    jogXData.thickness = jogXData.increment - bladeThickness;
                    updateIncrementDisplay();
                    updateThicknessDisplay();
                }
                updateSliceCounterDisplay();
            }
        }
        else {
            // Disable jog if active
            if (mpg.isEnabled()) {
                mpg.setEnabled(false);
                showButtonSafe(WINBUTTON_ACTIVATE_JOG, 0);
                ClearCore::ConnectorUsb.SendLine("Disabling jog for total slices editing");
            }

            // Bind to field editor
            ui.bindField(WINBUTTON_SET_TOTAL_SLICES, LEDDIGITS_TOTAL_SLICES,
                reinterpret_cast<float*>(&jogXData.totalSlices), 1.0f, 1000.0f, 1.0f, 0);
            showButtonSafe(WINBUTTON_SET_TOTAL_SLICES, 1);
        }
        break;
    }
}

void JogXScreen::updatePositionDisplay() {
    float currentPos = MotionController::Instance().getAxisPosition(AXIS_X);
    float displayPos = jogXData.useStockZero ? (currentPos - jogXData.positionZero) : currentPos;

    // Check if the position is negative
    bool isNegative = (displayPos < 0.0f);

    // Static variable to track LED state changes for debug purposes
    static bool lastNegativeState = false;

    // Update the negative indicator LED - try both types
    // GENIE_OBJ_LED (14) - standard LED in GenieArduino library
    genie.WriteObject(GENIE_OBJ_LED, LED_NEGATIVE_INDICATOR, isNegative ? 1 : 0);

    // Also try USER_LED type (19) - custom LED component
    genie.WriteObject(GENIE_OBJ_USER_LED, LED_NEGATIVE_INDICATOR, isNegative ? 1 : 0);

    // Only log when the state changes
    if (isNegative != lastNegativeState) {
        lastNegativeState = isNegative;

        ClearCore::ConnectorUsb.Send("Position: ");
        ClearCore::ConnectorUsb.Send(displayPos);
        ClearCore::ConnectorUsb.Send(" isNegative: ");
        ClearCore::ConnectorUsb.SendLine(isNegative ? "YES" : "NO");
    }

    // Scale to 3 decimal places (thousandths of an inch)
    int32_t scaledPos = static_cast<int32_t>(round(displayPos * 1000.0f));
    genie.WriteObject(GENIE_OBJ_LED_DIGITS, LEDDIGITS_SAW_POSITION, static_cast<uint16_t>(abs(scaledPos)));
}

void JogXScreen::captureZero() {
    // Set the current position as zero reference
    jogXData.positionZero = MotionController::Instance().getAxisPosition(AXIS_X);
    jogXData.useStockZero = true;

    // Log the zero position for debugging
    ClearCore::ConnectorUsb.Send("Zero position captured: ");
    ClearCore::ConnectorUsb.SendLine(jogXData.positionZero);

    // Visual feedback
    showButtonSafe(WINBUTTON_CAPTURE_ZERO, 1);
    delay(200);
    showButtonSafe(WINBUTTON_CAPTURE_ZERO, 0);

    // Update the display
    updatePositionDisplay();
}

void JogXScreen::captureStockLength() {
    // Get current position and check if it's negative
    float currentPos = MotionController::Instance().getAxisPosition(AXIS_X);
    float displayPos = jogXData.useStockZero ? (currentPos - jogXData.positionZero) : currentPos;

    // If the position is negative, don't capture and show an error
    if (displayPos < 0.0f) {
        // Log the error
        ClearCore::ConnectorUsb.SendLine("Error: Cannot capture stock length when position is negative");

        // Visual feedback - quick flash to indicate error
        showButtonSafe(WINBUTTON_CAPTURE_STOCK_LENGTH, 1);
        delay(50);  // Brief flash to indicate error
        showButtonSafe(WINBUTTON_CAPTURE_STOCK_LENGTH, 0);
        delay(50);
        showButtonSafe(WINBUTTON_CAPTURE_STOCK_LENGTH, 1);
        delay(50);
        showButtonSafe(WINBUTTON_CAPTURE_STOCK_LENGTH, 0);
        return;
    }

    // Proceed with capturing stock length
    jogXData.stockLength = fabsf(currentPos - jogXData.positionZero);

    // Visual feedback
    showButtonSafe(WINBUTTON_CAPTURE_STOCK_LENGTH, 1);
    delay(200);
    showButtonSafe(WINBUTTON_CAPTURE_STOCK_LENGTH, 0);

    // Update displays
    updateStockLengthDisplay();
    calculateTotalSlices();
    updateTotalSlicesDisplay();
    updateSliceCounterDisplay();
}


void JogXScreen::captureIncrement() {
    // Get current position and check if it's negative
    float currentPos = MotionController::Instance().getAxisPosition(AXIS_X);
    float displayPos = jogXData.useStockZero ? (currentPos - jogXData.positionZero) : currentPos;

    // If the position is negative, don't capture and show an error
    if (displayPos < 0.0f) {
        // Log the error
        ClearCore::ConnectorUsb.SendLine("Error: Cannot capture increment when position is negative");

        // Visual feedback - quick flash to indicate error
        showButtonSafe(WINBUTTON_CAPTURE_INCREMENT, 1);
        delay(50);  // Brief flash to indicate error
        showButtonSafe(WINBUTTON_CAPTURE_INCREMENT, 0);
        delay(50);
        showButtonSafe(WINBUTTON_CAPTURE_INCREMENT, 1);
        delay(50);
        showButtonSafe(WINBUTTON_CAPTURE_INCREMENT, 0);
        return;
    }

    // Proceed with capturing increment
    jogXData.increment = fabsf(currentPos - jogXData.positionZero);

    // Visual feedback
    showButtonSafe(WINBUTTON_CAPTURE_INCREMENT, 1);
    delay(200);
    showButtonSafe(WINBUTTON_CAPTURE_INCREMENT, 0);

    // Update thickness based on the increment
    float bladeThickness = SettingsManager::Instance().settings().bladeThickness;
    jogXData.thickness = jogXData.increment - bladeThickness;

    // Update displays
    updateIncrementDisplay();
    updateThicknessDisplay();
    calculateTotalSlices();
    updateTotalSlicesDisplay();
    updateSliceCounterDisplay();
}
void JogXScreen::calculateTotalSlices() {
    // Calculate total slices based on stock length and increment
    if (jogXData.increment > 0.0f) {
        jogXData.totalSlices = static_cast<int>(floorf(jogXData.stockLength / jogXData.increment));
    }
    else {
        jogXData.totalSlices = 0;
    }
}

void JogXScreen::calculateAndSetIncrement() {
    // Calculate increment by dividing stock length by totalSlices
    if (jogXData.totalSlices > 0) {
        jogXData.increment = jogXData.stockLength / jogXData.totalSlices;

        // Calculate thickness (increment minus blade thickness)
        float bladeThickness = SettingsManager::Instance().settings().bladeThickness;
        jogXData.thickness = jogXData.increment - bladeThickness;

        // Visual feedback
        showButtonSafe(WINBUTTON_DIVIDE_SET, 1);
        delay(200);
        showButtonSafe(WINBUTTON_DIVIDE_SET, 0);

        // Update displays
        updateIncrementDisplay();
        updateThicknessDisplay();
        updateSliceCounterDisplay();
    }
}

void JogXScreen::updateStockLengthDisplay() {
    // Scale to 3 decimal places
    int32_t scaledLength = static_cast<int32_t>(round(jogXData.stockLength * 1000.0f));
    genie.WriteObject(GENIE_OBJ_LED_DIGITS, LEDDIGITS_STOCK_LENGTH, static_cast<uint16_t>(scaledLength));
}

void JogXScreen::updateIncrementDisplay() {
    // Scale to 3 decimal places
    int32_t scaledIncrement = static_cast<int32_t>(round(jogXData.increment * 1000.0f));
    genie.WriteObject(GENIE_OBJ_LED_DIGITS, LEDDIGITS_INCREMENT, static_cast<uint16_t>(scaledIncrement));
}

void JogXScreen::updateThicknessDisplay() {
    // Scale to 3 decimal places
    int32_t scaledThickness = static_cast<int32_t>(round(jogXData.thickness * 1000.0f));
    genie.WriteObject(GENIE_OBJ_LED_DIGITS, LEDDIGITS_CUT_THICKNESS, static_cast<uint16_t>(scaledThickness));
}

void JogXScreen::updateTotalSlicesDisplay() {
    genie.WriteObject(GENIE_OBJ_LED_DIGITS, LEDDIGITS_TOTAL_SLICES, static_cast<uint16_t>(jogXData.totalSlices));
}

void JogXScreen::updateSliceCounterDisplay() {
    // Calculate current slice based on position
    if (jogXData.increment > 0.0f) {
        float currentPos = MotionController::Instance().getAxisPosition(AXIS_X);
        float relativePos = jogXData.useStockZero ? (currentPos - jogXData.positionZero) : currentPos;

        // Only count completed slices (floor division)
        jogXData.currentSlice = static_cast<int>(floorf(relativePos / jogXData.increment));

        // Constrain to valid range
        if (jogXData.currentSlice < 0) jogXData.currentSlice = 0;
        if (jogXData.currentSlice > jogXData.totalSlices) jogXData.currentSlice = jogXData.totalSlices;
    }
    else {
        jogXData.currentSlice = 0;
    }

    genie.WriteObject(GENIE_OBJ_LED_DIGITS, LEDDIGITS_SLICE_COUNTER, static_cast<uint16_t>(jogXData.currentSlice));
}

void JogXScreen::update() {
    // Update position continuously
    updatePositionDisplay();

    // Other updates as needed
    static uint32_t lastUpdateTime = 0;
    uint32_t currentTime = ClearCore::TimingMgr.Milliseconds();

    // Update other displays periodically (every 500ms)
    if (currentTime - lastUpdateTime > 500) {
        lastUpdateTime = currentTime;
        updateSliceCounterDisplay();
    }
}
