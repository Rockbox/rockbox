/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2004 Matthias Wientapper, 2014 Thomas Orgis
 * Changes by Thomas from original metronome plugin:
 * - tick and tock sounds (square sine)
 * - differing meters
 * - programmable tracks (parts with differing settings)
 * - Changing of tempo, including tapping, has been disabled for now. A future
 *   merged plugin might add that back and know two UI modes:
 *   1. Opened without file, defaulting to endless meter without emphasis.
 *      User can change tempo and tap. Perhaps there's a way to configure
 *      some meters, but buttons are rare.
 *   2. Opened with tempomap file, acting as a player for the predefined
 *      click track.
 * 
 * The second of the modes above is implemented right now, with mode 1
 * approximated by defaulting to 4/4@120 bpm (but no tempo change).
 * Controls:
 * - select: start/stop playback
 * - up/down: Volume control. Why can't I use the actual volume buttons? Would
 *   free up buttons for tempo change, for example, or better navigation.
 * - left/right: Advance by bars in each direction (would like to offer same
 *   as normal player; press once for beginning/end and jumping in playlist,
 *   hold for seeking around).
 * - Power/cancel: exit
 * 
 * Currently supported syntax in tempomap files (ending .tempo, associated with
 * metronome2 in Rockbox), a line for each part, [] optional:
 *    [name: ]bars [meter ]tempo
 * Example
 *    part_I: 12 3/4 133
 * for a part named "part_I" (no spaces!), 12 bars long, in 3/4 meter with
 * tempo 133. The tempo _always_ means quarter notes per minute, independent
 * of meter! That is what http://das.nasophon.de/klick/ does, from which the
 * file format originates.
 * 
 * It is actually feasible to edit these small files using the text editor
 * plugin on the device itself. Thus, I'm not sure if editing capability
 * in the metronome plugin is that necessary. There is something reassuring
 * in knowing that using the metronome won't change you programmed track.
 * 
 * Oh: Development happens exclusively on a Sansa Clip+. I try to be
 * somewhat agnostic in the GUI.
 * Alrighty then,
 * 
 * Thomas.
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
#include "lib/pluginlib_actions.h"
#include "lib/pluginlib_exit.h"

/* 1024 is around 1 ms resolution. Is plenty, unless you want ticks for
   32th notes and smaller for odd tempi. */
const unsigned int timerfreq_div = 1024;

/* Prepared tick/tock sounds as C source. */
#if CONFIG_CODEC != SWCODEC
static unsigned char tick_sound[] = {
#include "metronome2/tick_mp3.h"
};
static unsigned char tock_sound[] = {
#include "metronome2/tock_mp3.h"
};
#else
static signed short tick_sound[] = {
#include "metronome2/tick_s16.h"
};
static signed short tock_sound[] = {
#include "metronome2/tock_s16.h"
};
#endif

#define METRONOME_QUIT          PLA_EXIT

/* Redefining buttons just for my Clip+, subject to change. */

/* Hey, isn't it possible to use the actual volume buttons?! */
#define METRONOME_VOL_UP        PLA_UP
#define METRONOME_VOL_DOWN      PLA_DOWN
#define METRONOME_VOL_UP_REP    PLA_UP_REPEAT
#define METRONOME_VOL_DOWN_REP  PLA_DOWN_REPEAT

/* TODO: Make this a player that can use the normal playlists.
   Buttons then should be mapped more consistently with normal playback,
   including skipping to prev/next track. */

#define METRONOME_BACK          PLA_LEFT
#define METRONOME_FORW          PLA_RIGHT
#define METRONOME_BACK_REP      PLA_LEFT_REPEAT
#define METRONOME_FORW_REP      PLA_RIGHT_REPEAT
/* #define METRONOME_TAP           PLA_SELECT_REL */
#define METRONOME_PLAY         PLA_SELECT
/* #define METRONOME_PLAY          PLA_SELECT_REPEAT */
/* #define METRONOME_PLAY          PLA_PLAY */


const struct button_mapping *plugin_contexts[] =
{
    pla_main_ctx,
};
#define PLA_ARRAY_COUNT sizeof(plugin_contexts)/sizeof(plugin_contexts[0])

