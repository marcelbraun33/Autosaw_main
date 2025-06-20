#include "JogXScreen.h"
#include "MotionController.h"
#include "ScreenManager.h"
#include "UIInputManager.h"
#include "MPGJogManager.h"
#include "SettingsManager.h"
#include "JogUtilities.h"
#include <cmath>
#include "Config.h"
#include <vector>
#include "CutSequenceController.h"

#ifndef LED_NEGATIVE_INDICATOR
#define LED_NEGATIVE_INDICATOR 0
#endif

extern Genie genie;

float JogXScreen::m_cutThickness = 0.0f; // Initialize static member

JogXScreen::JogXScreen(ScreenManager& mgr) : _mgr(mgr) {}

void JogXScreen::onShow() {
    // Setup MPG mode
    MPGJogManager::Instance().setEnabled(true);
    MPGJogManager::Instance().setAxis(AXIS_X);

    // Only set zero button state
    auto& cutData = _mgr.GetCutData();
    showButtonSafe(WINBUTTON_CAPTURE_ZERO, cutData.useStockZero ? 1 : 0);

    // Let update() handle the rest
}

void JogXScreen::onHide() {
    MPGJogManager::Instance().setEnabled(false);
    for (int i = 0; i <= WINBUTTON_SET_TOTAL_SLICES; ++i) {
        showButtonSafe(i, 0);
    }

    // Only use GENIE_OBJ_LED, not USER_LED
    genie.WriteObject(GENIE_OBJ_LED, LED_NEGATIVE_INDICATOR, 0);

    UIInputManager::Instance().unbindField();
}

void JogXScreen::updatePositionDisplay() {
    auto& cutData = _mgr.GetCutData();

    // Use absolute encoder position instead of motor position
    float current = MotionController::Instance().getAbsoluteAxisPosition(AXIS_X);

    float display = cutData.useStockZero ? (current - cutData.positionZero) : current;
    bool negative = (display < 0.0f);
    static bool lastNeg = false;

    // Only use GENIE_OBJ_LED, not USER_LED
    genie.WriteObject(GENIE_OBJ_LED, LED_NEGATIVE_INDICATOR, negative ? 1 : 0);

    if (negative != lastNeg) {
        lastNeg = negative;
        ClearCore::ConnectorUsb.Send("Position: ");
        ClearCore::ConnectorUsb.Send(display);
        ClearCore::ConnectorUsb.Send(" Absolute: ");
        ClearCore::ConnectorUsb.Send(current);
        ClearCore::ConnectorUsb.Send(" isNegative: ");
        ClearCore::ConnectorUsb.SendLine(negative ? "YES" : "NO");
    }
    int32_t scaled = static_cast<int32_t>(round(fabs(display) * 1000.0f));
    genie.WriteObject(GENIE_OBJ_LED_DIGITS, LEDDIGITS_SAW_POSITION,
        static_cast<uint16_t>(scaled));
}

void JogXScreen::setStockSlicesTimesIncrement() {
    auto& cutData = _mgr.GetCutData();

    // Skip if increment is zero or invalid
    if (cutData.increment <= 0.0f || cutData.totalSlices <= 0) {
        // Visual feedback for invalid operation
        showButtonSafe(WINBUTTON_SET_STOCK_SLICES_X_INC, 1);
        delay(50);
        showButtonSafe(WINBUTTON_SET_STOCK_SLICES_X_INC, 0);
        delay(50);
        showButtonSafe(WINBUTTON_SET_STOCK_SLICES_X_INC, 1);
        delay(50);
        showButtonSafe(WINBUTTON_SET_STOCK_SLICES_X_INC, 0);
        return;
    }

    // Calculate new stock length by multiplying increment by total slices
    float newStockLength = cutData.increment * cutData.totalSlices;

    ClearCore::ConnectorUsb.Send("Setting stock length to: ");
    ClearCore::ConnectorUsb.Send(cutData.increment);
    ClearCore::ConnectorUsb.Send(" x ");
    ClearCore::ConnectorUsb.Send(cutData.totalSlices);
    ClearCore::ConnectorUsb.Send(" = ");
    ClearCore::ConnectorUsb.SendLine(newStockLength);

    // Set the new stock length
    cutData.stockLength = newStockLength;

    // Visual feedback for successful operation
    showButtonSafe(WINBUTTON_SET_STOCK_SLICES_X_INC, 1);
    delay(200);
    showButtonSafe(WINBUTTON_SET_STOCK_SLICES_X_INC, 0);

    // Update displays
    updateStockLengthDisplay();
    updateTotalSlicesDisplay();
    updateSliceCounterDisplay();
}

