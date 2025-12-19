#pragma once
#include <stdint.h>

enum IAPFIDTokenValuesMicrophoneCaps {
    IAPFIDTokenValuesMicrophoneCaps_StereoInput           = 1 << 0,
    IAPFIDTokenValuesMicrophoneCaps_StereoMonoInputSwitch = 1 << 1,
    IAPFIDTokenValuesMicrophoneCaps_VariableRecordLevel   = 1 << 2,
    IAPFIDTokenValuesMicrophoneCaps_RecordLevelLimit      = 1 << 3,
    IAPFIDTokenValuesMicrophoneCaps_DuplexAudio           = 1 << 4,
};

struct IAPFIDTokenValuesMicrophoneCapsToken {
    uint8_t  length;    /* = 0x06 */
    uint8_t  type;      /* = 0x01 */
    uint8_t  subtype;   /* = 0x00 */
    uint32_t caps_bits; /* IAPFIDTokenValuesMicrophoneCaps */
} __attribute__((packed));

struct IAPFIDTokenValuesMicrophoneCapsAck {
    uint8_t length;  /* = 0x03 */
    uint8_t type;    /* = 0x01 */
    uint8_t subtype; /* = 0x00 */
    uint8_t status;  /* IAPFIDTokenValuesAckStatus */
} __attribute__((packed));