struct part
{
    char label[20];
    unsigned int bars;          /* Duration of part in bars. */
    unsigned int beats_per_bar; /* 3 in 3/4 */
    unsigned int base_beat;     /* 4 in 3/4 to adjust bpm value */
    unsigned int bpm;           /* 1/4 notes per minute */
};
#define PART_MAX 200
static struct part part_spec[PART_MAX];  /* Storage for parts 0 to 199. */

static unsigned int positive(unsigned int value)
{
    return value > 0 ? value : 1;
}

void set_part(unsigned int i, const char* label, unsigned int bars,
              unsigned int beats_per_bar, unsigned int base_beat,
              unsigned int bpm)
{
    struct part* ps = &part_spec[i];
    strncpy(ps->label, label, sizeof(ps->label)-1);
    ps->label[sizeof(ps->label)-1] = 0;
    ps->bars = bars;
    /* At least one safeguard to avoid division by zero. */
    ps->beats_per_bar = positive(beats_per_bar);
    ps->base_beat = positive(base_beat);
    ps->bpm = positive(bpm);
}

static unsigned int part  = 0; /* Current part. */
static unsigned int parts = 0; /* Number of preconfigured parts. */
static int loop = 0; /* Needed? */
static unsigned int beat = 0;
static unsigned int bar  = 0; /* How big shall this become? */
static int period   = 0;
static int minitick = 0;
static bool beating = false; /* A beat is/was playing and count needs to increase. */

static bool sound_active = false;
static bool sound_paused = true;

static char buffer[30];

static bool reset_tap = false;
static int tap_count    = 0;
static int tap_time     = 0;
static int tap_timeout  = 0;

int bpm_step_counter = 0;

#if CONFIG_CODEC != SWCODEC

#define MET_IS_PLAYING rb->mp3_is_playing()
#define MET_PLAY_STOP rb->mp3_play_stop()

static void callback(const void** start, size_t* size)
{
    (void)start; /* unused parameter, avoid warning */
    *size = 0; /* end of data */
    sound_active = false;
    rb->led(0);
}

/* Wondering: Should one prevent playing again while sound_active == true? */

static void play_tick(void)
{
    sound_active = true;
    rb->led(1);
    rb->mp3_play_data(tick_sound, sizeof(tick_sound), callback);
    rb->mp3_play_pause(true); /* kickoff audio */ 
}

static void play_tock(void)
{
    sound_active = true;
    rb->led(1);
    rb->mp3_play_data(tock_sound, sizeof(tock_sound), callback);
    rb->mp3_play_pause(true); /* kickoff audio */ 
}

#else /*  CONFIG_CODEC == SWCODEC */

#define MET_IS_PLAYING rb->pcm_is_playing()
#define MET_PLAY_STOP rb->audio_stop()


bool need_to_play = false;

/* Really necessary? Cannot just play mono? */
short tick_buf[sizeof(tick_sound)*2];
short tock_buf[sizeof(tock_sound)*2];

/* Convert the mono samples to interleaved stereo */
static void prepare_buffers(void)
{
    size_t i;
    for(i = 0;i < sizeof(tick_sound)/sizeof(short);i++)
      tick_buf[i*2] = tick_buf[i*2+1] = tick_sound[i];
    for(i = 0;i < sizeof(tock_sound)/sizeof(short);i++)
      tock_buf[i*2] = tock_buf[i*2+1] = tock_sound[i];
}

static void play_tick(void)
{
    rb->pcm_play_data(NULL, NULL, tick_buf, sizeof(tick_buf));
}

static void play_tock(void)
{
    rb->pcm_play_data(NULL, NULL, tock_buf, sizeof(tock_buf));
}

#endif /* CONFIG_CODEC != SWCODEC */

static void calc_period(void)
{
    /* (60*freq_div)/bpm always for quarter notes, half that for eights */
    period = 60*timerfreq_div*4/part_spec[part].base_beat/part_spec[part].bpm-1;
}

static void metronome_draw(struct screen* display, int state);
static void draw_display(int state)
{
    FOR_NB_SCREENS(i)
        metronome_draw(rb->screens[i], state);
}

