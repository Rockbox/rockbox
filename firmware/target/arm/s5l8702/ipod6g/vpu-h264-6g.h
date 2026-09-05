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
 * S5L8702 VPU-B H.264 Baseline hardware decoder.
 *
 * Drives the VPU-B block on iPod Classic 6G (S5L8702).
 * Full hardware decode: CAVLC, dequant, IDCT, intra/inter prediction,
 * motion compensation, and in-loop deblocking.
 *
 * Supports: H.264 Baseline profile, level <= 3.0, up to ten ordered slices
 * per frame. The single-slice path is proven bit-perfect against ffmpeg;
 * multi-slice support follows the same Apple VPU descriptor interface.
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

#ifndef VPU_H264_6G_H
#define VPU_H264_6G_H

#include "hw_h264.h"

#include <stdint.h>
#include <stddef.h>

/* Returns minimum buffer size needed for given max dimensions.
 * Caller must provide a contiguous buffer of at least this size
 * to vpu_h264_open(). Accounts for alignment padding. */
size_t vpu_h264_buf_size(int max_w, int max_h);

/* Initialize decoder. Allocates internal buffers from buf via bump
 * allocator. Powers on VPU-B hardware and IRQ.
 * Returns context pointer (inside buf) or NULL on error. */
struct vpu_h264 *vpu_h264_open(void *buf, size_t buf_size,
                                int max_w, int max_h);

/* Feed avcC decoder configuration (from MP4 container).
 * Parses SPS and PPS NALUs from the avcC blob.
 * Returns 0 on success, -1 on error. */
int vpu_h264_configure(struct vpu_h264 *v,
                        const uint8_t *avcc, int avcc_len);

/* Feed one raw NALU (including nal_header byte, excluding length prefix).
 * Accepts EBSP (emulation prevention bytes intact).
 * Handles SPS (7), PPS (8), IDR slice (5), non-IDR slice (1) internally.
 * For slice NALUs: triggers synchronous HW decode (~5ms).
 *
 * Returns:  1 = frame decoded (call vpu_h264_get_frame)
 *           0 = NALU consumed (SPS/PPS/SEI, no frame output)
 *          -1 = error (STATUS1 bits or timeout) */
int vpu_h264_decode_nalu(struct vpu_h264 *v,
                          const uint8_t *nalu, int nalu_len);

/* Decode one length-prefixed AVC sample as a complete picture.  All slices
 * are submitted together, matching the two-slice files produced by iTunes'
 * "Create iPod or iPhone Version" action. */
int vpu_h264_decode_sample(struct vpu_h264 *v, const uint8_t *sample,
                           int sample_len, int nalu_len_size);

/* Get pointers to last decoded frame planes (YCbCr 4:2:0 planar).
 * Pointers are cached aliases; cache is already invalidated.
 * stride is the coded luma pitch; w/h are the cropped display dimensions.
 * Valid until the next decode call. */
void vpu_h264_get_frame(const struct vpu_h264 *v,
                         const uint8_t **y, const uint8_t **cb,
                         const uint8_t **cr, int *w, int *h, int *stride);

/* Shutdown decoder. Powers off VPU-B hardware. */
void vpu_h264_close(struct vpu_h264 *v);

#endif /* VPU_H264_6G_H */
