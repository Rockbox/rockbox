#pragma once
#include <stdint.h>

/* [1] P.357 Table 4-242 Storage lingo commands */

enum IAPStorageCommandID {
    IAPStorageCommandID_IPodAck             = 0x00, /* from dev, ipod-ack.h */
    IAPStorageCommandID_GetIPodCaps         = 0x01, /* from acc, no payload */
    IAPStorageCommandID_RetIPodCaps         = 0x02, /* from dev, ipod-caps.h */
    IAPStorageCommandID_RetIPodFileHandle   = 0x04, /* from dev, ipod-file-handle.h */
    IAPStorageCommandID_WriteIPodFileData   = 0x07, /* from acc, write-ipod-file-data.h */
    IAPStorageCommandID_CloseIPodFile       = 0x08, /* from acc, ipod-file-handle.h */
    IAPStorageCommandID_GetIPodFreeSpace    = 0x10, /* from acc, no payload */
    IAPStorageCommandID_RetIPodFreeSpace    = 0x11, /* from dev, ipod-free-space.h */
    IAPStorageCommandID_OpenIPodFeatureFile = 0x12, /* from acc, ipod-file-handle.h */
    IAPStorageCommandID_AccessoryAck        = 0x80, /* from acc, acc-ack.h */
    IAPStorageCommandID_GetAccessoryCaps    = 0x81, /* from dev, no payload */
    IAPStorageCommandID_RetAccessoryCaps    = 0x82, /* from acc, */
};

#include "storage/acc-ack.h"
#include "storage/accessory-caps.h"
#include "storage/ipod-ack.h"
#include "storage/ipod-caps.h"
#include "storage/ipod-file-handle.h"
#include "storage/ipod-free-space.h"
#include "storage/write-ipod-file-data.h"
