#pragma once
#include <stdint.h>

/* [1] P.336 Table 4-212 Sports lingo command summary */
enum IAPSportsCommandID {
    IAPSportsCommandID_AccessoryAck        = 0x00, /* from acc, general/acc-ack.h */
    IAPSportsCommandID_GetAccessoryVersion = 0x01, /* from dev, no payload */
    IAPSportsCommandID_RetAccessoryVersion = 0x02, /* from acc, accessory-version.h */
    IAPSportsCommandID_GetAccessoryCaps    = 0x03, /* from dev, no payload */
    IAPSportsCommandID_RetAccessoryCaps    = 0x04, /* from acc, accessory-caps.h */
    IAPSportsCommandID_IPodAck             = 0x80, /* from dev, general/ipod-ack.h */
    IAPSportsCommandID_GetIPodCaps         = 0x83, /* from acc, no payload */
    IAPSportsCommandID_RetIPodCaps         = 0x84, /* from dev, ipod-caps.h */
    IAPSportsCommandID_GetUserIndex        = 0x85, /* from acc, no payload */
    IAPSportsCommandID_RetUserIndex        = 0x86, /* from dev, user-index.h */
    IAPSportsCommandID_GetUserData         = 0x88, /* from acc, user-data.h */
    IAPSportsCommandID_RetUserData         = 0x89, /* from dev, user-data.h */
    IAPSportsCommandID_SetUserData         = 0x8A, /* from acc, user-data.h */
};

#include "sports/accessory-caps.h"
#include "sports/accessory-version.h"
#include "sports/ipod-caps.h"
#include "sports/user-data.h"
#include "sports/user-index.h"
