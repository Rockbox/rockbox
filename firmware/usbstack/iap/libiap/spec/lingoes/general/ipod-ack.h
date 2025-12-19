#pragma once
#include <stdint.h>

/* [1] P.124 3.3.2 Command 0x02: iPodAck */

enum IAPAckStatus {
    IAPAckStatus_Success                     = 0x00,
    IAPAckStatus_EUnknownDatabaseOrSessionID = 0x01,
    IAPAckStatus_ECommandFailed              = 0x02,
    IAPAckStatus_EOutOfResource              = 0x03,
    IAPAckStatus_EBadParameter               = 0x04,
    IAPAckStatus_EUnknownID                  = 0x05,
    IAPAckStatus_CommandPending              = 0x06,
    IAPAckStatus_ENotAuthenticated           = 0x07,
    IAPAckStatus_EBadAuthVersion             = 0x08,
    IAPAckStatus_EAccPowerModeRequestFailed  = 0x09,
    IAPAckStatus_EInvalidCert                = 0x0A,
    IAPAckStatus_EInvalidCertPerm            = 0x0B,
    IAPAckStatus_EFileInUse                  = 0x0C,
    IAPAckStatus_EInvalidFileHandle          = 0x0D,
    IAPAckStatus_EDirectoryNotEmpty          = 0x0E,
    IAPAckStatus_EOperationTimedOut          = 0x0F,
    IAPAckStatus_ECommandUnavailable         = 0x10,
    IAPAckStatus_EBadWire                    = 0x11,
    IAPAckStatus_ESelectionNotGenius         = 0x12,
    IAPAckStatus_MultiDataRecvSuccess        = 0x13,
    IAPAckStatus_ELingoBusy                  = 0x14,
    IAPAckStatus_EMaxNumAcc                  = 0x15,
    IAPAckStatus_HIDIndexInUse               = 0x16,
    IAPAckStatus_EDropped                    = 0x17,
    IAPAckStatus_EInvalidVideoSettings       = 0x18,
};

struct IAPIPodAckPayload {
    uint8_t status; /* IAPAckStatus */
    uint8_t id;
} __attribute__((packed));

struct IAPIPodAckCommandPendingPayload {
    uint8_t  status; /* = IAPAckStatus_CommandPending */
    uint8_t  id;
    uint32_t time_ms;
} __attribute__((packed));

/* [1] P.389 Table 4-292 Multisection iPodAck packet */
struct IAPIPodAckMultiSectPayload {
    uint8_t  status; /* IAPAckStatus_IAPAckStatus_MultiDataRecvSuccess */
    uint8_t  id;
    uint16_t sect_cur;
};

struct IAPIPodAckEDroppedPayload {
    uint8_t  status; /* = IAPAckStatus_EDropped */
    uint8_t  id;     /* = IAPCommandIDGeneral_AccessoryDataTransfer */
    uint16_t session_id;
    uint32_t num_dropped_bytes;
} __attribute__((packed));
