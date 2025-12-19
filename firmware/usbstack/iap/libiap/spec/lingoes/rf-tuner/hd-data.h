#pragma once
#include <stdint.h>

/* [1] P.319 4.7.45 Command 0x2B: RetHDDataReadyStatus */

enum IAPHDDataType {
    IAPHDDataType_PSD                 = 0x00,
    IAPHDDataType_SISStationIDNumber  = 0x02,
    IAPHDDataType_SISStationNameShort = 0x03,
    IAPHDDataType_SISStationNameLong  = 0x04,
    IAPHDDataType_SISALFN             = 0x05,
    IAPHDDataType_SISStationLocation  = 0x06,
    IAPHDDataType_SISStationMessage   = 0x07,
    IAPHDDataType_SISSlogan           = 0x08,
    IAPHDDataType_SISParameterMessage = 0x09,
};

struct IAPRetHDDataReadyStatusPayload {
    uint32_t status; /* 1 << IAPHDDataType | ... */
} __attribute__((packed));

struct IAPGetHDDataPayload {
    uint8_t type; /* IAPHDDataType */
} __attribute__((packed));

struct IAPRetHDDataPayload {
    uint8_t type;   /* IAPHDDataType */
    uint8_t data[]; /* TODO: add difinitions */
} __attribute__((packed));

struct IAPRetHDDataNotifyMaskPayload {
    uint32_t status; /* 1 << IAPHDDataType | ... */
} __attribute__((packed));

struct IAPSetHDDataNotifyMaskPayload {
    uint32_t status; /* 1 << IAPHDDataType | ... */
} __attribute__((packed));

struct IAPHDDataReadyNotifyPayload {
    uint8_t type;   /* IAPHDDataType */
    uint8_t data[]; /* TODO: add difinitions */
} __attribute__((packed));
