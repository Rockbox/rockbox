/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2026 Mauricio Garrido
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

#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "debug.h"
#include "logf.h"

#include "sys_display.h"

#include <pspdisplay.h>
#include <pspkernel.h>
#include <pspge.h>
#include <pspgu.h>

#define PSP_SLICE_SIZE  (32)
#define PSP_LINE_SIZE   (512)
#define SCREEN_WIDTH    (480)
#define SCREEN_HEIGHT   (272)

struct rect {
    int x, y, w, h;
};

struct display_context {
    void *vram_base;
    int frame;                     /* The current draw frame for double-buffered displays. */
    unsigned int frame_offset;     /* Byte offset of the start of the second frame. */
    int bytes_per_pixel;           /* Bytes per pixel of current screen */
    int pitch;                     /* Pitch */
    void *pixels;                  /* Pixels pointer */
} display_ctx;

static const struct rect fullscreen_rect = { 0, 0, 480, 272 };

/* sceGu* display list */
static unsigned int list[4096] __attribute__((aligned(16)));

/* return a suitable psm value for a particular bpp */
static inline int psm_value(int bpp) {
    if (bpp == 15)
        return GU_PSM_5551;
    if (bpp == 16)
        return GU_PSM_5650;
    return GU_PSM_8888;
}

static void gu_init(int bpp) {
    void *dispbuffer = (void*) 0;
    void *drawbuffer = (void*) display_ctx.frame_offset;

    sceGuInit();
    sceGuStart(GU_DIRECT, list);
    sceGuDispBuffer(SCREEN_WIDTH, SCREEN_HEIGHT, dispbuffer, PSP_LINE_SIZE);
    sceGuDrawBuffer(psm_value(bpp), drawbuffer, PSP_LINE_SIZE);
    sceGuClear(GU_COLOR_BUFFER_BIT | GU_DEPTH_BUFFER_BIT);
    sceGuOffset(2048 - (SCREEN_WIDTH / 2), 2048 - (SCREEN_HEIGHT / 2));
    sceGuViewport(2048, 2048, SCREEN_WIDTH, SCREEN_HEIGHT);
    sceGuScissor(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
    sceGuEnable(GU_SCISSOR_TEST);
    sceGuFrontFace(GU_CW);

    sceGuFinish();
    sceGuSync(0, 0);

    sceDisplayWaitVblankStart();
    sceGuDisplay(1);
}

void sys_set_video_mode(int bpp)
{
    int disp_pitch = 512, draw_pitch = 512, pixel_format;
    struct display_context *ctx = &display_ctx;

    switch(bpp) {
    case 15: /* 5-5-5-1 */
        disp_pitch *= 2;
        draw_pitch *= 2;
        pixel_format = PSP_DISPLAY_PIXEL_FORMAT_5551;
        ctx->bytes_per_pixel = 2;
        break;
    case 16: /* 5-6-5 */
        disp_pitch *= 2;
        draw_pitch *= 2;
        pixel_format = PSP_DISPLAY_PIXEL_FORMAT_565;
        ctx->bytes_per_pixel = 2;
        break;
    case 32: /* 8-8-8-8 */
        disp_pitch *= 4;
        draw_pitch *= 4;
        pixel_format = PSP_DISPLAY_PIXEL_FORMAT_8888;
        ctx->bytes_per_pixel = 4;
        break;
    default:
        logf("error: couldn't find requested mode\n");
        return;
    }

    ctx->frame = 0;
    ctx->frame_offset = 0;

    /* allocate display buffer */
    ctx->vram_base = (void *) 0x4000000;

    sceDisplaySetMode(0, SCREEN_WIDTH, SCREEN_HEIGHT);
    sceDisplaySetFrameBuf(ctx->vram_base, 512, pixel_format,
        PSP_DISPLAY_SETBUF_NEXTFRAME);

    ctx->pitch = draw_pitch;

    /* double buffer */
    ctx->frame_offset = disp_pitch * SCREEN_HEIGHT;

    /* Set the draw buffer to the second frame. */
    ctx->frame = 1;

    /* set current screen pointer */
    ctx->pixels = (void *) ((uint32_t) ctx->vram_base + ctx->frame_offset);

    /* DEBUGF("vram_base_alloc0: %p\n", ctx->vram_base); */
    /* DEBUGF("vram_base_alloc1: %p\n", ctx->pixels); */
    /* DEBUGF("vram_base_nonalloc: %p\n", (void *) ((uint32_t) ctx->pixels + ctx->frame_offset)); */

    gu_init(bpp);
}

static void argb_to_abgr_row(const uint8_t* src_argb, uint8_t* dst_abgr, int width)
{
    int x;
    uint32_t *src_argb32 = (uint32_t *) src_argb;
    uint32_t *dst_abgr32 = (uint32_t *) dst_abgr;
    for (x = 0; x < width; x += 8) {
        *(dst_abgr32) = __builtin_bswap32(*(src_argb32)) >> 8;
        *(dst_abgr32+1) = __builtin_bswap32(*(src_argb32+1)) >> 8;
        *(dst_abgr32+2) = __builtin_bswap32(*(src_argb32+2)) >> 8;
        *(dst_abgr32+3) = __builtin_bswap32(*(src_argb32+3)) >> 8;
        *(dst_abgr32+4) = __builtin_bswap32(*(src_argb32+4)) >> 8;
        *(dst_abgr32+5) = __builtin_bswap32(*(src_argb32+5)) >> 8;
        *(dst_abgr32+6) = __builtin_bswap32(*(src_argb32+6)) >> 8;
        *(dst_abgr32+7) = __builtin_bswap32(*(src_argb32+7)) >> 8;
        src_argb32 += 8;
        dst_abgr32 += 8;
    }
}

int hw_accel_blit_abgr(const uint8_t *src_argb, const struct rect *srcrect, int src_pitch,
                       uint8_t *dst_abgr, int dst_pitch)
{
    int y;
    for (y = 0; y < srcrect->h; ++y) {
        argb_to_abgr_row(src_argb, dst_abgr, srcrect->w);
        src_argb += src_pitch;
        dst_abgr += dst_pitch;
    }

    return 0;
}

void sys_blit_surface_rect(void *src, int x, int y, int width, int height)
{
    struct display_context *ctx = &display_ctx;
    const struct rect rect = { x, y, width, height };

   	size_t element = (y * 512) + x;
    fb_data *dst = (fb_data *) ctx->pixels + element;
    hw_accel_blit_abgr(src, &rect, LCD_WIDTH * FB_DATA_SZ,
        (void *) dst, ctx->pitch);
}

void sys_blit_surface(void *src)
{
    struct display_context *ctx = &display_ctx;
    hw_accel_blit_abgr(src, &fullscreen_rect, LCD_WIDTH * FB_DATA_SZ,
        ctx->pixels, ctx->pitch);
}

int sys_flip_screen(void)
{
    void *new_pixels;
    struct display_context *ctx = &display_ctx;

    sceKernelDcacheWritebackAll();
    ctx->frame ^= 1;
    new_pixels = (void *) ((uint32_t) ctx->vram_base +
            (ctx->frame_offset * ctx->frame));
    ctx->pixels = new_pixels;
    sceGuSwapBuffers();

    return 0;
}