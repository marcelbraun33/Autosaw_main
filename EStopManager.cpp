// core/EStopManager.cpp
#include "EStopManager.h"
#include "Config.h"
#include "MotionController.h"
#include <ClearCore.h>

EStopManager& EStopManager::Instance() {
    static EStopManager inst;
    return inst;
}

EStopManager::EStopManager()
    : _relayEnabled(false)
    , _resetRequested(false)
    , _prevSafeState(true)
    , _autoReset(true)
{
}

void EStopManager::setup() {
    // Configure E-Stop pin as digital input
    ESTOP_INPUT_PIN.Mode(Connector::INPUT_DIGITAL);
    // (Optionally configure pull-up if your wiring supports it:
    //  ESTOP_INPUT_PIN.Pullup(true); )

    // Configure safety relay pin as digital output
    SAFETY_RELAY_OUTPUT.Mode(Connector::OUTPUT_DIGITAL);

    // Read initial state (HIGH = safe on a NC switch)
    bool safe = ESTOP_INPUT_PIN.State();
    _prevSafeState = safe;

    // Drive relay accordingly
    _relayEnabled = safe;
    SAFETY_RELAY_OUTPUT.State(_relayEnabled);

    ClearCore::ConnectorUsb.Send("EStopManager started, safe? ");
    ClearCore::ConnectorUsb.SendLine(safe ? "YES" : "NO");
}

void EStopManager::update() {
    bool safe = ESTOP_INPUT_PIN.State();

    // Edge: tripped now, was safe → drop relay immediately
    if (!safe && _relayEnabled) {
        _relayEnabled = false;
        SAFETY_RELAY_OUTPUT.State(false);
        ClearCore::ConnectorUsb.SendLine("!! E-STOP ACTIVATED !!");
        emergencyStop();
    }
    // Edge: just released
    else if (safe && !_prevSafeState) {
        ClearCore::ConnectorUsb.SendLine("E-STOP button released");
        if (_autoReset) _resetRequested = true;
    }
    // Safe again and reset was requested → re-energize
    else if (safe && !_relayEnabled && _resetRequested) {
        _relayEnabled = true;
        _resetRequested = false;
        SAFETY_RELAY_OUTPUT.State(true);
        ClearCore::ConnectorUsb.SendLine("Safety relay re-enabled");
    }

    _prevSafeState = safe;
}

bool EStopManager::isActivated() const {
    // Pin LOW = tripped on a NC switch
    return !ESTOP_INPUT_PIN.State();
}

bool EStopManager::isSafetyRelayEnabled() const {
    return _relayEnabled;
}

void EStopManager::requestReset() {
    if (!isActivated()) _resetRequested = true;
}

void EStopManager::setAutoReset(bool on) {
    _autoReset = on;
}

void EStopManager::emergencyStop() {
    // make sure your MotionController has a StopSpindle()
    MotionController::Instance().StopSpindle();
    // if you have axis stops, call them too:
    // MotionController::Instance().StopFence();
    // MotionController::Instance().StopTable();
    // MotionController::Instance().StopRotary();
}
