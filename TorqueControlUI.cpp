#include "TorqueControlUI.h"
#include "MotionController.h"
#include "SettingsManager.h"
#include "UIInputManager.h"

extern Genie genie;

TorqueControlUI::TorqueControlUI()
    : _cutPressureLedId(0), _targetFeedRateLedId(0), _torqueGaugeId(0), _liveFeedRateLedId(0),
    _currentMode(ADJUSTMENT_NONE), _tempCutPressure(70.0f), _tempFeedRate(1.0f),
    _lastEncoderPos(0), _isCuttingActive(false) {
}

void TorqueControlUI::init(uint16_t cutPressureLedId, uint16_t targetFeedRateLedId,
    uint16_t torqueGaugeId, uint16_t liveFeedRateLedId) {
    _cutPressureLedId = cutPressureLedId;
    _targetFeedRateLedId = targetFeedRateLedId;
    _torqueGaugeId = torqueGaugeId;
    _liveFeedRateLedId = liveFeedRateLedId;
}

void TorqueControlUI::onShow() {
    // Load current settings
    auto& settings = SettingsManager::Instance().settings();

#ifdef SETTINGS_HAS_CUT_PRESSURE
    _tempCutPressure = settings.cutPressure;
#else
    _tempCutPressure = 70.0f;
#endif

    _tempFeedRate = settings.feedRate / 25.0f; // Scale to 0-1 range

    // Reset mode
    _currentMode = ADJUSTMENT_NONE;
    _isCuttingActive = false;

    // Initialize displays
    updateCutPressureDisplay();
    updateFeedRateDisplay();

    // Initialize gauge at zero
    genie.WriteObject(GENIE_OBJ_IGAUGE, _torqueGaugeId, 0);

    // Reset encoder tracking
    _lastEncoderPos = ClearCore::EncoderIn.Position();
}

void TorqueControlUI::onHide() {
    // Exit any active adjustment mode and save
    if (_currentMode != ADJUSTMENT_NONE) {
        exitAdjustmentMode();
    }
}

void TorqueControlUI::update() {
    auto& motion = MotionController::Instance();

    // Update torque gauge if in torque-controlled feed
    if (motion.isInTorqueControlledFeed(AXIS_Y)) {
        updateTorqueGauge();

        // Update live feed rate display if configured
        if (_liveFeedRateLedId > 0) {
            float currentFeedRate = motion.YAxisInstance().DebugGetCurrentFeedRate();
            genie.WriteObject(GENIE_OBJ_LED_DIGITS, _liveFeedRateLedId,
                static_cast<uint16_t>(currentFeedRate * 100.0f));
        }
    }
    else if (_liveFeedRateLedId > 0) {
        // Clear feed rate display when not cutting
        genie.WriteObject(GENIE_OBJ_LED_DIGITS, _liveFeedRateLedId, 0);
    }

    // Handle encoder input if in adjustment mode
    if (_currentMode != ADJUSTMENT_NONE) {
        handleEncoderInput();
    }
}

void TorqueControlUI::toggleCutPressureAdjustment() {
    if (_currentMode == ADJUSTMENT_CUT_PRESSURE) {
        // Exit cut pressure adjustment
        exitAdjustmentMode();
    }
    else {
        // Exit feed rate adjustment if active
        if (_currentMode == ADJUSTMENT_FEED_RATE) {
            exitAdjustmentMode();
        }

        // Enter cut pressure adjustment
        _currentMode = ADJUSTMENT_CUT_PRESSURE;

        // Load current value
#ifdef SETTINGS_HAS_CUT_PRESSURE
        _tempCutPressure = SettingsManager::Instance().settings().cutPressure;
#else
        _tempCutPressure = 70.0f;
#endif

        // Reset encoder tracking
        _lastEncoderPos = ClearCore::EncoderIn.Position();
        UIInputManager::Instance().resetRaw();

        // Update display
        updateCutPressureDisplay();

        ClearCore::ConnectorUsb.Send("[TorqueControlUI] Entering cut pressure adjustment, value: ");
        ClearCore::ConnectorUsb.SendLine(_tempCutPressure);
    }
}

