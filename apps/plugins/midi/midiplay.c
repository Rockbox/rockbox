/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2005 Karl Kurbjun based on midi2wav by Stepan Moskovchenko
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
#include "plugin.h"
#include "guspat.h"
#include "midiutil.h"
#include "synth.h"
#include "sequencer.h"
#include "midifile.h"


/* variable button definitions */
#if (CONFIG_KEYPAD == IRIVER_H100_PAD) || (CONFIG_KEYPAD == IRIVER_H300_PAD)
#define MIDI_QUIT       BUTTON_OFF
#define MIDI_FFWD       BUTTON_RIGHT
#define MIDI_VOL_UP     BUTTON_UP
#define MIDI_VOL_DOWN   BUTTON_DOWN
#define MIDI_REWIND     BUTTON_LEFT
#define MIDI_RC_QUIT    BUTTON_RC_STOP
#define MIDI_PLAYPAUSE  BUTTON_ON

#elif (CONFIG_KEYPAD == IPOD_4G_PAD) || (CONFIG_KEYPAD == IPOD_3G_PAD) || \
      (CONFIG_KEYPAD == IPOD_1G2G_PAD)
#define MIDI_QUIT       (BUTTON_SELECT | BUTTON_MENU)
#define MIDI_FFWD       BUTTON_RIGHT
#define MIDI_REWIND     BUTTON_LEFT
#define MIDI_VOL_UP     BUTTON_SCROLL_FWD
#define MIDI_VOL_DOWN   BUTTON_SCROLL_BACK
#define MIDI_PLAYPAUSE  BUTTON_PLAY


#elif (CONFIG_KEYPAD == GIGABEAT_PAD)
#define MIDI_QUIT       BUTTON_POWER
#define MIDI_FFWD       BUTTON_RIGHT
#define MIDI_REWIND     BUTTON_LEFT
#define MIDI_VOL_UP     BUTTON_UP
#define MIDI_VOL_DOWN   BUTTON_DOWN
#define MIDI_PLAYPAUSE  BUTTON_A


#elif (CONFIG_KEYPAD == GIGABEAT_S_PAD)
#define MIDI_QUIT       BUTTON_POWER
#define MIDI_FFWD       BUTTON_RIGHT
#define MIDI_REWIND     BUTTON_LEFT
#define MIDI_VOL_UP     BUTTON_UP
#define MIDI_VOL_DOWN   BUTTON_DOWN
#define MIDI_PLAYPAUSE  BUTTON_PLAY


#elif (CONFIG_KEYPAD == SANSA_E200_PAD)
#define MIDI_QUIT       BUTTON_POWER
#define MIDI_FFWD       BUTTON_RIGHT
#define MIDI_REWIND     BUTTON_LEFT
#define MIDI_VOL_UP     BUTTON_SCROLL_FWD
#define MIDI_VOL_DOWN   BUTTON_SCROLL_BACK
#define MIDI_PLAYPAUSE  BUTTON_UP

#elif (CONFIG_KEYPAD == SANSA_FUZE_PAD)
#define MIDI_QUIT       (BUTTON_HOME|BUTTON_REPEAT)
#define MIDI_FFWD       BUTTON_RIGHT
#define MIDI_REWIND     BUTTON_LEFT
#define MIDI_VOL_UP     BUTTON_SCROLL_FWD
#define MIDI_VOL_DOWN   BUTTON_SCROLL_BACK
#define MIDI_PLAYPAUSE  BUTTON_UP


#elif (CONFIG_KEYPAD == SANSA_C200_PAD) || \
(CONFIG_KEYPAD == SANSA_CLIP_PAD) || \
(CONFIG_KEYPAD == SANSA_M200_PAD)
#define MIDI_QUIT       BUTTON_POWER
#define MIDI_FFWD       BUTTON_RIGHT
#define MIDI_REWIND     BUTTON_LEFT
#define MIDI_VOL_UP     BUTTON_VOL_UP
#define MIDI_VOL_DOWN   BUTTON_VOL_DOWN
#define MIDI_PLAYPAUSE  BUTTON_UP


