#include "AutoCutScreen.h"
#include "screenmanager.h" 
#include "CycleManager.h"

extern Genie genie; // Add this line

AutoCutScreen::AutoCutScreen(ScreenManager& mgr) : _mgr(mgr) {}

void AutoCutScreen::onShow() {
    // Initialize auto-cut state here
    if (!CycleManager::Instance().hasActiveCycle()) {
        CycleManager::Instance().startAutoCutCycle(_mgr.GetCutData());
    }
    _cycle = static_cast<AutoCutCycle*>(CycleManager::Instance().currentCycle());

    updateDisplays();
}

void AutoCutScreen::onHide() {
    // Cleanup or disable auto-cut state here
}

void AutoCutScreen::handleEvent(const genieFrame& e) {
    // Handle auto-cut events here
}

void AutoCutScreen::update() {
    if (CycleManager::Instance().hasActiveCycle()) {
        _cycle = static_cast<AutoCutCycle*>(CycleManager::Instance().currentCycle());
    }
    updateDisplays();
}

void AutoCutScreen::pauseCycle() {
    if (_cycle) {
        _cycle->pause();
    }
}

void AutoCutScreen::resumeCycle() {
    if (_cycle) {
        _cycle->resume();
    }
}

void AutoCutScreen::cancelCycle() {
    if (_cycle) {
        _cycle->cancel();
    }
}

void AutoCutScreen::updateDisplays() {
    if (!_cycle) return;

    // Feed rate display (in/min)
    genie.WriteObject(GENIE_OBJ_LED_DIGITS, 13, static_cast<uint16_t>(_cycle->getFeedRate()));

    // Add other display updates as needed
}
