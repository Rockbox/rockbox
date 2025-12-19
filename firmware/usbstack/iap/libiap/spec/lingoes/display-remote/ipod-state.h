#pragma once
#include <stdint.h>

/* common state types */

/* [1] P.266 Table 4-74 Apple device state data */
enum IAPIPodStateType {
    IAPIPodStateType_TrackTimePositionMSec  = 0x00,
    IAPIPodStateType_TrackPlaybackIndex     = 0x01,
    IAPIPodStateType_ChapterIndex           = 0x02,
    IAPIPodStateType_PlayStatus             = 0x03,
    IAPIPodStateType_Volume                 = 0x04,
    IAPIPodStateType_Power                  = 0x05,
    IAPIPodStateType_EQSetting              = 0x06,
    IAPIPodStateType_ShuffleSetting         = 0x07,
    IAPIPodStateType_RepeatSetting          = 0x08,
    IAPIPodStateType_DateTimeSetting        = 0x09,
    IAPIPodStateType_AlarmSetting           = 0x0A,
    IAPIPodStateType_BacklightLevel         = 0x0B,
    IAPIPodStateType_HoldSwitchState        = 0x0C,
    IAPIPodStateType_SoundCheckState        = 0x0D,
    IAPIPodStateType_AudiobookSpeeed        = 0x0E,
    IAPIPodStateType_TrackTimePositionSec   = 0x0F,
    IAPIPodStateType_AbsoluteVolume         = 0x10,
    IAPIPodStateType_TrackCaps              = 0x11,
    IAPIPodStateType_PlaybackEngineContents = 0x12,
};

struct IAPIPodStatePayload {
    uint8_t type; /* IAPIPodStateType */
    uint8_t data[];
} __attribute__((packed));

/* [1] P.257 Table 4-61 Event notification data */

struct IAPIPodStateTrackTimePositionMSecPayload {
    uint8_t  type; /* = IAPIPodStateType_TrackTimePositionMSec */
    uint32_t position_ms;
} __attribute__((packed));

struct IAPIPodStateTrackPlaybackIndexPayload {
    uint8_t  type; /* = IAPIPodStateType_TrackPlaybackIndex */
    uint32_t index;
} __attribute__((packed));

struct IAPIPodStateChapterIndexPayload {
    uint8_t  type; /* = IAPIPodStateType_ChapterIndex */
    uint32_t index;
    uint16_t chapter_count;
    uint16_t chapter_index;
} __attribute__((packed));

enum IAPIPodStatePlayStatus {
    IAPIPodStatePlayStatus_PlaybackStopped      = 0x00,
    IAPIPodStatePlayStatus_Playing              = 0x01,
    IAPIPodStatePlayStatus_PlaybackPaused       = 0x02,
    IAPIPodStatePlayStatus_FastForward          = 0x03,
    IAPIPodStatePlayStatus_FastRewind           = 0x04,
    IAPIPodStatePlayStatus_EndFastForwardRewind = 0x05,
};

struct IAPIPodStatePlayStatusPayload {
    uint8_t type;   /* = IAPIPodStateType_PlayStatus */
    uint8_t status; /* IAPIPodStatePlayStatus */
} __attribute__((packed));

struct IAPIPodStateVolumePayload {
    uint8_t type; /* = IAPIPodStateType_Volume */
    uint8_t mute_state;
    uint8_t ui_volume;
} __attribute__((packed));

enum IAPIPodStatePowerState {
    IAPIPodStatePowerState_InternalLow      = 0x00,
    IAPIPodStatePowerState_Internal         = 0x01,
    IAPIPodStatePowerState_ExternalBattery  = 0x02,
    IAPIPodStatePowerState_External         = 0x03,
    IAPIPodStatePowerState_ExternalCharging = 0x04,
    IAPIPodStatePowerState_ExternalCharged  = 0x05,
};

struct IAPIPodStatePowerPayload {
    uint8_t type;        /* = IAPIPodStateType_Power */
    uint8_t power_state; /* IAPIPodStatePowerState */
    uint8_t battery_level;
} __attribute__((packed));

struct IAPIPodStateEQSettingPayload {
    uint8_t  type; /* = IAPIPodStateType_EQSetting */
    uint32_t eq_index;
} __attribute__((packed));

enum IAPIPodStateShuffleSettingState {
    IAPIPodStateShuffleSettingState_Off    = 0x00,
    IAPIPodStateShuffleSettingState_Tracks = 0x01,
    IAPIPodStateShuffleSettingState_Albums = 0x02,
};

struct IAPIPodStateShuffleSettingPayload {
    uint8_t type;          /* = IAPIPodStateType_ShuffleSetting */
    uint8_t shuffle_state; /* IAPIPodStateShuffleSettingState */
} __attribute__((packed));

