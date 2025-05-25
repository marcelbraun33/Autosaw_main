#include "JogXScreen.h"
#include "MotionController.h"
#include "ScreenManager.h"
#include "UIInputManager.h"
#include "MPGJogManager.h"
#include "SettingsManager.h"
#include <cmath>
#include "Config.h"

// Define LED indicator constants if not already defined in Config.h
#ifndef LED_NEGATIVE_INDICATOR
#define LED_NEGATIVE_INDICATOR 0   // Form1: LED indicator for negative value
#endif

// GenieArduino standard object types
#define GENIE_OBJ_DIPSW        0
#define GENIE_OBJ_KNOB         1
#define GENIE_OBJ_ROCKERSW     2
#define GENIE_OBJ_ROTARYSW     3
#define GENIE_OBJ_SLIDER       4
#define GENIE_OBJ_TRACKBAR     5
#define GENIE_OBJ_WINBUTTON    6
#define GENIE_OBJ_ANGULAR_METER 7
#define GENIE_OBJ_COOL_GAUGE   8
#define GENIE_OBJ_CUSTOM_DIGITS 9
#define GENIE_OBJ_FORM         10
#define GENIE_OBJ_GAUGE        11
#define GENIE_OBJ_IMAGE        12
#define GENIE_OBJ_KEYBOARD     13
#define GENIE_OBJ_LED          14
#define GENIE_OBJ_LED_DIGITS   15
#define GENIE_OBJ_METER        16
#define GENIE_OBJ_STRINGS      17
#define GENIE_OBJ_THERMOMETER  18
#define GENIE_OBJ_USER_LED     19
#define GENIE_OBJ_VIDEO        20
#define GENIE_OBJ_STATIC_TEXT  21
#define GENIE_OBJ_SOUND        22
#define GENIE_OBJ_TIMER        23

extern Genie genie;

// Storage for position values
static struct {
    float positionZero = 0.0f;
    float stockLength = 0.0f;
    float increment = 0.0f;
    float thickness = 0.0f;
    int totalSlices = 0;
    int currentSlice = 0;
    bool useStockZero = false;
} jogXData;

void JogXScreen::onShow() {
    MPGJogManager::Instance().setEnabled(true);
    MPGJogManager::Instance().setAxis(AXIS_X);

    for (int i = 0; i <= WINBUTTON_SET_TOTAL_SLICES; ++i) {
        showButtonSafe(i, 0);
    }

    genie.WriteObject(GENIE_OBJ_LED, LED_NEGATIVE_INDICATOR, 0);
    genie.WriteObject(GENIE_OBJ_USER_LED, LED_NEGATIVE_INDICATOR, 0);

    updatePositionDisplay();
    updateStockLengthDisplay();
    updateIncrementDisplay();
    updateThicknessDisplay();
    updateTotalSlicesDisplay();
    updateSliceCounterDisplay();
}

void JogXScreen::onHide() {
    MPGJogManager::Instance().setEnabled(false);
    for (int i = 0; i <= WINBUTTON_SET_TOTAL_SLICES; ++i) {
        showButtonSafe(i, 0);
    }
    genie.WriteObject(GENIE_OBJ_LED, LED_NEGATIVE_INDICATOR, 0);
    genie.WriteObject(GENIE_OBJ_USER_LED, LED_NEGATIVE_INDICATOR, 0);
    UIInputManager::Instance().unbindField();
}