void advance_beat(void)
{
    struct part *ps = &part_spec[part];

    /* Do as little as possible before calc_period() for adequate timing. */
    if(++beat == ps->beats_per_bar)
    {
        beat = 0;
        /* Only increment bar counter if there is a limit to count towards. */
        if(ps->bars && ++bar == ps->bars)
        {
            bar = 0;
            if(++part == parts)
            {
                part = 0;
                if(!loop) sound_paused = true;
            }
            /* How smooth is the timing here? */
            calc_period();
        }
    }
}

/* Stopping playback means incrementing the beat. Normally, it would be
   incremented after the passing of the current note duration, naturally
   while starting the next one. */
static void pause(void)
{
    if(beating)
    {
        /* Finish the current beat. */
        advance_beat();
        beating = false;
    }
    sound_paused = true;
    draw_display(0);
}

static void unpause(void)
{
    sound_paused = false;
    minitick = period; /* Start playing immediately (or after a millisecond). */
    /* Display update will happen on the tick. */
}

/* Beat counting happens here. Assuming 4/4 for now. */
static void play_ticktock(void)
{
    if(beating) advance_beat();

    if(sound_paused)
    {
        beating = false;
        draw_display(0);
    }
    else
    {
        beating = true;
        draw_display(beat ? 2 : 1);
        /* In the end, I want to introduce some blinking, so there's a display draw every time.
           Even as long as it's only updating on every bar change, having it constantly in the code
           path feels more predictable. */
        if(!beat) play_tick();
        else      play_tock();
    }
}

/* State: 0: blank/title, 1: tick, 2: tock */
static void metronome_draw(struct screen* display, int state)
{
    int textlen;
    struct part *ps;
    textlen = display->lcdwidth / display->getcharwidth();
    ps = &part_spec[part];
    display->clear_display();

#ifdef HAVE_LCD_BITMAP 
    display->setfont(FONT_SYSFIXED);
    switch(state)
    {
        case 0:
            if(sound_paused)
            {
                display->puts(0, 0, "Metronome2");
                display->hline(0, display->lcdwidth, 12);
            }
        break;
        case 1:
            display->fillrect(0, 0, display->lcdwidth, 12);
            /* display->puts(,0, "Tick"); */
        break;
        case 2:
            display->fillrect(0, display->lcdheight-13, display->lcdwidth, 12);
            /* display->puts(8,0, "Tock"); */
        break;
    }
#endif /* HAVE_LCD_BITMAP */

    rb->snprintf(buffer, sizeof(buffer), "%u/%u@%u "
    ,   ps->beats_per_bar, ps->base_beat
    ,   ps->bpm);
#ifdef HAVE_LCD_BITMAP
    display->puts(0,3, buffer);
#else
    display->puts(0,0, buffer);
#endif /* HAVE_LCD_BITMAP */

/* Simulator prints format %+02d ... real Rockbox doesn't. */
    rb->snprintf(buffer, sizeof(buffer), "V%d",
                 rb->global_settings->volume);
#ifdef HAVE_LCD_BITMAP
    display->puts(textlen-rb->strlen(buffer), 3, buffer);
#else
    display->puts(0,1, buffer);
#endif /* HAVE_LCD_BITMAP */

    if(rb->strlen(ps->label))
    {
        rb->snprintf(buffer, sizeof(buffer), "\"%s\"", ps->label);
        display->puts((textlen-rb->strlen(buffer))/2, 4, buffer);
    }

/* Wildly guessing here with puts(). */
    rb->snprintf(buffer, sizeof(buffer), "P%u/%u: B%u/%u",
                 part+1, parts, bar+1, ps->bars);
#ifdef HAVE_LCD_BITMAP
    display->puts(0, 5, buffer);
#else
    display->puts(0, 5, buffer);
#endif /* HAVE_LCD_BITMAP */


#ifdef HAVE_LCD_BITMAP
#if 0
    if(sound_paused)
        display->puts(0,2,METRONOME_MSG_START);
    else
        display->puts(0,2,METRONOME_MSG_STOP);
#endif
    display->setfont(FONT_UI);
#endif /* HAVE_LCD_BITMAP */
    display->update();
}