void TorqueControlUI::toggleFeedRateAdjustment() {
    if (_currentMode == ADJUSTMENT_FEED_RATE) {
        // Exit feed rate adjustment
        exitAdjustmentMode();
    }
    else {
        // Exit cut pressure adjustment if active
        if (_currentMode == ADJUSTMENT_CUT_PRESSURE) {
            exitAdjustmentMode();
        }

        // Enter feed rate adjustment
        _currentMode = ADJUSTMENT_FEED_RATE;

        // Load current value
        auto& settings = SettingsManager::Instance().settings();
        _tempFeedRate = settings.feedRate / 25.0f;

        // Reset encoder tracking
        _lastEncoderPos = ClearCore::EncoderIn.Position();
        UIInputManager::Instance().resetRaw();

        // Update display
        updateFeedRateDisplay();

        ClearCore::ConnectorUsb.Send("[TorqueControlUI] Entering feed rate adjustment, value: ");
        ClearCore::ConnectorUsb.Send(_tempFeedRate * 100.0f, 0);
        ClearCore::ConnectorUsb.SendLine("%");
    }
}

void TorqueControlUI::incrementValue() {
    if (_currentMode == ADJUSTMENT_CUT_PRESSURE) {
        _tempCutPressure += CUT_PRESSURE_INCREMENT;
        if (_tempCutPressure > MAX_CUT_PRESSURE) {
            _tempCutPressure = MAX_CUT_PRESSURE;
        }
        updateCutPressureDisplay();
        applyLiveAdjustments();
    }
    else if (_currentMode == ADJUSTMENT_FEED_RATE) {
        _tempFeedRate += FEED_RATE_INCREMENT;
        if (_tempFeedRate > MAX_FEED_RATE) {
            _tempFeedRate = MAX_FEED_RATE;
        }
        updateFeedRateDisplay();
        applyLiveAdjustments();
    }
}

void TorqueControlUI::decrementValue() {
    if (_currentMode == ADJUSTMENT_CUT_PRESSURE) {
        _tempCutPressure -= CUT_PRESSURE_INCREMENT;
        if (_tempCutPressure < MIN_CUT_PRESSURE) {
            _tempCutPressure = MIN_CUT_PRESSURE;
        }
        updateCutPressureDisplay();
        applyLiveAdjustments();
    }
    else if (_currentMode == ADJUSTMENT_FEED_RATE) {
        _tempFeedRate -= FEED_RATE_INCREMENT;
        if (_tempFeedRate < MIN_FEED_RATE) {
            _tempFeedRate = MIN_FEED_RATE;
        }
        updateFeedRateDisplay();
        applyLiveAdjustments();
    }
}

void TorqueControlUI::exitAdjustmentMode() {
    if (_currentMode == ADJUSTMENT_NONE) return;

    auto& settings = SettingsManager::Instance().settings();

    if (_currentMode == ADJUSTMENT_CUT_PRESSURE) {
#ifdef SETTINGS_HAS_CUT_PRESSURE
        settings.cutPressure = _tempCutPressure;
#endif
        ClearCore::ConnectorUsb.Send("[TorqueControlUI] Cut pressure saved: ");
        ClearCore::ConnectorUsb.SendLine(_tempCutPressure);
    }
    else if (_currentMode == ADJUSTMENT_FEED_RATE) {
        settings.feedRate = _tempFeedRate * 25.0f;
        ClearCore::ConnectorUsb.Send("[TorqueControlUI] Feed rate saved: ");
        ClearCore::ConnectorUsb.Send(_tempFeedRate * 100.0f, 0);
        ClearCore::ConnectorUsb.SendLine("%");
    }

    SettingsManager::Instance().save();
    _currentMode = ADJUSTMENT_NONE;
}

void TorqueControlUI::setCuttingActive(bool active) {
    _isCuttingActive = active;
}

