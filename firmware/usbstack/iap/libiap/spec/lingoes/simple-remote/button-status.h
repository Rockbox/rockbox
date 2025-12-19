#pragma once
#include <stdint.h>

enum IAPContextButtonBits {
    /* bits1 */
    IAPContextButtonStatusButtons_PlayPause  = 1 << 0,
    IAPContextButtonStatusButtons_VolumeUp   = 1 << 1,
    IAPContextButtonStatusButtons_VolumeDown = 1 << 2,
    IAPContextButtonStatusButtons_NextTrack  = 1 << 3,
    IAPContextButtonStatusButtons_PrevTrack  = 1 << 4,
    IAPContextButtonStatusButtons_NextAlbum  = 1 << 5,
    IAPContextButtonStatusButtons_PrevAlbum  = 1 << 6,
    IAPContextButtonStatusButtons_Stop       = 1 << 7,
    /* bits2 */
    IAPContextButtonStatusButtons_PlayResume            = 1 << 0,
    IAPContextButtonStatusButtons_Pause                 = 1 << 1,
    IAPContextButtonStatusButtons_MuteToggle            = 1 << 2,
    IAPContextButtonStatusButtons_NextChapter           = 1 << 3,
    IAPContextButtonStatusButtons_PrevChapter           = 1 << 4,
    IAPContextButtonStatusButtons_NextPlaylist          = 1 << 5,
    IAPContextButtonStatusButtons_PrevPlaylist          = 1 << 6,
    IAPContextButtonStatusButtons_ShuffleSettingAdvance = 1 << 7,
    /* bits3 */
    IAPContextButtonStatusButtons_RepeatSettingAdvance = 1 << 0,
    IAPContextButtonStatusButtons_PowerOn              = 1 << 1,
    IAPContextButtonStatusButtons_PowerOff             = 1 << 2,
    IAPContextButtonStatusButtons_Backlight30Seconds   = 1 << 3,
    IAPContextButtonStatusButtons_BeginFastForward     = 1 << 4,
    IAPContextButtonStatusButtons_BeginRewind          = 1 << 5,
    IAPContextButtonStatusButtons_Menu                 = 1 << 6,
    IAPContextButtonStatusButtons_Select               = 1 << 7,
    /* bits4 */
    IAPContextButtonStatusButtons_UpArrow      = 1 << 0,
    IAPContextButtonStatusButtons_DownArrow    = 1 << 1,
    IAPContextButtonStatusButtons_BacklightOff = 1 << 2,
};

enum IAPImageButtonBits {
    /* bits1 */
    IAPImageButtonBits_PlayPause      = 1 << 0,
    IAPImageButtonBits_NextImage      = 1 << 1,
    IAPImageButtonBits_PrevImage      = 1 << 2,
    IAPImageButtonBits_Stop           = 1 << 3,
    IAPImageButtonBits_PlayResume     = 1 << 4,
    IAPImageButtonBits_Pause          = 1 << 5,
    IAPImageButtonBits_ShuffleAdvance = 1 << 6,
    IAPImageButtonBits_RepeatAdvance  = 1 << 7,
};

enum IAPVideoButtonBits {
    /* bits1 */
    IAPVideoButtonBits_PlayPause  = 1 << 0,
    IAPVideoButtonBits_NextVideo  = 1 << 1,
    IAPVideoButtonBits_PrevVideo  = 1 << 2,
    IAPVideoButtonBits_Stop       = 1 << 3,
    IAPVideoButtonBits_PlayResume = 1 << 4,
    IAPVideoButtonBits_Pause      = 1 << 5,
    IAPVideoButtonBits_BeginFF    = 1 << 6,
    IAPVideoButtonBits_BeginREW   = 1 << 7,
    /* bits1 */
    IAPVideoButtonBitsNextChapter = 1 << 0,
    IAPVideoButtonBitsPrevChapter = 1 << 1,
    IAPVideoButtonBitsNextFrame   = 1 << 2,
    IAPVideoButtonBitsPrevFrame   = 1 << 3,
};

enum IAPAudioButtonBits {
    /* bits1 */
    IAPAudioButtonBits_PlayPause  = 1 << 0,
    IAPAudioButtonBits_VolumeUp   = 1 << 1,
    IAPAudioButtonBits_VolumeDown = 1 << 2,
    IAPAudioButtonBits_NextTrack  = 1 << 3,
    IAPAudioButtonBits_PrevTrack  = 1 << 4,
    IAPAudioButtonBits_NextAlbum  = 1 << 5,
    IAPAudioButtonBits_PrevAlbum  = 1 << 6,
    IAPAudioButtonBits_Stop       = 1 << 7,
    /* bits2 */
    IAPAudioButtonBits_PlayResume            = 1 << 0,
    IAPAudioButtonBits_Pause                 = 1 << 1,
    IAPAudioButtonBits_MuteToggle            = 1 << 2,
    IAPAudioButtonBits_NextChapter           = 1 << 3,
    IAPAudioButtonBits_PrevChapter           = 1 << 4,
    IAPAudioButtonBits_NextPlaylist          = 1 << 5,
    IAPAudioButtonBits_PrevPlaylist          = 1 << 6,
    IAPAudioButtonBits_ShuffleSettingAdvance = 1 << 7,
    /* bits3 */
    IAPAudioButtonBits_RepeatSettingAdvance = 1 << 0,
    IAPAudioButtonBits_BeginFF              = 1 << 1,
    IAPAudioButtonBits_BeginREW             = 1 << 2,
    IAPAudioButtonBits_Record               = 1 << 3,
};

struct IAPButtonStatusPayload {
    uint8_t bits1; /* IAP{Context,Image,Video,Audio}ButtonBits(bits1) */
    uint8_t bits2; /* IAP{Context,Image,Video,Audio}ButtonBits(bits2), optional */
    uint8_t bits3; /* IAP{Context,Image,Video,Audio}ButtonBits(bits3), optional */
    uint8_t bits4; /* IAP{Context,Image,Video,Audio}ButtonBits(bits4), optional */
} __attribute__((packed));
