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


#include "rbconfig.h"
#include "rb_lcd.h"
#include "rb_main.h"
#include "tia_audio.h"

#ifndef ATARI_NO_SOUND


struct pcm pcm = {
    .audio_on = true,
    .stereo_setting = 1, // stereo; TODO: make enum?
    .buf_frames = 3, // minimum = 1
};


#define AUDIOBUF_SZ     ( (ATARI_SAMPLER/25)*3*2 )  // we go for a (minimum) frame rate of 25 fps,
                                                    // 3 buffers, and stereo samples
#define AUDIOBUF_SZ_BYTES  (AUDIOBUF_SZ * sizeof(SAMPLE))  // stereo, 16 bit samples

static SAMPLE pcmbuf[AUDIOBUF_SZ];


void atari_pcm_init(void) COLD_ATTR;
void atari_pcm_init(void)
{
    rb->pcm_play_stop();

#if INPUT_SRC_CAPS != 0
    /* Select playback */
    rb->audio_set_input_source(AUDIO_SRC_PLAYBACK, SRCF_PLAYBACK);
    rb->audio_set_output_source(AUDIO_SRC_PLAYBACK);
#endif

    rb->pcm_set_frequency(ATARI_SAMPLER);
    DEBUGF("AUDIO: setting sample rate to %d\n", ATARI_SAMPLER);

    pcm.chn[0].noise =
    pcm.chn[1].noise = 0x1000; /* must be nonzero */
    pcm.chn[0].poly4 =
    pcm.chn[1].poly4 = 0x143b; /* 001010000111011 (15 bit) */
    pcm.chn[0].div31 =
    pcm.chn[1].div31 = 0x7ffc0000; /* 1111111111111000000000000000000 (31 bit) */
    pcm.chn[0].poly5 =
    pcm.chn[1].poly5 = 0x167c6ea1; /* 0010110011111000110111010100001 (31 bit) */
    pcm.chn[0].div31_pulse =
    pcm.chn[1].div31_pulse = 0x20000800; /* 0100000000000000000100000000000 (31 bit) */
}

void atari_pcm_stop(void)
{
    rb->pcm_play_stop();
    rb->pcm_set_frequency(HW_SAMPR_DEFAULT);
}


/* calculate buffer size according to framerate
 * Assumes audio is stopped. Must be called before
 * paused game is resumed. */
void set_audiobuf(void) COLD_ATTR;
void set_audiobuf(void)
{
    /* delete whole audio buffer and start from scratch */
    rb->memset(pcmbuf, 0, sizeof(pcmbuf));
    /* start at first buffer chunk */
    pcm.buf_play = &pcmbuf[0];

    if (screen.lockfps <= 0) // set double of default for "as fast as possible"
        pcm.buf_len = ATARI_SAMPLER / 120;
    else
        pcm.buf_len = ATARI_SAMPLER / screen.lockfps;
    pcm.buf_len *= 2; /* stereo */

    unsigned size = pcm.buf_len;

    pcm.buf_write = pcm.buf_play + (pcm.buf_frames+1) * size;
    if (pcm.buf_write + pcm.buf_len >= &pcmbuf[AUDIOBUF_SZ]) {
        pcm.buf_write = &pcmbuf[0];
    }
}


// ------------------------------------------------------------

/* TIA write functions concerning audio */

void tia_audc0(unsigned a, unsigned b)
{
    (void) a;
    struct channel *c = &pcm.chn[0];
    b &= 0x0f;
    if (b >= 12) {  /* audio clock divided by 3 */
        if (c->ctrl < 12) {
            c->freq *= 3;  // Note that 1 must be added to freq!
        }
    }
    else {
        if (c->ctrl >= 12) {    /* audio clock divided by 3 */
            c->freq /= 3;  // Note that 1 must be added to freq!
        }
    }
    c->ctrl = b;
}

void tia_audc1(unsigned a, unsigned b)
{
    (void) a;
    struct channel *c = &pcm.chn[1];
    b &= 0x0f;
    if (b >= 12) {  /* audio clock divided by 3 */
        if (c->ctrl < 12) {
            c->freq *= 3;  // Note that 1 must be added to freq!
        }
    }
    else {
        if (c->ctrl >= 12) {    /* audio clock divided by 3 */
            c->freq /= 3;  // Note that 1 must be added to freq!
        }
    }
    c->ctrl = b;
}

void tia_audf0(unsigned a, unsigned b)
{
    (void) a;
    struct channel *c = &pcm.chn[0];
    c->freq = (b & 0x1f) + 1;
    if (c->ctrl >= 12)
        c->freq *= 3;  // Note that 1 must be added to freq!
}

