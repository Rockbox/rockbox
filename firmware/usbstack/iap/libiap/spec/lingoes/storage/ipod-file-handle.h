#pragma once
#include <stdint.h>

/* [1] P.360 4.11.6 Command 0x04: RetiPodFileHandle */

struct IAPRetIPodFileHandlePayload {
    uint8_t handle;
} __attribute__((packed));

struct IAPCloseIPodFilePayload {
    uint8_t handle;
} __attribute__((packed));

enum IAPOpenIPodFeatureFileFeatureType {
    IAPOpenIPodFeatureFileFeatureType_TadioTagging           = 0x01,
    IAPOpenIPodFeatureFileFeatureType_CardioEquipmentWorkout = 0x02,
};

enum IAPOpenIPodFeatureFileOptionsMask {
    IAPOpenIPodFeatureFileOptionsMask_AppendFileData  = 1 << 0,
    IAPOpenIPodFeatureFileOptionsMask_AppendIPodInfo  = 1 << 1,
    IAPOpenIPodFeatureFileOptionsMask_InsertSignature = 1 << 3,
};

struct IAPOpenIPodFeatureFilePayload {
    uint8_t  feature_type; /* IAPOpenIPodFeatureFileFeatureType */
    uint32_t options_mask; /* IAPOpenIPodFeatureFileOptionsMask */
    uint8_t  file_data[];
} __attribute__((packed));

