#include "SemiAutoScreen.h"
#include "screenmanager.h"
#include "SemiAutoCycle.h"
#include "UIInputManager.h"
#include <cmath>
#include "MotionController.h"
#include "Config.h"

extern Genie genie;

SemiAutoScreen::SemiAutoScreen(ScreenManager& mgr) : _mgr(mgr) {}

void SemiAutoScreen::onShow() {
    // Start a new semi-auto cycle if not already active
    if (!CycleManager::Instance().hasActiveCycle()) {
        CycleManager::Instance().startSemiAutoCycle(_mgr.GetCutData());
    }
    // Always refresh the pointer to the current cycle
    _cycle = static_cast<SemiAutoCycle*>(CycleManager::Instance().currentCycle());

    // Wire up axes to the cycle
    if (_cycle) {
        _cycle->setAxes(&MotionController::Instance().getYAxis());
    }

    _feedRateAdjustActive = false;
    _mpgLastValue = 0;

    updateDisplays();
    updateButtonStates();
}

void SemiAutoScreen::onHide() {
    // Optionally cancel the cycle when leaving the screen
    // CycleManager::Instance().cancelCycle();
    UIInputManager::Instance().unbindField();
}

void SemiAutoScreen::handleEvent(const genieFrame& e) {
    if (!_cycle) return;
    switch (e.reportObject.index) {
    case 10: _cycle->setSpindleOn(!_cycle->isSpindleOn()); break;
    case 11: _cycle->feedToStop(); break;
    case 12: if (_cycle->isPaused()) _cycle->resume(); else _cycle->pause(); break;
    case 13: _cycle->cancel(); break;
    case 14: // INC +
        if (_cycle->getState() == SemiAutoCycle::Ready) {
            float inc = _mgr.GetCutData().increment;
            MotionController::Instance().getXAxis().Jog(+inc, 1.0f);
        }
        break;
    case 50: // INC -
        if (_cycle->getState() == SemiAutoCycle::Ready) {
            float inc = _mgr.GetCutData().increment;
            MotionController::Instance().getXAxis().Jog(-inc, 1.0f);
        }
        break;

        // Feed rate adjust button
    case 15:
    {
        auto& ui = UIInputManager::Instance();

        // If already editing a field, toggle off
        if (ui.isEditing()) {
            if (ui.isFieldActive(15)) { // 15 is the button index
                ui.unbindField();
                _feedRateAdjustActive = false;
                genie.WriteObject(GENIE_OBJ_WINBUTTON, 15, 0);
            }
            else {
                // If editing another field, just toggle this button off
                genie.WriteObject(GENIE_OBJ_WINBUTTON, 15, 0);
            }
        }
        else {
            // Start feed rate editing mode - use actual reference to feed rate
            ui.bindField(15, 7, // Button 15 controlling LED Digits 7
                &_cycle->getFeedRateRef(), 1.0f, 50.0f, 0.5f, 1);
            _feedRateAdjustActive = true;
            genie.WriteObject(GENIE_OBJ_WINBUTTON, 15, 1);
        }
    }
    break;

    // Cut pressure adjust button
    case 41:
    {
        auto& ui = UIInputManager::Instance();

        // If already editing a field, toggle off
        if (ui.isEditing()) {
            if (ui.isFieldActive(41)) { // 41 is the button index
                ui.unbindField();
                genie.WriteObject(GENIE_OBJ_WINBUTTON, 41, 0);
            }
            else {
                // If editing another field, just toggle this button off
                genie.WriteObject(GENIE_OBJ_WINBUTTON, 41, 0);
            }
        }
        else {
            // Start cut pressure editing mode
            ui.bindField(41, 27, // Button 41 controlling LED Digits 27
                &_cycle->getCutPressureRef(), 0.0f, 100.0f, 1.0f, 0);
            genie.WriteObject(GENIE_OBJ_WINBUTTON, 41, 1);
        }
    }
    break;

    case 42: if (_cycle->isAtRetract()) _cycle->moveTableToStart(); else _cycle->moveTableToRetract(); break;

    default: break;
    }
    updateDisplays();
    updateButtonStates();
}

void SemiAutoScreen::update() {
    CycleManager::Instance().update();
    _cycle = static_cast<SemiAutoCycle*>(CycleManager::Instance().currentCycle());

    // If feed rate is being edited, real-time update LED and cycle
    if (_feedRateAdjustActive && _cycle) {
        // Update offset LED (LED 2)
        genie.WriteObject(GENIE_OBJ_LED, 2, _cycle->isFeedRateOffsetActive() ? 1 : 0);

        // Update the active movement if a feed is in progress
        if (_cycle->getState() == SemiAutoCycle::FeedingToStop) {
            // No need to manually update velocity - UIInputManager handles this
            // when it updates the feed rate value through the bound field
        }
    }

    updateDisplays();
    updateButtonStates();
}

void SemiAutoScreen::updateDisplays() {
    if (!_cycle) return;

    genie.WriteObject(GENIE_OBJ_LED, 2, _cycle->isFeedRateOffsetActive() ? 1 : 0);
    genie.WriteObject(GENIE_OBJ_LED, 4, _cycle->isCutPressureOffsetActive() ? 1 : 0);
    genie.WriteObject(GENIE_OBJ_LED_DIGITS, 27, static_cast<uint16_t>(_cycle->getCutPressure() * 100.0f));
    genie.WriteObject(GENIE_OBJ_LED_DIGITS, 28, static_cast<uint16_t>(_cycle->currentThickness() * 1000.0f));
    genie.WriteObject(GENIE_OBJ_LED_DIGITS, 29, static_cast<uint16_t>(_cycle->distanceToGo() * 1000.0f));
    genie.WriteObject(GENIE_OBJ_LED_DIGITS, 32, _cycle->currentCutIndex());
    genie.WriteObject(GENIE_OBJ_LED_DIGITS, 33, _cycle->totalSlices());
    genie.WriteObject(GENIE_OBJ_LED_DIGITS, 6, static_cast<uint16_t>(_cycle->commandedRPM()));
    genie.WriteObject(GENIE_OBJ_LED_DIGITS, 7, static_cast<uint16_t>(_cycle->getFeedRate() * 100.0f));
}

void SemiAutoScreen::updateButtonStates() {
    if (!_cycle) return;
    bool isLatched = (_cycle->getState() == SemiAutoCycle::FeedingToStop) ||
        (_cycle->getState() == SemiAutoCycle::Returning);
    genie.WriteObject(GENIE_OBJ_WINBUTTON, 11, isLatched ? 1 : 0); // Feed to Stop button
    genie.WriteObject(GENIE_OBJ_WINBUTTON, 12, _cycle->isPaused() ? 1 : 0);
    genie.WriteObject(GENIE_OBJ_WINBUTTON, 10, _cycle->isSpindleOn() ? 1 : 0);
}

