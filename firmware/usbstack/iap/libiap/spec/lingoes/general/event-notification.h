#pragma once
#include <stdint.h>

/* [1] P.184 Table 3-112 Notification bitmask bits */

enum IAPEventNotificationEvents {
    IAPSetEventNotificationEvents_FlowControl         = 1 << 2,
    IAPSetEventNotificationEvents_RadioTagging        = 1 << 3,
    IAPSetEventNotificationEvents_Camera              = 1 << 4,
    IAPSetEventNotificationEvents_ChargingInfo        = 1 << 5,
    IAPSetEventNotificationEvents_DatabaseChanged     = 1 << 9,
    IAPSetEventNotificationEvents_AppBundleName       = 1 << 10,
    IAPSetEventNotificationEvents_SessionSpaceAvail   = 1 << 11,
    IAPSetEventNotificationEvents_CommandComplete     = 1 << 13,
    IAPSetEventNotificationEvents_IPodOutMode         = 1 << 15,
    IAPSetEventNotificationEvents_BluetoothConnection = 1 << 17,
    IAPSetEventNotificationEvents_AppDisplayName      = 1 << 19,
    IAPSetEventNotificationEvents_AssistiveTouch      = 1 << 20,
};

struct IAPSetEventNotificationPayload {
    uint64_t mask; /* IAPEventNotificationEvents */
} __attribute__((packed));

struct IAPRetEventNotificationPayload {
    uint64_t mask; /* IAPEventNotificationEvents */
} __attribute__((packed));

struct IAPRetSupportedEventNotificationPayload {
    uint64_t mask; /* IAPEventNotificationEvents */
} __attribute__((packed));