void tia_audf1(unsigned a, unsigned b)
{
    (void) a;
    struct channel *c = &pcm.chn[1];
    c->freq = (b & 0x1f) + 1;
    if (c->ctrl >= 12)
        c->freq *= 3;  // Note that 1 must be added to freq!
}

void tia_audv0(unsigned a, unsigned b)
{
    (void) a;
    struct channel *c = &pcm.chn[0];
    c->vol = b & 0x0f;
}

void tia_audv1(unsigned a, unsigned b)
{
    (void) a;
    struct channel *c = &pcm.chn[1];
    c->vol = b & 0x0f;
}


// --------------------------------------------------------------


/* returns PCM Value */
static int get_sample(struct channel *c)
{
    switch (c->ctrl) {
    case 0x00:
    case 0x0b:
        c->cur_v = c->vol;
        break;
    case 0x01:
        --c->cur_f;
        if (c->cur_f <= 0) {
            c->cur_f = c->freq;
            /* 15 bit shift register */
            if (c->poly4 & 0x0001) {
                c->cur_v = c->vol;
                c->poly4 |= 0x8000;    /* rotate bit pattern */
            }
            else {
                c->cur_v = 0;
            }
            c->poly4 >>= 1;
        }
        break;
    case 0x02:
        --c->cur_f;
        if (c->cur_f <= 0) {
            c->cur_f = c->freq;
            /* 15 bit shift register */
            if ((c->poly4 & 0x0001) && (c->div31 & 0x00000001)) {
                c->cur_v = c->vol;
                c->poly4 |= 0x8000;    /* rotate bit pattern */
                c->div31 |= 0x80000000;    /* rotate bit pattern */
            }
            else {
                c->cur_v = 0;
                if (c->poly4 & 0x0001)
                    c->poly4 |= 0x8000;    /* rotate bit pattern */
                if (c->div31 & 0x00000001)
                    c->div31 |= 0x80000000;    /* rotate bit pattern */
            }
            c->poly4 >>= 1;
            c->div31 >>= 1;
        }
        break;
    case 0x03:
        --c->cur_f;
        if (c->cur_f <= 0) {
            c->cur_f = c->freq;
            /* 15 bit shift register */
            if ((c->poly4 & 0x0001) && (c->poly5 & 0x00000001)) {
                c->cur_v = c->vol;
                c->poly4 |= 0x8000;    /* rotate bit pattern */
                c->poly5 |= 0x80000000;    /* rotate bit pattern */
            }
            else {
                c->cur_v = 0;
                if (c->poly4 & 0x0001)
                    c->poly4 |= 0x8000;    /* rotate bit pattern */
                if (c->poly5 & 0x00000001)
                    c->poly5 |= 0x80000000;    /* rotate bit pattern */
            }
            c->poly4 >>= 1;
            c->poly5 >>= 1;
        }
        break;
    case 0x06:
    case 0x0a:
    case 0x0e:
        --c->cur_f;
        if (c->cur_f <= 0) {
            c->cur_f = c->freq;
            /* 31 bit shift register */
            if (c->div31 & 0x00000001) {
                c->cur_v = c->vol;
                c->div31 |= 0x80000000;    /* rotate bit pattern */
            }
            else {
                c->cur_v = 0;
            }
            c->div31 >>= 1;
        }
        break;
    case 0x07:
    case 0x09:
    case 0x0f:
        --c->cur_f;
        if (c->cur_f <= 0) {
            c->cur_f = c->freq;
            /* 31 bit shift register */
            if (c->poly5 & 0x00000001) {
                c->cur_v = c->vol;
                c->poly5 |= 0x80000000;    /* rotate bit pattern */
            }
            else {
                c->cur_v = 0;
            }
            c->poly5 >>= 1;
        }
        break;
    case 0x08:  /* white noise */
        --c->cur_f;
        if (c->cur_f <= 0) {
            c->cur_f = c->freq;
            if (c->noise < 0) { /* highest bit set */
                c->noise <<= 1;
                c->noise ^= 0x2d;
                c->cur_v = c->vol;
            }
            else {
                c->noise <<= 1;
                c->cur_v = 0;
            }
        }
        break;

    default:
        /* sqare wave */
        --c->cur_f;
        if (c->cur_f <= 0) {
            c->cur_f = c->freq;
            if (c->cur_v) {
                c->cur_v = 0;
            }
            else {
                c->cur_v = c->vol;
            }
        }
    }
    return c->cur_v;
}


// --------------------------------------------------------------

#if 0
/* no samplerate conversion (for testing */
# define SR_BASE     44100
# define SR_TARGET   44100
#else
# define SR_BASE     31440 /* NTSC rate; should be 31200 for PAL version */
# define SR_TARGET   ATARI_SAMPLER
#endif

