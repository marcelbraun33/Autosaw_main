// EStopManager.cpp
#include "EStopManager.h"
#include "Config.h"
#include "MotionController.h"
#include <ClearCore.h>

EStopManager& EStopManager::Instance() {
    static EStopManager instance;
    return instance;
}

EStopManager::EStopManager() :
    safetyRelayEnabled(false),
    resetRequested(false),
    prevEStopState(false),
    autoResetEnabled(true)  // Enable auto-reset by default
{
}

void EStopManager::setup() {
    // Configure E-Stop input pin
    ESTOP_INPUT_PIN.Mode(Connector::INPUT_DIGITAL);

    // Configure safety relay output pin
    SAFETY_RELAY_OUTPUT.Mode(Connector::OUTPUT_DIGITAL);

    // Initial state check - E-Stop is typically Normally Closed (NC)
    // so HIGH means not activated (safe condition)
    bool eStopNotActivated = ESTOP_INPUT_PIN.State();
    prevEStopState = eStopNotActivated;

    // Set initial state of the safety relay
    safetyRelayEnabled = eStopNotActivated;
    SAFETY_RELAY_OUTPUT.State(safetyRelayEnabled);

    // Use ConnectorUsb instead of Serial for ClearCore
    ClearCore::ConnectorUsb.Send("E-Stop status at startup: ");
    ClearCore::ConnectorUsb.SendLine(eStopNotActivated ? "Not activated" : "ACTIVATED");
}

void EStopManager::update() {
    // Read the current E-Stop state
    bool eStopNotActivated = ESTOP_INPUT_PIN.State();

    // If E-Stop is activated (signal goes LOW from the NC switch)
    if (!eStopNotActivated && safetyRelayEnabled) {
        // Disable safety relay immediately
        safetyRelayEnabled = false;
        SAFETY_RELAY_OUTPUT.State(false);
        ClearCore::ConnectorUsb.SendLine("E-STOP ACTIVATED - Safety relay disabled");

        // Notify motion controller to perform emergency stop
        emergencyStop();
    }
    // Detect E-Stop button release (transition from activated to not activated)
    else if (eStopNotActivated && !prevEStopState) {
        ClearCore::ConnectorUsb.SendLine("E-Stop button released");

        // Auto-reset feature - automatically request reset when E-Stop is released
        if (autoResetEnabled) {
            requestReset();
        }
    }
    // If E-Stop is cleared but safety relay is still disabled and reset requested
    else if (eStopNotActivated && !safetyRelayEnabled && resetRequested) {
        // Re-enable safety relay when reset is requested
        safetyRelayEnabled = true;
        SAFETY_RELAY_OUTPUT.State(true);
        resetRequested = false;
        ClearCore::ConnectorUsb.SendLine("E-Stop cleared - Safety relay re-enabled");
    }

    // Save current state for edge detection
    prevEStopState = eStopNotActivated;
}

bool EStopManager::isActivated() const {
    return !ESTOP_INPUT_PIN.State();
}

bool EStopManager::isSafetyRelayEnabled() const {
    return safetyRelayEnabled;
}

void EStopManager::requestReset() {
    if (!isActivated()) {
        resetRequested = true;
        ClearCore::ConnectorUsb.SendLine("Reset requested - will re-enable if E-Stop is clear");
    }
}

void EStopManager::setAutoReset(bool enabled) {
    autoResetEnabled = enabled;
}

bool EStopManager::isAutoResetEnabled() const {
    return autoResetEnabled;
}

void EStopManager::emergencyStop() {
    // Call stop spindle on the motion controller
    MotionController::Instance().StopSpindle();
}