/* helper function to change the volume by a certain amount, +/-
   ripped from video.c */
static void change_volume(int delta)
{
    int minvol = rb->sound_min(SOUND_VOLUME);
    int maxvol = rb->sound_max(SOUND_VOLUME);
    int vol = rb->global_settings->volume + delta;

    if (vol > maxvol) vol = maxvol;
    else if (vol < minvol) vol = minvol;
    if (vol != rb->global_settings->volume) {
        rb->sound_set(SOUND_VOLUME, vol);
        rb->global_settings->volume = vol;
        draw_display(0);
    }
}

static void timer_callback(void)
{
    if(!sound_paused && minitick == period/2)
    {
        /* Clear blinker. */
        draw_display(0);
    }

    if(minitick >= period){
        minitick = 0;
        if(!sound_active && !sound_paused && !tap_count) {
#if CONFIG_CODEC == SWCODEC
            /* On SWCODEC we can't call play_tock() directly from an ISR. */
            need_to_play = true;
#else
            play_ticktock();
#endif
            rb->reset_poweroff_timer();
        }
    }
    else {
        minitick++;
    }

    if (tap_count) {
        tap_time++;
        if (tap_count > 1 && tap_time > tap_timeout)
            tap_count = 0;
    }
}

static void cleanup(void)
{
    rb->timer_unregister();
    MET_PLAY_STOP; /* stop audio ISR */
    rb->led(0);
#if CONFIG_CODEC == SWCODEC
    rb->pcm_set_frequency(HW_SAMPR_DEFAULT);
#endif
}

/* TODO: error handling */
/*
    Parse these lines:
        [label:] bars [meter] tempo[-tempo2/accel] [pattern]
    Pattern is such: Xxx.xxx ... X for tick, x for tock, "." for nothing.
    Meter needs to contain "/", as 4/4, 6/8. Tempo always refers to 1/4 note.
    "#" at beginning marks comment.
    Not bothering with encoding issues here.
*/
void parse_part(char *line)
{
    char *saveptr;
    char *token;
    char *toktok;
    struct part *ps;
    size_t len;

    /* Silently refuse to play more parts than possible. */
    if(parts >= PART_MAX) return;
    /* At least one byte is always there! Skip comments quickly. */
    if(line[0] == '#') return;

    ps = &part_spec[parts++];
    token = rb->strtok_r(line, " \t", &saveptr);
    if(!token) goto parse_part_revert;

    ps->label[0] = 0;
    len = rb->strlen(token);
    if(token[len-1] == ':')
    {
        if(--len >= sizeof(ps->label)) len = sizeof(ps->label)-1;
        rb->memcpy(ps->label, token, len);
        ps->label[sizeof(ps->label)-1] = 0;
        token = rb->strtok_r(NULL, " \t", &saveptr);
    }
    if(!token) goto parse_part_revert;

    ps->bars = (unsigned int) rb->atoi(token);
    if(ps->bars < 1) ps->bars = 1; /* No endless part from file. */
    token = rb->strtok_r(NULL, " \t", &saveptr);

    /* At least tempo must follow now. */
    if(!token) goto parse_part_revert;

    /* But could be meter first. */
    if( (toktok = rb->strchr(token, '/')) )
    {
        *(toktok++) = 0; /* Split at /. */
        ps->beats_per_bar = positive(rb->atoi(token));
        ps->base_beat = positive(rb->atoi(toktok));
        token = rb->strtok_r(NULL, " \t", &saveptr);
    }
    else
    {
        /* Default to 4/4 */
        ps->beats_per_bar = 4;
        ps->base_beat = 4;
    }

    /* Still, at least tempo must follow now. */
    if(!token) goto parse_part_revert;

    /* TODO: Parse acceleration (after adding support for that). */
    ps->bpm = positive(rb->atoi(token));
    token = rb->strtok_r(NULL, " \t", &saveptr);
    if(token)
    {
        /* TODO: Parse pattern (after adding support for that). */
    }
    return;

    /* Remove the added part after some error. */
parse_part_revert:
    --parts;
}

void step_back(void)
{
    beating = false;
    beat = 0;
    if(bar) --bar;
    else if(part)
    {
        --part;
        bar = positive(part_spec[part].bars)-1;
    }
    calc_period();
    minitick = period;
}

