/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
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

/*
 * GPU presenting for the bottom screen: instead of the rotated CPU copy,
 * the frame goes into a linear staging buffer, a display transfer tiles it
 * into a VRAM texture and citro2d draws one quad (its projection does the
 * rotation). Measured on an Old 3DS: 1.8 ms of CPU per frame against 4.1 ms.
 *
 * Kept apart from lcd-bitmap-3ds.c: citro3d pulls in all of <3ds.h>, which
 * clashes with Rockbox headers.
 *
 * Sequence per update, with nothing yielding between begin and end:
 *   lcd_gpu_try_begin() -> lcd_gpu_copy_rows() -> lcd_gpu_end()
 */

#include <string.h>
#include <3ds.h>
#include <citro3d.h>
#include <citro2d.h>

#include "lcd-gpu-3ds.h"

#define TEX_W  512
#define TEX_H  256

/* Verified on hardware: no FLIP_VERT, subtexture top = 1. Keep top > bottom,
 * citro2d takes top < bottom for a rotated tex3ds subtexture. */
#define TEX_TRANSFER_FLAGS \
    (GX_TRANSFER_FLIP_VERT(0) | GX_TRANSFER_OUT_TILED(1) | GX_TRANSFER_RAW_COPY(0) | \
     GX_TRANSFER_IN_FORMAT(GX_TRANSFER_FMT_RGB565) | \
     GX_TRANSFER_OUT_FORMAT(GX_TRANSFER_FMT_RGB565) | \
     GX_TRANSFER_SCALING(GX_TRANSFER_SCALE_NO))

#define SCREEN_TRANSFER_FLAGS \
    (GX_TRANSFER_FLIP_VERT(0) | GX_TRANSFER_OUT_TILED(0) | GX_TRANSFER_RAW_COPY(0) | \
     GX_TRANSFER_IN_FORMAT(GX_TRANSFER_FMT_RGB565) | \
     GX_TRANSFER_OUT_FORMAT(GX_TRANSFER_FMT_RGB565) | \
     GX_TRANSFER_SCALING(GX_TRANSFER_SCALE_NO))

static C3D_RenderTarget *target;
static C3D_Tex tex;
static Tex3DS_SubTexture subtex;
static C2D_Image img;
static u16 *staging;      /* linear, TEX_W stride: what is on screen */
static int lcd_w, lcd_h;
static bool tex_ready, c3d_ready, c2d_ready;
static bool ready;

bool lcd_gpu_ready(void)
{
    return ready;
}

bool lcd_gpu_init(int width, int height)
{
    lcd_w = width;
    lcd_h = height;

    c3d_ready = C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
    if (!c3d_ready)
        goto fail;
    c2d_ready = C2D_Init(8);  /* one quad per frame */
    if (!c2d_ready)
        goto fail;
    C2D_Prepare();
    /* one opaque full-screen quad per frame: no depth buffer, no clear */
    C3D_DepthTest(false, GPU_ALWAYS, GPU_WRITE_COLOR);

    /* the LCD's native portrait shape */
    target = C3D_RenderTargetCreate(240, 320, GPU_RB_RGB565, -1);
    if (!target)
        goto fail;
    C3D_RenderTargetSetOutput(target, GFX_BOTTOM, GFX_LEFT, SCREEN_TRANSFER_FLAGS);

    tex_ready = C3D_TexInitVRAM(&tex, TEX_W, TEX_H, GPU_RGB565);
    if (!tex_ready)
        goto fail;
    C3D_TexSetFilter(&tex, GPU_NEAREST, GPU_NEAREST);

    subtex = (Tex3DS_SubTexture){ width, height, 0.0f, 1.0f,
                                  (float)width / TEX_W,
                                  1.0f - (float)height / TEX_H };
    img = (C2D_Image){ &tex, &subtex };

    staging = linearAlloc(TEX_W * TEX_H * 2);
    if (!staging)
        goto fail;
    memset(staging, 0, TEX_W * TEX_H * 2);
    GSPGPU_FlushDataCache(staging, TEX_W * TEX_H * 2);

    ready = true;
    return true;

fail:
    lcd_gpu_exit();
    return false;
}

void lcd_gpu_exit(void)
{
    ready = false;
    if (staging) {
        linearFree(staging);
        staging = NULL;
    }
    if (tex_ready) {
        C3D_TexDelete(&tex);
        tex_ready = false;
    }
    if (target) {
        C3D_RenderTargetDelete(target);
        target = NULL;
    }
    if (c2d_ready) {
        C2D_Fini();
        c2d_ready = false;
    }
    if (c3d_ready) {
        C3D_Fini();
        c3d_ready = false;
    }
}

bool lcd_gpu_try_begin(void)
{
    return C3D_FrameBegin(C3D_FRAME_NONBLOCK);
}

bool lcd_gpu_begin(void)
{
    return C3D_FrameBegin(0);
}

bool lcd_gpu_present_pending(void)
{
    return gspIsPresentPending(GSP_SCREEN_BOTTOM);
}

void lcd_gpu_copy_rows(const u16 *src, int src_stride, int x, int y, int w, int h)
{
    const u16 *s = src + y * src_stride + x;
    u16 *d = staging + y * TEX_W + x;

    for (int row = 0; row < h; row++) {
        memcpy(d, s, w * 2);
        s += src_stride;
        d += TEX_W;
    }
}

void lcd_gpu_end(void)
{
    /* nearly all updates are full screen: no dirty tracking */
    GSPGPU_FlushDataCache(staging, TEX_W * lcd_h * 2);

    /* inside a frame this only queues the transfer, ahead of the draw */
    C3D_SyncDisplayTransfer((u32 *)staging, GX_BUFFER_DIM(TEX_W, TEX_H),
                            (u32 *)tex.data, GX_BUFFER_DIM(TEX_W, TEX_H),
                            TEX_TRANSFER_FLAGS);
    C2D_SceneBegin(target);
    C2D_DrawImageAt(img, 0.0f, 0.0f, 0.5f, NULL, 1.0f, 1.0f);

    /* C3D_FrameEnd transfers into the back buffer: as in the CPU path, it
     * must no longer be on screen. The caller normally waited already
     * (lcd_gpu_present_pending), this only blocks in early boot or panic */
    while (gspIsPresentPending(GSP_SCREEN_BOTTOM))
        gspWaitForVBlank1();
    C3D_FrameEnd(0);
}
