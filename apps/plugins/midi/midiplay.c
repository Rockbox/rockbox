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

PLUGIN_HEADER
PLUGIN_IRAM_DECLARE

/* variable button definitions */
#if CONFIG_KEYPAD == RECORDER_PAD
#define BTN_QUIT     BUTTON_OFF
#define BTN_RIGHT    BUTTON_RIGHT
#define BTN_UP       BUTTON_UP
#define BTN_DOWN     BUTTON_DOWN
#define BTN_LEFT     BUTTON_LEFT
#define BTN_PLAY     BUTTON_PLAY

#elif CONFIG_KEYPAD == ONDIO_PAD
#define BTN_QUIT         BUTTON_OFF
#define BTN_RIGHT        BUTTON_RIGHT
#define BTN_UP           BUTTON_UP
#define BTN_DOWN         BUTTON_DOWN
#define BTN_LEFT         BUTTON_LEFT
#define BTN_PLAY         (BUTTON_MENU | BUTTON_OFF)


#elif (CONFIG_KEYPAD == IRIVER_H100_PAD) || (CONFIG_KEYPAD == IRIVER_H300_PAD)
#define BTN_QUIT         BUTTON_OFF
#define BTN_RIGHT        BUTTON_RIGHT
#define BTN_UP           BUTTON_UP
#define BTN_DOWN         BUTTON_DOWN
#define BTN_LEFT         BUTTON_LEFT
#define BTN_RC_QUIT      BUTTON_RC_STOP
#define BTN_PLAY         BUTTON_ON

#elif (CONFIG_KEYPAD == IPOD_4G_PAD) || (CONFIG_KEYPAD == IPOD_3G_PAD) || \
      (CONFIG_KEYPAD == IPOD_1G2G_PAD)
#define BTN_QUIT         (BUTTON_SELECT | BUTTON_MENU)
#define BTN_RIGHT        BUTTON_RIGHT
#define BTN_LEFT         BUTTON_LEFT
#define BTN_UP           BUTTON_SCROLL_FWD
#define BTN_DOWN         BUTTON_SCROLL_BACK
#define BTN_PLAY         BUTTON_PLAY


#elif (CONFIG_KEYPAD == GIGABEAT_PAD)
#define BTN_QUIT         BUTTON_POWER
#define BTN_RIGHT        BUTTON_RIGHT
#define BTN_LEFT         BUTTON_LEFT
#define BTN_UP           BUTTON_UP
#define BTN_DOWN         BUTTON_DOWN
#define BTN_PLAY         BUTTON_A


#elif (CONFIG_KEYPAD == GIGABEAT_S_PAD)
#define BTN_QUIT         BUTTON_POWER
#define BTN_RIGHT        BUTTON_RIGHT
#define BTN_LEFT         BUTTON_LEFT
#define BTN_UP           BUTTON_UP
#define BTN_DOWN         BUTTON_DOWN
#define BTN_PLAY         BUTTON_PLAY


#elif (CONFIG_KEYPAD == SANSA_E200_PAD)
#define BTN_QUIT         BUTTON_POWER
#define BTN_RIGHT        BUTTON_RIGHT
#define BTN_LEFT         BUTTON_LEFT
#define BTN_UP           BUTTON_SCROLL_FWD
#define BTN_DOWN         BUTTON_SCROLL_BACK
#define BTN_PLAY         BUTTON_UP


#elif (CONFIG_KEYPAD == SANSA_C200_PAD)
#define BTN_QUIT         BUTTON_POWER
#define BTN_RIGHT        BUTTON_RIGHT
#define BTN_LEFT         BUTTON_LEFT
#define BTN_UP           BUTTON_VOL_UP
#define BTN_DOWN         BUTTON_VOL_DOWN
#define BTN_PLAY         BUTTON_UP


#elif CONFIG_KEYPAD == IAUDIO_X5M5_PAD
#define BTN_QUIT         BUTTON_POWER
#define BTN_RIGHT        BUTTON_RIGHT
#define BTN_LEFT         BUTTON_LEFT
#define BTN_UP           BUTTON_UP
#define BTN_DOWN         BUTTON_DOWN
#define BTN_PLAY         BUTTON_PLAY


#elif CONFIG_KEYPAD == IRIVER_H10_PAD
#define BTN_QUIT         BUTTON_POWER
#define BTN_RIGHT        BUTTON_RIGHT
#define BTN_LEFT         BUTTON_LEFT
#define BTN_UP           BUTTON_SCROLL_UP
#define BTN_DOWN         BUTTON_SCROLL_DOWN
#define BTN_PLAY         BUTTON_PLAY


