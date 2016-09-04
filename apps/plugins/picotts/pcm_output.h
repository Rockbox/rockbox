/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * PCM output buffer declarations
 *
 * Copyright (c) 2007 Michael Sevakis
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
#ifndef PCM_OUTPUT_H
#define PCM_OUTPUT_H

#ifndef ALIGNED_ATTR
#define ALIGNED_ATTR(x) __attribute__((aligned(x)))
#endif

#define PCM_HDR_SIZE        (sizeof (struct pcm_frame_header))
#define PCMOUT_BUFSIZE       ((48000/2)*4) /* 1/2 s @48kHz */
#define PCMOUT_HIGH_WM       ((PCMOUT_BUFSIZE*3)/4)
#define PCMOUT_PLAY_WM       (PCMOUT_BUFSIZE/4)
#define PCMOUT_LOW_WM        (PCMOUT_BUFSIZE/16)
#define PCMOUT_EMPTY_WM      (0)
struct pcm_frame_header   /* Header added to pcm data every time a decoded
                             audio frame is sent out */
{
    uint32_t size;        /* size of this frame - including header */
    unsigned char data[]; /* open array of audio data */
} ALIGNED_ATTR(4);

bool pcm_output_init(void *mem, size_t *size);
void pcm_output_exit(void);
void pcm_output_flush(void);
void pcm_output_play_pause(bool play);
void pcm_output_stop(void);
void pcm_output_drain(void);
void * pcm_output_get_buffer(ssize_t *size);
bool pcm_output_commit_data(ssize_t size);
bool pcm_output_empty(void);

#endif /* PCM_OUTPUT_H */
