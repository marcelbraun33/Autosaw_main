
// HomingHelper.cpp
#include "HomingHelper.h"
#include "ScreenManager.h"
#include <ClearCore.h>

HomingHelper::HomingHelper(const HomingParams& p)
    : _p(p)
    , _state(State::Idle)
    , _stamp(0)
    , _startTime(0)
    , _lastState(State::Idle)
    , _hasBeenHomed(false)
{
    // Ensure fast phase speed is tuned relative to slow
    _p.fastVel = static_cast<uint32_t>(_p.slowVel * 2);
}

bool HomingHelper::start() {
    if (_state != State::Idle) return false;
    if (_p.motor->StatusReg().bit.AlertsPresent) {
        _p.motor->ClearAlerts();
    }
    _startTime = _stamp = ClearCore::TimingMgr.Milliseconds();

    if (_hasBeenHomed) {
        // Soft-limit rehome: move from current position back to zero
        int32_t currentSteps = _p.motor->PositionRefCommanded();
        if (currentSteps != 0) {
            // use three times the slow approach speed for rehome
            uint32_t rehomeSpeed = _p.slowVel * 3;
            _p.motor->VelMax(rehomeSpeed);
            _p.motor->Move(-currentSteps);
            _state = State::Finalize;
        }
        else {
            // already at zero
            _p.motor->PositionRefSet(0);
            _state = State::Idle;
        }
        return true;
    }

    // First true homing sequence
    _state = State::FastApproach;
    return true;
}

void HomingHelper::process() {
    uint32_t now = ClearCore::TimingMgr.Milliseconds();
    if (_state == State::Idle || _state == State::Failed) return;

    // Global timeout
    if (now - _stamp > _p.timeoutMs) {
        ClearCore::ConnectorUsb.SendLine("[Homing] TIMEOUT – aborting");
        ScreenManager::Instance().ShowManualMode();
        _state = State::Failed;
        return;
    }

    // Log transitions
    if (_state != _lastState) {
        uint32_t elapsed = now - _startTime;
        switch (_state) {
        case State::FastApproach:
            ClearCore::ConnectorUsb.Send("[Homing][+"); ClearCore::ConnectorUsb.Send(elapsed);
            ClearCore::ConnectorUsb.SendLine("ms] FastApproach"); break;
        case State::Dwell:
            ClearCore::ConnectorUsb.Send("[Homing][+"); ClearCore::ConnectorUsb.Send(elapsed);
            ClearCore::ConnectorUsb.SendLine("ms] Dwell"); break;
        case State::SlowApproach:
            ClearCore::ConnectorUsb.Send("[Homing][+"); ClearCore::ConnectorUsb.Send(elapsed);
            ClearCore::ConnectorUsb.SendLine("ms] SlowApproach"); break;
        case State::WaitForStop:
            ClearCore::ConnectorUsb.Send("[Homing][+"); ClearCore::ConnectorUsb.Send(elapsed);
            ClearCore::ConnectorUsb.SendLine("ms] WaitForStop"); break;
        case State::Backoff:
            ClearCore::ConnectorUsb.Send("[Homing][+"); ClearCore::ConnectorUsb.Send(elapsed);
            ClearCore::ConnectorUsb.SendLine("ms] Backoff"); break;
        case State::Finalize:
            ClearCore::ConnectorUsb.Send("[Homing][+"); ClearCore::ConnectorUsb.Send(elapsed);
            ClearCore::ConnectorUsb.SendLine("ms] Finalize"); break;
        default: break;
        }
        _lastState = _state;
    }

    // Clear alerts
    if (_p.motor->StatusReg().bit.AlertsPresent) {
        _p.motor->ClearAlerts();
    }

    switch (_state) {
    case State::FastApproach:
        _p.motor->VelMax(_p.fastVel);
        _p.motor->MoveVelocity(-static_cast<int32_t>(_p.fastVel));
        _stamp = now;
        _state = State::Dwell;
        break;

    case State::Dwell:
        if (now - _stamp >= _p.dwellMs) {
            _p.motor->VelMax(_p.slowVel);
            _p.motor->MoveVelocity(-static_cast<int32_t>(_p.slowVel));
            _stamp = now;
            _state = State::SlowApproach;
        }
        break;

    case State::SlowApproach:
        _state = State::WaitForStop;
        break;

    case State::WaitForStop:
        if (_p.motor->HlfbState() == MotorDriver::HLFB_ASSERTED) {
            _p.motor->MoveStopAbrupt();
            _stamp = now;
            _state = State::Backoff;
        }
        break;

    case State::Backoff: {
        int32_t steps = static_cast<int32_t>(_p.backoffUnits * _p.stepsPerUnit);
        _p.motor->Move(steps);
        _state = State::Finalize;
        break;
    }

    case State::Finalize:
        if (_p.motor->StepsComplete()) {
            _p.motor->PositionRefSet(0);
            _hasBeenHomed = true;
            _state = State::Idle;
        }
        break;

    default: break;
    }
}

bool HomingHelper::isBusy()   const { return _state != State::Idle && _state != State::Failed; }
bool HomingHelper::hasFailed() const { return _state == State::Failed; }
