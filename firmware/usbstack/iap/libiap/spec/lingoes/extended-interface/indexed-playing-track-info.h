#pragma once
#include <stdint.h>

/* [1] P.408 5.1.13 Command 0x000C: GetIndexedPlayingTrackInfo */

enum IAPExtendedIndexedPlayingTrackInfoType {
    IAPExtendedIndexedPlayingTrackInfoType_TrackCapsInfo     = 0x00,
    IAPExtendedIndexedPlayingTrackInfoType_PodcastName       = 0x01,
    IAPExtendedIndexedPlayingTrackInfoType_TrackReleaseDate  = 0x02,
    IAPExtendedIndexedPlayingTrackInfoType_TrackDescription  = 0x03,
    IAPExtendedIndexedPlayingTrackInfoType_TrackSongLyrics   = 0x04,
    IAPExtendedIndexedPlayingTrackInfoType_TrackGenre        = 0x05,
    IAPExtendedIndexedPlayingTrackInfoType_TrackComposer     = 0x06,
    IAPExtendedIndexedPlayingTrackInfoType_TrackArtworkCount = 0x07,
};

struct IAPExtendedGetIndexedPlayingTrackInfoPayload {
    uint8_t  type; /* IAPExtendedIndexedPlayingTrackInfoType */
    uint32_t track_index;
    uint16_t chapter_index;
} __attribute__((packed));

struct IAPExtendedRetIndexedPlayingTrackInfoPayload {
    uint8_t type; /* IAPExtendedIndexedPlayingTrackInfoType */
    uint8_t data[];
} __attribute__((packed));

struct IAPExtendedRetIndexedPlayingTrackInfoTrackCapsInfoPayload {
    uint8_t  type;       /* = IAPExtendedRetIndexedPlayingTrackInfo_TrackCapsInfo */
    uint32_t track_caps; /* IAPIPodStateTrackCapBits */
    uint32_t track_total_ms;
    uint16_t chapter_count;
} __attribute__((packed));

struct IAPExtendedRetIndexedPlayingTrackInfoPodcastNamePayload {
    uint8_t type; /* = IAPExtendedRetIndexedPlayingTrackInfo_PodcastName */
    char    name[];
} __attribute__((packed));

struct IAPExtendedRetIndexedPlayingTrackInfoTrackReleaseDatePayload {
    uint8_t  type; /* = IAPExtendedRetIndexedPlayingTrackInfo_TrackReleaseDate */
    uint8_t  seconds;
    uint8_t  minutes;
    uint8_t  hours;
    uint8_t  day;
    uint8_t  month;
    uint16_t year;
    uint8_t  weekday;
} __attribute__((packed));

struct IAPExtendedRetIndexedPlayingTrackInfoTrackDescriptionPayload {
    uint8_t  type;      /* = IAPExtendedRetIndexedPlayingTrackInfo_TrackDescription */
    uint8_t  info_bits; /* IAPIndexedPlayingTrackInfoLyricsInfoBits */
    uint16_t index;
    char     description[];
} __attribute__((packed));

struct IAPExtendedRetIndexedPlayingTrackInfoTrackSongLyricsPayload {
    uint8_t  type;      /* = IAPExtendedRetIndexedPlayingTrackInfo_TrackSongLyrics */
    uint8_t  info_bits; /* IAPIndexedPlayingTrackInfoLyricsInfoBits */
    uint16_t index;
    char     lyrics[];
} __attribute__((packed));

struct IAPExtendedRetIndexedPlayingTrackInfoTrackGenrePayload {
    uint8_t type; /* = IAPExtendedRetIndexedPlayingTrackInfo_TrackGenre */
    char    genre[];
} __attribute__((packed));

struct IAPExtendedRetIndexedPlayingTrackInfoTrackComposerPayload {
    uint8_t type; /* = IAPExtendedRetIndexedPlayingTrackInfo_TrackComposer */
    char    composer[];
} __attribute__((packed));

struct IAPExtendedRetIndexedPlayingTrackInfoTrackArtworkCountPayload {
    uint8_t type; /* = IAPExtendedRetIndexedPlayingTrackInfo_TrackArtworkCount */
    struct {
        uint16_t format; /* IAPArtworkPixelFormats */
        uint16_t count;
    } __attribute__((packed)) data[]; /* or 0x08 to indicate no counts */
} __attribute__((packed));
