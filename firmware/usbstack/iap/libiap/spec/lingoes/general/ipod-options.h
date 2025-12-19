#pragma once
#include <stdint.h>

enum IAPRetIPodOptionsState {
    IAPRetIPodOptionsState_VideoOutputSupport        = 1 << 0,
    IAPRetIPodOptionsState_SetiPodPreferencesSupport = 1 << 1,
};

struct IAPRetIPodOptionsPayload {
    uint64_t state; /* IAPRetIPodOptionsState */
} __attribute__((packed));
