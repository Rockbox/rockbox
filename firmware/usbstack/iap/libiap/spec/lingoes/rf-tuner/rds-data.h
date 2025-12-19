#pragma once
#include <stdint.h>

/* [1] P.311 4.7.33 Command 0x1C: GetRdsData */

struct IAPGetRdsDataPayload {
    uint8_t type; /* IAPRdsData{Parsed,Raw}Type */
};

struct IAPRetRdsDataPayload {
    uint8_t type;   /* IAPRdsData{Parsed,Raw}Type */
    uint8_t data[]; /* TODO: add definitions */
};

struct IAPReadyNotifyPayload {
    uint8_t type;   /* IAPRdsData{Parsed,Raw}Type */
    uint8_t data[]; /* TODO: add definitions */
};
