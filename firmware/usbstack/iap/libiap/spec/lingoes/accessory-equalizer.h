#pragma once
#include <stdint.h>

/* [1] P.331 Table 4-200 Accessory Equalizer lingo command summary */
enum IAPAccessoryEqualizerCommandID {
    IAPAccessoryEqualizerCommandID_AccessoryAck      = 0x00, /* from acc, general/acc-ack.h */
    IAPAccessoryEqualizerCommandID_GetCurrentEQIndex = 0x01, /* from dev, no payload */
    IAPAccessoryEqualizerCommandID_RetCurrentEQIndex = 0x02, /* from acc, current-eq-index.h */
    IAPAccessoryEqualizerCommandID_SetCurrentEQIndex = 0x03, /* from dev, current-eq-index.h */
    IAPAccessoryEqualizerCommandID_GetEQSettingCount = 0x04, /* from dev, no payload */
    IAPAccessoryEqualizerCommandID_RetEQSettingCount = 0x05, /* from acc, eq-setting-count.h */
    IAPAccessoryEqualizerCommandID_GetEQIndexName    = 0x06, /* from dev, */
    IAPAccessoryEqualizerCommandID_RetEQIndexName    = 0x07, /* from acc, */
};

#include "accessory-equalizer/current-eq-index.h"
#include "accessory-equalizer/eq-index-name.h"
#include "accessory-equalizer/eq-setting-count.h"