#elif CONFIG_KEYPAD == MROBE500_PAD
#define BTN_QUIT         BUTTON_POWER
#define BTN_RIGHT        BUTTON_RIGHT
#define BTN_LEFT         BUTTON_LEFT
#define BTN_UP           BUTTON_RC_PLAY
#define BTN_DOWN         BUTTON_RC_DOWN
#define BTN_PLAY         BUTTON_RC_HEART


#elif (CONFIG_KEYPAD == MROBE100_PAD)
#define BTN_QUIT         BUTTON_POWER
#define BTN_RIGHT        BUTTON_RIGHT
#define BTN_LEFT         BUTTON_LEFT
#define BTN_UP           BUTTON_UP
#define BTN_DOWN         BUTTON_DOWN
#define BTN_PLAY         BUTTON_DISPLAY


#elif CONFIG_KEYPAD == IAUDIO_M3_PAD
#define BTN_QUIT         BUTTON_RC_REC
#define BTN_RIGHT        BUTTON_RC_FF
#define BTN_LEFT         BUTTON_RC_REW
#define BTN_UP           BUTTON_RC_VOL_UP
#define BTN_DOWN         BUTTON_RC_VOL_DOWN
#define BTN_PLAY         BUTTON_RC_PLAY


#elif CONFIG_KEYPAD == COWOND2_PAD
#define BTN_QUIT         BUTTON_POWER

#elif CONFIG_KEYPAD == IAUDIO67_PAD
#define BTN_QUIT         BUTTON_POWER
#define BTN_RIGHT        BUTTON_RIGHT
#define BTN_LEFT         BUTTON_LEFT
#define BTN_UP           BUTTON_STOP
#define BTN_DOWN         BUTTON_PLAY
#define BTN_PLAY         BUTTON_MENU

#else
#error No keymap defined!
#endif

#ifdef HAVE_TOUCHSCREEN
#ifndef BTN_QUIT
#define BTN_QUIT         BUTTON_TOPLEFT
#endif
#ifndef BTN_RIGHT
#define BTN_RIGHT        BUTTON_MIDRIGHT
#endif
#ifndef BTN_LEFT
#define BTN_LEFT         BUTTON_MIDLEFT
#endif
#ifndef BTN_UP
#define BTN_UP           BUTTON_TOPMIDDLE
#endif
#ifndef BTN_DOWN
#define BTN_DOWN         BUTTON_BOTTOMMIDDLE
#endif
#ifndef BTN_PLAY
#define BTN_PLAY         BUTTON_CENTER
#endif
#endif

#undef SYNC

#ifdef SIMULATOR
#define SYNC
#endif

struct MIDIfile * mf IBSS_ATTR;

int numberOfSamples IBSS_ATTR; /* the number of samples in the current tick */
int playingTime IBSS_ATTR;  /* How many seconds into the file have we been playing? */
int samplesThisSecond IBSS_ATTR;    /* How many samples produced during this second so far? */

long bpm IBSS_ATTR;

int32_t gmbuf[BUF_SIZE*NBUF];
static unsigned int samples_in_buf;

int quit=0;
const struct plugin_api * rb;

static int midimain(const void * filename);

enum plugin_status plugin_start(const struct plugin_api* api, const void* parameter)
{
    int retval = 0;


    PLUGIN_IRAM_INIT(api)

    rb = api;
    if(parameter == NULL)
    {
        rb->splash(HZ*2, " Play .MID file ");
        return PLUGIN_OK;
    }
    rb->lcd_setfont(0);

#if defined(HAVE_ADJUSTABLE_CPU_FREQ)
    rb->cpu_boost(true);
#endif

    printf("%s", parameter);
    /*   rb->splash(HZ, true, parameter); */

#ifdef RB_PROFILE
    rb->profile_thread();
#endif

    retval = midimain(parameter);

#ifdef RB_PROFILE
    rb->profstop();
#endif

    rb->pcm_play_stop();
    rb->pcm_set_frequency(HW_SAMPR_DEFAULT);

#if defined(HAVE_ADJUSTABLE_CPU_FREQ)
    rb->cpu_boost(false);
#endif
    rb->splash(HZ, "FINISHED PLAYING");

    if(retval == -1)
        return PLUGIN_ERROR;
    return PLUGIN_OK;
}

bool swap=0;
bool lastswap=1;

static inline void synthbuf(void)
{
    int32_t *outptr;
    int i=BUF_SIZE;

#ifndef SYNC
    if(lastswap==swap) return;
    lastswap=swap;

    outptr=(swap ? gmbuf : gmbuf+BUF_SIZE);
#else
    outptr=gmbuf;
#endif

    /* synth samples for as many whole ticks as we can fit in the buffer */
    for(; i >= numberOfSamples; i -= numberOfSamples)
    {
        synthSamples((int32_t*)outptr, numberOfSamples);
        outptr += numberOfSamples;
        if( tick() == 0 )
            quit=1;
    }

    /* how many samples did we write to the buffer? */
    samples_in_buf = BUF_SIZE-i;

}

