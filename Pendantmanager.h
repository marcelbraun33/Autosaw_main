// PendantManager.h
#pragma once

#include <cstdint>
#include <ClearCore.h>

/// Manages the pendant selector and screen switching
class PendantManager {
public:
    static PendantManager& Instance();

    void Init();
    void SetEnabled(bool enabled);
    bool IsEnabled() const;
    void Update();
    void SyncScreenToKnob();
    int  ReadSelector() const;
    int  LastKnownSelector() const;

    /// Reset the last-known selector so no “phantom” triggers
    void SetLastKnownSelector(int selector);

private:
    PendantManager() = default;
    ~PendantManager() = default;
    PendantManager(const PendantManager&) = delete;
    PendantManager& operator=(const PendantManager&) = delete;

    bool    active = false;
    int     lastSelectorValue = -1;   // only one declaration
    int     _pendingValue = -1;
    uint32_t _pendingStartUs = 0;
    static constexpr uint32_t DEBOUNCE_US = 100000;

    void HandleSelectorChange(int selector);
};