#elif CONFIG_KEYPAD == IAUDIO_X5M5_PAD
#define MIDI_QUIT       BUTTON_POWER
#define MIDI_FFWD       BUTTON_RIGHT
#define MIDI_REWIND     BUTTON_LEFT
#define MIDI_VOL_UP     BUTTON_UP
#define MIDI_VOL_DOWN   BUTTON_DOWN
#define MIDI_PLAYPAUSE  BUTTON_PLAY


#elif CONFIG_KEYPAD == IRIVER_H10_PAD
#define MIDI_QUIT       BUTTON_POWER
#define MIDI_FFWD       BUTTON_RIGHT
#define MIDI_REWIND     BUTTON_LEFT
#define MIDI_VOL_UP     BUTTON_SCROLL_UP
#define MIDI_VOL_DOWN   BUTTON_SCROLL_DOWN
#define MIDI_PLAYPAUSE  BUTTON_PLAY


#elif CONFIG_KEYPAD == MROBE500_PAD
#define MIDI_QUIT       BUTTON_POWER
#define MIDI_FFWD       BUTTON_RIGHT
#define MIDI_REWIND     BUTTON_LEFT
#define MIDI_VOL_UP     BUTTON_RC_PLAY
#define MIDI_VOL_DOWN   BUTTON_RC_DOWN
#define MIDI_PLAYPAUSE  BUTTON_RC_HEART


#elif (CONFIG_KEYPAD == MROBE100_PAD)
#define MIDI_QUIT       BUTTON_POWER
#define MIDI_FFWD       BUTTON_RIGHT
#define MIDI_REWIND     BUTTON_LEFT
#define MIDI_VOL_UP     BUTTON_UP
#define MIDI_VOL_DOWN   BUTTON_DOWN
#define MIDI_PLAYPAUSE  BUTTON_DISPLAY


#elif CONFIG_KEYPAD == IAUDIO_M3_PAD
#define MIDI_QUIT       BUTTON_RC_REC
#define MIDI_FFWD       BUTTON_RC_FF
#define MIDI_REWIND     BUTTON_RC_REW
#define MIDI_VOL_UP     BUTTON_RC_VOL_UP
#define MIDI_VOL_DOWN   BUTTON_RC_VOL_DOWN
#define MIDI_PLAYPAUSE  BUTTON_RC_PLAY


#elif CONFIG_KEYPAD == COWON_D2_PAD
#define MIDI_QUIT       BUTTON_POWER

#elif CONFIG_KEYPAD == IAUDIO67_PAD
#define MIDI_QUIT       BUTTON_POWER
#define MIDI_FFWD       BUTTON_RIGHT
#define MIDI_REWIND     BUTTON_LEFT
#define MIDI_VOL_UP     BUTTON_STOP
#define MIDI_VOL_DOWN   BUTTON_PLAY
#define MIDI_PLAYPAUSE  BUTTON_MENU

#elif CONFIG_KEYPAD == CREATIVEZVM_PAD
#define MIDI_QUIT       BUTTON_BACK
#define MIDI_FFWD       BUTTON_RIGHT
#define MIDI_REWIND     BUTTON_LEFT
#define MIDI_VOL_UP     BUTTON_UP
#define MIDI_VOL_DOWN   BUTTON_DOWN
#define MIDI_PLAYPAUSE  BUTTON_PLAY

#elif CONFIG_KEYPAD == CREATIVE_ZENXFI3_PAD
#define MIDI_QUIT       (BUTTON_PLAY|BUTTON_REPEAT)
#define MIDI_FFWD       BUTTON_MENU
#define MIDI_REWIND     BUTTON_BACK
#define MIDI_VOL_UP     BUTTON_VOL_UP
#define MIDI_VOL_DOWN   BUTTON_VOL_DOWN
#define MIDI_PLAYPAUSE  (BUTTON_PLAY|BUTTON_REL)

#elif CONFIG_KEYPAD == PHILIPS_HDD1630_PAD
#define MIDI_QUIT       BUTTON_POWER
#define MIDI_FFWD       BUTTON_RIGHT
#define MIDI_REWIND     BUTTON_LEFT
#define MIDI_VOL_UP     BUTTON_UP
#define MIDI_VOL_DOWN   BUTTON_DOWN
#define MIDI_PLAYPAUSE  BUTTON_MENU

