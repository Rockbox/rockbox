/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2020 Sebastian Leonhardt
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


#ifndef TIA_AUDIO_H
#define TIA_AUDIO_H

#ifndef ATARI_NO_SOUND


#ifdef SIMULATOR
# define ATARI_SAMPLER      SAMPR_44
#else /* !SIMULATOR */

#if defined(HW_HAVE_16)
# define ATARI_SAMPLER      SAMPR_16
#elif defined(HW_HAVE_22)
# define ATARI_SAMPLER      SAMPR_22
#else
# define ATARI_SAMPLER      SAMPR_44
#endif

#endif /* !SIMULATOR */


typedef int16_t SAMPLE;


struct channel {
    /* atari register settings */
    uint8_t     freq;
    uint8_t     vol;
    uint8_t     ctrl;
    /* current values */
    int8_t      cur_v;   // current pcm value
    int8_t      cur_f; // current value of the divider
    int32_t     noise; /* must be nonzero */
    uint32_t    poly4;
    uint32_t    div31;
    uint32_t    poly5;
    uint32_t    div31_pulse;
};

struct pcm {
    bool        audio_on;
    uint8_t     dsp_setting;
    uint8_t     stereo_setting;
    /* current values */
    SAMPLE      *buf_play;  /* current buffer to play */
    SAMPLE      *buf_write; /* buffer to write to */
    size_t      buf_len;    /* buffer length in samples (stereo-aware) */
    uint8_t     buf_frames; /* number of frames to buffer sound before playback */
    struct channel chn[2];
};

extern struct pcm pcm;


void atari_pcm_init(void);
void atari_pcm_stop(void);
void start_audio(void);
void stop_audio(void);
void make_sound(void);

void tia_audc0(unsigned a, unsigned b);
void tia_audc1(unsigned a, unsigned b);
void tia_audf0(unsigned a, unsigned b);
void tia_audf1(unsigned a, unsigned b);
void tia_audv0(unsigned a, unsigned b);
void tia_audv1(unsigned a, unsigned b);


#endif /* ATARI_NO_SOUND */

#endif /* TIA_AUDIO_H */
