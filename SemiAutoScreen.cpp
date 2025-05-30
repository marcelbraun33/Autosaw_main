#include "SemiAutoScreen.h"
#include "screenmanager.h"
#include "SemiAutoCycle.h"
#include "UIInputManager.h"
#include <cmath>
#include "MotionController.h"


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

    updateDisplays();
    updateButtonStates();
}



void SemiAutoScreen::onHide() {
    // Optionally cancel the cycle when leaving the screen
    // CycleManager::Instance().cancelCycle();
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


    case 15: /* UI input for feed rate, then _cycle->setFeedRate(newRate); */ break;
    case 41: /* UI input for cut pressure, then _cycle->setCutPressure(newPressure); */ break;
    case 42: if (_cycle->isAtRetract()) _cycle->moveTableToStart(); else _cycle->moveTableToRetract(); break;

    default: break;
    }
    updateDisplays();
    updateButtonStates();
}

void SemiAutoScreen::update() {
    CycleManager::Instance().update();
    _cycle = static_cast<SemiAutoCycle*>(CycleManager::Instance().currentCycle());
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
    genie.WriteObject(GENIE_OBJ_WINBUTTON, 12, _cycle->isPaused() ? 1 : 0);
    genie.WriteObject(GENIE_OBJ_WINBUTTON, 10, _cycle->isSpindleOn() ? 1 : 0);
}