#elif CONFIG_KEYPAD == PHILIPS_HDD6330_PAD
#define MIDI_QUIT       BUTTON_POWER
#define MIDI_FFWD       BUTTON_RIGHT
#define MIDI_REWIND     BUTTON_LEFT
#define MIDI_VOL_UP     BUTTON_UP
#define MIDI_VOL_DOWN   BUTTON_DOWN
#define MIDI_PLAYPAUSE  BUTTON_MENU

#elif CONFIG_KEYPAD == PHILIPS_SA9200_PAD
#define MIDI_QUIT       BUTTON_POWER
#define MIDI_FFWD       BUTTON_NEXT
#define MIDI_REWIND     BUTTON_PREV
#define MIDI_VOL_UP     BUTTON_UP
#define MIDI_VOL_DOWN   BUTTON_DOWN
#define MIDI_PLAYPAUSE  BUTTON_MENU

#elif CONFIG_KEYPAD == ONDAVX747_PAD
#define MIDI_QUIT       BUTTON_POWER
#elif CONFIG_KEYPAD == ONDAVX777_PAD
#define MIDI_QUIT       BUTTON_POWER

#elif (CONFIG_KEYPAD == SAMSUNG_YH820_PAD) || \
      (CONFIG_KEYPAD == SAMSUNG_YH92X_PAD)
#define MIDI_QUIT       (BUTTON_PLAY|BUTTON_REPEAT)
#define MIDI_FFWD       BUTTON_RIGHT
#define MIDI_REWIND     BUTTON_LEFT
#define MIDI_VOL_UP     BUTTON_UP
#define MIDI_VOL_DOWN   BUTTON_DOWN
#define MIDI_PLAYPAUSE  BUTTON_PLAY

#elif CONFIG_KEYPAD == PBELL_VIBE500_PAD
#define MIDI_QUIT       BUTTON_REC
#define MIDI_FFWD       BUTTON_NEXT
#define MIDI_REWIND     BUTTON_PREV
#define MIDI_VOL_UP     BUTTON_UP
#define MIDI_VOL_DOWN   BUTTON_DOWN
#define MIDI_PLAYPAUSE  BUTTON_PLAY

#elif CONFIG_KEYPAD == MPIO_HD200_PAD
#define MIDI_QUIT       (BUTTON_REC | BUTTON_PLAY)
#define MIDI_FFWD       BUTTON_FF
#define MIDI_REWIND     BUTTON_REW
#define MIDI_VOL_UP     BUTTON_VOL_UP
#define MIDI_VOL_DOWN   BUTTON_VOL_DOWN
#define MIDI_PLAYPAUSE  BUTTON_PLAY

#elif CONFIG_KEYPAD == MPIO_HD300_PAD
#define MIDI_QUIT       (BUTTON_MENU | BUTTON_REPEAT)
#define MIDI_FFWD       BUTTON_FF
#define MIDI_REWIND     BUTTON_REW
#define MIDI_VOL_UP     BUTTON_UP
#define MIDI_VOL_DOWN   BUTTON_DOWN
#define MIDI_PLAYPAUSE  BUTTON_PLAY

#elif CONFIG_KEYPAD == SANSA_FUZEPLUS_PAD
#define MIDI_QUIT       BUTTON_POWER
#define MIDI_FFWD       BUTTON_RIGHT
#define MIDI_REWIND     BUTTON_LEFT
#define MIDI_VOL_UP     BUTTON_UP
#define MIDI_VOL_DOWN   BUTTON_DOWN
#define MIDI_PLAYPAUSE  BUTTON_PLAYPAUSE

#elif CONFIG_KEYPAD == SANSA_CONNECT_PAD
#define MIDI_QUIT       BUTTON_POWER
#define MIDI_FFWD       BUTTON_RIGHT
#define MIDI_REWIND     BUTTON_LEFT
#define MIDI_VOL_UP     BUTTON_UP
#define MIDI_VOL_DOWN   BUTTON_DOWN
#define MIDI_PLAYPAUSE  BUTTON_SELECT