void JogXScreen::handleEvent(const genieFrame& e) {
    if (e.reportObject.cmd != GENIE_REPORT_EVENT || e.reportObject.object != GENIE_OBJ_WINBUTTON)
        return;

    auto& cutData = _mgr.GetCutData();
    auto& ui = UIInputManager::Instance();
    auto& mpg = MPGJogManager::Instance();
    static float tempSlices = 10.0f;
    float newIncrement;

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

    case WINBUTTON_INC_PLUS: {
        // Replace old code with JogUtilities::Increment
        JogUtilities::Increment(cutData, AXIS_X);
        showButtonSafe(WINBUTTON_INC_PLUS, 1);
        delay(100);
        showButtonSafe(WINBUTTON_INC_PLUS, 0);
        break;
    }

    case WINBUTTON_INC_MINUS: {
        // Use absolute position from encoder tracker
        float cur = MotionController::Instance().getAbsoluteAxisPosition(AXIS_X);
        MotionController::Instance().moveTo(AXIS_X, cur - cutData.increment, 1.0f);
        showButtonSafe(WINBUTTON_INC_MINUS, 1);
        delay(100);
        showButtonSafe(WINBUTTON_INC_MINUS, 0);
        break;
    }

    case WINBUTTON_GO_TO_ZERO:
        goToZero();
        break;

    case WINBUTTON_DIVIDE_SET:
        if (cutData.totalSlices <= 0 || cutData.stockLength <= 0.0f) {
            for (int i = 0; i < 2; ++i) {
                showButtonSafe(WINBUTTON_DIVIDE_SET, 1);
                delay(50);
                showButtonSafe(WINBUTTON_DIVIDE_SET, 0);
                delay(50);
            }
            break;
        }
        newIncrement = cutData.stockLength / cutData.totalSlices;
        setIncrement(newIncrement);
        showButtonSafe(WINBUTTON_DIVIDE_SET, 1);
        delay(200);
        showButtonSafe(WINBUTTON_DIVIDE_SET, 0);
        break;

    case WINBUTTON_SET_STOCK_SLICES_X_INC:
        setStockSlicesTimesIncrement();
        break;

    case WINBUTTON_ACTIVATE_JOG:
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
        if (ui.isEditing() && ui.isFieldActive(WINBUTTON_SET_STOCK_LENGTH)) {
            ui.unbindField();
            showButtonSafe(WINBUTTON_SET_STOCK_LENGTH, 0);
            calculateTotalSlices();
            updateTotalSlicesDisplay();
            updateSliceCounterDisplay();
        }
        else if (!ui.isEditing()) {
            if (mpg.isEnabled()) {
                mpg.setEnabled(false);
                showButtonSafe(WINBUTTON_ACTIVATE_JOG, 0);
            }
            ui.bindField(WINBUTTON_SET_STOCK_LENGTH, LEDDIGITS_STOCK_LENGTH,
                &cutData.stockLength, 0.0f, 100.0f, 0.001f, 3);
            showButtonSafe(WINBUTTON_SET_STOCK_LENGTH, 1);
        }
        break;

    case WINBUTTON_SET_CUT_THICKNESS:
        if (ui.isEditing() && ui.isFieldActive(WINBUTTON_SET_CUT_THICKNESS)) {
            ui.unbindField();
            showButtonSafe(WINBUTTON_SET_CUT_THICKNESS, 0);
            if (cutData.thickness < 0.0f) cutData.thickness = 0.0f;
            float blade = SettingsManager::Instance().settings().bladeThickness;
            newIncrement = cutData.thickness + blade;
            setIncrement(newIncrement);
        }
        else if (!ui.isEditing()) {
            if (mpg.isEnabled()) {
                mpg.setEnabled(false);
                showButtonSafe(WINBUTTON_ACTIVATE_JOG, 0);
            }
            if (cutData.thickness < 0.0f) cutData.thickness = 0.0f;
            int32_t scaled = static_cast<int32_t>(round(cutData.thickness * 1000.0f));
            genie.WriteObject(GENIE_OBJ_LED_DIGITS, LEDDIGITS_CUT_THICKNESS, static_cast<uint16_t>(scaled));
            ui.bindField(WINBUTTON_SET_CUT_THICKNESS, LEDDIGITS_CUT_THICKNESS,
                &cutData.thickness, 0.0f, 10.0f, 0.001f, 3);
            showButtonSafe(WINBUTTON_SET_CUT_THICKNESS, 1);
        }
        break;

    case WINBUTTON_SET_TOTAL_SLICES:
        if (ui.isEditing() && ui.isFieldActive(WINBUTTON_SET_TOTAL_SLICES)) {
            cutData.totalSlices = static_cast<int>(roundf(tempSlices));
            ui.unbindField();
            showButtonSafe(WINBUTTON_SET_TOTAL_SLICES, 0);
            
            // RESET POSITION TRACKING when total slices changes
            CutSequenceController::Instance().reset();
            ClearCore::ConnectorUsb.SendLine("Position tracking reset due to total slices change");
        }
        else if (!ui.isEditing()) {
            if (mpg.isEnabled()) {
                mpg.setEnabled(false);
                showButtonSafe(WINBUTTON_ACTIVATE_JOG, 0);
            }
            tempSlices = (cutData.totalSlices > 0 ? cutData.totalSlices : 10);
            genie.WriteObject(GENIE_OBJ_LED_DIGITS, LEDDIGITS_TOTAL_SLICES,
                static_cast<uint16_t>(tempSlices));
            ui.bindField(WINBUTTON_SET_TOTAL_SLICES, LEDDIGITS_TOTAL_SLICES,
                &tempSlices, 1.0f, 1000.0f, 1.0f, 0);
            showButtonSafe(WINBUTTON_SET_TOTAL_SLICES, 1);
        }
        break;
    }
}

