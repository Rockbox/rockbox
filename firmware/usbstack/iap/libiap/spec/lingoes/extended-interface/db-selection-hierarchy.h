#pragma once
#include <stdint.h>

/* [1] P.443 5.1.54 Command 0x003B: ResetDBSelectionHierarchy */

enum IAPResetDBSelectionHierarchySelection {
    IAPResetDBSelectionHierarchySelection_Audio = 0x01,
    IAPResetDBSelectionHierarchySelection_Video = 0x02,
};

struct IAPResetDBSelectionHierarchyPayload {
    uint8_t selection; /* IAPResetDBSelectionHierarchySelection */
} __attribute__((packed));