#elif CONFIG_KEYPAD == SAMSUNG_YPR0_PAD
#define MIDI_QUIT       BUTTON_BACK
#define MIDI_FFWD       BUTTON_RIGHT
#define MIDI_REWIND     BUTTON_LEFT
#define MIDI_VOL_UP     BUTTON_UP
#define MIDI_VOL_DOWN   BUTTON_DOWN
#define MIDI_PLAYPAUSE  BUTTON_USER

#elif (CONFIG_KEYPAD == HM60X_PAD) || \
    (CONFIG_KEYPAD == HM801_PAD)
#define MIDI_QUIT       BUTTON_POWER
#define MIDI_FFWD       BUTTON_RIGHT
#define MIDI_REWIND     BUTTON_LEFT
#define MIDI_VOL_UP     BUTTON_UP
#define MIDI_VOL_DOWN   BUTTON_DOWN
#define MIDI_PLAYPAUSE  BUTTON_SELECT

#elif (CONFIG_KEYPAD == SONY_NWZ_PAD)
#define MIDI_QUIT       BUTTON_BACK
#define MIDI_FFWD       BUTTON_RIGHT
#define MIDI_REWIND     BUTTON_LEFT
#define MIDI_VOL_UP     BUTTON_UP
#define MIDI_VOL_DOWN   BUTTON_DOWN
#define MIDI_PLAYPAUSE  BUTTON_PLAY

#elif (CONFIG_KEYPAD == CREATIVE_ZEN_PAD)
#define MIDI_QUIT       BUTTON_BACK
#define MIDI_FFWD       BUTTON_RIGHT
#define MIDI_REWIND     BUTTON_LEFT
#define MIDI_VOL_UP     BUTTON_UP
#define MIDI_VOL_DOWN   BUTTON_DOWN
#define MIDI_PLAYPAUSE  BUTTON_PLAYPAUSE

#elif CONFIG_KEYPAD == DX50_PAD
#define MIDI_QUIT       BUTTON_POWER
#define MIDI_FFWD       BUTTON_RIGHT
#define MIDI_REWIND     BUTTON_LEFT
#define MIDI_VOL_UP     BUTTON_VOL_UP
#define MIDI_VOL_DOWN   BUTTON_VOL_DOWN

#elif CONFIG_KEYPAD == CREATIVE_ZENXFI2_PAD
#define MIDI_QUIT       BUTTON_POWER

#elif CONFIG_KEYPAD == AGPTEK_ROCKER_PAD
#define MIDI_QUIT       BUTTON_POWER
#define MIDI_FFWD       BUTTON_RIGHT
#define MIDI_REWIND     BUTTON_LEFT
#define MIDI_VOL_UP     BUTTON_VOLUP
#define MIDI_VOL_DOWN   BUTTON_VOLDOWN
#define MIDI_PLAYPAUSE  BUTTON_SELECT

#elif CONFIG_KEYPAD == XDUOO_X3_PAD || CONFIG_KEYPAD == XDUOO_X3II_PAD || CONFIG_KEYPAD == XDUOO_X20_PAD
#define MIDI_QUIT         BUTTON_POWER
#define MIDI_FFWD         BUTTON_NEXT
#define MIDI_REWIND       BUTTON_PREV
#define MIDI_VOL_UP       BUTTON_VOL_UP
#define MIDI_VOL_DOWN     BUTTON_VOL_DOWN
#define MIDI_PLAYPAUSE    BUTTON_PLAY

#elif CONFIG_KEYPAD == FIIO_M3K_PAD
#define MIDI_QUIT         BUTTON_POWER
#define MIDI_FFWD         BUTTON_NEXT
#define MIDI_REWIND       BUTTON_PREV
#define MIDI_VOL_UP       BUTTON_VOL_UP
#define MIDI_VOL_DOWN     BUTTON_VOL_DOWN
#define MIDI_PLAYPAUSE    BUTTON_PLAY

#elif CONFIG_KEYPAD == IHIFI_770_PAD || CONFIG_KEYPAD == IHIFI_800_PAD
#define MIDI_QUIT        BUTTON_POWER
#define MIDI_FFWD        BUTTON_VOL_DOWN
#define MIDI_REWIND      BUTTON_HOME
#define MIDI_VOL_UP      BUTTON_PREV
#define MIDI_VOL_DOWN    BUTTON_NEXT
#define MIDI_PLAYPAUSE   BUTTON_PLAY

