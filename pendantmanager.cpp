
// PendantManager.cpp
#include "PendantManager.h"
#include "ScreenManager.h"
#include "Config.h"
#include <ClearCore.h>

PendantManager& PendantManager::Instance() {
    static PendantManager instance;
    return instance;
}

void PendantManager::Init() {
    SELECTOR_PIN_X.Mode(Connector::INPUT_DIGITAL);
    SELECTOR_PIN_Y.Mode(Connector::INPUT_DIGITAL);
    SELECTOR_PIN_Z.Mode(Connector::INPUT_DIGITAL);
    SELECTOR_PIN_SETUP.Mode(Connector::INPUT_DIGITAL);
    SELECTOR_PIN_AUTOCUT.Mode(Connector::INPUT_DIGITAL);
    RANGE_PIN_X10.Mode(Connector::INPUT_DIGITAL);
    RANGE_PIN_X100.Mode(Connector::INPUT_DIGITAL);
}

void PendantManager::SetEnabled(bool enabled) {
    active = enabled;
}

bool PendantManager::IsEnabled() const {
    return active;
}

void PendantManager::Update() {
    if (!active) return;

    int raw = ReadSelector();
    uint32_t now = micros();

    if (raw == lastSelectorValue) {
        _pendingValue = -1;
        return;
    }
    if (raw != _pendingValue) {
        _pendingValue = raw;
        _pendingStartUs = now;
        return;
    }
    if (now - _pendingStartUs >= DEBOUNCE_US) {
        lastSelectorValue = raw;
        HandleSelectorChange(raw);
        _pendingValue = -1;
    }
}

int PendantManager::ReadSelector() const {
    int value = 0;
    if (SELECTOR_PIN_X.State())     value |= 0x01;
    if (SELECTOR_PIN_Y.State())     value |= 0x02;
    if (SELECTOR_PIN_Z.State())     value |= 0x04;
    if (SELECTOR_PIN_SETUP.State()) value |= 0x08;
    if (SELECTOR_PIN_AUTOCUT.State())value |= 0x10;
    return value;
}

int PendantManager::LastKnownSelector() const {
    return lastSelectorValue;
}

// Reset the internal knob position so it won’t retrigger immediately
void PendantManager::SetLastKnownSelector(int selector) {
    lastSelectorValue = selector;
}

void PendantManager::HandleSelectorChange(int selector) {
    if (!active) return;
    switch (selector) {
    case 0x01: ScreenManager::Instance().ShowJogX();    break;
    case 0x02: ScreenManager::Instance().ShowJogY();    break;
    case 0x04: ScreenManager::Instance().ShowJogZ();    break;
    case 0x08: ScreenManager::Instance().ShowSemiAuto(); break;
    case 0x10: ScreenManager::Instance().ShowAutoCut();  break;
    default:   ScreenManager::Instance().ShowManualMode();break;
    }
}

void PendantManager::SyncScreenToKnob() {
    if (!active) return;
    HandleSelectorChange(ReadSelector());
}
