#pragma once
#include <stdint.h>

/* [1] P.134 Table 3-24 Device Lingoes Spoken bits */
enum IAPIdentifyDeviceLingoesLingoBits {
    IAPIdentifyDeviceLingoesLingoBits_General            = 1 << 0,
    IAPIdentifyDeviceLingoesLingoBits_SimpleRemote       = 1 << 2,
    IAPIdentifyDeviceLingoesLingoBits_DisplayRemote      = 1 << 3,
    IAPIdentifyDeviceLingoesLingoBits_ExtendedInterface  = 1 << 4,
    IAPIdentifyDeviceLingoesLingoBits_AccessoryPower     = 1 << 5,
    IAPIdentifyDeviceLingoesLingoBits_USBHostMode        = 1 << 6,
    IAPIdentifyDeviceLingoesLingoBits_RFTuner            = 1 << 7,
    IAPIdentifyDeviceLingoesLingoBits_AccessoryEqualizer = 1 << 8,
    IAPIdentifyDeviceLingoesLingoBits_Sports             = 1 << 9,
    IAPIdentifyDeviceLingoesLingoBits_DigitalAudio       = 1 << 10,
    IAPIdentifyDeviceLingoesLingoBits_Storage            = 1 << 12,
};

/* [1] P.135 Table 3-25 IdentifyDeviceLingoes Options bits */
enum IAPIdentifyDeviceLingoesOptions {
    IAPIdentifyDeviceLingoesOptions_NoAuth        = 0b00,
    IAPIdentifyDeviceLingoesOptions_DeferAuth     = 0b01,
    IAPIdentifyDeviceLingoesOptions_ImmediateAuth = 0b10,
};

struct IAPIdentifyDeviceLingoesPayload {
    uint32_t lingoes_bits; /* IAPIdentifyDeviceLingoesLingoBits */
    uint32_t options;      /* IAPIdentifyDeviceLingoesOptions */
    uint32_t device_id;
} __attribute__((packed));