#elif CONFIG_KEYPAD == EROSQ_PAD
#define MIDI_QUIT         BUTTON_POWER
#define MIDI_FFWD         BUTTON_NEXT
#define MIDI_REWIND       BUTTON_PREV
#define MIDI_VOL_UP       BUTTON_VOL_UP
#define MIDI_VOL_DOWN     BUTTON_VOL_DOWN
#define MIDI_PLAYPAUSE    BUTTON_PLAY

#else
#error No keymap defined!
#endif

#ifdef HAVE_TOUCHSCREEN
#ifndef MIDI_QUIT
#define MIDI_QUIT       BUTTON_TOPLEFT
#endif
#ifndef MIDI_FFWD
#define MIDI_FFWD       BUTTON_MIDRIGHT
#endif
#ifndef MIDI_REWIND
#define MIDI_REWIND     BUTTON_MIDLEFT
#endif
#ifndef MIDI_VOL_UP
#define MIDI_VOL_UP     BUTTON_TOPMIDDLE
#endif
#ifndef MIDI_VOL_DOWN
#define MIDI_VOL_DOWN   BUTTON_BOTTOMMIDDLE
#endif
#ifndef MIDI_PLAYPAUSE
#define MIDI_PLAYPAUSE  BUTTON_CENTER
#endif
#endif

#undef SYNC

#ifdef SIMULATOR
#define SYNC
#endif

#ifndef ALIGNED_ATTR
#define ALIGNED_ATTR(x) __attribute__((aligned(x)))
#endif

struct MIDIfile * mf IBSS_ATTR;

int number_of_samples IBSS_ATTR; /* the number of samples in the current tick */
int playing_time IBSS_ATTR;  /* How many seconds into the file have we been playing? */
int samples_this_second IBSS_ATTR;    /* How many samples produced during this second so far? */
long bpm IBSS_ATTR;

#ifndef SYNC
/* Small silence clip. ~5.80ms @ 44.1kHz */
static int32_t silence[256] ALIGNED_ATTR(4) = { 0 };

static int32_t gmbuf[BUF_SIZE * NBUF] ALIGNED_ATTR(4);

static volatile bool swap = false;
static volatile bool lastswap = true;
#else
static int32_t gmbuf[BUF_SIZE] ALIGNED_ATTR(4);
#endif

static volatile size_t samples_in_buf;

static volatile bool midi_end = false;
static volatile bool quit = false;

static int32_t samp_buf[MAX_SAMPLES * 2] IBSS_ATTR;

static struct dsp_config *dsp;
static struct dsp_buffer src;
static struct dsp_buffer dst;

static inline void synthbuf(void)
{
    int32_t *outptr;
    int available = BUF_SIZE;

#ifndef SYNC
    if (lastswap == swap)
        return;

    outptr = (swap ? gmbuf : gmbuf+BUF_SIZE);
#else
    outptr = gmbuf;
#endif

#if defined(HAVE_ADJUSTABLE_CPU_FREQ)
    rb->cpu_boost(true);
#endif
    /* synth samples for as many whole ticks as we can fit in the buffer */
    while (available > 0)
    {
        if ((dst.remcount <= 0) && !midi_end)
        {
            int nsamples = synthSamples(samp_buf, number_of_samples);
            if (nsamples < number_of_samples)
                number_of_samples -= nsamples;
            else if (!tick())
                midi_end = true;    /* no more midi data to play */
            src.remcount = nsamples;
            src.pin[0]    = &samp_buf[0];
            src.pin[1]    = &samp_buf[1];
            src.proc_mask = 0;
        }
        dst.remcount = 0;
        dst.bufcount = available;
        dst.p16out = (int16_t *)outptr;
        rb->dsp_process(dsp, &src, &dst);
        if (dst.remcount > 0)
        {
            outptr += dst.remcount;
            available -= dst.remcount;
        }
        else if (midi_end)
            break;
    }

    /* how many samples did we write to the buffer? */
    samples_in_buf = BUF_SIZE - available;
#ifndef SYNC
    lastswap = swap;
#endif
#if defined(HAVE_ADJUSTABLE_CPU_FREQ)
    rb->cpu_boost(false);
#endif
}

