#pragma once
#include <stdint.h>

/* [1] P.161 Table 3-67 FIDTokenValues tokens */
/* = 0x{type}{subtype} */
enum IAPFIDTokenTypes {
    IAPFIDTokenTypes_Identify                   = 0x0000,
    IAPFIDTokenTypes_AccCaps                    = 0x0001,
    IAPFIDTokenTypes_AccInfo                    = 0x0002,
    IAPFIDTokenTypes_IPodPreference             = 0x0003,
    IAPFIDTokenTypes_EAProtocol                 = 0x0004,
    IAPFIDTokenTypes_BundleSeedIDPref           = 0x0005,
    IAPFIDTokenTypes_ScreenInfo                 = 0x0007,
    IAPFIDTokenTypes_EAProtocolMetadata         = 0x0008,
    IAPFIDTokenTypes_AccDigitalAudioSampleRates = 0x000E,
    IAPFIDTokenTypes_AccDigitalAudioVideoDelay  = 0x000F,
    IAPFIDTokenTypes_MicrophoneCaps             = 0x0100,
};

/* [1] P.161 Table 3-66 FIDTokenValues field format */
struct IAPFIDTokenValuesToken {
    uint8_t length;
    uint8_t type;
    uint8_t subtype;
    uint8_t data[];
} __attribute__((packed));

/* [1] P.170 Table 3-86 Acknowledgment status codes */
enum IAPFIDTokenValuesAckStatus {
    IAPFIDTokenValuesIdentifyAckStatus_Accepted       = 0x00,
    IAPFIDTokenValuesIdentifyAckStatus_RequiredFailed = 0x01,
    IAPFIDTokenValuesIdentifyAckStatus_OptionalFailed = 0x02,
    IAPFIDTokenValuesIdentifyAckStatus_NotSupported   = 0x03,
    IAPFIDTokenValuesIdentifyAckStatus_LingoBusy      = 0x04,
    IAPFIDTokenValuesIdentifyAckStatus_MaxConnections = 0x05,
};

#include "fid-token-values/acc-caps.h"
#include "fid-token-values/acc-digital-audio-sample-rates.h"
#include "fid-token-values/acc-digital-audio-video-delay.h"
#include "fid-token-values/acc-info.h"
#include "fid-token-values/bundle-seed-id-pref.h"
#include "fid-token-values/ea-protocol-metadata.h"
#include "fid-token-values/ea-protocol.h"
#include "fid-token-values/identify.h"
#include "fid-token-values/ipod-preference.h"
#include "fid-token-values/microphone-caps.h"
#include "fid-token-values/screen-info.h"

struct IAPSetFIDTokenValuesPayload {
    uint8_t num_token_values;
    uint8_t data[]; /* packed token values */
};

struct IAPAckFIDTokenValuesPayload {
    uint8_t num_token_value_acks;
    uint8_t data[]; /* packed token value acks */
};
