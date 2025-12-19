#pragma once
#include <stdint.h>

/* [1] P.548 Table C-37 Accessory Power lingo command summary */
enum IAPAccessoryPowerCommandID {
    IAPAccessoryPowerCommandID_BeginHighPower = 0x02,
    IAPAccessoryPowerCommandID_EndHighPower   = 0x03,
};
