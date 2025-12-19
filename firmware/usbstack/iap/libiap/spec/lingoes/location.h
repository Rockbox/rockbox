#pragma once
#include <stdint.h>

/* [1] P.371 Table 4-266 Location lingo commands */
enum IAPLocationCommandID {
    IAPLocationCommandID_AccessoryAck        = 0x00, /* from acc */
    IAPLocationCommandID_GetAccessoryCaps    = 0x01, /* from dev */
    IAPLocationCommandID_AccessoryCaps       = 0x02, /* from acc */
    IAPLocationCommandID_GetAccessoryControl = 0x03, /* from dev */
    IAPLocationCommandID_RetAccessoryControl = 0x04, /* from acc */
    IAPLocationCommandID_SetAccessoryControl = 0x05, /* from dev */
    IAPLocationCommandID_GetAccessoryData    = 0x06, /* from dev */
    IAPLocationCommandID_RetAccessoryData    = 0x07, /* from acc */
    IAPLocationCommandID_SetAccessoryData    = 0x08, /* from dev */
    IAPLocationCommandID_AsyncAccessoryData  = 0x09, /* from acc */
    IAPLocationCommandID_IPodAck             = 0x80, /* from dev */
};

/* TODO: add payload definitions */