static void get_more(const void** start, size_t* size)
{
#ifndef SYNC
    swap = !swap;
    if(lastswap == swap)
    {
        *start = silence;
        *size = sizeof(silence);
        swap = !swap;
        return;
    }
    else if (samples_in_buf)
        *start = swap ? (gmbuf + BUF_SIZE) : gmbuf;
#else
    synthbuf();  /* For some reason midiplayer crashes when an update is forced */
    if (samples_in_buf)
        *start = gmbuf;
#endif
    else
    {
        *start = NULL;
        quit = true;    /* this was the last buffer to play */
    }
    *size = samples_in_buf*sizeof(int32_t);
}

static int midimain(const void * filename)
{
    int a, notes_used, vol;
    bool is_playing = true;  /* false = paused */

#if defined(HAVE_ADJUSTABLE_CPU_FREQ)
    rb->cpu_boost(true);
#endif
    midi_debug("Loading file");
    mf = loadFile(filename);

    if (mf == NULL)
    {
        midi_debug("Error loading file.");
#if defined(HAVE_ADJUSTABLE_CPU_FREQ)
        rb->cpu_boost(false);
#endif
        return -1;
    }

    if (initSynth(mf, ROCKBOX_DIR "/patchset/patchset.cfg",
        ROCKBOX_DIR "/patchset/drums.cfg") == -1)
    {
#if defined(HAVE_ADJUSTABLE_CPU_FREQ)
        rb->cpu_boost(false);
#endif
        return -1;
    }

    rb->talk_force_shutup();
    rb->pcm_play_stop();
#if INPUT_SRC_CAPS != 0
    /* Select playback */
    rb->audio_set_input_source(AUDIO_SRC_PLAYBACK, SRCF_PLAYBACK);
    rb->audio_set_output_source(AUDIO_SRC_PLAYBACK);
#endif

    dst.remcount = 0;
    dsp = rb->dsp_get_config(CODEC_IDX_AUDIO);
    rb->dsp_configure(dsp, DSP_RESET, 0);
    rb->dsp_configure(dsp, DSP_FLUSH, 0);
    rb->dsp_configure(dsp, DSP_SET_OUT_FREQUENCY, rb->mixer_get_frequency());
#ifdef HAVE_PITCHCONTROL
    rb->sound_set_pitch(PITCH_SPEED_100);
    rb->dsp_set_timestretch(PITCH_SPEED_100);
#endif
    rb->dsp_configure(dsp, DSP_SET_SAMPLE_DEPTH, 22);
    rb->dsp_configure(dsp, DSP_SET_FREQUENCY, SAMPLE_RATE); /* 44100 22050 11025 */
    rb->dsp_configure(dsp, DSP_SET_STEREO_MODE, STEREO_INTERLEAVED);

    /*
        * tick() will do one MIDI clock tick. Then, there's a loop here that
        * will generate the right number of samples per MIDI tick. The whole
        * MIDI playback is timed in terms of this value.. there are no forced
        * delays or anything. It just produces enough samples for each tick, and
        * the playback of these samples is what makes the timings right.
        *
        * This seems to work quite well. On a laptop, anyway.
        */

    midi_debug("Okay, starting sequencing");

    bpm = mf->div*1000000/tempo;
    number_of_samples = SAMPLE_RATE/bpm;

    /* Skip over any junk in the beginning of the file, so start playing */
    /* after the first note event */
    do
    {
        notes_used = 0;
        for (a = 0; a < MAX_VOICES; a++)
            if (voices[a].isUsed)
                notes_used++;
        tick();
    } while (notes_used == 0);

#if defined(HAVE_ADJUSTABLE_CPU_FREQ)
    rb->cpu_boost(false);
#endif

    playing_time = 0;
    samples_this_second = 0;

#ifndef SYNC
    synthbuf();
#endif

    rb->pcmbuf_fade(false, true);
    rb->mixer_channel_play_data(PCM_MIXER_CHAN_PLAYBACK, get_more, NULL, 0);

    while (!quit)
    {
    #ifndef SYNC
        synthbuf();
    #endif
        rb->yield();

        /* Prevent idle poweroff */
        rb->reset_poweroff_timer();

        /* Code taken from Oscilloscope plugin */
        switch (rb->button_get(false))
        {
            case MIDI_VOL_UP:
            case MIDI_VOL_UP | BUTTON_REPEAT:
            {
                vol = rb->global_settings->volume;
                if (vol < rb->sound_max(SOUND_VOLUME))
                {
                    vol++;
                    rb->sound_set(SOUND_VOLUME, vol);
                    rb->global_settings->volume = vol;
                }
                break;
            }

            case MIDI_VOL_DOWN:
            case MIDI_VOL_DOWN | BUTTON_REPEAT:
            {
                vol = rb->global_settings->volume;
                if (vol > rb->sound_min(SOUND_VOLUME))
                {
                    vol--;
                    rb->sound_set(SOUND_VOLUME, vol);
                    rb->global_settings->volume = vol;
                }
                break;
            }

            case MIDI_REWIND:
            {
                /* Rewinding is tricky. Basically start the file over */
                /* but run through the tracks without the synth running */
                rb->mixer_channel_stop(PCM_MIXER_CHAN_PLAYBACK);
                rb->dsp_configure(dsp, DSP_FLUSH, 0);
                dst.remcount = 0;
#if defined(HAVE_ADJUSTABLE_CPU_FREQ)
                rb->cpu_boost(true);
#endif
                seekBackward(5);
#if defined(HAVE_ADJUSTABLE_CPU_FREQ)
                rb->cpu_boost(false);
#endif
#ifndef SYNC
                lastswap = !swap;
                synthbuf();
#endif
                midi_debug("Rewind to %d:%02d\n", playing_time/60, playing_time%60);
                if (is_playing)
                    rb->mixer_channel_play_data(PCM_MIXER_CHAN_PLAYBACK, get_more, NULL, 0);
                break;
            }

            case MIDI_FFWD:
            {
                rb->mixer_channel_stop(PCM_MIXER_CHAN_PLAYBACK);
                rb->dsp_configure(dsp, DSP_FLUSH, 0);
                dst.remcount = 0;
#if defined(HAVE_ADJUSTABLE_CPU_FREQ)
                rb->cpu_boost(true);
#endif
                seekForward(5);
#if defined(HAVE_ADJUSTABLE_CPU_FREQ)
                rb->cpu_boost(false);
#endif
#ifndef SYNC
                lastswap = !swap;
                synthbuf();
#endif
                midi_debug("Skip to %d:%02d\n", playing_time/60, playing_time%60);
                if (is_playing)
                    rb->mixer_channel_play_data(PCM_MIXER_CHAN_PLAYBACK, get_more, NULL, 0);
                break;
            }

            case MIDI_PLAYPAUSE:
            {
                is_playing = !is_playing;
                midi_debug("%s %d:%02d\n",
                           is_playing ? "Playing from" : "Paused at",
                           playing_time/60, playing_time%60);
                rb->mixer_channel_play_pause(PCM_MIXER_CHAN_PLAYBACK, is_playing);
                break;
            }

#ifdef MIDI_RC_QUIT
            case MIDI_RC_QUIT:
#endif
            case MIDI_QUIT:
                quit = true;
        }
    }

    rb->pcmbuf_fade(false, false);
    rb->mixer_channel_stop(PCM_MIXER_CHAN_PLAYBACK);

    return 0;
}

enum plugin_status plugin_start(const void* parameter)
{
    int retval;

    if (parameter == NULL)
    {
        rb->splash(HZ*2, " Play .MID file ");
        return PLUGIN_OK;
    }
    rb->lcd_setfont(FONT_SYSFIXED);

    midi_debug("%s", parameter);
    /*   rb->splash(HZ, true, parameter); */

#ifdef RB_PROFILE
    rb->profile_thread();
#endif

    retval = midimain(parameter);

#ifdef RB_PROFILE
    rb->profstop();
#endif

    rb->splash(HZ, "FINISHED PLAYING");

    if (retval == -1)
        return PLUGIN_ERROR;
    return PLUGIN_OK;
}
