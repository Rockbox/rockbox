#pragma once
#include <stdint.h>

struct IAPSetPlayStatusChangeNotification1BytePayload {
    uint8_t enable;
} __attribute__((packed));

enum IAPStatusChangeNotificationBits {
    IAPStatusChangeNotificationBits_Basic                         = 1 << 0,
    IAPStatusChangeNotificationBits_Extended                      = 1 << 1,
    IAPStatusChangeNotificationBits_TrackIndex                    = 1 << 2,
    IAPStatusChangeNotificationBits_TrackTimeOffsetMSec           = 1 << 3,
    IAPStatusChangeNotificationBits_TrackTimeOffsetSec            = 1 << 4,
    IAPStatusChangeNotificationBits_ChapterIndex                  = 1 << 5,
    IAPStatusChangeNotificationBits_ChapterTimeOffsetMSec         = 1 << 6,
    IAPStatusChangeNotificationBits_ChapterTimeOffsetSec          = 1 << 7,
    IAPStatusChangeNotificationBits_TrackUniqueID                 = 1 << 8,
    IAPStatusChangeNotificationBits_TrackMediaType                = 1 << 9,
    IAPStatusChangeNotificationBits_TrackLyricsReady              = 1 << 10,
    IAPStatusChangeNotificationBits_TrackCapsChanged              = 1 << 11,
    IAPStatusChangeNotificationBits_PlaybackEngineContentsChanged = 1 << 12,

};

struct IAPSetPlayStatusChangeNotification4BytesPayload {
    uint32_t mask; /* IAPStatusChangeNotificationBits */
} __attribute__((packed));

/* [1] P.426 5.1.36 Command 0x0027: PlayStatusChangeNotification */

enum IAPStatusChangeNotificationType {
    IAPStatusChangeNotificationType_PlaybackStopped               = 0x00,
    IAPStatusChangeNotificationType_TrackIndex                    = 0x01,
    IAPStatusChangeNotificationType_PlaybackFEWSeekStop           = 0x02,
    IAPStatusChangeNotificationType_PlaybackREWSeekStop           = 0x03,
    IAPStatusChangeNotificationType_TrackTimeOffsetMSec           = 0x04,
    IAPStatusChangeNotificationType_ChapterIndex                  = 0x05,
    IAPStatusChangeNotificationType_PlaybackStatusExtended        = 0x06,
    IAPStatusChangeNotificationType_TrackTimeOffsetSec            = 0x07,
    IAPStatusChangeNotificationType_ChapterTimeOffsetMSec         = 0x08,
    IAPStatusChangeNotificationType_ChapterTimeOffsetSec          = 0x09,
    IAPStatusChangeNotificationType_TrackUniqueID                 = 0x0A,
    IAPStatusChangeNotificationType_TrackPlaybackMode             = 0x0B,
    IAPStatusChangeNotificationType_TrackLyricsReady              = 0x0C,
    IAPStatusChangeNotificationType_TrackCapsChanged              = 0x0D,
    IAPStatusChangeNotificationType_PlaybackEngineContentsChanged = 0x0E,
};

struct IAPPlayStatusChangeNotificationPayload {
    uint8_t type;
    uint8_t data[];
} __attribute__((packed));

struct IAPPlayStatusChangeNotificationPlaybackStoppedPayload {
    uint8_t type; /* = IAPStatusChangeNotificationType_PlaybackStopped */
} __attribute__((packed));

struct IAPPlayStatusChangeNotificationTrackIndexPayload {
    uint8_t  type; /* = IAPStatusChangeNotificationType_TrackIndex */
    uint32_t index;
} __attribute__((packed));

struct IAPPlayStatusChangeNotificationPlaybackFEWSeekStopPayload {
    uint8_t type; /* = IAPStatusChangeNotificationType_PlaybackFEWSeekStop */
} __attribute__((packed));

struct IAPPlayStatusChangeNotificationPlaybackREWSeekStopPayload {
    uint8_t type; /* = IAPStatusChangeNotificationType_PlaybackREWSeekStop */
} __attribute__((packed));

