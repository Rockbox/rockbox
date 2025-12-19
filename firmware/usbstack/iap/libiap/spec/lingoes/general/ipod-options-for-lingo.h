#pragma once
#include <stdint.h>

struct IAPGetIPodOptionsForLingoPayload {
    uint8_t lingo_id;
} __attribute__((packed));

/* [1] P.192 Table 3-132 RetiPodOptionsForLingo option bits */

enum IAPRetIPodOptionsForLingoGeneralBits {
    IAPRetIPodOptionsForLingoGeneralBits_LineOutUsage              = 1 << 0,
    IAPRetIPodOptionsForLingoGeneralBits_VideOutput                = 1 << 1,
    IAPRetIPodOptionsForLingoGeneralBits_VideoNTSCFormat           = 1 << 2,
    IAPRetIPodOptionsForLingoGeneralBits_VideoPALFormat            = 1 << 3,
    IAPRetIPodOptionsForLingoGeneralBits_VideoCompositeConnection  = 1 << 4,
    IAPRetIPodOptionsForLingoGeneralBits_VideoSVideoConnection     = 1 << 5,
    IAPRetIPodOptionsForLingoGeneralBits_VideoComponentConnection  = 1 << 6,
    IAPRetIPodOptionsForLingoGeneralBits_VideoClosedCaptioning     = 1 << 7,
    IAPRetIPodOptionsForLingoGeneralBits_VideoAspectRatio4x3       = 1 << 8,
    IAPRetIPodOptionsForLingoGeneralBits_VideoAspectRatio16x9      = 1 << 9,
    IAPRetIPodOptionsForLingoGeneralBits_VideoSubtitle             = 1 << 10,
    IAPRetIPodOptionsForLingoGeneralBits_VideoAltAudioChannel      = 1 << 11,
    IAPRetIPodOptionsForLingoGeneralBits_AppCommnunication         = 1 << 13,
    IAPRetIPodOptionsForLingoGeneralBits_DeviceNotification        = 1 << 14,
    IAPRetIPodOptionsForLingoGeneralBits_PauseOnRemovalControl     = 1 << 19,
    IAPRetIPodOptionsForLingoGeneralBits_RestrictedIDPS            = 1 << 23,
    IAPRetIPodOptionsForLingoGeneralBits_AppLaunchRequest          = 1 << 24,
    IAPRetIPodOptionsForLingoGeneralBits_AudioRouteSwitch          = 1 << 26,
    IAPRetIPodOptionsForLingoGeneralBits_USBHostModeHardwareInvoke = 1 << 27,
    IAPRetIPodOptionsForLingoGeneralBits_USBHostModeAudioOutput    = 1 << 28,
    IAPRetIPodOptionsForLingoGeneralBits_USBHostModeAudioInput     = 1 << 29,
    IAPRetIPodOptionsForLingoGeneralBits_USBHostModeNoMaxCurrent   = 1 << 30,
};

enum IAPRetIPodOptionsForLingoSimpleRemoteBits {
    IAPRetIPodOptionsForLingoSimpleRemoteBits_ContextSpecificControls = 1 << 0,
    IAPRetIPodOptionsForLingoSimpleRemoteBits_AudioMediaControls      = 1 << 1,
    IAPRetIPodOptionsForLingoSimpleRemoteBits_VideoMediaControls      = 1 << 2,
    IAPRetIPodOptionsForLingoSimpleRemoteBits_ImageMediaControls      = 1 << 3,
    IAPRetIPodOptionsForLingoSimpleRemoteBits_SportsMediaControls     = 1 << 4,
    IAPRetIPodOptionsForLingoSimpleRemoteBits_CameraMediaControls     = 1 << 8,
    IAPRetIPodOptionsForLingoSimpleRemoteBits_USBHIDCommands          = 1 << 9,
    IAPRetIPodOptionsForLingoSimpleRemoteBits_VoiceOverControls       = 1 << 10,
    IAPRetIPodOptionsForLingoSimpleRemoteBits_VoiceOverPreferences    = 1 << 11,
    IAPRetIPodOptionsForLingoSimpleRemoteBits_AssistiveTouchCursor    = 1 << 12,
};

