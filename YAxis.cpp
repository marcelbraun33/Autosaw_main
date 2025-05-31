// YAxis.cpp
#include "YAxis.h"
#include "Config.h"
#include "ClearCore.h"

static constexpr float MAX_ACCELERATION = 100000.0f;  // steps/s^2

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
    params.dwellMs = 100;                  // shortened dwell
    params.backoffUnits = HOMING_BACKOFF_INCH;
    params.timeoutMs = HOMING_TIMEOUT_MS;

    _homingHelper = new HomingHelper(params);
}

YAxis::~YAxis() {
    delete _homingHelper;
}

void YAxis::Jog(float deltaInches, float velocityScale) {
    MoveTo(GetPosition() + deltaInches, velocityScale);
}

void YAxis::Setup() {
    _motor->HlfbMode(MotorDriver::HLFB_MODE_HAS_BIPOLAR_PWM);
    _motor->HlfbCarrier(MotorDriver::HLFB_CARRIER_482_HZ);
    _motor->VelMax(MAX_VELOCITY_Y);
    _motor->AccelMax(MAX_ACCELERATION);
    ClearAlerts();
    // don't enable yet—only on-demand in StartHoming()
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
    // enable on-demand to prevent MSP auto-homing
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

    // feedback
    _currentPos = static_cast<float>(_motor->PositionRefCommanded()) / _stepsPerInch;
    _isMoving = !_motor->StepsComplete();
    _torquePct = GetTorquePercent();

    // homing state
    if (_homingHelper->isBusy()) {
        _homingHelper->process();
    }
    else if (!_isHomed && !_homingHelper->hasFailed()) {
        _isHomed = true;
        _hasBeenHomed = true;
        ClearCore::ConnectorUsb.SendLine("[Y-Axis] Homing complete");
    }
}
// Add this to YAxis.cpp:
float YAxis::GetActualPosition() const {
    // If you have actual position feedback from the motor:
    // return static_cast<float>(_motor->PositionRefFeedback()) / _stepsPerInch;
    
    // If not, use the commanded position as fallback:
    return GetPosition(); // This calls the existing GetPosition() method
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
    // soft-limit clamp and stop
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

    _motor->VelMax(static_cast<uint32_t>(MAX_VELOCITY_Y * velocityScale));
   // ClearCore::ConnectorUsb.Send("[Y-Axis] MoveTo: ");
   // ClearCore::ConnectorUsb.Send(_targetPos);
  //  ClearCore::ConnectorUsb.SendLine(" inches");
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
}

float YAxis::GetPosition() const {
    return static_cast<float>(_motor->PositionRefCommanded()) / _stepsPerInch;
}

float YAxis::GetTorquePercent() const {
    return (_motor->HlfbState() == MotorDriver::HLFB_ASSERTED) ? 100.0f : 0.0f;
}

bool YAxis::IsMoving() const { return !_motor->StepsComplete(); }
bool YAxis::IsHomed()  const { return _isHomed; }
bool YAxis::IsHoming() const { return _homingHelper->isBusy(); }

void YAxis::UpdateVelocity(float velocityScale) {
    if (_motor && _isSetup) {
        uint32_t newVelMax = static_cast<uint32_t>(MAX_VELOCITY_Y * velocityScale);
        _motor->VelMax(newVelMax);

        // Debug output
       // ClearCore::ConnectorUsb.Send("[YAxis::UpdateVelocity] Called. ");
       // ClearCore::ConnectorUsb.Send("velocityScale: ");
       // ClearCore::ConnectorUsb.Send(velocityScale, 4); // 4 decimal places
       // ClearCore::ConnectorUsb.Send("  newVelMax: ");
       // ClearCore::ConnectorUsb.Send(newVelMax);
       // ClearCore::ConnectorUsb.Send("  IsMoving: ");
       // ClearCore::ConnectorUsb.SendLine(_isMoving ? "YES" : "NO");

        // Optional: print current position
       // ClearCore::ConnectorUsb.Send("[YAxis::UpdateVelocity] CurrentPos: ");
       // ClearCore::ConnectorUsb.SendLine(GetPosition(), 4);
    }
    else {
        //ClearCore::ConnectorUsb.SendLine("[YAxis::UpdateVelocity] Not ready (_motor or _isSetup false)");
    }
}

