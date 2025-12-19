#pragma once
#include <stdint.h>

/* [1] P.444 5.1.55 Command 0x003C: GetDBiTunesInfo */

enum IAPITunesMetadataType {
    IAPITunesMetadataType_UID             = 0x00,
    IAPITunesMetadataType_LastSyncDate    = 0x01,
    IAPITunesMetadataType_AudioTrackCount = 0x02,
    IAPITunesMetadataType_VideoTrackCount = 0x03,
    IAPITunesMetadataType_AudiobookCount  = 0x04,
    IAPITunesMetadataType_PhotoCount      = 0x05,
};

struct IAPGetDBITunesInfoPayload {
    uint8_t metadata_type; /* IAPITunesMetadataType */
} __attribute__((packed));

struct IAPRetDBITunesInfoPayload {
    uint8_t metadata_type; /* = IAPITunesMetadataType */
    uint8_t data[];
} __attribute__((packed));

struct IAPRetDBITunesInfoUIDPayload {
    uint8_t metadata_type; /* = IAPITunesMetadataType_UID */
    uint8_t uid[8];
} __attribute__((packed));

struct IAPRetDBITunesInfoLastSyncDatePayload {
    uint8_t  metadata_type; /* = IAPITunesMetadataType_LastSyncDate */
    uint8_t  seconds;
    uint8_t  minute;
    uint8_t  hour;
    uint8_t  day;
    uint8_t  month;
    uint16_t year;
} __attribute__((packed));

struct IAPRetDBITunesInfoCountPayload {
    uint8_t  metadata_type; /* = IAPITunesMetadataType_{AudioTrack,VideoTrack,Audiobook,Photo}Count */
    uint32_t count;
} __attribute__((packed));
