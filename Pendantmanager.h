#pragma once

#include <ClearCore.h>

class PendantManager {
public:
    static PendantManager& Instance();

    void Init();
    void SetEnabled(bool enabled);
    bool IsEnabled();
    void Update();
    void SyncScreenToKnob();
    int ReadSelector();
    int LastKnownSelector();

private:
    PendantManager() = default;
    ~PendantManager() = default;

    PendantManager(const PendantManager&) = delete;
    PendantManager& operator=(const PendantManager&) = delete;
    PendantManager(PendantManager&&) = delete;
    PendantManager& operator=(PendantManager&&) = delete;

    void HandleSelectorChange(int selector);
    bool active = false;
    int lastSelectorValue = -1;
    int _pendingValue = -1;
    uint32_t _pendingStartUs = 0;
    static constexpr uint32_t DEBOUNCE_US = 100000;
};
