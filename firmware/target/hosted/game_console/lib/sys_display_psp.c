/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
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

#include "system.h"
#include "kernel.h"
#include "sys_display.h"

#include <pspdisplay.h>
#include <pspkernel.h>
#include <pspge.h>
#include <pspgu.h>

#define PSP_SLICE_SIZE  (32)
#define PSP_LINE_SIZE   (512)
#define PSP_VRAM_UNCACHED 0x40000000  /* uncached alias of VRAM */
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
} display_ctx;

static const struct rect fullscreen_rect = { 0, 0, 480, 272 };

static void ge_init(void);

/* VRAM is written through the uncached alias: faster than cached stores plus
 * a cache writeback, and nothing is left behind for the display to miss */
static void *frame_uncached(int frame)
{
    struct display_context *ctx = &display_ctx;
    return (void *) (((uint32_t) ctx->vram_base | PSP_VRAM_UNCACHED) +
                     ctx->frame_offset * frame);
}

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


    /* DEBUGF("vram_base_alloc0: %p\n", ctx->vram_base); */
    /* DEBUGF("vram_base_alloc1: %p\n", ctx->pixels); */
    /* DEBUGF("vram_base_nonalloc: %p\n", (void *) ((uint32_t) ctx->pixels + ctx->frame_offset)); */

    gu_init(bpp);
    ge_init();

    /* Retire cached aliases before writing only through uncached ones */
    sceKernelDcacheWritebackInvalidateAll();
    memset(frame_uncached(0), 0, 2 * ctx->frame_offset);
    __asm__ volatile("sync" : : : "memory");
}

static void argb_to_abgr_row(const uint32_t *src, uint32_t *dst, int width)
{
    int x;
    for (x = 0; x + 8 <= width; x += 8) {
        dst[0] = __builtin_bswap32(src[0]) >> 8;
        dst[1] = __builtin_bswap32(src[1]) >> 8;
        dst[2] = __builtin_bswap32(src[2]) >> 8;
        dst[3] = __builtin_bswap32(src[3]) >> 8;
        dst[4] = __builtin_bswap32(src[4]) >> 8;
        dst[5] = __builtin_bswap32(src[5]) >> 8;
        dst[6] = __builtin_bswap32(src[6]) >> 8;
        dst[7] = __builtin_bswap32(src[7]) >> 8;
        src += 8;
        dst += 8;
    }
    for (; x < width; x++)
        *dst++ = __builtin_bswap32(*src++) >> 8;
}

static void cpu_copy(const fb_data *fb, const struct rect *r, int frame)
{
    const uint32_t *src = (const uint32_t *)fb + r->y * LCD_WIDTH + r->x;
    uint32_t *dst = (uint32_t *)frame_uncached(frame) + r->y * PSP_LINE_SIZE + r->x;

    for (int y = 0; y < r->h; y++) {
        argb_to_abgr_row(src, dst, r->w);
        src += LCD_WIDTH;
        dst += PSP_LINE_SIZE;
    }

    __asm__ volatile("sync" : : : "memory");
}

/*
 * The GE can't swap red and blue in a copy, but it can read the framebuffer as
 * a 32-bit indexed texture: the CLUT shift picks one channel as the index, a
 * gray ramp returns it, and the pixel mask stores it to the right channel. So
 * three passes over the frame do the conversion, while the CPU runs the other
 * Rockbox threads.
 *
 * Measured on both PSPs at 222 MHz, per full frame: 8.1 ms for the GE, of
 * which 3 ms is the copy that frees the framebuffer, with 0.1 ms of CPU,
 * against 7.1 ms of CPU for cpu_copy(). The passes read from a copy in spare
 * VRAM (25.8 ms when reading main RAM) in narrow sprites, which suit the
 * texture cache (32.7 ms as one full-width sprite).
 */
#define GE_SLICE_WIDTH 32

struct ge_vertex {
    unsigned short u, v;
    short x, y, z;
};

