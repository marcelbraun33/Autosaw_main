#include "HomingHelper.h"
#include "ScreenManager.h"  // for redirect on total timeout

HomingHelper::HomingHelper(const HomingParams& p)
    : _p(p), _state(State::Idle), _stamp(0) {
}

bool HomingHelper::start() {
    if (_state != State::Idle) return false;
    // clear any alerts first
    if (_p.motor->StatusReg().bit.AlertsPresent) {
        _p.motor->ClearAlerts();
    }
    _stamp = ClearCore::TimingMgr.Milliseconds();
    _state = State::FastApproach;
    return true;
}

void HomingHelper::process() {
    uint32_t now = ClearCore::TimingMgr.Milliseconds();
    if (_state == State::Idle) return;

    // global timeout
    if (now - _stamp > _p.timeoutMs) {
        // give user feedback
        ClearCore::ConnectorUsb.SendLine("[HomingHelper] TIMEOUT, aborting");
        // go back to manual screen
        ScreenManager::Instance().ShowManualMode();
        _state = State::Failed;
        return;
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
        // nothing here—just waiting to hit HLFB
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
            _state = State::Idle;
        }
        break;

    case State::Failed:
    case State::Idle:
    default:
        // do nothing
        break;
    }
}

bool HomingHelper::isBusy()   const { return _state != State::Idle && _state != State::Failed; }
bool HomingHelper::hasFailed() const { return _state == State::Failed; }
