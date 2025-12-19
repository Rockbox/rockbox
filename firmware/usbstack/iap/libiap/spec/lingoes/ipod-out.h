#pragma once
#include <stdint.h>

/* [1] P.366 Table 4-258 iPod Out lingo command summary */
enum IAPIPodOutCommandID {
    IAPIPodOutCommandID_IPodAck                   = 0x00, /* from dev, general/ipod-ack.h */
    IAPIPodOutCommandID_GetIPodOutOptions         = 0x01, /* from acc, ipod-out-options.h */
    IAPIPodOutCommandID_RetIPodOutOptions         = 0x02, /* from dev, ipod-out-options.h */
    IAPIPodOutCommandID_SetIPodOutOptions         = 0x03, /* from acc, ipod-out-options.h */
    IAPIPodOutCommandID_AccessoryStateChangeEvent = 0x04, /* from acc, */
};

#include "ipod-out/accessory-state-change-event.h"
#include "ipod-out/ipod-out-options.h"