void JogXScreen::handleEvent(const genieFrame& e) {
    if (e.reportObject.cmd != GENIE_REPORT_EVENT || e.reportObject.object != GENIE_OBJ_WINBUTTON)
        return;

    auto& ui = UIInputManager::Instance();
    auto& mpg = MPGJogManager::Instance();
    static float tempSlices = 10.0f;    // unified edit buffer for totalSlices
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

    case WINBUTTON_DIVIDE_SET:
        // quick sanity flashes if invalid
        if (jogXData.totalSlices <= 0 || jogXData.stockLength <= 0.0f) {
            for (int i = 0; i < 2; ++i) {
                showButtonSafe(WINBUTTON_DIVIDE_SET, 1);
                delay(50);
                showButtonSafe(WINBUTTON_DIVIDE_SET, 0);
                delay(50);
            }
            break;
        }
        newIncrement = jogXData.stockLength / jogXData.totalSlices;
        setIncrement(newIncrement);
        showButtonSafe(WINBUTTON_DIVIDE_SET, 1);
        delay(200);
        showButtonSafe(WINBUTTON_DIVIDE_SET, 0);
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
                &jogXData.stockLength, 0.0f, 100.0f, 0.001f, 3);
            showButtonSafe(WINBUTTON_SET_STOCK_LENGTH, 1);
        }
        break;

    case WINBUTTON_SET_CUT_THICKNESS:
        if (ui.isEditing() && ui.isFieldActive(WINBUTTON_SET_CUT_THICKNESS)) {
            ui.unbindField();
            showButtonSafe(WINBUTTON_SET_CUT_THICKNESS, 0);
            if (jogXData.thickness < 0.0f) jogXData.thickness = 0.0f;
            float blade = SettingsManager::Instance().settings().bladeThickness;
            newIncrement = jogXData.thickness + blade;
            setIncrement(newIncrement);
        }
        else if (!ui.isEditing()) {
            if (mpg.isEnabled()) {
                mpg.setEnabled(false);
                showButtonSafe(WINBUTTON_ACTIVATE_JOG, 0);
            }
            if (jogXData.thickness < 0.0f) jogXData.thickness = 0.0f;
            int32_t scaled = static_cast<int32_t>(round(jogXData.thickness * 1000.0f));
            genie.WriteObject(GENIE_OBJ_LED_DIGITS, LEDDIGITS_CUT_THICKNESS, static_cast<uint16_t>(scaled));
            ui.bindField(WINBUTTON_SET_CUT_THICKNESS, LEDDIGITS_CUT_THICKNESS,
                &jogXData.thickness, 0.0f, 10.0f, 0.001f, 3);
            showButtonSafe(WINBUTTON_SET_CUT_THICKNESS, 1);
        }
        break;

    case WINBUTTON_SET_TOTAL_SLICES:
        if (ui.isEditing() && ui.isFieldActive(WINBUTTON_SET_TOTAL_SLICES)) {
            // commit user’s value
            jogXData.totalSlices = static_cast<int>(roundf(tempSlices));
            ui.unbindField();
            showButtonSafe(WINBUTTON_SET_TOTAL_SLICES, 0);
        }
        else if (!ui.isEditing()) {
            if (mpg.isEnabled()) {
                mpg.setEnabled(false);
                showButtonSafe(WINBUTTON_ACTIVATE_JOG, 0);
            }
            // seed buffer & display
            tempSlices = (jogXData.totalSlices > 0 ? jogXData.totalSlices : 10);
            genie.WriteObject(GENIE_OBJ_LED_DIGITS, LEDDIGITS_TOTAL_SLICES,
                static_cast<uint16_t>(tempSlices));
            ui.bindField(WINBUTTON_SET_TOTAL_SLICES, LEDDIGITS_TOTAL_SLICES,
                &tempSlices, 1.0f, 1000.0f, 1.0f, 0);
            showButtonSafe(WINBUTTON_SET_TOTAL_SLICES, 1);
        }
        break;
    }
}

void JogXScreen::updatePositionDisplay() {
    float current = MotionController::Instance().getAxisPosition(AXIS_X);
    float display = jogXData.useStockZero ? (current - jogXData.positionZero) : current;
    bool negative = (display < 0.0f);
    static bool lastNeg = false;
    genie.WriteObject(GENIE_OBJ_LED, LED_NEGATIVE_INDICATOR, negative ? 1 : 0);
    genie.WriteObject(GENIE_OBJ_USER_LED, LED_NEGATIVE_INDICATOR, negative ? 1 : 0);
    if (negative != lastNeg) {
        lastNeg = negative;
        ClearCore::ConnectorUsb.Send("Position: ");
        ClearCore::ConnectorUsb.Send(display);
        ClearCore::ConnectorUsb.Send(" isNegative: ");
        ClearCore::ConnectorUsb.SendLine(negative ? "YES" : "NO");
    }
    int32_t scaled = static_cast<int32_t>(round(fabs(display) * 1000.0f));
    genie.WriteObject(GENIE_OBJ_LED_DIGITS, LEDDIGITS_SAW_POSITION,
        static_cast<uint16_t>(scaled));
}

void JogXScreen::captureZero() {
    jogXData.positionZero = MotionController::Instance().getAxisPosition(AXIS_X);
    jogXData.useStockZero = true;
    ClearCore::ConnectorUsb.Send("Zero position captured: ");
    ClearCore::ConnectorUsb.SendLine(jogXData.positionZero);
    showButtonSafe(WINBUTTON_CAPTURE_ZERO, 1);
    delay(200); showButtonSafe(WINBUTTON_CAPTURE_ZERO, 0);
    updatePositionDisplay();
}

void JogXScreen::captureStockLength() {
    float current = MotionController::Instance().getAxisPosition(AXIS_X);
    float display = jogXData.useStockZero ? (current - jogXData.positionZero) : current;
    if (display < 0.0f) {
        ClearCore::ConnectorUsb.SendLine("Error: Cannot capture stock length when negative");
        showButtonSafe(WINBUTTON_CAPTURE_STOCK_LENGTH, 1);
        delay(50); showButtonSafe(WINBUTTON_CAPTURE_STOCK_LENGTH, 0);
        delay(50); showButtonSafe(WINBUTTON_CAPTURE_STOCK_LENGTH, 1);
        delay(50); showButtonSafe(WINBUTTON_CAPTURE_STOCK_LENGTH, 0);
        return;
    }
    jogXData.stockLength = display;
    showButtonSafe(WINBUTTON_CAPTURE_STOCK_LENGTH, 1);
    delay(200); showButtonSafe(WINBUTTON_CAPTURE_STOCK_LENGTH, 0);
    updateStockLengthDisplay();
    calculateTotalSlices();
    updateTotalSlicesDisplay();
    updateSliceCounterDisplay();
}

