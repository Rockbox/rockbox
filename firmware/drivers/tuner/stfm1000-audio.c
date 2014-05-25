/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2016 by Amaury Pouly
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
#include "dma-imx233.h"
#include "icoll-imx233.h"
#include "dri-imx233.h"
#include "pcm.h"
#include "pcm_sampr.h"
//#define LOGF_ENABLE
#include "logf.h"
#include "queue.h"
#include "thread.h"
#include "usb.h"

#include "stfm1000-internal.h"

struct dri_dma_command_t
{
    struct apb_dma_command_t dma;
    /* padded to next multiple of cache line size (32 bytes) */
    uint32_t pad[5];
} __attribute__((packed)) CACHEALIGN_ATTR;

__ENSURE_STRUCT_CACHE_FRIENDLY(struct dri_dma_command_t)

/* The radio sends us data at a rate of 44.1 kS/s where each sample
 * has four values: L+R, L-R, RDS and RSSI. The RDS stream is in fact running
 * at 38 kS/S so some samples have a special value of 0xffff meaning "no data".
 * Finally RSSI is a floating pointer number encoded with an 11-bit mantissa
 * (the 11 msb's) and an exponent (the 5 lsb's) without bias. */
#define FRAMES_PER_BUFFER   1000

struct dri_sample_t
{
    uint16_t lpr; /* left+right or mono */
    uint16_t lmr; /* left-right */
    uint16_t rssi; /* encoded rssi */
    uint16_t rds; /* rds sample */
} __attribute__((packed));

struct audio_sample_t
{
    uint16_t left;
    uint16_t right;
} __attribute__((packed));

#define NR_BUFFERS      3

/* we use at least two buffers: on the DRI side, one is being filled by DMA while
 * the others are being processed. On the DAC side, one is being played while the
 * others are being filled or waiting to be played. */
static struct dri_dma_command_t dri_dma[NR_BUFFERS];
static struct dri_sample_t dri_buffer[NR_BUFFERS][FRAMES_PER_BUFFER];
static struct audio_sample_t dac_buffer[NR_BUFFERS][FRAMES_PER_BUFFER];
static volatile int cur_dri_buffer = 0; /* buffer being filled by DRI */
static volatile int cur_dac_buffer = 0; /* buffer being played by DAC */
static volatile enum
{
    PLAYING, /* audio is playing */
    STOPPED_WAIT_DRI, /* audio has stopped, waiting for next DRI completion */
    STOPPED_WAIT_RESTART, /* audio has stopped, waiting for thread to restart */
    STOPPED, /* audio is stopped and should not restart */
} playback_status;
/* audio state variables */
static bool soft_mute; /* soft mute */
static bool force_mono; /* force mono */
static int raw_stream; /* send raw stream instead of audio */

/* Buffer management:
 *
 * When playback_status == PLAYING, the system can be in either one of two states:
 * - dac_buffer[cur_dac_buffer] is being played by pcm
 * - dri_buffer[cur_dri_buffer] is either being filled by dri dma
 * OR
 * - dac_buffer[cur_dac_buffer] is being played by pcm
 * - dri_buffer[cur_dri_buffer] is either being processed by dri dma IRQ
 * - dac_buffer[cur_dri_buffer] is being filled by dri dma IRQ
 *
 * It is thus clear that we must never have cur_dri_buffer == cur_dac_buffer,
 * otherwise we would be playing data being filled. Thus the pcm callback
 * will stop if cur_dac_buffer ever becomes equal to cur_dri_buffer, which
 * means that the DRI interface is not feeding information fast enough. */

static long stfm_stack[DEFAULT_STACK_SIZE/sizeof(long)];
static struct event_queue stfm_queue;

/* thread messages */
enum
{
    Q_RESTART_AUDIO, /* restart audio */
};

static void dri_pcm_cb(const void **start, size_t *size)
{
    /* disable IRQ (initial buffering call might not be in IRQ context) */
    const int oldlevel = disable_irq_save();
    /* stop if asked */
    if(playback_status == STOPPED)
    {
        logf("stfm: stopped");
    }
    /* we have to stop if we catch up on DRI */
    else if(cur_dac_buffer == cur_dri_buffer)
    {
        playback_status = STOPPED_WAIT_DRI;
        logf("stfm: stopped, wait dri");
    }
    else
    {
        playback_status = PLAYING;
        *start = dac_buffer[cur_dac_buffer];
        *size = sizeof(dac_buffer[cur_dac_buffer]);
        /* advance to next buffer */
        cur_dac_buffer = (cur_dac_buffer + 1) % NR_BUFFERS;
    }
    /* restore IRQ */
    restore_irq(oldlevel);
}

