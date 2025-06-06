#include "YAxis.h"
#include "Config.h"
#include "ClearCore.h"
#include "EncoderPositionTracker.h"
#include "DynamicFeed.h"

static constexpr float MAX_VELOCITY = 10000.0f;      // steps/s
static constexpr float MAX_ACCELERATION = 100000.0f; // steps/s^2

YAxis::YAxis()
    : _stepsPerInch(TABLE_STEPS_PER_INCH)
    , _motor(&MOTOR_TABLE_Y)
    , _isSetup(false)
    , _isHomed(false)
    , _isMoving(false)
    , _currentPos(0.0f)
    , _torquePct(0.0f)
    , _hasBeenHomed(false)
{
    HomingParams params;
    params.motor = _motor;
    params.stepsPerUnit = _stepsPerInch;
    params.fastVel = HOME_VEL_FAST;
    params.slowVel = HOME_VEL_SLOW;
    params.dwellMs = 100;
    params.backoffUnits = HOMING_BACKOFF_INCH;
    params.timeoutMs = HOMING_TIMEOUT_MS;

    _homingHelper = new HomingHelper(params);
    _dynamicFeed = new DynamicFeed(this, _stepsPerInch, _motor);
}

YAxis::~YAxis() {
    delete _homingHelper;
    delete _dynamicFeed;
}

void YAxis::Setup() {
    _motor->HlfbMode(MotorDriver::HLFB_MODE_HAS_BIPOLAR_PWM);
    _motor->HlfbCarrier(MotorDriver::HLFB_CARRIER_482_HZ);
    _motor->VelMax(MAX_VELOCITY);
    _motor->AccelMax(MAX_ACCELERATION);
    ClearAlerts();
    _isSetup = true;
    ClearCore::ConnectorUsb.SendLine("[Y-Axis] Setup complete");
}

void YAxis::ClearAlerts() {
    ClearCore::ConnectorUsb.SendLine("[Y-Axis] Checking for alerts");
    if (_motor->StatusReg().bit.AlertsPresent) {
        ClearCore::ConnectorUsb.SendLine("[Y-Axis] Alerts present:");
        if (_motor->AlertReg().bit.MotionCanceledInAlert)
            ClearCore::ConnectorUsb.SendLine(" - MotionCanceledInAlert");
        if (_motor->AlertReg().bit.MotionCanceledPositiveLimit)
            ClearCore::ConnectorUsb.SendLine(" - MotionCanceledPositiveLimit");
        if (_motor->AlertReg().bit.MotionCanceledNegativeLimit)
            ClearCore::ConnectorUsb.SendLine(" - MotionCanceledNegativeLimit");
        if (_motor->AlertReg().bit.MotionCanceledSensorEStop)
            ClearCore::ConnectorUsb.SendLine(" - MotionCanceledSensorEStop");
        if (_motor->AlertReg().bit.MotionCanceledMotorDisabled)
            ClearCore::ConnectorUsb.SendLine(" - MotionCanceledMotorDisabled");
        if (_motor->AlertReg().bit.MotorFaulted) {
            ClearCore::ConnectorUsb.SendLine(" - MotorFaulted");
            _motor->EnableRequest(false);
            Delay_ms(100);
            _motor->EnableRequest(true);
            Delay_ms(100);
        }
        _motor->ClearAlerts();
        ClearCore::ConnectorUsb.SendLine("[Y-Axis] Alerts cleared");
    }
    else {
        ClearCore::ConnectorUsb.SendLine("[Y-Axis] No alerts present");
    }
}

bool YAxis::StartHoming() {
    if (!_isSetup) {
        ClearCore::ConnectorUsb.SendLine("[Y-Axis] Cannot start homing: not setup");
        return false;
    }
    _motor->EnableRequest(true);
    if (_motor->StatusReg().bit.AlertsPresent)
        ClearAlerts();

    bool ok = _homingHelper->start();
    if (ok) {
        _isHomed = false;
        ClearCore::ConnectorUsb.SendLine("[Y-Axis] Homing started");
    }
    return ok;
}

void YAxis::Update() {
    if (!_isSetup)
        return;

    // Update position and torque
    _currentPos = static_cast<float>(_motor->PositionRefCommanded()) / _stepsPerInch;
    _isMoving = !_motor->StepsComplete();
    _torquePct = _dynamicFeed->updateTorqueMeasurement();

    // --- DEBUG: Log torque and feed state for troubleshooting ---
    static uint32_t lastDebugLog = 0;
    uint32_t now = ClearCore::TimingMgr.Milliseconds();
    if (now - lastDebugLog > 500) {
        lastDebugLog = now;
        ClearCore::ConnectorUsb.Send("[Y-Axis][DEBUG] inTorqueControlFeed: ");
        ClearCore::ConnectorUsb.Send(IsInTorqueControlledFeed() ? "true" : "false");
        ClearCore::ConnectorUsb.Send(", isMoving: ");
        ClearCore::ConnectorUsb.Send(_isMoving ? "true" : "false");
        ClearCore::ConnectorUsb.Send(", torquePct: ");
        ClearCore::ConnectorUsb.Send(_torquePct, 1);
        ClearCore::ConnectorUsb.Send(", torqueTarget: ");
        ClearCore::ConnectorUsb.Send(GetTorqueTarget(), 1);
        ClearCore::ConnectorUsb.Send(", feedRate: ");
        ClearCore::ConnectorUsb.Send(DebugGetCurrentFeedRate() * 100.0f, 1);
        ClearCore::ConnectorUsb.SendLine("%");
    }
    // ------------------------------------------------------------

    // Check if we're in torque-controlled feed mode
    if (IsInTorqueControlledFeed()) {
        // Let dynamic feed module handle updates and check for completion
        if (_dynamicFeed->update(_currentPos)) {
            Stop();
        }
        return;
    }

    // Homing state
    if (_homingHelper->isBusy()) {
        _homingHelper->process();
    }
    else if (!_isHomed && !_homingHelper->hasFailed()) {
        _isHomed = true;
        _hasBeenHomed = true;
        EncoderPositionTracker::Instance().resetPositionAfterHoming();
        ClearCore::ConnectorUsb.SendLine("[Y-Axis] Homing complete, encoder position reset");
    }
}

