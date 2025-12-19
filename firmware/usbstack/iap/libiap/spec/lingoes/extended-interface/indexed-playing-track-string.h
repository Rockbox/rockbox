#pragma once
#include <stdint.h>

struct IAPGetIndexedPlayingTrackStringPayload {
    uint32_t index;
} __attribute__((packed));

/* IAPGetIndexedPlayingTrackTitlePayload = IAPGetIndexedPlayingTrackStringPayload */
/* IAPGetIndexedPlayingTrackArtistNamePayload = IAPGetIndexedPlayingTrackStringPayload */
/* IAPGetIndexedPlayingTrackAlbumNamePayload = IAPGetIndexedPlayingTrackStringPayload */

/*
struct IAPReturnIndexedPlayingTrackStringPayload {
    char string[];
} __attribute__((packed));
*/

/* IAPReturnIndexedPlayingTrackTitlePayload = IAPReturnIndexedPlayingTrackStringPayload */
/* IAPReturnIndexedPlayingTrackArtistNamePayload = IAPReturnIndexedPlayingTrackStringPayload */
/* IAPReturnIndexedPlayingTrackAlbumNamePayload = IAPReturnIndexedPlayingTrackStringPayload */
