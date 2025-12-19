#pragma once
#include <stdint.h>

enum IAPLocalicationInfoType {
    IAPLocalicationInfoType_Language = 0x00,
    IAPLocalicationInfoType_Region   = 0x01,
};

struct IAPGetLocalizationInfoPayload {
    uint8_t type; /* IAPLocalicationInfoType */
} __attribute__((packed));

struct IAPRetLocalizationInfoPayload {
    uint8_t type; /* IAPLocalicationInfoType */
    char    value[];
} __attribute__((packed));