enum IAPIPodStateRepeatSettingState {
    IAPIPodStateRepeatSettingState_Off = 0x00,
    IAPIPodStateRepeatSettingState_One = 0x01,
    IAPIPodStateRepeatSettingState_All = 0x02,
};

struct IAPIPodStateRepeatSettingPayload {
    uint8_t type;         /* = IAPIPodStateType_RepeatSetting */
    uint8_t repeat_state; /* IAPIPodStateRepeatSettingState */
} __attribute__((packed));

struct IAPIPodStateDateTimeSettingPayload {
    uint8_t  type; /* = IAPIPodStateType_DateTimeSetting */
    uint16_t year;
    uint8_t  month;
    uint8_t  day;
    uint8_t  hour;
    uint8_t  minute;
} __attribute__((packed));

struct IAPIPodStateAlarmSettingPayload {
    uint8_t type; /* = IAPIPodStateType_AlarmSetting */
    uint8_t deprecated[3];
} __attribute__((packed));

struct IAPIPodStateBacklightLevelPayload {
    uint8_t type; /* = IAPIPodStateType_BacklightLevel */
    uint8_t level;
} __attribute__((packed));

struct IAPIPodStateHoldSwitchStatePayload {
    uint8_t type; /* = IAPIPodStateType_HoldSwitchState */
    uint8_t state;
} __attribute__((packed));

struct IAPIPodStateSoundCheckStatePayload {
    uint8_t type; /* = IAPIPodStateType_SoundCheckState */
    uint8_t state;
} __attribute__((packed));

enum IAPIPodStateAudiobookSpeeed {
    IAPIPodStateAudiobookSpeeed_Slower = 0xFF,
    IAPIPodStateAudiobookSpeeed_Normal = 0x00,
    IAPIPodStateAudiobookSpeeed_Faster = 0x01,
};

struct IAPIPodStateAudiobookSpeeedPayload {
    uint8_t type;  /* = IAPIPodStateType_AudiobookSpeeed */
    uint8_t speed; /* IAPIPodStateAudiobookSpeeed */
} __attribute__((packed));

struct IAPIPodStateTrackTimePositionSecPayload {
    uint8_t  type; /* = IAPIPodStateType_TrackTimePositionSec */
    uint16_t position_s;
} __attribute__((packed));

struct IAPIPodStateAbsoluteVolumePayload {
    uint8_t type; /* = IAPIPodStateType_AbsoluteVolume */
    uint8_t mute_state;
    uint8_t ui_volume;
    uint8_t absolute_volume;
} __attribute__((packed));

enum IAPIPodStateTrackCapBits {
    IAPIPodStateTrackCapBits_IsAudiobook            = 1 << 0,
    IAPIPodStateTrackCapBits_HasChapters            = 1 << 1,
    IAPIPodStateTrackCapBits_HasAlbumArts           = 1 << 2,
    IAPIPodStateTrackCapBits_HasLyrics              = 1 << 3,
    IAPIPodStateTrackCapBits_IsPodcast              = 1 << 4,
    IAPIPodStateTrackCapBits_HasReleaseDate         = 1 << 5,
    IAPIPodStateTrackCapBits_HasDescription         = 1 << 6,
    IAPIPodStateTrackCapBits_HasVideo               = 1 << 7,
    IAPIPodStateTrackCapBits_IsQueued               = 1 << 8,
    IAPIPodStateTrackCapBits_GenerateGeniusPlaylist = 1 << 13,
    IAPIPodStateTrackCapBits_IsITunesUEpisode       = 1 << 14,
};

struct IAPIPodStateTrackCapsPayload {
    uint8_t  type; /* = IAPIPodStateType_TrackCaps */
    uint32_t caps; /* IAPIPodStateTrackCapBits */
} __attribute__((packed));

struct IAPIPodStatePlaybackEngineContentsPayload {
    uint8_t  type; /* = IAPIPodStateType_PlaybackEngineContents */
    uint32_t count;
} __attribute__((packed));

/* event notification */
struct IAPSetRemoteEventNotificationPayload {
    uint32_t mask; /* (1 << IAPIPodStateType) | ... */
} __attribute__((packed));

/* IAPRemoteEventNotificationPayload = IAPIPodStatePayload */

/* get/ret state info */
struct IAPGetIPodStateInfoPayload {
    uint8_t type; /* = IAPIPodStateType */
} __attribute__((packed));

/* RetIPodStateInfoPayload = IAPIPodStatePayload */
/* SetIPodStateInfoPayload = IAPIPodStatePayload */