static unsigned int ge_clut[256] __attribute__((aligned(64)));
static struct ge_vertex ge_vertices[2 * (SCREEN_WIDTH / GE_SLICE_WIDTH + 2)]
    __attribute__((aligned(16)));

/* above the main thread: it only starts lists and waits for them */
#define GE_THREAD_PRIORITY 20

static SceUID ge_thread = -1;
static SceUID ge_event;
static struct semaphore ge_source_free;
static struct semaphore ge_idle;
static volatile bool ge_quit = false;

static void ge_init(void)
{
    for (int i = 0; i < 256; i++)
        ge_clut[i] = 0xff000000u | (i * 0x010101u);
    sceKernelDcacheWritebackRange(ge_clut, sizeof(ge_clut));
}

static bool ge_usable(const fb_data *fb)
{
    /* Waiting yields, which needs interrupts on (not at boot or in a panic) */
    int oldlevel = disable_irq_save();
    restore_irq(oldlevel);
    return oldlevel == 0 && ((uintptr_t)fb & 15) == 0;
}

/* The job for the GE thread */
static struct {
    const fb_data *fb;
    struct rect rect;
    int frame;
    bool flip;
} ge_job;

static void ge_finish_list(void)
{
    sceGuFinish();
    sceGuSync(GU_SYNC_FINISH, GU_SYNC_WHAT_DONE);
}

static void ge_release(struct semaphore *s)
{
    sim_enter_irq_handler();
    semaphore_release(s);
    sim_exit_irq_handler();
}

/* Copy to spare VRAM: when this ends the framebuffer is free again */
static void ge_stage(void *stage)
{
    const struct rect *r = &ge_job.rect;

    sceGuStart(GU_DIRECT, list);
    sceGuCopyImage(GU_PSM_8888, r->x, r->y, r->w, r->h, LCD_WIDTH,
                   (void *)ge_job.fb, r->x, r->y, PSP_LINE_SIZE, stage);
    sceGuTexSync();
    ge_finish_list();
}

static void ge_convert(void *stage)
{
    static const struct { int shift; unsigned int mask; } pass[3] = {
        { 16, 0xffffff00 }, /* source red   -> byte 0 */
        {  8, 0xffff00ff }, /* source green -> byte 1 */
        {  0, 0xff00ffff }, /* source blue  -> byte 2 */
    };
    struct display_context *ctx = &display_ctx;
    const struct rect *r = &ge_job.rect;

    int n = 0;
    for (int x = r->x; x < r->x + r->w; x += GE_SLICE_WIDTH) {
        int end = MIN(x + GE_SLICE_WIDTH, r->x + r->w);
        ge_vertices[n++] = (struct ge_vertex){ x, r->y, x, r->y, 0 };
        ge_vertices[n++] = (struct ge_vertex){ end, r->y + r->h, end, r->y + r->h, 0 };
    }
    sceKernelDcacheWritebackRange(ge_vertices, sizeof(ge_vertices));

    sceGuStart(GU_DIRECT, list);
    sceGuDrawBufferList(GU_PSM_8888, (void *)(ctx->frame_offset * ge_job.frame),
                        PSP_LINE_SIZE);
    sceGuDisable(GU_DEPTH_TEST);
    sceGuDisable(GU_BLEND);
    sceGuDisable(GU_ALPHA_TEST);
    sceGuEnable(GU_TEXTURE_2D);
    sceGuTexFunc(GU_TFX_REPLACE, GU_TCC_RGB);
    sceGuTexFilter(GU_NEAREST, GU_NEAREST);
    sceGuTexWrap(GU_CLAMP, GU_CLAMP);
    sceGuTexScale(1.0f, 1.0f);
    sceGuTexOffset(0.0f, 0.0f);
    sceGuClutLoad(256 / 8, ge_clut);
    sceGuTexMode(GU_PSM_T32, 0, 0, 0);
    sceGuTexImage(0, 512, 512, PSP_LINE_SIZE, stage);
    for (int i = 0; i < 3; i++) {
        sceGuClutMode(GU_PSM_8888, pass[i].shift, 0xff, 0);
        sceGuPixelMask(pass[i].mask);
        sceGuTexFlush();
        sceGuDrawArray(GU_SPRITES,
                       GU_TEXTURE_16BIT | GU_VERTEX_16BIT | GU_TRANSFORM_2D,
                       n, NULL, ge_vertices);
    }
    sceGuPixelMask(0);
    ge_finish_list();
}

