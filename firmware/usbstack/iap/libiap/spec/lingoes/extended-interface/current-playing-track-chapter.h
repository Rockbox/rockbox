#pragma once
#include <stdint.h>

struct IAPReturnCurrentPlayingTrackChapterInfoPayload {
    uint32_t index;
    uint32_t count;
} __attribute__((packed));

struct IAPSetCurrentPlayingTrackChapterPayload {
    uint32_t index;
} __attribute__((packed));

struct IAPGetCurrentPlayingTrackChapterPlayStatusPayload {
    uint32_t index;
} __attribute__((packed));

struct IAPReturnCurrentPlayingTrackChapterPlayStatusPayload {
    uint32_t length_ms;
    uint32_t elapsed_ms;
} __attribute__((packed));

struct IAPGetCurrentPlayingTrackChapterNamePayload {
    uint32_t index;
} __attribute__((packed));

/*
struct IAPReturnCurrentPlayingTrackChapterNamePayload {
    char name[];
} __attribute__((packed));
*/
