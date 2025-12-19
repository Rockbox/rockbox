#pragma once
#include <stdint.h>

enum IAPAccInfoType {
    IAPAccInfoType_AccInfoCaps              = 0x00, /* G1 -> R2 */
    IAPAccInfoType_AccName                  = 0x01, /* G1 -> R1 */
    IAPAccInfoType_MinDeviceFirmwareVersion = 0x02, /* G2 -> R3 */
    IAPAccInfoType_MinLingoVersion          = 0x03, /* G3 -> R4 */
    IAPAccInfoType_FirmwareVersion          = 0x04, /* G1 -> R5 */
    IAPAccInfoType_HardwareVersion          = 0x05, /* G1 -> R5 */
    IAPAccInfoType_Manufacture              = 0x06, /* G1 -> R2 */
    IAPAccInfoType_ModelNumber              = 0x07, /* G1 -> R2 */
    IAPAccInfoType_SerialNumber             = 0x08, /* G1 -> R2 */
    IAPAccInfoType_MaxPayloadSize           = 0x09, /* G1 -> R6 */
    IAPAccInfoType_SupportedStatusTypes     = 0x0B, /* G1 -> R7 */
};

/* G1 */
struct IAPGetAccInfoPayload {
    uint8_t type; /* IAPAccInfoType */
} __attribute__((packed));

/* G2 */
struct IAPGetAccInfoMinDeviceFirmwareVersionPayload {
    uint8_t  type; /* = IAPAccInfoType_MinDeviceFirmwareVersion */
    uint32_t model_id;
} __attribute__((packed));

/* G3 */
struct IAPGetAccInfoMinLingoVersionPayload {
    uint8_t type; /* = IAPAccInfoType_MinLingoVersion */
    uint8_t lingo_id;
};

/* R1 */
struct IAPRetAccInfoPayload {
    uint8_t type; /* IAPAccInfoType */
    char    value[];
};

/* R2 */
struct IAPRetAccInfoCapsPayload {
    uint8_t  type; /* = IAPAccInfoType_AccInfoCaps */
    uint32_t caps;
} __attribute__((packed));

/* R3 */
struct IAPRetAccInfoMinDeviceFirmwareVersionPayload {
    uint8_t type; /* = IAPAccInfoType_MinDeviceFirmwareVersion */
};

/* R4 */
struct IAPRetAccInfoMinLingoVersionPayload {
    uint8_t type; /* = IAPAccInfoType_MinLingoVersion */
    uint8_t lingo_id;
    uint8_t major;
    uint8_t minor;
};

/* R5 */
struct IAPRetAccInfoFirmHardVersionPayload {
    uint8_t type; /* = IAPAccInfoType_{Firmware,Hardware}Version */
    uint8_t major;
    uint8_t minor;
    uint8_t revision;
} __attribute__((packed));

/* R6 */
struct IAPRetAccInfoMaxPayloadSizePayload {
    uint8_t  type; /* = IAPAccInfoType_MaxPayloadSize */
    uint16_t max_payload_size;
} __attribute__((packed));

enum IAPAccInfoStatusTypes {
    IAPAccInfoStatusTypes_Bluetooth      = 0b0010,
    IAPAccInfoStatusTypes_FaultCondition = 0b0100,
};

/* R7 */
struct IAPRetAccInfoSupportedStatusTypes {
    uint8_t  type;         /* = IAPAccInfoType_SupportedStatusTypes */
    uint32_t status_types; /* IAPAccInfoStatusTypes */
} __attribute__((packed));