void get_more(unsigned char** start, size_t* size)
{
#ifndef SYNC
    if(lastswap!=swap)
    {
        printf("Buffer miss!"); // Comment out the printf to make missses less noticable.
    }

#else
    synthbuf();  // For some reason midiplayer crashes when an update is forced
#endif

    *size = samples_in_buf*sizeof(int32_t);
#ifndef SYNC
    *start = (unsigned char*)((swap ? gmbuf : gmbuf + BUF_SIZE));
    swap=!swap;
#else
    *start = (unsigned char*)(gmbuf);
#endif
}

static int midimain(const void * filename)
{
    int notesUsed = 0;
    int a=0;
    printf("Loading file");
    mf= loadFile(filename);

    if(mf == NULL)
    {
        printf("Error loading file.");
        return -1;
    }

    if (initSynth(mf, ROCKBOX_DIR "/patchset/patchset.cfg",
        ROCKBOX_DIR "/patchset/drums.cfg") == -1)
        return -1;

    rb->pcm_play_stop();
#if INPUT_SRC_CAPS != 0
    /* Select playback */
    rb->audio_set_input_source(AUDIO_SRC_PLAYBACK, SRCF_PLAYBACK);
    rb->audio_set_output_source(AUDIO_SRC_PLAYBACK);
#endif
    rb->pcm_set_frequency(SAMPLE_RATE); // 44100 22050 11025

    /*
        * tick() will do one MIDI clock tick. Then, there's a loop here that
        * will generate the right number of samples per MIDI tick. The whole
        * MIDI playback is timed in terms of this value.. there are no forced
        * delays or anything. It just produces enough samples for each tick, and
        * the playback of these samples is what makes the timings right.
        *
        * This seems to work quite well. On a laptop, anyway.
        */

    printf("Okay, starting sequencing");

    bpm=mf->div*1000000/tempo;
    numberOfSamples=SAMPLE_RATE/bpm;



    /* Skip over any junk in the beginning of the file, so start playing */
    /* after the first note event */
    do
    {
        notesUsed = 0;
        for(a=0; a<MAX_VOICES; a++)
            if(voices[a].isUsed)
                notesUsed++;
        tick();
    } while(notesUsed == 0);

    playingTime = 0;
    samplesThisSecond = 0;

    synthbuf();
    rb->pcm_play_data(&get_more, NULL, 0);

    int isPlaying = 1;  /* 0 = paused */
    int vol=0;

    while(!quit)
    {
    #ifndef SYNC
        synthbuf();
    #endif
        rb->yield();

        /* Prevent idle poweroff */
        rb->reset_poweroff_timer();

        /* Code taken from Oscilloscope plugin */
        switch(rb->button_get(false))
        {
                case BTN_UP:
                case BTN_UP | BUTTON_REPEAT:
                    vol = rb->global_settings->volume;
                    if (vol < rb->sound_max(SOUND_VOLUME))
                    {
                        vol++;
                        rb->sound_set(SOUND_VOLUME, vol);
                        rb->global_settings->volume = vol;
                    }
                    break;

                case BTN_DOWN:
                case BTN_DOWN | BUTTON_REPEAT:
                    vol = rb->global_settings->volume;
                    if (vol > rb->sound_min(SOUND_VOLUME))
                    {
                        vol--;
                        rb->sound_set(SOUND_VOLUME, vol);
                        rb->global_settings->volume = vol;
                    }
                    break;


                case BTN_LEFT:
                {
                    /* Rewinding is tricky. Basically start the file over */
                    /* but run through the tracks without the synth running */
                    rb->pcm_play_stop();
                    seekBackward(5);
                    printf("Rewind to %d:%02d\n", playingTime/60, playingTime%60);

                    if(isPlaying)
                        rb->pcm_play_data(&get_more, NULL, 0);
                    break;
                }

                case BTN_RIGHT:
                {
                    rb->pcm_play_stop();
                    seekForward(5);
                    printf("Skip to %d:%02d\n", playingTime/60, playingTime%60);

                    if(isPlaying)
                        rb->pcm_play_data(&get_more, NULL, 0);
                    break;
                }

                case BTN_PLAY:
                {
                    if(isPlaying == 1)
                    {
                        printf("Paused at %d:%02d\n", playingTime/60, playingTime%60);
                        isPlaying = 0;
                        rb->pcm_play_stop();
                    } else
                    {
                        printf("Playing from %d:%02d\n", playingTime/60, playingTime%60);
                        isPlaying = 1;
                        rb->pcm_play_data(&get_more, NULL, 0);
                    }
                    break;
                }

#ifdef BTN_RC_QUIT
                case BTN_RC_QUIT:
#endif
                case BTN_QUIT:
                    quit=1;
        }


    }

    return 0;
}
