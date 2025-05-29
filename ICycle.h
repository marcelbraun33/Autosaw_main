// controllers/ICycle.h
#pragma once

#include <cstdint>

/// Error codes for a cycle (always None, since no timeouts/faults are implemented here)
enum class CycleError : uint8_t {
    None,
    AxisFault,
    Unknown
};

/// Interface for a single motion‐cycle phase
class ICycle {
public:
    virtual ~ICycle() = default;

    /// Kick off this cycle (nonblocking).
    virtual void start() = 0;

    /// Abort immediately.
    virtual void stop() = 0;

    /// True once the motion completed successfully.
    virtual bool isDone() const = 0;

    /// True if this cycle hit an error.
    virtual bool hasError() const = 0;

    /// If hasError()==true, the error code.
    virtual CycleError error() const = 0;
};