enum IAPRetIPodOptionsForLingoDisplayRemoteBits {
    IAPRetIPodOptionsForLingoDisplayRemoteBits_UIVolumeControl        = 1 << 0,
    IAPRetIPodOptionsForLingoDisplayRemoteBits_AbsoluteVolumeControl  = 1 << 1,
    IAPRetIPodOptionsForLingoDisplayRemoteBits_GeniusPlaylistCreation = 1 << 2,
};

enum IAPRetIPodOptionsForLingoExtendedInterfaceBits {
    IAPRetIPodOptionsForLingoExtendedInterfaceBits_VideoBrowsing                    = 1 << 0,
    IAPRetIPodOptionsForLingoExtendedInterfaceBits_InfoCommands                     = 1 << 1,
    IAPRetIPodOptionsForLingoExtendedInterfaceBits_NestedPlaylists                  = 1 << 2,
    IAPRetIPodOptionsForLingoExtendedInterfaceBits_GeniusPlaylistCreationAndRefresh = 1 << 3,
    IAPRetIPodOptionsForLingoExtendedInterfaceBits_DisplayImages                    = 1 << 4,
    IAPRetIPodOptionsForLingoExtendedInterfaceBits_CategoryListAccess               = 1 << 5,
    IAPRetIPodOptionsForLingoExtendedInterfaceBits_PlayControl                      = 1 << 6,
    IAPRetIPodOptionsForLingoExtendedInterfaceBits_UIDBasedCommands                 = 1 << 7,
};

/*
enum IAPRetIPodOptionsForLingoAccessoryPowerBits {
};
*/

enum IAPRetIPodOptionsForLingoUSBHostModeBits {
    IAPRetIPodOptionsForLingoUSBHostModeBits_USBHostModeHardwareInvoke = 1 << 0, /* deprecated */
    IAPRetIPodOptionsForLingoUSBHostModeBits_USBHostModeFirmwareInvoke = 1 << 1,
};

enum IAPRetIPodOptionsForLingoRFTunerBits {
    IAPRetIPodOptionsForLingoRFTunerBits_RDSRawMode    = 1 << 0,
    IAPRetIPodOptionsForLingoRFTunerBits_HDRadioTuning = 1 << 1,
    IAPRetIPodOptionsForLingoRFTunerBits_AMRadioTuning = 1 << 2,
};

/*
enum IAPRetIPodOptionsForLingoAccessoryEqualizerBits {
};
*/

enum IAPRetIPodOptionsForLingoSportsBits {
    IAPRetIPodOptionsForLingoSportsBits_NikePlusEquipment = 1 << 1,
};

enum IAPRetIPodOptionsForLingoDigitalAudioBits {
    IAPRetIPodOptionsForLingoDigitalAudioBits_SetVideoDelayEnabled   = 1 << 0,
    IAPRetIPodOptionsForLingoDigitalAudioBits_SampleRateCapsFIDToken = 1 << 2,
    IAPRetIPodOptionsForLingoDigitalAudioBits_VideoDelayFIDToken     = 1 << 3,
};

enum IAPRetIPodOptionsForLingoStorageBits {
    IAPRetIPodOptionsForLingoStorageBits_ITunesTagging     = 1 << 0,
    IAPRetIPodOptionsForLingoStorageBits_NikePlusEquipment = 1 << 1,
};

enum IAPRetIPodOptionsForLingoIPodOutBits {
    IAPRetIPodOptionsForLingoIPodOutBits_WheelUIModeAvail = 1 << 0,
    IAPRetIPodOptionsForLingoIPodOutBits_PALVideoEnabled  = 1 << 2,
};

enum IAPRetIPodOptionsForLingoLocationBits {
    IAPRetIPodOptionsForLingoLocationBits_AcceptNMEAData             = 1 << 0,
    IAPRetIPodOptionsForLingoLocationBits_SendLocationAssistanceData = 1 << 1,
};

struct IAPRetIPodOptionsForLingoPayload {
    uint8_t  lingo_id;
    uint64_t bits;
} __attribute__((packed));