void TorqueControlUI::updateButtonStates(uint16_t cutPressureButtonId, uint16_t feedRateButtonId) {
    if (cutPressureButtonId > 0) {
        bool pressureActive = (_currentMode == ADJUSTMENT_CUT_PRESSURE);
        genie.WriteObject(GENIE_OBJ_WINBUTTON, cutPressureButtonId, pressureActive ? 1 : 0);
    }

    if (feedRateButtonId > 0) {
        bool feedRateActive = (_currentMode == ADJUSTMENT_FEED_RATE);
        genie.WriteObject(GENIE_OBJ_WINBUTTON, feedRateButtonId, feedRateActive ? 1 : 0);
    }
}

void TorqueControlUI::updateCutPressureDisplay() {
    genie.WriteObject(GENIE_OBJ_LED_DIGITS, _cutPressureLedId,
        static_cast<uint16_t>(_tempCutPressure * 10.0f));
}

void TorqueControlUI::updateFeedRateDisplay() {
    genie.WriteObject(GENIE_OBJ_LED_DIGITS, _targetFeedRateLedId,
        static_cast<uint16_t>(_tempFeedRate * 100.0f));
}

void TorqueControlUI::updateTorqueGauge() {
    auto& motion = MotionController::Instance();

    float torque = motion.getTorquePercent(AXIS_Y);
    float targetTorque = motion.getTorqueTarget(AXIS_Y);
    float torquePercentage = (torque / targetTorque) * 100.0f;

    // Clamp to gauge range
    if (torquePercentage > 125.0f) torquePercentage = 125.0f;
    if (torquePercentage < 0.0f) torquePercentage = 0.0f;

    uint16_t gaugeValue = static_cast<uint16_t>(torquePercentage);
    genie.WriteObject(GENIE_OBJ_IGAUGE, _torqueGaugeId, gaugeValue);
}

void TorqueControlUI::handleEncoderInput() {
    int32_t currentEncoderPos = ClearCore::EncoderIn.Position();
    int32_t delta = currentEncoderPos - _lastEncoderPos;

    if (abs(delta) >= ENCODER_COUNTS_PER_CLICK) {
        _lastEncoderPos = currentEncoderPos;

        if (_currentMode == ADJUSTMENT_CUT_PRESSURE) {
            float changeAmount = (delta > 0) ? CUT_PRESSURE_INCREMENT : -CUT_PRESSURE_INCREMENT;
            _tempCutPressure += changeAmount;

            if (_tempCutPressure < MIN_CUT_PRESSURE) _tempCutPressure = MIN_CUT_PRESSURE;
            if (_tempCutPressure > MAX_CUT_PRESSURE) _tempCutPressure = MAX_CUT_PRESSURE;

            updateCutPressureDisplay();
            applyLiveAdjustments();

            ClearCore::ConnectorUsb.Send("[TorqueControlUI] Cut pressure adjusted via encoder: ");
            ClearCore::ConnectorUsb.SendLine(_tempCutPressure);
        }
        else if (_currentMode == ADJUSTMENT_FEED_RATE) {
            float changeAmount = (delta > 0) ? FEED_RATE_INCREMENT : -FEED_RATE_INCREMENT;
            _tempFeedRate += changeAmount;

            if (_tempFeedRate < MIN_FEED_RATE) _tempFeedRate = MIN_FEED_RATE;
            if (_tempFeedRate > MAX_FEED_RATE) _tempFeedRate = MAX_FEED_RATE;

            updateFeedRateDisplay();
            applyLiveAdjustments();

            ClearCore::ConnectorUsb.Send("[TorqueControlUI] Feed rate adjusted via encoder: ");
            ClearCore::ConnectorUsb.Send(_tempFeedRate * 100.0f, 0);
            ClearCore::ConnectorUsb.SendLine("%");
        }
    }
}

void TorqueControlUI::applyLiveAdjustments() {
    auto& motion = MotionController::Instance();

    // Apply changes immediately if in active cutting mode
    if (motion.isInTorqueControlledFeed(AXIS_Y)) {
        if (_currentMode == ADJUSTMENT_CUT_PRESSURE) {
            motion.setTorqueTarget(AXIS_Y, _tempCutPressure);
        }
        else if (_currentMode == ADJUSTMENT_FEED_RATE) {
            motion.YAxisInstance().UpdateFeedRate(_tempFeedRate);
        }
    }
}