#pragma once
#include <stdint.h>

/* [1] P.346 Table 4-232 Digital Audio lingo command summary */
enum IAPDigitalAudioCommandID {
    IAPDigitalAudioCommandID_AccessoryAck               = 0x00, /* from acc, general/acc-ack.h */
    IAPDigitalAudioCommandID_IPodAck                    = 0x01, /* from dev, general/ipod-ack.h */
    IAPDigitalAudioCommandID_GetAccessorySampleRateCaps = 0x02, /* from dev, no payload */
    IAPDigitalAudioCommandID_RetAccessorySampleRateCaps = 0x03, /* from acc, accessory-sample-rate-caps.h */
    IAPDigitalAudioCommandID_TrackNewAudioAttributes    = 0x04, /* from acc, track-new-audio-attributes.h */
    IAPDigitalAudioCommandID_SetVideoDelay              = 0x05, /* from acc, set-video-delay.h */
};

#include "digital-audio/accessory-sample-rate-caps.h"
#include "digital-audio/set-video-delay.h"
#include "digital-audio/track-new-audio-attributes.h"
