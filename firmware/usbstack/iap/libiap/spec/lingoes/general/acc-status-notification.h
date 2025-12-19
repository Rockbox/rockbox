#pragma once
#include <stdint.h>

struct IAPSetAccStatusNotificationPayload {
    uint32_t mask; /* IAPAccInfoStatusTypes */
} __attribute__((packed));

struct IAPRetAccStatusNotificationPayload {
    uint32_t mask; /* IAPAccInfoStatusTypes */
} __attribute__((packed));

struct IAPAccStatusNotificationPayload {
    uint32_t type; /* IAPAccInfoStatusTypes */
    uint8_t  params[];
} __attribute__((packed));

/* [1] P.181 Table 3-108 AccessoryStatusNotification parameters */
/* TODO: add definitions */
