
// === ScreenManager.cpp ===
#include "ScreenManager.h"
#include "Config.h"
#include <ClearCore.h>
extern Genie genie;  // reference to global UI instance


ScreenManager& ScreenManager::Instance() {
    static ScreenManager inst;
    return inst;
}

ScreenManager::ScreenManager() = default;

void ScreenManager::Init() {
    // Show splash briefly then manual mode
    writeForm(FORM_SPLASH);
    Delay_ms(500);
    ShowManualMode();
}

void ScreenManager::writeForm(uint8_t formId) {
    // *** DEBUG LOGGING ***
    ClearCore::ConnectorUsb.Send("[SM] writeForm: from ");
    ClearCore::ConnectorUsb.Send(_currentForm);
    ClearCore::ConnectorUsb.Send(" → ");
    ClearCore::ConnectorUsb.SendLine(formId);

    if (_currentForm == formId) return;
    if (_currentScreen) _currentScreen->onHide();
    _lastForm = _currentForm;
    _currentForm = formId;
    genie.WriteObject(GENIE_OBJ_FORM, formId, 0);
    _currentScreen = currentScreen();
    if (_currentScreen) _currentScreen->onShow();
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
    default:                return nullptr;
    }
}
