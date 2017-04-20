/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 by Tomek Malesinski
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
#include "system.h"
#include "audio.h"
#include "string.h"
#include "pcm-internal.h"

#define DMA_BUF_SAMPLES 0x100

short __attribute__((section(".dmabuf"))) dma_buf_left[DMA_BUF_SAMPLES];
short __attribute__((section(".dmabuf"))) dma_buf_right[DMA_BUF_SAMPLES];

const int16_t* p IBSS_ATTR;
unsigned long p_frames IBSS_ATTR;

void pcm_play_dma_lock(void)
{
}

void pcm_play_dma_unlock(void)
{
}

void pcm_play_dma_send_frames(const void *addr, unsigned long frames)
{
    p = addr;
    p_frames = frames;
}

void pcm_play_dma_prepare(void)
{
}

void pcm_play_dma_stop(void)
{
}

void pcm_play_dma_pause(bool pause)
{
    (void)pause;
}

static inline void fill_dma_buf(int offset)
{
    short *l, *r, *lend;

    l = dma_buf_left + offset;
    lend = l + DMA_BUF_SAMPLES / 2;
    r = dma_buf_right + offset;

    if (pcm_playing && !pcm_paused)
    {
        do
        {
            unsigned long count;
            const int16_t *tmp_p;
            count = MIN(p_frames, (unsigned long)(lend - l));
            tmp_p = p;
            p_frames -= count * 4;

            if ((int)l & 3)
            {
                *l++ = *tmp_p++;
                *r++ = *tmp_p++;
                count--;
            }
            while (count >= 4)
            {
                asm("ldmia %0!, {r0, r1, r2, r3}\n\t"
                    "and   r4, r0, %3\n\t"
                    "orr   r4, r4, r1, lsl #16\n\t"
                    "and   r5, r2, %3\n\t"
                    "orr   r5, r5, r3, lsl #16\n\t"
                    "stmia %1!, {r4, r5}\n\t"
                    "bic   r4, r1, %3\n\t"
                    "orr   r4, r4, r0, lsr #16\n\t"
                    "bic   r5, r3, %3\n\t"
                    "orr   r5, r5, r2, lsr #16\n\t"
                    "stmia %2!, {r4, r5}"
                    : "+r" (tmp_p), "+r" (l), "+r" (r)
                    : "r" (0xffff)
                    : "r0", "r1", "r2", "r3", "r4", "r5", "memory");
                count -= 4;
            }
            while (count > 0)
            {
                *l++ = *tmp_p++;
                *r++ = *tmp_p++;
                count--;
            }
            p = tmp_p;

            if (l >= lend)
                return;

            pcm_play_dma_complete(0);
        }
        while (p_frames);
    }

    if (l < lend)
    {
        memset(l, 0, sizeof(short) * (lend - l));
        memset(r, 0, sizeof(short) * (lend - l));
    }
}

static void audio_irq(void)
{
    unsigned long st = DMAINTSTAT & ~DMAINTEN;
    int i;
    for (i = 0; i < 2; i++)
        if (st & (1 << i))
        {
            fill_dma_buf((i == 1) ? 0 : DMA_BUF_SAMPLES / 2);
            DMAINTSTAT = 1 << i;
        }
}

unsigned long physical_address(void *p)
{
    unsigned long adr = (unsigned long)p;
    return (MMUBLOCK((adr >> 21) & 0xf) << 21) | (adr & ((1 << 21) - 1));
}

void pcm_dma_init(const struct pcm_hw_settings *settings)
{
    int i;

    memset(dma_buf_left, 0, sizeof(dma_buf_left));
    memset(dma_buf_right, 0, sizeof(dma_buf_right));

    for (i = 0; i < 8; i++)
    {
        DMASRC(i) = 0;
        DMADEST(i) = 0;
        DMALEN(i) = 0x1ffff;
        DMAR0C(i) = 0;
        DMAR10(i) = 0;
        DMAR1C(i) = 0;
    }

    DMAINTSTAT = 0xc000ffff;
    DMAINTEN = 0xc000ffff;
    
    DMASRC(0) = physical_address(dma_buf_left);
    DMADEST(0) = 0x80200280;
    DMALEN(0) = 0xff;
    DMAR1C(0) = 0;
    DMAR0C(0) = 0x40408;

    DMASRC(1) = physical_address(dma_buf_right);
    DMADEST(1) = 0x80200284;
    DMALEN(1) = 0xff;
    DMAR1C(1) = 0;
    DMAR0C(1) = 0x40409;

    irq_set_int_handler(0x1b, audio_irq);
    irq_enable_int(0x1b);

    DMAINTSTAT = 1;
    DMAINTSTAT = 2;
    DMAINTEN &= ~3;
    DMAR10(0) |= 1;
    DMAR10(1) |= 1;

    (void)settings;
}

void pcm_dma_apply_settings(const struct pcm_hw_settings *settings)
{
    (void)settings;
}

unsigned long pcm_play_dma_get_frames_waiting(void)
{
    return p_frames;
}

const void * pcm_play_dma_get_peak_buffer(unsigned long *frames_rem)
{
    unsigned long addr = (unsigned long)*(volatile const int16_t **)&p;
    *frames_rem = p_frames;
    return (void *)((addr + 2) & ~3);
}

void audiohw_set_volume(int value)
{
    int tmp = (60 - value * 4) & 0xff;
    CODECVOL = tmp | (tmp << 8);
}