static void stfm_thread(void) NORETURN_ATTR;
static void stfm_thread(void)
{
    struct queue_event ev;
    while(1)
    {
        queue_wait(&stfm_queue, &ev);
        if(ev.id == Q_RESTART_AUDIO)
        {
            logf("stfm: restart");
            /* No need to protect against IRQ: if we received this message,
             * no one can write playback status. Watch the order: we
             * first update the dac index to the last processed DRI buffer
             * to maximise delay until the next potential underflow. We then
             * start playing (this will change the state) */
            cur_dac_buffer = (cur_dri_buffer + NR_BUFFERS - 1) % NR_BUFFERS;
            pcm_play_data(dri_pcm_cb, NULL, NULL, 0);
        }
    }
}

static void process_audio(struct audio_sample_t *dac_buf, struct dri_sample_t *dri_buf)
{
    for(size_t i = 0; i < FRAMES_PER_BUFFER; i++)
    {
        if(raw_stream)
        {
            dac_buf[i].left = dri_buf[i].rssi;
            dac_buf[i].right = dri_buf[i].rds;
        }
        else
        {
            /* force mono for now */
            dac_buf[i].left = dac_buf[i].right = dri_buf[i].lpr;
        }
    }
}

void INT_DRI_DMA(void)
{
    discard_dcache_range(dri_buffer[cur_dri_buffer], sizeof(dri_buffer[cur_dri_buffer]));
    struct dri_sample_t *dri_buf = dri_buffer[cur_dri_buffer];
    struct audio_sample_t *dac_buf = dac_buffer[cur_dri_buffer];
    /* process data */
    if(soft_mute)
        memset(dac_buf, 0, FRAMES_PER_BUFFER * sizeof(struct audio_sample_t));
    else
        process_audio(dac_buf, dri_buf);
    /* advance buffer */
    cur_dri_buffer = (cur_dri_buffer + 1) % NR_BUFFERS;
    /* if playback was stopped and is waiting for a DRI buffer, notify thread */
    if(playback_status == STOPPED_WAIT_DRI)
    {
        logf("stfm: stopped, wait restart");
        playback_status = STOPPED_WAIT_RESTART;
        queue_post(&stfm_queue, Q_RESTART_AUDIO, 0);
    }

    imx233_dma_clear_channel_interrupt(APB_DRI);
}

void stfm1000_audio_run(bool en)
{
    if(en)
    {
        imx233_dri_enable(true);
        pcm_play_stop();
        pcm_set_frequency(FREQ_44);
        pcm_apply_settings();

        imx233_dma_reset_channel(APB_DRI);
        imx233_icoll_enable_interrupt(INT_SRC_DRI_DMA, true);
        imx233_dma_enable_channel_interrupt(APB_DRI, true);

        for(int i = 0; i < NR_BUFFERS; i++)
        {
            dri_dma[i].dma.next = &dri_dma[(i + 1) % NR_BUFFERS].dma;
            dri_dma[i].dma.buffer = (void *)dri_buffer[i];
            dri_dma[i].dma.cmd = BF_OR(APB_CHx_CMD, COMMAND_V(WRITE), IRQONCMPLT(1),
                CHAIN(1), XFER_COUNT(sizeof(dri_buffer[i])));
        }
        cur_dri_buffer = 0;
        cur_dac_buffer = 0;
        playback_status = STOPPED_WAIT_DRI;
        /* dma subsystem will make sure cached stuff is written to memory */
        imx233_dma_start_command(APB_DRI, &dri_dma[0].dma);
        imx233_dri_run(true);
    }
    else
    {
        /* avoid any race condition where we would stop audio but then dri would
         * restart it because we stop dri */
        const int oldlevel = disable_irq_save();
        playback_status = STOPPED;
        restore_irq(oldlevel);
        pcm_play_stop();
        imx233_dma_reset_channel(APB_DRI);
        imx233_icoll_enable_interrupt(INT_SRC_DRI_DMA, false);
        imx233_dri_run(false);
        imx233_dri_enable(false);
    }
}

bool stfm1000_audio_softmute(bool mute)
{
    bool oldval = soft_mute;
    soft_mute = mute;
    return oldval;
}

void stfm1000_audio_set_mono(bool mono)
{
    force_mono = mono;
}

void stfm1000_audio_set_raw_stream(int val)
{
    raw_stream = val;
}

void stfm1000_audio_init(void)
{
    queue_init(&stfm_queue, false);
    create_thread(stfm_thread, stfm_stack, sizeof(stfm_stack), 0,
            "stfm" IF_PRIO(, PRIORITY_REALTIME) IF_COP(, CPU));
}