void JogXScreen::captureZero() {
    auto& cutData = _mgr.GetCutData();
    cutData.useStockZero = !cutData.useStockZero;

    if (cutData.useStockZero) {
        // Use absolute position from encoder tracker
        cutData.positionZero = MotionController::Instance().getAbsoluteAxisPosition(AXIS_X);
        ClearCore::ConnectorUsb.Send("Zero position captured (absolute): ");
        ClearCore::ConnectorUsb.SendLine(cutData.positionZero);
        showButtonSafe(WINBUTTON_CAPTURE_ZERO, 1);
        
        // RESET POSITION TRACKING when zero changes
        CutSequenceController::Instance().reset();
        ClearCore::ConnectorUsb.SendLine("Position tracking reset due to zero position change");
    }
    else {
        ClearCore::ConnectorUsb.SendLine("Zero offset removed, reverting to machine coordinates");
        showButtonSafe(WINBUTTON_CAPTURE_ZERO, 0);
    }
    updatePositionDisplay();
}

void JogXScreen::goToZero() {
    auto& cutData = _mgr.GetCutData();
    if (cutData.useStockZero) {
        MotionController::Instance().moveToWithRate(AXIS_X, cutData.positionZero, 0.5f);
        ClearCore::ConnectorUsb.SendLine("Moving to captured zero position");
    }
    else {
        MotionController::Instance().moveToWithRate(AXIS_X, 0.0f, 0.5f);
        ClearCore::ConnectorUsb.SendLine("Moving to absolute machine zero");
    }
    showButtonSafe(WINBUTTON_GO_TO_ZERO, 1);
    delay(200);
    showButtonSafe(WINBUTTON_GO_TO_ZERO, 0);
}

void JogXScreen::captureStockLength() {
    auto& cutData = _mgr.GetCutData();

    // Use absolute position from encoder tracker
    float current = MotionController::Instance().getAbsoluteAxisPosition(AXIS_X);

    float display = cutData.useStockZero ? (current - cutData.positionZero) : current;
    if (display < 0.0f) {
        ClearCore::ConnectorUsb.SendLine("Error: Cannot capture stock length when negative");
        showButtonSafe(WINBUTTON_CAPTURE_STOCK_LENGTH, 1);
        delay(50); showButtonSafe(WINBUTTON_CAPTURE_STOCK_LENGTH, 0);
        delay(50); showButtonSafe(WINBUTTON_CAPTURE_STOCK_LENGTH, 1);
        delay(50); showButtonSafe(WINBUTTON_CAPTURE_STOCK_LENGTH, 0);
        return;
    }
    cutData.stockLength = display;
    showButtonSafe(WINBUTTON_CAPTURE_STOCK_LENGTH, 1);
    delay(200); showButtonSafe(WINBUTTON_CAPTURE_STOCK_LENGTH, 0);
    
    // RESET POSITION TRACKING when stock length changes
    CutSequenceController::Instance().reset();
    ClearCore::ConnectorUsb.SendLine("Position tracking reset due to stock length change");
    
    updateStockLengthDisplay();
    calculateTotalSlices();
    updateTotalSlicesDisplay();
    updateSliceCounterDisplay();
}

