#pragma once
#include <stdint.h>

enum IAPIndexedPlayingTrackInfoType {
    IAPIndexedPlayingTrackInfoType_TrackCapsInfo   = 0x00,
    IAPIndexedPlayingTrackInfoType_ChapterTimeName = 0x01,
    IAPIndexedPlayingTrackInfoType_ArtistName      = 0x02,
    IAPIndexedPlayingTrackInfoType_AlbumName       = 0x03,
    IAPIndexedPlayingTrackInfoType_GenreName       = 0x04,
    IAPIndexedPlayingTrackInfoType_TrackTitle      = 0x05,
    IAPIndexedPlayingTrackInfoType_ComposerName    = 0x06,
    IAPIndexedPlayingTrackInfoType_Lyrics          = 0x07,
    IAPIndexedPlayingTrackInfoType_ArtworkCount    = 0x08,
};

struct IAPGetIndexedPlayingTrackInfoPayload {
    uint8_t  type; /* IAPIndexedPlayingTrackInfoType */
    uint32_t track_index;
    uint16_t chapter_index;
} __attribute__((packed));

struct IAPRetIndexedPlayingTrackInfoPayload {
    uint8_t type; /* IAPIndexedPlayingTrackInfoType */
    uint8_t data[];
} __attribute__((packed));

struct IAPRetIndexedPlayingTrackInfoTrackCapsInfoPayload {
    uint8_t  type;       /* = IAPIndexedPlayingTrackInfoType_TrackCapsInfo */
    uint32_t track_caps; /* IAPIPodStateTrackCapBits */
    uint32_t track_total_ms;
    uint16_t chapter_count;
} __attribute__((packed));

struct IAPRetIndexedPlayingTrackInfoChapterTimeNamePayload {
    uint8_t  type; /* = IAPIndexedPlayingTrackInfoType_ChapterTimeName */
    uint32_t offset_ms;
    char     name[];
} __attribute__((packed));

struct IAPRetIndexedPlayingTrackInfoArtistNamePayload {
    uint8_t type; /* = IAPIndexedPlayingTrackInfoType_ArtistName */
    char    name[];
} __attribute__((packed));

struct IAPRetIndexedPlayingTrackInfoAlbumNamePayload {
    uint8_t type; /* = IAPIndexedPlayingTrackInfoType_AlbumName */
    char    name[];
} __attribute__((packed));

struct IAPRetIndexedPlayingTrackInfoGenreNamePayload {
    uint8_t type; /* = IAPIndexedPlayingTrackInfoType_GenreName */
    char    name[];
} __attribute__((packed));

struct IAPRetIndexedPlayingTrackInfoTrackTitlePayload {
    uint8_t type; /* = IAPIndexedPlayingTrackInfoType_TrackTitle */
    char    name[];
} __attribute__((packed));

struct IAPRetIndexedPlayingTrackInfoComposerNamePayload {
    uint8_t type; /* = IAPIndexedPlayingTrackInfoType_ComposerName */
    char    name[];
} __attribute__((packed));

enum IAPIndexedPlayingTrackInfoLyricsInfoBits {
    IAPIndexedPlayingTrackInfoLyricsInfoBits_Series = 1 << 0,
    IAPIndexedPlayingTrackInfoLyricsInfoBits_Last   = 1 << 1,
};

struct IAPRetIndexedPlayingTrackInfoLyricsPayload {
    uint8_t  type;      /* = IAPIndexedPlayingTrackInfoType_Lyrics */
    uint8_t  info_bits; /* IAPIndexedPlayingTrackInfoLyricsInfoBits */
    uint16_t index;
    char     lyrics[];
} __attribute__((packed));

struct IAPArtworkCount {
    uint16_t format; /* IAPArtworkPixelFormats */
    uint16_t count;
} __attribute__((packed));

struct IAPRetIndexedPlayingTrackInfoArtworkCountPayload {
    uint8_t                type;   /* = IAPIndexedPlayingTrackInfoType_ArtworkCount */
    struct IAPArtworkCount data[]; /* or 0x08 to indicate no counts */
} __attribute__((packed));
