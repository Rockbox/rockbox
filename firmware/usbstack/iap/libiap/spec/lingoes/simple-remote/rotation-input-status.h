#pragma once
#include <stdint.h>

enum IAPRotationInputStatusControllerType {
    IAPRotationInputStatusControllerType_FreeWheel = 0x00,
};

enum IAPRotationInputStatusRotationDirection {
    IAPRotationInputStatusRotationDirection_Left  = 0x00, /* CCW */
    IAPRotationInputStatusRotationDirection_Right = 0x01, /* CW */
};

enum IAPRotationInputStatusRotationAction {
    IAPRotationInputStatusRotationAction_Completed  = 0x00,
    IAPRotationInputStatusRotationAction_InProgress = 0x01,
    IAPRotationInputStatusRotationAction_Repeat     = 0x02,
};

enum IAPRotationInputStatusRotationType {
    IAPRotationInputStatusRotationType_Detent = 0x00,
    IAPRotationInputStatusRotationType_Degree = 0x01,
};

struct IAPRotationInputStatusPayload {
    uint32_t user_action_duration_ms;
    uint8_t  source;             /* IAPIPodOutButtonStatusSource */
    uint8_t  controller_type;    /* IAPRotationInputStatusControllerType */
    uint8_t  rotation_direction; /* IAPRotationInputStatusRotationDirection */
    uint8_t  rotation_action;    /* IAPRotationInputStatusRotationAction */
    uint8_t  rotation_type;      /* IAPRotationInputStatusRotationType */
    uint16_t detents_or_degrees_moved;
    uint16_t max_detents_or_degrees_moved;
} __attribute__((packed));
