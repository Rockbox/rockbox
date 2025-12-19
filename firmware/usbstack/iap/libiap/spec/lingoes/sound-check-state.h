#pragma once
#include <stdint.h>

struct IAPRetSoundCheckStatePayload {
    uint8_t sound_check_status;
} __attribute__((packed));

struct IAPSetSoundCheckStatePayload {
    uint8_t sound_check_status;
    uint8_t restore_on_exit;
} __attribute__((packed));
