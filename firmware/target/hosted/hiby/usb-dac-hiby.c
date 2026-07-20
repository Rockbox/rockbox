/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2026 by Michael McAllister
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

/* Host-PCM pump for the HiBy USB DAC gadget mode.
 *
 * The vendor kernel's UAC gadget function "uac_sa" exposes the char device
 * /dev/uac_sa, which delivers the host's PCM: the isochronous OUT frames are
 * converted to left-justified S16-in-S32 stereo and queued in a kernel ring,
 * drained with read(). read() is non-blocking and returns -1 until at least
 * the requested count (which must be <= the kernel ring/2) is buffered.
 *
 * A pump thread drains it into a small single-producer/single-consumer
 * ring, the mixer's get_more callback hands that PCM to the CS43131 through
 * the normal ALSA output path, the same way firmware/usbstack/usb_audio.c
 * drives its host-PCM playback.
 */

#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "config.h"
#include "kernel.h"
#include "pcm_mixer.h"
#include "pcm_sampr.h"
#include "system.h"
#include "thread.h"
#include "usb.h"
#include "usb-dac-hiby.h"

/*#define LOGF_ENABLE*/
#include "logf.h"

#define UAC_SA_DEV        "/dev/uac_sa"
/* ioctl: fills int[3] = { fmt, rate_hz, bits } */
#define UAC_SA_GET_STATUS 1
/* uac_sa delivers S32 stereo: 8 bytes per frame */
#define UAC_SA_FRAME_SIZE 8

/* Ring of S16 stereo frames. A power of two so head/tail wrap cleanly. */
#define DAC_RING_FRAMES   8192  /* ~186 ms at 44.1 kHz */
#define DAC_CHUNK_FRAMES  512   /* mixer buffer granularity */
/* one read()'s worth. Must stay <= the kernel ring/2 */
#define DAC_READ_BYTES    (DAC_CHUNK_FRAMES * UAC_SA_FRAME_SIZE)

static int16_t dac_ring[DAC_RING_FRAMES * 2];
static volatile unsigned int dac_head;  /* frames produced; pump only */
static volatile unsigned int dac_tail;  /* frames consumed; mixer only */
static pthread_t dac_thread;
static volatile bool dac_running;
static int dac_fd = -1;

/* Mixer callback (PCM feed context). Returns the next chunk from the ring,
 * zero-padded with silence on underrun so the stream never stalls. Double
 * buffered so the returned pointer stays valid until the following call. */
static void dac_get_more(const void **start, size_t *size)
{
    static int16_t out[2][DAC_CHUNK_FRAMES * 2];
    static int which;
    int16_t *buf;

    which ^= 1;
    buf = out[which];

    unsigned int tail = dac_tail;
    unsigned int avail = dac_head - tail;         /* wrap-safe */
    avail = MIN(avail, DAC_CHUNK_FRAMES);

    for (unsigned int i = 0; i < avail; i++)
    {
        unsigned int idx = (tail + i) & (DAC_RING_FRAMES - 1);
        buf[2 * i]     = dac_ring[2 * idx];
        buf[2 * i + 1] = dac_ring[2 * idx + 1];
    }
    memset(&buf[2 * avail], 0,
           (DAC_CHUNK_FRAMES - avail) * 2 * sizeof(int16_t));
    dac_tail = tail + avail;

    *start = buf;
    *size = DAC_CHUNK_FRAMES * 2 * sizeof(int16_t);
}

/* Drain /dev/uac_sa into the ring. The driver's only file
 * operations are open/read/ioctl (there is no poll()), so retry with a
 * short sleep when the host has not queued a full chunk yet.
 *
 * This is a raw pthread, not a Rockbox thread, so it must use usleep()
 * rather than the cooperative sleep() (which only the scheduler's own
 * threads may call). */
static void *dac_pump_thread(void *arg)
{
    int32_t rbuf[DAC_READ_BYTES / sizeof(int32_t)];
    (void)arg;

    while (dac_running)
    {
        unsigned int head = dac_head;

        /* Leave room for a full read; else wait for the mixer to drain */
        if (head - dac_tail > DAC_RING_FRAMES - DAC_CHUNK_FRAMES)
        {
            usleep(1000);
            continue;
        }

        ssize_t n = read(dac_fd, rbuf, DAC_READ_BYTES);
        if (n <= 0)
        {
            usleep(1000);       /* no host data queued yet */
            continue;
        }

        int frames = n / UAC_SA_FRAME_SIZE;
        for (int i = 0; i < frames; i++)
        {
            unsigned int idx = (head + i) & (DAC_RING_FRAMES - 1);
            dac_ring[2 * idx]     = (int16_t)(rbuf[2 * i]     >> 16);
            dac_ring[2 * idx + 1] = (int16_t)(rbuf[2 * i + 1] >> 16);
        }
        dac_head = head + frames;
    }

    return NULL;
}

bool usb_dac_start(void)
{
    static const struct mixer_play_cbs cbs = { .get_more = dac_get_more };
    int st[3] = {0, 0, 0};
    unsigned int rate;

    /* The node is created asynchronously when the UDC binds; during boot
     * the hotplug helper can lag behind, so wait briefly for it. This runs
     * on a Rockbox thread, so sleep() yields to the cooperative scheduler
     * instead of stalling every other thread the way usleep() would. */
    for (int tries = 50; tries > 0; tries--)
    {
        dac_fd = open(UAC_SA_DEV, O_RDWR);
        if (dac_fd >= 0)
            break;
        sleep(HZ / 50);     /* 20 ms */
    }
    if (dac_fd < 0)
    {
        logf("uac_sa open failed (%d)", errno);
        return false;
    }

    /* Play at the rate the host negotiated (reported by the kernel). */
    ioctl(dac_fd, UAC_SA_GET_STATUS, st);
    if (st[1] >= SAMPR_8 && st[1] <= SAMPR_192)
        rate = (unsigned int)st[1];
    else
        rate = SAMPR_44;
    mixer_set_frequency(rate);

    dac_head = dac_tail = 0;
    dac_running = true;
    if (pthread_create(&dac_thread, NULL, dac_pump_thread, NULL) != 0)
    {
        logf("uac_sa pump thread failed");
        dac_running = false;
        close(dac_fd);
        dac_fd = -1;
        return false;
    }

    mixer_channel_set_amplitude(PCM_MIXER_CHAN_USBAUDIO, MIX_AMP_UNITY);
    mixer_channel_play_data(PCM_MIXER_CHAN_USBAUDIO, &cbs, NULL, 0);
    return true;
}

void usb_dac_stop(void)
{
    if (!dac_running)
        return;

    dac_running = false;
    pthread_join(dac_thread, NULL);
    mixer_channel_stop(PCM_MIXER_CHAN_USBAUDIO);

    if (dac_fd >= 0)
    {
        close(dac_fd);
        dac_fd = -1;
    }
}

bool usb_audio_get_active(void)
{
    return dac_running;
}