/* All waiting for the GE happens here: the Rockbox thread sleeps on a
 * semaphore that this thread releases like an interrupt handler */
static int ge_thread_run(SceSize args, void *argp)
{
    (void)args;
    (void)argp;
    struct display_context *ctx = &display_ctx;

    /* third VRAM buffer, after the two frames */
    void *stage = (void *)((uintptr_t)ctx->vram_base + 2 * ctx->frame_offset);

    while (1)
    {
        sceKernelWaitSema(ge_event, 1, NULL);
        if (ge_quit)
            break;

        ge_stage(stage);
        ge_release(&ge_source_free);

        ge_convert(stage);
        if (ge_job.flip)
        {
            ctx->frame ^= 1;
            sceGuSwapBuffers();
        }
        ge_release(&ge_idle);
    }

    return 0;
}

static void ge_start_thread(void)
{
    semaphore_init(&ge_source_free, 1, 0);
    semaphore_init(&ge_idle, 1, 1);

    ge_event = sceKernelCreateSema("rb_ge", 0, 0, 1, NULL);
    ge_thread = sceKernelCreateThread("rb_ge", ge_thread_run, GE_THREAD_PRIORITY,
                                      16 * 1024, PSP_THREAD_ATTR_USER, NULL);
    sceKernelStartThread(ge_thread, 0, NULL);
}

/* Returns once the framebuffer may change: the conversion and the flip go on
 * in the background, the next update waits for them */
static void ge_present(const fb_data *fb, const struct rect *r, bool flip)
{
    if (ge_thread < 0)
        ge_start_thread();

    semaphore_wait(&ge_idle, TIMEOUT_BLOCK);

    ge_job.fb = fb;
    ge_job.rect = *r;
    ge_job.flip = flip;
    ge_job.frame = flip ? display_ctx.frame : display_ctx.frame ^ 1;

    sceKernelDcacheWritebackRange((const uint32_t *)fb + r->y * LCD_WIDTH,
                                  r->h * LCD_WIDTH * sizeof(uint32_t));

    sceKernelSignalSema(ge_event, 1);
    semaphore_wait(&ge_source_free, TIMEOUT_BLOCK);
}

/* A partial update goes to the visible frame, without a flip. Its rows are
 * always the latest, while the other frame is one update behind, so copying
 * there and flipping would bring back old pixels outside the rectangle. */
void sys_update_rect(const fb_data *fb, int x, int y, int width, int height)
{
    const struct rect rect = { x, y, width, height };

    if (ge_usable(fb)) {
        ge_present(fb, &rect, false);
        return;
    }
    cpu_copy(fb, &rect, display_ctx.frame ^ 1);
}

void sys_update(const fb_data *fb)
{
    struct display_context *ctx = &display_ctx;

    if (ge_usable(fb)) {
        ge_present(fb, &fullscreen_rect, true);
        return;
    }
    cpu_copy(fb, &fullscreen_rect, ctx->frame);
    ctx->frame ^= 1;
    sceGuSwapBuffers();
}

void sys_display_quit(void)
{
    if (ge_thread < 0)
        return;

    semaphore_wait(&ge_idle, TIMEOUT_BLOCK);

    ge_quit = true;
    sceKernelSignalSema(ge_event, 1);
    sceKernelWaitThreadEnd(ge_thread, NULL);
    sceKernelDeleteThread(ge_thread);
    sceKernelDeleteSema(ge_event);
    ge_thread = -1;
}
