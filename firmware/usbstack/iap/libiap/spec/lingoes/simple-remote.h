#pragma once
#include <stdint.h>

/* [1] P.213 Table 4-3 Simple remote lingo command summary */
enum IAPSimpleRemoteCommandID {
    IAPSimpleRemoteCommandID_ContextButtonStatus             = 0x00, /* from acc, button-status.h */
    IAPSimpleRemoteCommandID_IPodAck                         = 0x01, /* from dev, general/ipod-ack.h */
    IAPSimpleRemoteCommandID_ImageButtonStatus               = 0x02, /* from acc, button-status.h, deprecated */
    IAPSimpleRemoteCommandID_VideoButtonStatus               = 0x03, /* from acc, button-status.h */
    IAPSimpleRemoteCommandID_AudioButtonStatus               = 0x04, /* from acc, button-status.h */
    IAPSimpleRemoteCommandID_IPodOutButtonStatus             = 0x0B, /* from acc, ipod-out-button-status.h */
    IAPSimpleRemoteCommandID_RotationInputStatus             = 0x0C, /* from acc, rotation-input-status.h */
    IAPSimpleRemoteCommandID_RadioButtonStatus               = 0x0D, /* from acc, radio-button-status.h */
    IAPSimpleRemoteCommandID_CameraButtonStatus              = 0x0E, /* from acc, camera-button-status.h */
    IAPSimpleRemoteCommandID_RegisterDescriptor              = 0x0F, /* from acc, hid.h */
    IAPSimpleRemoteCommandID_IPodHIDReport                   = 0x10, /* from acc, hid.h */
    IAPSimpleRemoteCommandID_AccessoryHIDReport              = 0x11, /* from dev, hid.h */
    IAPSimpleRemoteCommandID_UnregisterDescriptor            = 0x12, /* from acc, hid.h */
    IAPSimpleRemoteCommandID_VoiceOverEvent                  = 0x13, /* from acc, voice-over-event.h */
    IAPSimpleRemoteCommandID_GetVoiceOverParameter           = 0x14, /* from acc, voice-over-parameter.h */
    IAPSimpleRemoteCommandID_RetVoiceOverParameter           = 0x15, /* from dev, voice-over-parameter.h */
    IAPSimpleRemoteCommandID_SetVoiceOverParameter           = 0x16, /* from acc, voice-over-parameter.h */
    IAPSimpleRemoteCommandID_GetCurrentVoiceOverItemProperty = 0x17, /* from acc, current-voice-over-item-property.h */
    IAPSimpleRemoteCommandID_RetCurrentVoiceOverItemProperty = 0x18, /* from dev, current-voice-over-item-property.h */
    IAPSimpleRemoteCommandID_SetVoiceOverContext             = 0x19, /* from acc, set-voice-over-context.h */
    IAPSimpleRemoteCommandID_VoiceOverParameterChanged       = 0x2A, /* from dev, voice-over-parameter.h */
    IAPSimpleRemoteCommandID_AccessoryAck                    = 0x81, /* from acc, general/acc-ack.h */
};

#include "simple-remote/button-status.h"
#include "simple-remote/camera-button-status.h"
#include "simple-remote/current-voice-over-item-property.h"
#include "simple-remote/hid.h"
#include "simple-remote/ipod-out-button-status.h"
#include "simple-remote/radio-button-status.h"
#include "simple-remote/rotation-input-status.h"
#include "simple-remote/set-voice-over-context.h"
#include "simple-remote/voice-over-event.h"
#include "simple-remote/voice-over-parameter.h"