void JogXScreen::captureIncrement() {
    float current = MotionController::Instance().getAxisPosition(AXIS_X);
    float display = jogXData.useStockZero ? (current - jogXData.positionZero) : current;
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
    if (jogXData.increment > 0.0f)
        jogXData.totalSlices = static_cast<int>(floorf(jogXData.stockLength / jogXData.increment));
    else
        jogXData.totalSlices = 0;
}

void JogXScreen::setIncrement(float newIncrement) {
    ClearCore::ConnectorUsb.SendLine("--------- SET INCREMENT ---------");
    ClearCore::ConnectorUsb.Send("Previous increment: ");
    ClearCore::ConnectorUsb.SendLine(jogXData.increment);
    ClearCore::ConnectorUsb.Send("New increment (raw): ");
    ClearCore::ConnectorUsb.SendLine(newIncrement);

    if (newIncrement < 0.001f) {
        newIncrement = 0.001f;
        ClearCore::ConnectorUsb.SendLine("WARNING: Minimum increment enforced");
    }

    jogXData.increment = newIncrement;
    float bladeThickness = SettingsManager::Instance().settings().bladeThickness;
    jogXData.thickness = jogXData.increment - bladeThickness;
    if (jogXData.thickness < 0.0f) {
        jogXData.thickness = 0.0f;
        ClearCore::ConnectorUsb.SendLine("WARNING: Cut thickness negative, set to 0");
    }
    ClearCore::ConnectorUsb.Send("Calculated thickness: ");
    ClearCore::ConnectorUsb.SendLine(jogXData.thickness);

    calculateTotalSlices();
    ClearCore::ConnectorUsb.Send("Calculated total slices: ");
    ClearCore::ConnectorUsb.SendLine(jogXData.totalSlices);

    updateIncrementDisplay();
    updateThicknessDisplay();
    updateTotalSlicesDisplay();
    updateSliceCounterDisplay();

    ClearCore::ConnectorUsb.SendLine("--------- SET INCREMENT COMPLETE ---------");
}

void JogXScreen::updateStockLengthDisplay() {
    int32_t scaled = static_cast<int32_t>(round(jogXData.stockLength * 1000.0f));
    genie.WriteObject(GENIE_OBJ_LED_DIGITS, LEDDIGITS_STOCK_LENGTH, static_cast<uint16_t>(scaled));
}

void JogXScreen::updateIncrementDisplay() {
    if (jogXData.increment < 0.001f) jogXData.increment = 0.001f;
    int32_t scaled = static_cast<int32_t>(round(jogXData.increment * 1000.0f));
    genie.WriteObject(GENIE_OBJ_LED_DIGITS, LEDDIGITS_INCREMENT, static_cast<uint16_t>(scaled));
}

void JogXScreen::updateThicknessDisplay() {
    if (jogXData.thickness < 0.0f) jogXData.thickness = 0.0f;
    else if (jogXData.thickness > 10.0f) jogXData.thickness = 10.0f;
    ClearCore::ConnectorUsb.Send("Updating thickness display to: ");
    ClearCore::ConnectorUsb.SendLine(jogXData.thickness);
    int32_t scaled = static_cast<int32_t>(round(jogXData.thickness * 1000.0f));
    genie.WriteObject(GENIE_OBJ_LED_DIGITS, LEDDIGITS_CUT_THICKNESS, static_cast<uint16_t>(scaled));
}

void JogXScreen::updateTotalSlicesDisplay() {
    genie.WriteObject(GENIE_OBJ_LED_DIGITS, LEDDIGITS_TOTAL_SLICES, static_cast<uint16_t>(jogXData.totalSlices));
}

void JogXScreen::updateSliceCounterDisplay() {
    int available = 0;
    if (jogXData.increment > 0.0f)
        available = static_cast<int>(floorf(jogXData.stockLength / jogXData.increment));
    genie.WriteObject(GENIE_OBJ_LED_DIGITS, LEDDIGITS_SLICE_COUNTER, static_cast<uint16_t>(available));
}

void JogXScreen::update() {
    updatePositionDisplay();
    static uint32_t last = 0;
    uint32_t now = ClearCore::TimingMgr.Milliseconds();
    if (now - last > 500) {
        last = now;
        auto& ui = UIInputManager::Instance();
        if (!ui.isEditing()) {
            float bladeThickness = SettingsManager::Instance().settings().bladeThickness;
            float newTh = jogXData.increment - bladeThickness;
            if (fabs(newTh - jogXData.thickness) > 0.0001f) {
                jogXData.thickness = newTh < 0.0f ? 0.0f : newTh;
                updateThicknessDisplay();
            }
            updateSliceCounterDisplay();
        }
    }
}
