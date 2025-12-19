#pragma once
#include <stdint.h>

enum IAPCameraButtonStatus {
    IAPCameraButtonStatus_Up   = 0x00,
    IAPCameraButtonStatus_Down = 0x01,
};

struct IAPCameraButtonStatusPayload {
    uint8_t status; /* IAPCameraButtonStatus */
};