void JogXScreen::captureIncrement() {
    auto& cutData = _mgr.GetCutData();

    // Use absolute position from encoder tracker
    float current = MotionController::Instance().getAbsoluteAxisPosition(AXIS_X);

    float display = cutData.useStockZero ? (current - cutData.positionZero) : current;
    if (display < 0.0f) {
        ClearCore::ConnectorUsb.SendLine("Error: Cannot capture increment when negative");
        showButtonSafe(WINBUTTON_CAPTURE_INCREMENT, 1);
        delay(50); showButtonSafe(WINBUTTON_CAPTURE_INCREMENT, 0);
        delay(50); showButtonSafe(WINBUTTON_CAPTURE_INCREMENT, 1);
        delay(50); showButtonSafe(WINBUTTON_CAPTURE_INCREMENT, 0);
        return;
    }
    showButtonSafe(WINBUTTON_CAPTURE_INCREMENT, 1);
    delay(200); showButtonSafe(WINBUTTON_CAPTURE_INCREMENT, 0);
    setIncrement(fabs(display));
}

void JogXScreen::calculateTotalSlices() {
    auto& cutData = _mgr.GetCutData();
    if (cutData.increment > 0.0f)
        cutData.totalSlices = static_cast<int>(floorf(cutData.stockLength / cutData.increment));
    else
        cutData.totalSlices = 0;
}

void JogXScreen::setIncrement(float newIncrement) {
    auto& cutData = _mgr.GetCutData();
    ClearCore::ConnectorUsb.SendLine("--------- SET INCREMENT ---------");
    ClearCore::ConnectorUsb.Send("Previous increment: ");
    ClearCore::ConnectorUsb.SendLine(cutData.increment);
    ClearCore::ConnectorUsb.Send("New increment (raw): ");
    ClearCore::ConnectorUsb.SendLine(newIncrement);

    if (newIncrement < 0.001f) {
        newIncrement = 0.001f;
        ClearCore::ConnectorUsb.SendLine("WARNING: Minimum increment enforced");
    }

    cutData.increment = newIncrement;
    float bladeThickness = SettingsManager::Instance().settings().bladeThickness;
    cutData.thickness = cutData.increment - bladeThickness;
    if (cutData.thickness < 0.0f) {
        cutData.thickness = 0.0f;
        ClearCore::ConnectorUsb.SendLine("WARNING: Cut thickness negative, set to 0");
    }
    ClearCore::ConnectorUsb.Send("Calculated thickness: ");
    ClearCore::ConnectorUsb.SendLine(cutData.thickness);

    calculateTotalSlices();
    ClearCore::ConnectorUsb.Send("Calculated total slices: ");
    ClearCore::ConnectorUsb.SendLine(cutData.totalSlices);

    // RESET POSITION TRACKING when increment changes
    CutSequenceController::Instance().reset();
    ClearCore::ConnectorUsb.SendLine("Position tracking reset due to increment change");

    updateIncrementDisplay();
    updateThicknessDisplay();
    updateTotalSlicesDisplay();
    updateSliceCounterDisplay();

    ClearCore::ConnectorUsb.SendLine("--------- SET INCREMENT COMPLETE ---------");
}

void JogXScreen::updateStockLengthDisplay() {
    auto& cutData = _mgr.GetCutData();
    int32_t scaled = static_cast<int32_t>(round(cutData.stockLength * 1000.0f));
    genie.WriteObject(GENIE_OBJ_LED_DIGITS, LEDDIGITS_STOCK_LENGTH, static_cast<uint16_t>(scaled));
}

void JogXScreen::updateIncrementDisplay() {
    auto& cutData = _mgr.GetCutData();
    if (cutData.increment < 0.001f) cutData.increment = 0.001f;
    int32_t scaled = static_cast<int32_t>(round(cutData.increment * 1000.0f));
    genie.WriteObject(GENIE_OBJ_LED_DIGITS, LEDDIGITS_INCREMENT, static_cast<uint16_t>(scaled));
}

