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
    UIInputManager::Instance().unbindField();
}

void SemiAutoScreen::handleEvent(const genieFrame& e) {
    if (!_cycle) return;
    switch (e.reportObject.index) {
    case 10:
        _cycle->setSpindleOn(!_cycle->isSpindleOn());
        break;
    case 11:
        _cycle->feedToStop();
        break;
    case 12:
        if (_cycle->isPaused())
            _cycle->resume();
        else
            _cycle->pause();
        break;
    case 13:
        _cycle->cancel();
        break;
    case 14:  // INC +
        if (_cycle->getState() == SemiAutoCycle::Ready) {
            float inc = _mgr.GetCutData().increment;
            MotionController::Instance().getXAxis().Jog(+inc, 1.0f);
        }
        break;
    case 50:  // INC -
        if (_cycle->getState() == SemiAutoCycle::Ready) {
            float inc = _mgr.GetCutData().increment;
            MotionController::Instance().getXAxis().Jog(-inc, 1.0f);
        }
        break;

        // Feed rate adjust button
    case 15: {
        auto& ui = UIInputManager::Instance();
        if (ui.isEditing()) {
            if (ui.isFieldActive(15)) {
                ui.unbindField();
                _feedRateAdjustActive = false;
                genie.WriteObject(GENIE_OBJ_WINBUTTON, 15, 0);
            }
            else {
                genie.WriteObject(GENIE_OBJ_WINBUTTON, 15, 0);
            }
        }
        else {
            // Start feed‐rate editing mode
            ui.bindField(
                15,       // button index
                7,        // LED digits index
                &_cycle->getFeedRateRef(),
                1.0f,     // min
                25.0f,    // max
                0.1f,     // step
                1         // decimal places
            );
            _feedRateAdjustActive = true;
            genie.WriteObject(GENIE_OBJ_WINBUTTON, 15, 1);
        }
    } break;

           // Cut pressure adjust button (unchanged)
    case 41: {
        auto& ui = UIInputManager::Instance();
        if (ui.isEditing()) {
            if (ui.isFieldActive(41)) {
                ui.unbindField();
                genie.WriteObject(GENIE_OBJ_WINBUTTON, 41, 0);
            }
            else {
                genie.WriteObject(GENIE_OBJ_WINBUTTON, 41, 0);
            }
        }
        else {
            ui.bindField(
                41,       // button index
                27,       // LED digits index
                &_cycle->getCutPressureRef(),
                0.0f,     // min
                100.0f,   // max
                1.0f,     // step
                0         // decimal places
            );
            genie.WriteObject(GENIE_OBJ_WINBUTTON, 41, 1);
        }
    } break;

           // Homing button (unchanged)
    case 42: {
        auto& mc = MotionController::Instance();
        MotionController::MotionStatus status = mc.getStatus();
        if (status.yHomed) {
            mc.moveToWithRate(AXIS_Y, 0.0f, 1.0f);
            ClearCore::ConnectorUsb.SendLine("[SemiAutoScreen] Moving Y to home position");
        }
        else {
            ClearCore::ConnectorUsb.SendLine("[SemiAutoScreen] Starting Y axis homing");
            mc.StartHomingAxis(AXIS_Y);
        }
    } break;

    default:
        break;
    }

    // ALWAYS update the displays & button states after handling an event
    updateDisplays();
    updateButtonStates();
}

void SemiAutoScreen::update() {
    CycleManager::Instance().update();
    _cycle = static_cast<SemiAutoCycle*>(CycleManager::Instance().currentCycle());

    // Track feed‐rate changes
    static float prevFeedRate = 0.0f;

    if (_cycle) {
        float currentFeedRate = _cycle->getFeedRate();

        // Whenever the on‐screen feed rate actually changes, call setFeedRate()!
        if (std::abs(currentFeedRate - prevFeedRate) > 0.001f) {
            ClearCore::ConnectorUsb.Send("[SemiAutoScreen] Feed rate changed: ");
            ClearCore::ConnectorUsb.Send(prevFeedRate);
            ClearCore::ConnectorUsb.Send(" -> ");
            ClearCore::ConnectorUsb.SendLine(currentFeedRate);

            // <— THIS IS THE KEY LINE TO PROPAGATE THE OVERRIDE
            _cycle->setFeedRate(currentFeedRate);

            prevFeedRate = currentFeedRate;
        }

        // Update LED digit for feed‐rate, and LED indicator if offset is active
        genie.WriteObject(GENIE_OBJ_LED_DIGITS, 7, static_cast<uint16_t>(_cycle->getFeedRate() * 10.0f));
        genie.WriteObject(GENIE_OBJ_LED, 2, _cycle->isFeedRateOffsetActive() ? 1 : 0);
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
    genie.WriteObject(GENIE_OBJ_LED_DIGITS, 6, static_cast<uint16_t>(_cycle->commandedRPM()));

    // Show feed rate (1 decimal place)
    genie.WriteObject(GENIE_OBJ_LED_DIGITS, 7, static_cast<uint16_t>(_cycle->getFeedRate() * 10.0f));
}

void SemiAutoScreen::updateButtonStates() {
    if (!_cycle) return;
    bool isLatched = (_cycle->getState() == SemiAutoCycle::FeedingToStop) ||
        (_cycle->getState() == SemiAutoCycle::Returning);
    genie.WriteObject(GENIE_OBJ_WINBUTTON, 11, isLatched ? 1 : 0);
    genie.WriteObject(GENIE_OBJ_WINBUTTON, 12, _cycle->isPaused() ? 1 : 0);
    genie.WriteObject(GENIE_OBJ_WINBUTTON, 10, _cycle->isSpindleOn() ? 1 : 0);
}
