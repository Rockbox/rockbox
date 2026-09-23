/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2025-2026 David Cormier
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#ifndef HW_H264_H
#define HW_H264_H

#include <stddef.h>
#include <stdint.h>

struct vpu_h264;

/* The container, audio, synchronization, and UI layers live in the plugin.
 * Only the target hardware decoder is exported by core. */
struct hw_h264_api
{
    size_t (*buf_size)(int max_w, int max_h);
    struct vpu_h264 *(*open)(void *buf, size_t buf_size,
                             int max_w, int max_h);
    int (*configure)(struct vpu_h264 *decoder,
                     const uint8_t *avcc, int avcc_len);
    int (*decode_sample)(struct vpu_h264 *decoder, const uint8_t *sample,
                         int sample_len, int nalu_len_size);
    void (*get_frame)(const struct vpu_h264 *decoder,
                      const uint8_t **y, const uint8_t **cb,
                      const uint8_t **cr, int *w, int *h, int *stride);
    void (*close)(struct vpu_h264 *decoder);
};

extern const struct hw_h264_api target_hw_h264_api;

#endif /* HW_H264_H */
