// ScreenManager.cpp - Updated with SetupAutocutScreen
#include "ScreenManager.h"
#include "Config.h"
#include <ClearCore.h>
#include "MPGJogManager.h"

extern Genie genie;  // reference to global UI instance

ScreenManager& ScreenManager::Instance() {
    static ScreenManager inst;
    return inst;
}

ScreenManager::ScreenManager()
    : _manualScreen(*this),
    _settingsScreen(*this),
    _homingScreen(),
    _jogXScreen(*this),
    _jogYScreen(*this),
    _jogZScreen(*this),
    _semiAutoScreen(*this),
    _autoCutScreen(*this),
    _setupAutocutScreen(*this)  // NEW
{
    // Any other initialization here
}

void ScreenManager::Init() {
    // Show splash briefly then manual mode
    writeForm(FORM_SPLASH);
    Delay_ms(500);
    ShowManualMode();
}

void ScreenManager::writeForm(uint8_t formId) {
    // Don't redraw if already on this form
    if (_currentForm == formId) return;

    // Debug info
    ClearCore::ConnectorUsb.Send("[SM] writeForm: from ");
    ClearCore::ConnectorUsb.Send(_currentForm);
    ClearCore::ConnectorUsb.Send(" → ");
    ClearCore::ConnectorUsb.SendLine(formId);

    // Clean exit current screen
    if (_currentScreen) _currentScreen->onHide();

    // Disable jog mode when leaving jog screens
    if (_currentForm == FORM_JOG_X || _currentForm == FORM_JOG_Y || _currentForm == FORM_JOG_Z) {
        MPGJogManager::Instance().setEnabled(false);
    }

    // Track form state
    _lastForm = _currentForm;
    _currentForm = formId;

    // Change form and let the display handle its own transition
    genie.WriteObject(GENIE_OBJ_FORM, formId, 0);
    delay(100); // Give display time to fully load form before changes

    // Initialize the new screen
    _currentScreen = currentScreen();
    if (_currentScreen) _currentScreen->onShow();
}

void ScreenManager::clearAllLeds() {
    // Only clear a few specific LEDs that might cause rogue digits
    genie.WriteObject(GENIE_OBJ_LED, LED_NEGATIVE_INDICATOR, 0); // For JogXScreen
    genie.WriteObject(GENIE_OBJ_LED, LED_AT_START_POSITION_Y, 0); // For JogYScreen
    genie.WriteObject(GENIE_OBJ_LED, LED_READY, 0); // For SemiAutoScreen
    genie.WriteObject(GENIE_OBJ_LED, LED_FEED_RATE_OFFSET_F2, 0);
    genie.WriteObject(GENIE_OBJ_LED, LED_CUT_PRESSURE_OFFSET_F2, 0);
}

void ScreenManager::ShowSplash() { writeForm(FORM_SPLASH); }
void ScreenManager::ShowManualMode() { writeForm(FORM_MANUAL_MODE); }
void ScreenManager::ShowHoming() { writeForm(FORM_HOMING); }
void ScreenManager::ShowSettings() { writeForm(FORM_SETTINGS); }
void ScreenManager::ShowJogX() { writeForm(FORM_JOG_X); }
void ScreenManager::ShowJogY() { writeForm(FORM_JOG_Y); }
void ScreenManager::ShowJogZ() { writeForm(FORM_JOG_Z); }
void ScreenManager::ShowSemiAuto() { writeForm(FORM_SEMI_AUTO); }
void ScreenManager::ShowAutoCut() { writeForm(FORM_AUTOCUT); }
void ScreenManager::ShowSetupAutocut() {
    writeForm(FORM_SETUP_AUTOCUT);
}

void ScreenManager::Back() {
    if (_lastForm != _currentForm && _lastForm != 255)
        writeForm(_lastForm);
    else
        ShowManualMode();
}

Screen* ScreenManager::currentScreen() {
    switch (_currentForm) {
    case FORM_MANUAL_MODE:  return &_manualScreen;
    case FORM_SETTINGS:     return &_settingsScreen;
    case FORM_HOMING:       return &_homingScreen;
    case FORM_JOG_X:        return &_jogXScreen;
    case FORM_JOG_Y:        return &_jogYScreen;
    case FORM_JOG_Z:        return &_jogZScreen;
    case FORM_SEMI_AUTO:    return &_semiAutoScreen;
    case FORM_AUTOCUT:      return &_autoCutScreen;
    case FORM_SETUP_AUTOCUT: return &_setupAutocutScreen;  // NEW
    default:                return nullptr;
    }
}