// Updated to show thickness on all screens
void JogXScreen::updateThicknessDisplay() {
    auto& cutData = _mgr.GetCutData();
    if (cutData.thickness < 0.0f) cutData.thickness = 0.0f;
    else if (cutData.thickness > 10.0f) cutData.thickness = 10.0f;

    // Store in static member for access by other screens
    m_cutThickness = cutData.thickness;

    ClearCore::ConnectorUsb.Send("Updating thickness display to: ");
    ClearCore::ConnectorUsb.SendLine(cutData.thickness);

    // Calculate scaled value once
    int32_t scaled = static_cast<int32_t>(round(cutData.thickness * 1000.0f));
    uint16_t scaledValue = static_cast<uint16_t>(scaled);

    // Update all thickness displays across different forms
    genie.WriteObject(GENIE_OBJ_LED_DIGITS, LEDDIGITS_CUT_THICKNESS, scaledValue); // Form 1 (JogX)
    genie.WriteObject(GENIE_OBJ_LED_DIGITS, LEDDIGITS_THICKNESS_F2, scaledValue);  // Form 2 (SemiAuto)
    genie.WriteObject(GENIE_OBJ_LED_DIGITS, LEDDIGITS_THICKNESS_F5, scaledValue);  // Form 5 (AutoCut)
}

void JogXScreen::updateTotalSlicesDisplay() {
    auto& cutData = _mgr.GetCutData();
    genie.WriteObject(GENIE_OBJ_LED_DIGITS, LEDDIGITS_TOTAL_SLICES, static_cast<uint16_t>(cutData.totalSlices));

    // Ensure CutSequenceController is rebuilt with the correct number of increments
    float stockZero = cutData.useStockZero ? cutData.positionZero : 0.0f;
    float increment = cutData.increment;
    int totalSlices = cutData.totalSlices;
    // Only rebuild if increment and totalSlices are valid
    if (increment > 0.0f && totalSlices > 0) {
        CutSequenceController::Instance().buildXPositions(stockZero, increment, totalSlices);
    }
}

void JogXScreen::updateSliceCounterDisplay() {
    auto& cutData = _mgr.GetCutData();
    int available = 0;
    if (cutData.increment > 0.0f)
        available = static_cast<int>(floorf(cutData.stockLength / cutData.increment));
    genie.WriteObject(GENIE_OBJ_LED_DIGITS, LEDDIGITS_SLICE_COUNTER, static_cast<uint16_t>(available));
}

void JogXScreen::updateCutSequencePositions() {
    auto& cutData = _mgr.GetCutData();
    float stockZero = cutData.useStockZero ? cutData.positionZero : 0.0f;
    float increment = cutData.increment;
    int totalSlices = cutData.totalSlices;
    // Always rebuild with current values
    if (increment > 0.0f && totalSlices > 0) {
        CutSequenceController::Instance().buildXPositions(stockZero, increment, totalSlices);
    }
}

void JogXScreen::update() {
    updatePositionDisplay();
    static uint32_t last = 0;
    uint32_t now = ClearCore::TimingMgr.Milliseconds();
    if (now - last > 500) {
        last = now;
        auto& cutData = _mgr.GetCutData();
        auto& ui = UIInputManager::Instance();
        if (!ui.isEditing()) {
            float bladeThickness = SettingsManager::Instance().settings().bladeThickness;
            float newTh = cutData.increment - bladeThickness;
            if (fabs(newTh - cutData.thickness) > 0.0001f) {
                cutData.thickness = newTh < 0.0f ? 0.0f : newTh;
                updateThicknessDisplay();
            }
            updateSliceCounterDisplay();
        }
    }
}

float JogXScreen::GetCutThickness() {
    auto& cutData = ScreenManager::Instance().GetCutData();
    return cutData.thickness;
}

std::vector<float> JogXScreen::getIncrementPositions() const {
    auto& cutData = _mgr.GetCutData();
    float stockZero = cutData.useStockZero ? cutData.positionZero : 0.0f;
    float increment = cutData.increment;
    int totalSlices = cutData.totalSlices;

    std::vector<float> positions;
    for (int i = 0; i < totalSlices; ++i) {
        positions.push_back(stockZero + i * increment);
    }
    return positions;
}