struct IAPPlayStatusChangeNotificationTrackTimeOffsetMSecPayload {
    uint8_t  type; /* = IAPStatusChangeNotificationType_TrackTimeOffsetMSec */
    uint32_t offset_ms;
} __attribute__((packed));

struct IAPPlayStatusChangeNotificationChapterIndexPayload {
    uint8_t  type; /* = IAPStatusChangeNotificationType_ChapterIndex */
    uint32_t index;
} __attribute__((packed));

enum IAPPlayStatusChangeNotificationPlaybackStatusExtendedStates {
    IAPPlayStatusChangeNotificationPlaybackStatusExtendedStates_Stopped           = 0x02,
    IAPPlayStatusChangeNotificationPlaybackStatusExtendedStates_FFWSeekStarted    = 0x05,
    IAPPlayStatusChangeNotificationPlaybackStatusExtendedStates_REWSeekStarted    = 0x06,
    IAPPlayStatusChangeNotificationPlaybackStatusExtendedStates_FFWREWSeekStopped = 0x07,
    IAPPlayStatusChangeNotificationPlaybackStatusExtendedStates_Playing           = 0x0A,
    IAPPlayStatusChangeNotificationPlaybackStatusExtendedStates_Paused            = 0x0B,
};

struct IAPPlayStatusChangeNotificationPlaybackStatusExtendedPayload {
    uint8_t type;  /* = IAPStatusChangeNotificationType_PlaybackStatusExtended */
    uint8_t state; /* IAPPlayStatusChangeNotificationPlaybackStatusExtendedStates */
} __attribute__((packed));

struct IAPPlayStatusChangeNotificationTrackTimeOffsetSecPayload {
    uint8_t  type; /* = IAPStatusChangeNotificationType_TrackTimeOffsetSec */
    uint32_t offset_s;
} __attribute__((packed));

struct IAPPlayStatusChangeNotificationChapterTimeOffsetMSecPayload {
    uint8_t  type; /* = IAPStatusChangeNotificationType_ChapterTimeOffsetMSec */
    uint32_t offset_ms;
} __attribute__((packed));

struct IAPPlayStatusChangeNotificationChapterTimeOffsetSecPayload {
    uint8_t  type; /* = IAPStatusChangeNotificationType_ChapterTimeOffsetSec */
    uint32_t offset_s;
} __attribute__((packed));

struct IAPPlayStatusChangeNotificationTrackUniqueIDPayload {
    uint8_t type; /* = IAPStatusChangeNotificationType_TrackUniqueID */
    uint8_t id[8];
} __attribute__((packed));

enum IAPPlayStatusChangeNotificationTrackMediaTypePlayMode {
    IAPPlayStatusChangeNotificationTrackMediaTypePlayMode_Audio = 0x00,
    IAPPlayStatusChangeNotificationTrackMediaTypePlayMode_Video = 0x01,
};

struct IAPPlayStatusChangeNotificationTrackPlaybackModePayload {
    uint8_t type;      /* = IAPStatusChangeNotificationType_TrackPlaybackMode */
    uint8_t play_mode; /* IAPPlayStatusChangeNotificationTrackMediaTypePlayMode */
} __attribute__((packed));

struct IAPPlayStatusChangeNotificationTrackLyricsReadyPayload {
    uint8_t type; /* = IAPStatusChangeNotificationType_TrackLyricsReady */
} __attribute__((packed));

struct IAPPlayStatusChangeNotificationTrackCapsChangedPayload {
    uint8_t  type; /* = IAPStatusChangeNotificationType_TrackCapsChanged */
    uint32_t caps; /* IAPIPodStateTrackCapBits */
} __attribute__((packed));

struct IAPPlayStatusChangeNotificationPlaybackEngineContentsChangedPayload {
    uint8_t  type; /* = IAPStatusChangeNotificationType_PlaybackEngineContentsChanged */
    uint32_t count;
} __attribute__((packed));
