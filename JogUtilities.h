#pragma once
#include "CutData.h"
#include "MotionController.h"
#include "Config.h"

// Utility namespace for jog-related actions
namespace JogUtilities {

    // Move to zero (for X or Y axis, depending on context)
    inline void GoToZero(CutData& cutData, AxisId axis) {
        float target = cutData.useStockZero ? cutData.positionZero : 0.0f;
        MotionController::Instance().moveToWithRate(axis, target, 0.5f);
    }

    // Increment position
    inline void Increment(CutData& cutData, AxisId axis) {
        float cur = MotionController::Instance().getAbsoluteAxisPosition(axis);
        MotionController::Instance().moveTo(axis, cur + cutData.increment, 1.0f);
    }

    // Decrement position
    inline void Decrement(CutData& cutData, AxisId axis) {
        float cur = MotionController::Instance().getAbsoluteAxisPosition(axis);
        MotionController::Instance().moveTo(axis, cur - cutData.increment, 1.0f);
    }

}
