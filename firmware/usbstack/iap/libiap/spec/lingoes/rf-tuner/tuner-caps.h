#pragma once
#include <stdint.h>

/* [1] P.292 4.7.7 Command 0x02: RetTunerCaps */

enum IAPFMTunerResolutionID {
    _200kHz = 0b00,
    _100kHz = 0b01,
    _50kHz  = 0b10,
};

enum IAPAMTunerResolutionID {
    _10kHz = 0b0,
    _9kHz  = 0b1,
};

enum IAPTunerCapsBits {
    IAPTunerCapsBits_AMWorldwide                = 1 << 0,
    IAPTunerCapsBits_FMEuropeUS                 = 1 << 1,
    IAPTunerCapsBits_FMJapan                    = 1 << 2,
    IAPTunerCapsBits_FMWide                     = 1 << 3,
    IAPTunerCapsBits_HDRadio                    = 1 << 4,
    IAPTunerCapsBits_PowerOnOff                 = 1 << 8,
    IAPTunerCapsBits_StatusChangeNotification   = 1 << 9,
    IAPTunerCapsBits_MinFMResolution            = 3 << 16, /* IAPFMTunerResolutionID */
    IAPTunerCapsBits_TunerSeekUpDown            = 1 << 18,
    IAPTunerCapsBits_TunerSeekRSSIThreshold     = 1 << 19,
    IAPTunerCapsBits_ForcemonophonicMode        = 1 << 20,
    IAPTunerCapsBits_StereoBlend                = 1 << 21,
    IAPTunerCapsBits_FMTunerDeemphasisSelect    = 1 << 22,
    IAPTunerCapsBits_AMTuner9kHzResoluton       = 1 << 23,
    IAPTunerCapsBits_RDSData                    = 1 << 24,
    IAPTunerCapsBits_TunerChannelRSSIIndication = 1 << 25,
    IAPTunerCapsBits_StereoSourceIndicator      = 1 << 26,
    IAPTunerCapsBits_RDSRawMode                 = 1 << 27,
};

struct IAPRetTunerCapsPayload {
    uint32_t caps;      /* IAPTunerCapsBits */
    uint8_t  reserved1; /* = 0x01 */
    uint8_t  reserved2; /* = 0x00 */
} __attribute__((packed));
