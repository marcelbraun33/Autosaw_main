#include "ScreenManager.h"
extern Genie genie;

ScreenManager& ScreenManager::Instance() {
    static ScreenManager inst;
    return inst;
}

ScreenManager::ScreenManager() = default;

void ScreenManager::Init() {
    ShowManualMode(); // Initial screen
}

void ScreenManager::writeForm(uint8_t formId) {
    if (_currentForm == formId) {
        return; // Already showing this form
    }

    if (_currentScreen) {
        _currentScreen->onHide(); // Notify previous screen
    }

    _lastForm = _currentForm;
    _currentForm = formId;
    _currentScreen = currentScreen(); // Resolve new screen instance

    genie.WriteObject(GENIE_OBJ_FORM, formId, 0);

    if (_currentScreen) {
        _currentScreen->onShow(); // Notify new screen
    }

    Serial.print("writeForm called for form: ");
    Serial.println(formId);
}


void ScreenManager::ShowSplash() { writeForm(FORM_SPLASH); }

void ScreenManager::ShowManualMode() {
    writeForm(FORM_MANUAL_MODE);

    // Display current RPM on manual screen
    auto& S = SettingsManager::Instance().settings();
    genie.WriteObject(GENIE_OBJ_LED_DIGITS, LEDDIGITS_MANUAL_RPM, (uint16_t)S.defaultRPM);
}

void ScreenManager::ShowHoming() { writeForm(FORM_HOMING); }
void ScreenManager::ShowJogX() { writeForm(FORM_JOG_X); }
void ScreenManager::ShowJogY() { writeForm(FORM_JOG_Y); }
void ScreenManager::ShowJogZ() { writeForm(FORM_JOG_Z); }
void ScreenManager::ShowSemiAuto() { writeForm(FORM_SEMI_AUTO); }
void ScreenManager::ShowAutoCut() { writeForm(FORM_AUTOCUT); }
void ScreenManager::ShowSettings() { writeForm(FORM_SETTINGS); }

void ScreenManager::Back() {
    if (_lastForm != _currentForm && _lastForm != 255) {
        writeForm(_lastForm);
    }
    else {
        ShowManualMode();
    }
}

void ScreenManager::SetHomed(bool homed) {
    _homed = homed;
}

Screen* ScreenManager::currentScreen() {
    Serial.print("ScreenManager::currentScreen - _currentForm: ");
    Serial.println(_currentForm);

    switch (_currentForm) {
    case FORM_MANUAL_MODE:
        return &_manualModeScreen;
    case FORM_SETTINGS:
        return &_settingsScreen;
        // Add other screens as needed
    default:
        return nullptr;
    }
}
