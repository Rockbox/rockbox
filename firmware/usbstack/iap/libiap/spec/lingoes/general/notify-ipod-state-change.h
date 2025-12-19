#pragma once
#include <stdint.h>

enum IAPNotifyIPodStateChangeState {
    IAPNotifyIPodStateChangeState_LostContextHibernate = 0x01,
    IAPNotifyIPodStateChangeState_SaveContextHibernate = 0x02,
    IAPNotifyIPodStateChangeState_Sleep                = 0x03,
    IAPNotifyIPodStateChangeState_PowerOn              = 0x04,
};

struct IAPNotifyIPodStateChangePayload {
    uint8_t state; /* IAPNotifyIPodStateChangeState */
} __attribute__((packed));