void step_forw(void)
{
    beating = false;
    if(bar+1 == part_spec[part].bars)
    {
        if(++part == parts)
        {
            --part;
            /* The overall end ... we stay here, exactly. */
        }
        else
        {
            bar = 0; /* Advanced one part. */
            beat = 0;
            calc_period();
        }
    }
    else ++bar;

    minitick = period;
}

enum plugin_status plugin_start(const void* file)
{
    int button;
/*    static int last_button = BUTTON_NONE; */

    atexit(cleanup);

    if (MET_IS_PLAYING)
        MET_PLAY_STOP; /* stop audio IS */

#if CONFIG_CODEC != SWCODEC
    rb->bitswap(sound, sizeof(sound));
#else
    prepare_buffers();
#if INPUT_SRC_CAPS != 0
    /* Select playback */
    rb->audio_set_input_source(AUDIO_SRC_PLAYBACK, SRCF_PLAYBACK);
    rb->audio_set_output_source(AUDIO_SRC_PLAYBACK);
#endif
    rb->pcm_set_frequency(SAMPR_44);
#endif /* CONFIG_CODEC != SWCODEC */

    if(file)
    {
        parts = 0;
        int fd = rb->open(file, O_RDONLY);
        if(fd >= 0)
        {
            char line[128];
            /* I'm assuming that read_line always terminates. */
            while(rb->read_line(fd, line, sizeof(line)) > 0)
            {
                parse_part(line);
            }
        }
        rb->close(fd);
    }
    /* Ensure that at least one part is always present, for now. */
    if(!parts)
    {
        parts = 1;
        set_part(0,"endless", 0, 4, 4, 120);
    }

    calc_period();
    rb->timer_register(1, NULL, TIMER_FREQ/timerfreq_div, timer_callback IF_COP(, CPU));

    draw_display(0);

    /* main loop */
    while (true){
        reset_tap = true;
        /* TODO: I want to play a ticktock right away when pressing the button,
           not one period after. */
#if CONFIG_CODEC == SWCODEC
        button = pluginlib_getaction(TIMEOUT_NOBLOCK,plugin_contexts,PLA_ARRAY_COUNT);
        if (need_to_play)
        {
            need_to_play = false;
            play_ticktock();
        }
#else
        button = pluginlib_getaction(TIMEOUT_BLOCK,
                                     plugin_contexts,PLA_ARRAY_COUNT);
#endif /* SWCODEC */
        switch (button) {

            case METRONOME_QUIT:
                /* get out of here */
                return PLUGIN_OK;

            case METRONOME_PLAY:
                if(sound_paused) unpause();
                else             pause();
                break;
            case METRONOME_VOL_UP:
            case METRONOME_VOL_UP_REP:
                change_volume(1);
                draw_display(0);
                break;

            case METRONOME_VOL_DOWN:
            case METRONOME_VOL_DOWN_REP:
                change_volume(-1);
                draw_display(0);
                break;

/* I would like to jump to front/back (next track) with a single press and seek
   around with held buttons, like the normal player. Actually, I see the
   tempomap-backed metronome as a music player now ... and would like the
   tempomap files to be included in playlists. */

            case METRONOME_BACK:
#if 0
                beating = false;
                part = 0;
                bar = 0;
                beat = 0;
                calc_period();
                draw_display(0);
            break;
#endif
            case METRONOME_BACK_REP:
                step_back();
                draw_display(0);
            break;

            case METRONOME_FORW:
#if 0
                /* TODO: Jump to next track in playlist. */
                beating = false;
                part = positive(parts)-1;
                bar = positive(part_spec[part].bars)-1;
                beat = positive(part_spec[part].beats_per_bar)-1;
                calc_period();
                draw_display(0);
            break;
#endif

            case METRONOME_FORW_REP:
                step_forw();
                draw_display(0);
            break;

            default:
                exit_on_usb(button);
                reset_tap = false;
                break;

        }
/*        if (button)
            last_button = button; */
        if (reset_tap) {
            tap_count = 0;
        }
        rb->yield();
    }
}