/* ratio as 16.16 fixed point number */
#if SR_TARGET > SR_BASE
# define SR_RATIO    ((SR_BASE<<16)/SR_TARGET)
#else
# define SR_RATIO    ((SR_BASE<<16)/SR_TARGET - 0x10000)
#endif

/* convert atari samples (0..15) to Rockbox PCM values (-32k..+32k) */
#define A2PCM(x)    ((x)<<9)

void make_sound(void)
{
    if (pcm.buf_play == pcm.buf_write) {
        DEBUGF(" !! make_sound overruns buf_play !!\n");
    }

    SAMPLE *ptr = pcm.buf_write;

#if 0
    DEBUGF("Make Sound %08x\n", pcm.buf_write);
    DEBUGF("   f_div=%d,%d   vol=%d,%d   ctrl=%d,%d  "
               "cur_v=%d,%d   cur_f=%d,%d\n",
        pcm.chn[0].freq,    pcm.chn[1].freq,
        pcm.chn[0].vol,     pcm.chn[1].vol,
        pcm.chn[0].ctrl,    pcm.chn[1].ctrl,
        pcm.chn[0].cur_v,   pcm.chn[1].cur_v,
        pcm.chn[0].cur_f,   pcm.chn[1].cur_f);
#endif

# if SR_TARGET > SR_BASE
    // ensure we always start with a fresh audiol/audior!
    int32_t phase = 0x10000;
# elif SR_TARGET < SR_BASE
    int32_t phase = 0;
# endif

    for (unsigned i=0; i<pcm.buf_len; i+=2) {
        int audiol, audior;

# if SR_TARGET > SR_BASE    /* upsampling (eg from 31kHz to 44kHz) */
        if (phase >> 16) {
            audiol = get_sample(&pcm.chn[0]);
            audior = get_sample(&pcm.chn[1]);
            phase -= 0x10000;
        }
        phase += SR_RATIO;
        *ptr++ = A2PCM(audiol);
        *ptr++ = A2PCM(audior);
# elif SR_TARGET < SR_BASE  /* downsampling (eg from 31kHz to 16kHz) */
        audiol = get_sample(&pcm.chn[0]);
        audior = get_sample(&pcm.chn[1]);
        *ptr++ = A2PCM(audiol);
        *ptr++ = A2PCM(audior);
        phase += SR_RATIO;
        if (phase >> 16) {
            // dummy read; leave one out
            audiol = get_sample(&pcm.chn[0]);
            audior = get_sample(&pcm.chn[1]);
            phase -= 0x10000;
        }
# else /* don't convert sample rate */
        audiol = get_sample(&pcm.chn[0]);
        audior = get_sample(&pcm.chn[1]);
        audiol = A2PCM(audiol);
        audior = A2PCM(audior);
        *ptr++ = audiol;
        *ptr++ = audior;
# endif

        /* apply stereo setting */
        if (pcm.stereo_setting) {
            int pcml, pcmr;
            pcml = *(ptr-2);
            pcmr = *(ptr-1);
            if (pcm.stereo_setting == 2) {
                /* mono */
                pcml = pcml * 10 / 16 + pcmr * 10 / 16;
                pcmr = pcml;
            }
            else {
                /* arbitrary, nice sounding stereo */
                int tmp;
                tmp  = pcml * 12 / 16 + pcmr * 8 / 16;
                pcmr = pcml * 8 / 16 + pcmr * 12 / 16;
                pcml = tmp;
            }
            *(ptr-2) = pcml;
            *(ptr-1) = pcmr;
        }
    }

    size_t sz;
    sz = pcm.buf_len;
    pcm.buf_write += sz;
    if (pcm.buf_write + pcm.buf_len >= &pcmbuf[AUDIOBUF_SZ])
        pcm.buf_write = &pcmbuf[0];
}


// --------------------------------------------------------------


static void get_more(const void** start, size_t* size)
{
    //DEBUGF("   get more %08x\n", pcm.buf_play);

    /* calculate next get-more-buffer address */
    size_t sz;
    sz = pcm.buf_len;
    SAMPLE *to_play = pcm.buf_play + sz;
    if (to_play + pcm.buf_len >= &pcmbuf[AUDIOBUF_SZ])
        to_play = &pcmbuf[0];

    if (to_play == pcm.buf_write) {
        DEBUGF(" !! get_more() overruns buf_write !!\n");
    }
    else {
        pcm.buf_play = to_play;
    }
    *start = pcm.buf_play;
    *size = pcm.buf_len*sizeof(SAMPLE);
    rb->queue_post(&audiosync, 1, 0);
}



void start_audio(void)
{
    set_audiobuf();
    rb->pcm_play_data(&get_more, NULL, NULL, 0);
}

void stop_audio(void)
{
    rb->pcm_play_stop();
}

#endif /* ATARI_NO_SOUND */