bool YAxis::MoveTo(float positionInches, float velocityScale) {
    if (!_isSetup) {
        ClearCore::ConnectorUsb.SendLine("[Y-Axis] MoveTo failed: not setup");
        return false;
    }
    if (_homingHelper->isBusy()) {
        ClearCore::ConnectorUsb.SendLine("[Y-Axis] MoveTo failed: homing in progress");
        return false;
    }
    if (!_hasBeenHomed) {
        ClearCore::ConnectorUsb.SendLine("[Y-Axis] MoveTo failed: axis not homed yet");
        return false;
    }

    float desired = positionInches;
    if (desired < 0.0f || desired > MAX_Y_INCHES) {
        ClearCore::ConnectorUsb.SendLine("[Y-Axis] Soft limit reached, stopping");
        _motor->MoveStopAbrupt();
        _isMoving = false;
        return false;
    }

    _targetPos = desired;
    _isMoving = true;

    int32_t tgtSteps = static_cast<int32_t>(_targetPos * _stepsPerInch);
    int32_t curSteps = _motor->PositionRefCommanded();
    int32_t delta = tgtSteps - curSteps;
    if (delta == 0) return true;

    _motor->VelMax(static_cast<uint32_t>(MAX_VELOCITY * velocityScale));
    ClearCore::ConnectorUsb.Send("[Y-Axis] MoveTo: ");
    ClearCore::ConnectorUsb.Send(_targetPos);
    ClearCore::ConnectorUsb.SendLine(" inches");
    return _motor->Move(delta);
}

void YAxis::Stop() {
    if (_isMoving) {
        _motor->MoveStopDecel();
        _isMoving = false;
    }
}

void YAxis::EmergencyStop() {
    _motor->MoveStopAbrupt();
    _isMoving = false;
    
    // Make sure to abort any dynamic feed operations
    AbortTorqueControlledFeed();
}

float YAxis::GetPosition() const {
    return static_cast<float>(_motor->PositionRefCommanded()) / _stepsPerInch;
}

float YAxis::GetTorquePercent() const {
    return _dynamicFeed->getTorquePercent();
}

void YAxis::SetTorqueTarget(float targetPercent) {
    _dynamicFeed->setTorqueTarget(targetPercent);
}

float YAxis::GetTorqueTarget() const {
    return _dynamicFeed->getTorqueTarget();
}

bool YAxis::StartTorqueControlledFeed(float targetPosition, float initialVelocityScale) {
    if (!_isSetup) {
        ClearCore::ConnectorUsb.SendLine("[Y-Axis] Cannot start torque feed: not setup");
        return false;
    }
    if (_homingHelper->isBusy()) {
        ClearCore::ConnectorUsb.SendLine("[Y-Axis] Cannot start torque feed: homing in progress");
        return false;
    }
    if (!_hasBeenHomed) {
        ClearCore::ConnectorUsb.SendLine("[Y-Axis] Cannot start torque feed: axis not homed");
        return false;
    }

    _isMoving = true;
    return _dynamicFeed->start(targetPosition, initialVelocityScale);
}

void YAxis::UpdateFeedRate(float newVelocityScale) {
    if (!_isSetup || !_isMoving) return;
    _dynamicFeed->updateFeedRate(newVelocityScale);
}

bool YAxis::IsInTorqueControlledFeed() const {
    return _dynamicFeed->isActive();
}

void YAxis::AbortTorqueControlledFeed() {
    if (_dynamicFeed->isActive()) {
        _dynamicFeed->abort();
        Stop();
    }
}

float YAxis::DebugGetCurrentFeedRate() const {
    return _dynamicFeed->getCurrentFeedRate();
}

float YAxis::DebugGetTorquePercent() const {
    return _dynamicFeed->getTorquePercent();
}

bool YAxis::DebugIsInTorqueControlFeed() const {
    return _dynamicFeed->isActive();
}

bool YAxis::IsMoving() const { return !_motor->StepsComplete(); }
bool YAxis::IsHomed()  const { return _isHomed; }
bool YAxis::IsHoming() const { return _homingHelper->isBusy(); }
