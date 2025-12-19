#pragma once
#include <stdint.h>

enum IAPEndIDPSStatus {
    IAPEndIDPSStatus_Success          = 0x00,
    IAPEndIDPSStatus_Reset            = 0x01,
    IAPEndIDPSStatus_Abort            = 0x02,
    IAPEndIDPSStatus_AnotherTransport = 0x03,
};

struct IAPEndIDPSPayload {
    uint8_t status; /* IAPEndIDPSStatus */
};

enum IAPIDPSStatus {
    /* IAPEndIDPSStatus_Success */
    IAPIDPSStatus_Success                      = 0x00,
    IAPIDPSStatus_RequiredTokenRejected        = 0x01,
    IAPIDPSStatus_RequiredTokenMissing         = 0x02,
    IAPIDPSStatus_RequiredTokenRejectedMissing = 0x03,
    /* IAPEndIDPSStatus_Reset */
    IAPIDPSStatus_NotTimedOut = 0x04,
    IAPIDPSStatus_TimedOut    = 0x05,
    /* IAPEndIDPSStatus_Abort */
    IAPIDPSStatus_Aborted = 0x06,
    /* IAPEndIDPSStatus_AnotherTransport */
    IAPIDPSStatus_Failed = 0x07,

};

struct IAPIDPSStatusPayload {
    uint8_t status; /* IAPIDPSStatus */
};
