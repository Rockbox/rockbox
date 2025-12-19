#pragma once
#include <stdint.h>

/* [1] P.534 Table C-12 Microphone lingo command summary */
enum IAPMicrophoneCommandID {
    IAPMicrophoneCommandID_BeginRecord      = 0x00, /* deprecated */
    IAPMicrophoneCommandID_EndRecord        = 0x01, /* deprecated */
    IAPMicrophoneCommandID_BeginPlayback    = 0x02, /* deprecated */
    IAPMicrophoneCommandID_EndPlayback      = 0x03, /* deprecated */
    IAPMicrophoneCommandID_AccessoryAck     = 0x04, /* from acc, general/acc-ack.h */
    IAPMicrophoneCommandID_GetAccessoryAck  = 0x05, /* from dev, no payload */
    IAPMicrophoneCommandID_IPodModeChange   = 0x06, /* from dev, ipod-mode-change.h */
    IAPMicrophoneCommandID_GetAccessoryCaps = 0x07, /* from dev, no payload */
    IAPMicrophoneCommandID_RetAccessoryCaps = 0x08, /* from acc, acc-caps.h */
    IAPMicrophoneCommandID_GetAccessoryCtrl = 0x09, /* from dev, acc-ctrl.h */
    IAPMicrophoneCommandID_RetAccessoryCtrl = 0x0A, /* from acc, acc-ctrl.h */
    IAPMicrophoneCommandID_SetAccessoryCtrl = 0x0B, /* from dev, acc-ctrl.h */
};

#include "microphone/acc-caps.h"
#include "microphone/acc-ctrl.h"
#include "microphone/ipod-mode-change.h"
