/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Linus Nielsen Feltzing
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include "config.h"

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

#include "system.h"
#include "power.h"
#include "lcd.h"
#include "led.h"
#include "mpeg.h"
#include "audio.h"
#if CONFIG_CODEC == SWCODEC
#include "thread.h"
#include "pcm_playback.h"
#include "playback.h"
#include "enc_config.h"
#endif
#ifdef HAVE_UDA1380
#include "uda1380.h"
#endif
#ifdef HAVE_TLV320
#include "tlv320.h"
#endif
#include "recording.h"
#include "mp3_playback.h"
#include "mas.h"
#include "button.h"
#include "kernel.h"
#include "settings.h"
#include "lang.h"
#include "font.h"
#include "icons.h"
#include "icon.h"
#include "screens.h"
#include "peakmeter.h"
#include "statusbar.h"
#include "menu.h"
#include "sound_menu.h"
#include "timefuncs.h"
#include "debug.h"
#include "misc.h"
#include "tree.h"
#include "string.h"
#include "dir.h"
#include "errno.h"
#include "talk.h"
#include "atoi.h"
#include "sound.h"
#include "ata.h"
#include "splash.h"
#include "screen_access.h"
#include "action.h"
#include "radio.h"
#ifdef HAVE_RECORDING

#define PM_HEIGHT ((LCD_HEIGHT >= 72) ? 2 : 1)

#if CONFIG_KEYPAD == RECORDER_PAD
bool f2_rec_screen(void);
bool f3_rec_screen(void);
#endif

#define MAX_FILE_SIZE 0x7F800000 /* 2 GB - 4 MB */

int screen_update = NB_SCREENS;
bool remote_display_on = true;

/** File name creation **/
#if CONFIG_CODEC == SWCODEC

#ifdef IF_CNFN_NUM
/* current file number to assist in creating unique numbered filenames
   without actually having to create the file on disk */
static int file_number = -1;
#endif /* IF_CNFN_NUM */

#define REC_FILE_ENDING(rec_format) \
    (audio_formats[rec_format_afmt[rec_format]].ext_list)

#else  /* CONFIG_CODEC != SWCODEC */

/* default record file extension for HWCODEC */
#define REC_FILE_ENDING(rec_format) \
    (audio_formats[AFMT_MPA_L3].ext_list)

#endif /* CONFIG_CODEC == SWCODEC */

/* path for current file */
static char path_buffer[MAX_PATH];

/** Automatic Gain Control (AGC) **/
#ifdef HAVE_AGC
/* Timing counters:
 * peak_time is incremented every 0.2s, every 2nd run of record screen loop.
 * hist_time is incremented every 0.5s, display update.
 * peak_time is the counter of the peak hold read and agc process,
 * overflow every 13 years 8-)
 */
static long peak_time = 0;
static long hist_time = 0;

static short peak_valid_mem[4];
#define BAL_MEM_SIZE 24
static short balance_mem[BAL_MEM_SIZE];

/* Automatic Gain Control */
#define AGC_MODE_SIZE 5
#define AGC_SAFETY_MODE 0

static char* agc_preset_str[] =
{ "Off", "S", "L", "D", "M", "V" };
/*  "Off",
    "Safety (clip)",
    "Live (slow)",
    "DJ-Set (slow)",
    "Medium",
    "Voice (fast)"  */
#define AGC_CLIP 32766
#define AGC_PEAK 29883 /* fast gain reduction threshold -0.8dB */
#define AGC_HIGH 27254 /* accelerated gain reduction threshold -1.6dB */
#define AGC_IMG    823 /* threshold for balance control -32dB */
/* autogain high level thresholds (-3dB, -7dB, -4dB, -5dB, -5dB) */
const short agc_th_hi[AGC_MODE_SIZE] =
{ 23197, 14637, 21156, 18428, 18426 };
/* autogain low level thresholds (-14dB, -11dB, -6dB, -7dB, -8dB) */
const short agc_th_lo[AGC_MODE_SIZE] =
{ 6538, 9235, 16422, 14636, 13045 };
/* autogain threshold times [1/5s] or [200ms] */
const short agc_tdrop[AGC_MODE_SIZE] =
{ 900,  225, 150, 60, 8 };
const short agc_trise[AGC_MODE_SIZE] =
{ 9000, 750, 400, 150, 20 };
const short agc_tbal[AGC_MODE_SIZE] =
{ 4500, 500, 300, 100, 15 };
/* AGC operation */
static bool agc_enable = true;
static short agc_preset;
/* AGC levels */
static int agc_left = 0;
static int agc_right = 0;
/* AGC time since high target volume was exceeded */
static short agc_droptime = 0;
/* AGC time since volume fallen below low target */
static short agc_risetime = 0;
/* AGC balance time exceeding +/- 0.7dB */
static short agc_baltime = 0;
/* AGC maximum gain */
static short agc_maxgain;
#endif /* HAVE_AGC */

static void set_gain(void)
{
    if(global_settings.rec_source == AUDIO_SRC_MIC)
    {
        audio_set_recording_gain(global_settings.rec_mic_gain,
                                 0, AUDIO_GAIN_MIC);
    }
    else
    {
        /* AUDIO_SRC_LINEIN, AUDIO_SRC_SPDIF, AUDIO_SRC_FMRADIO */
        audio_set_recording_gain(global_settings.rec_left_gain,
                                 global_settings.rec_right_gain,
                                 AUDIO_GAIN_LINEIN);
    }
}

#ifdef HAVE_AGC
/* Read peak meter values & calculate balance.
 * Returns validity of peak values.
 * Used for automatic gain control and history diagram.
 */
bool read_peak_levels(int *peak_l, int *peak_r, int *balance)
{
    peak_meter_get_peakhold(peak_l, peak_r);
    peak_valid_mem[peak_time % 3] = *peak_l;
    if (((peak_valid_mem[0] == peak_valid_mem[1]) &&
         (peak_valid_mem[1] == peak_valid_mem[2])) &&
        ((*peak_l < 32767)
#ifndef SIMULATOR
         || ata_disk_is_active()
#endif         
         ))
            return false;

    if (*peak_r > *peak_l)
        balance_mem[peak_time % BAL_MEM_SIZE] = (*peak_l ? 
            MIN((10000 * *peak_r) / *peak_l - 10000, 15118) : 15118);
    else
        balance_mem[peak_time % BAL_MEM_SIZE] = (*peak_r ?
            MAX(10000 - (10000 * *peak_l) / *peak_r, -15118) : -15118);
    *balance = 0;
    int i;
    for (i = 0; i < BAL_MEM_SIZE; i++)
        *balance += balance_mem[i];
    *balance = *balance / BAL_MEM_SIZE;

    return true;
}

/* AGC helper function to check if maximum gain is reached */
bool agc_gain_is_max(bool left, bool right)
{
    /* range -128...+108 [0.5dB] */
    short gain_current_l;
    short gain_current_r;

    if (agc_preset == 0)
        return false;

    switch (global_settings.rec_source)
    {
    case AUDIO_SRC_LINEIN:
#ifdef HAVE_FMRADIO_IN
    case AUDIO_SRC_FMRADIO:
#endif
        gain_current_l = global_settings.rec_left_gain;
        gain_current_r = global_settings.rec_right_gain;
        break;
    default:
        gain_current_l = global_settings.rec_mic_gain;
        gain_current_r = global_settings.rec_mic_gain;
    }

    return ((left && (gain_current_l >= agc_maxgain)) ||
            (right && (gain_current_r >= agc_maxgain)));
}

void change_recording_gain(bool increment, bool left, bool right)
{
    int factor = (increment ? 1 : -1);

    switch (global_settings.rec_source)
    {
    case AUDIO_SRC_LINEIN:
#ifdef HAVE_FMRADIO_IN
    case AUDIO_SRC_FMRADIO:
#endif
        if(left) global_settings.rec_left_gain += factor;
        if (right) global_settings.rec_right_gain += factor;
        break;
    default:
         global_settings.rec_mic_gain += factor;
    }
}

/* 
 * Handle automatic gain control (AGC).
 * Change recording gain if peak_x levels are above or below
 * target volume for specified timeouts.
 */
void auto_gain_control(int *peak_l, int *peak_r, int *balance)
{
    int agc_mono;
    short agc_mode;
    bool increment;

    if (*peak_l > agc_left)
        agc_left = *peak_l;
    else
        agc_left -= (agc_left - *peak_l + 3) >> 2;
    if (*peak_r > agc_right)
        agc_right = *peak_r;
    else
        agc_right -= (agc_right - *peak_r + 3) >> 2;
    agc_mono = (agc_left + agc_right) / 2;
    
    agc_mode = abs(agc_preset) - 1;
    if (agc_mode < 0) {
        agc_enable = false;
        return;
    }

    if (agc_mode != AGC_SAFETY_MODE) {
        /* Automatic balance control - only if not in safety mode */
        if ((agc_left > AGC_IMG) && (agc_right > AGC_IMG))
        {
            if (*balance < -556)
            {
                if (*balance > -900)
                    agc_baltime -= !(peak_time % 4); /* 0.47 - 0.75dB */
                else if (*balance > -4125)
                    agc_baltime--;                   /* 0.75 - 3.00dB */
                else if (*balance > -7579)
                    agc_baltime -= 2;                /* 3.00 - 4.90dB */
                else
                    agc_baltime -= !(peak_time % 8); /* 4.90 - inf dB */
                if (agc_baltime > 0)
                    agc_baltime -= (peak_time % 2);
            }
            else if (*balance > 556)
            {
                if (*balance < 900)
                    agc_baltime += !(peak_time % 4);
                else if (*balance < 4125)
                    agc_baltime++;
                else if (*balance < 7579)
                    agc_baltime += 2;
                else
                    agc_baltime += !(peak_time % 8);
                if (agc_baltime < 0)
                    agc_baltime += (peak_time % 2);
            }

            if ((*balance * agc_baltime) < 0)
            {
                if (*balance < 0)
                    agc_baltime -= peak_time % 2;
                else
                    agc_baltime += peak_time % 2;
            }

            increment = ((agc_risetime / 2) > agc_droptime);

            if (agc_baltime < -agc_tbal[agc_mode])
            {
                if (!increment || !agc_gain_is_max(!increment, increment)) {
                    change_recording_gain(increment, !increment, increment);
                    set_gain();
                }
                agc_baltime = 0;
            }
            else if (agc_baltime > +agc_tbal[agc_mode])
            {
                if (!increment || !agc_gain_is_max(increment, !increment)) {
                    change_recording_gain(increment, increment, !increment);
                    set_gain();
                }
                agc_baltime = 0;
            }
        }
        else if (!(hist_time % 4))
        {
            if (agc_baltime < 0)
                agc_baltime++;
            else
                agc_baltime--;
        }
    }

    /* Automatic gain control */
    if ((agc_left > agc_th_hi[agc_mode]) || (agc_right > agc_th_hi[agc_mode]))
    {
        if ((agc_left > AGC_CLIP) || (agc_right > AGC_CLIP))
            agc_droptime += agc_tdrop[agc_mode] /
                            (global_settings.rec_agc_cliptime + 1);
        if (agc_left  > AGC_HIGH) {
            agc_droptime++;
            agc_risetime=0;
            if (agc_left > AGC_PEAK)
                agc_droptime += 2;
        }
        if (agc_right > AGC_HIGH) {
            agc_droptime++;
            agc_risetime=0;
            if (agc_right > AGC_PEAK)
                agc_droptime += 2;
        }
        if (agc_mono > agc_th_hi[agc_mode])
            agc_droptime++;
        else
            agc_droptime += !(peak_time % 2);
    
        if (agc_droptime >= agc_tdrop[agc_mode])
        {
            change_recording_gain(false, true, true);
            agc_droptime = 0;
            agc_risetime = 0;
            set_gain();
        }
        agc_risetime = MAX(agc_risetime - 1, 0);
    }
    else if (agc_mono < agc_th_lo[agc_mode])
    {
        if (agc_mono < (agc_th_lo[agc_mode] / 8))
            agc_risetime += !(peak_time % 5);
        else if (agc_mono < (agc_th_lo[agc_mode] / 2))
            agc_risetime += 2;
        else
            agc_risetime++;
    
        if (agc_risetime >= agc_trise[agc_mode]) {
            if ((agc_mode != AGC_SAFETY_MODE) &&
                (!agc_gain_is_max(true, true))) {
                change_recording_gain(true, true, true);
                set_gain();
            }
            agc_risetime = 0;
            agc_droptime = 0;
        }
        agc_droptime = MAX(agc_droptime - 1, 0);
    }
    else if (!(peak_time % 6)) /* on target level every 1.2 sec */
    {
        agc_risetime = MAX(agc_risetime - 1, 0);
        agc_droptime = MAX(agc_droptime - 1, 0);
    }
}
#endif /* HAVE_AGC */

static const char* const fmtstr[] =
{
    "%c%d %s",            /* no decimals */
    "%c%d.%d %s ",        /* 1 decimal */
    "%c%d.%02d %s "       /* 2 decimals */
};

char *fmt_gain(int snd, int val, char *str, int len)
{
    int i, d, numdec;
    const char *unit;
    char sign = ' ';

    val = sound_val2phys(snd, val);
    if(val < 0)
    {
        sign = '-';
        val = -val;
    }
    numdec = sound_numdecimals(snd);
    unit = sound_unit(snd);
    
    if(numdec)
    {
        i = val / (10*numdec);
        d = val % (10*numdec);
        snprintf(str, len, fmtstr[numdec], sign, i, d, unit);
    }
    else
        snprintf(str, len, fmtstr[numdec], sign, val, unit);
    
    return str;
}

static int cursor;

void adjust_cursor(void)
{
    int max_cursor;
    
    if(cursor < 0)
        cursor = 0;

#ifdef HAVE_AGC
    switch(global_settings.rec_source)
    {
    case AUDIO_SRC_MIC:
        if(cursor == 2)
            cursor = 4;
        else if(cursor == 3)
            cursor = 1;
    case AUDIO_SRC_LINEIN:
#ifdef HAVE_FMRADIO_IN
    case AUDIO_SRC_FMRADIO:
#endif
        max_cursor = 5;
        break;
    default:
        max_cursor = 0;
        break;
    }
#else
    switch(global_settings.rec_source)
    {
    case AUDIO_SRC_MIC:
        max_cursor = 1;
        break;
    case AUDIO_SRC_LINEIN:
#ifdef HAVE_FMRADIO_IN
    case AUDIO_SRC_FMRADIO:
#endif
        max_cursor = 3;
        break;
    default:
        max_cursor = 0;
        break;
    }
#endif /* HAVE_AGC */

    if(cursor > max_cursor)
        cursor = max_cursor;
}

char *rec_create_filename(char *buffer)
{
    char ext[16];

    if(global_settings.rec_directory)
        getcwd(buffer, MAX_PATH);
    else
        strncpy(buffer, rec_base_directory, MAX_PATH);

    snprintf(ext, sizeof(ext), ".%s",
             REC_FILE_ENDING(global_settings.rec_format));

#ifdef CONFIG_RTC 
    /* We'll wait at least up to the start of the next second so no duplicate
       names are created */
    return create_datetime_filename(buffer, buffer, "R", ext, true);
#else
    return create_numbered_filename(buffer, buffer, "rec_", ext, 4
                                    IF_CNFN_NUM_(, &file_number));
#endif
}

int rec_create_directory(void)
{
    int rc;
    
    /* Try to create the base directory if needed */
    if(global_settings.rec_directory == 0)
    {
        rc = mkdir(rec_base_directory, 0);
        if(rc < 0 && errno != EEXIST)
        {
            gui_syncsplash(HZ * 2, true,
                   "Can't create the %s directory. Error code %d.",
                   rec_base_directory, rc);
            return -1;
        }
        else
        {
            /* If we have created the directory, we want the dir browser to
               be refreshed even if we haven't recorded anything */
            if(errno != EEXIST)
                return 1;
        }
    }
    return 0;
}

#if CONFIG_CODEC == SWCODEC && !defined(SIMULATOR)

# ifdef HAVE_SPDIF_IN
#  ifdef HAVE_ADJUSTABLE_CPU_FREQ
static void rec_boost(bool state)
{
    static bool cpu_boosted = false;

    if (state != cpu_boosted)
    {
        cpu_boost(state);
        cpu_boosted = state;
    }
}
#  endif
# endif

/**
 * Selects an audio source for recording or playback
 * powers/unpowers related devices and sets up monitoring.
 * Here because it calls app code and used only for HAVE_RECORDING atm.
 * Would like it in pcm_record.c.
 *
 * Behaves like a firmware function in that it does not use global settings
 * to determine the state.
 *
 * The order of setting monitoring may need tweaking dependent upon the
 * selected source to get the smoothest transition.
 */
#if defined(HAVE_UDA1380)
#define ac_disable_recording uda1380_disable_recording
#define ac_enable_recording  uda1380_enable_recording
#define ac_set_monitor       uda1380_set_monitor
#elif defined(HAVE_TLV320)
#define ac_disable_recording tlv320_disable_recording
#define ac_enable_recording  tlv320_enable_recording
#define ac_set_monitor       tlv320_set_monitor
#endif

#ifdef HAVE_SPDIF_IN
#define rec_spdif_set_monitor(m) audio_spdif_set_monitor(m)
#else
#define rec_spdif_set_monitor(m)
#endif

void rec_set_source(int source, unsigned flags)
{
    /* Prevent pops from unneeded switching */
    static int last_source = AUDIO_SRC_PLAYBACK;
#ifdef HAVE_TLV320
    static bool last_recording = false;
#endif

    bool recording = flags & SRCF_RECORDING;
    /* Default to peakmeter record. */
    bool pm_playback = false;
    bool pm_enabled = true;

    /** Do power up/down of associated device(s) **/

    /** SPDIF **/
#ifdef HAVE_SPDIF_IN
    /* Always boost for SPDIF */
    if ((source == AUDIO_SRC_SPDIF) != (source == last_source))
        rec_boost(source == AUDIO_SRC_SPDIF);

#ifdef HAVE_SPDIF_POWER
    /* Check if S/PDIF output power should be switched off or on. NOTE: assumes
       both optical in and out is controlled by the same power source, which is
       the case on H1x0. */
    spdif_power_enable((source == AUDIO_SRC_SPDIF) ||
                       audio_get_spdif_power_setting());
#endif
#endif

    /** Tuner **/
#ifdef CONFIG_TUNER
    /* Switch radio off or on per source and flags. */
    if (source != AUDIO_SRC_FMRADIO)
        radio_stop();
    else if (flags & SRCF_FMRADIO_PAUSED)
        radio_pause();
    else
        radio_start();
#endif

    switch (source)
    {
        default:                        /* playback - no recording */
            source = AUDIO_SRC_PLAYBACK;
        case AUDIO_SRC_PLAYBACK:
            pm_playback = true;
            if (source == last_source)
                break;
            ac_disable_recording();
            ac_set_monitor(false);
            pcm_rec_mux(0);             /* line in */
            rec_spdif_set_monitor(-1);  /* silence it */
        break;

        case AUDIO_SRC_MIC:             /* recording only */
            if (source == last_source)
                break;
            ac_enable_recording(true);  /* source mic */
            pcm_rec_mux(0);             /* line in */
            rec_spdif_set_monitor(0);
        break;

        case AUDIO_SRC_LINEIN:          /* recording only */
            if (source == last_source)
                break;
            pcm_rec_mux(0);             /* line in */
            ac_enable_recording(false); /* source line */
            rec_spdif_set_monitor(0);
        break;

#ifdef HAVE_SPDIF_IN
        case AUDIO_SRC_SPDIF:           /* recording only */
            if (source == last_source)
                break;
            ac_disable_recording();
            audio_spdif_set_monitor(1);
        break;
#endif /* HAVE_SPDIF_IN */

#ifdef HAVE_FMRADIO_IN
        case AUDIO_SRC_FMRADIO:         /* recording and playback */
            if (!recording)
            {
                audio_set_recording_gain(sound_default(SOUND_LEFT_GAIN),
                        sound_default(SOUND_RIGHT_GAIN), AUDIO_GAIN_LINEIN);
                pm_playback = true;
                pm_enabled = false;
            }

            pcm_rec_mux(1);                     /* fm radio */

#ifdef HAVE_UDA1380
            if (source == last_source)
                break;
            /* I2S recording and playback */
            uda1380_enable_recording(false);    /* source line */
            uda1380_set_monitor(true);
#endif
#ifdef HAVE_TLV320
            /* I2S recording and analog playback */
            if (source == last_source && recording == last_recording)
                break;

            last_recording = recording;

            if (recording)
                tlv320_enable_recording(false); /* source line */
            else
            {
                tlv320_disable_recording();
                tlv320_set_monitor(true);       /* analog bypass */
            }
#endif

            rec_spdif_set_monitor(0);
        break;
/* #elif defined(CONFIG_TUNER)  */
/* Have radio but cannot record it */
/*      case AUDIO_SRC_FMRADIO: */
/*          break;              */
#endif /* HAVE_FMRADIO_IN */
    } /* end switch */

    peak_meter_playback(pm_playback);
    peak_meter_enabled = pm_enabled;

    last_source = source;
} /* rec_set_source */
#endif /* CONFIG_CODEC == SWCODEC && !defined(SIMULATOR) */

void rec_init_recording_options(struct audio_recording_options *options)
{
    options->rec_source            = global_settings.rec_source;
    options->rec_frequency         = global_settings.rec_frequency;
    options->rec_channels          = global_settings.rec_channels;
    options->rec_prerecord_time    = global_settings.rec_prerecord_time;
#if CONFIG_CODEC == SWCODEC
    options->rec_source_flags      = 0;
    options->enc_config.rec_format = global_settings.rec_format;
    global_to_encoder_config(&options->enc_config);
#else
    options->rec_quality           = global_settings.rec_quality;
    options->rec_editable          = global_settings.rec_editable;
#endif
}

void rec_set_recording_options(struct audio_recording_options *options)
{
#if CONFIG_CODEC != SWCODEC
    if (global_settings.rec_prerecord_time)
        talk_buffer_steal(); /* will use the mp3 buffer */
#endif

#ifdef HAVE_SPDIF_IN
#ifdef HAVE_SPDIF_POWER
    audio_set_spdif_power_setting(global_settings.spdif_enable);
#endif
#endif

#if CONFIG_CODEC == SWCODEC
    rec_set_source(options->rec_source,
                   options->rec_source_flags | SRCF_RECORDING);
#endif

    audio_set_recording_options(options);
}

/* steals mp3 buffer, creates unique filename and starts recording */
void rec_record(void)
{
#if CONFIG_CODEC != SWCODEC
    talk_buffer_steal(); /* we use the mp3 buffer */
#endif
    IF_CNFN_NUM_(file_number = -1;) /* Hit disk for number   */
    audio_record(rec_create_filename(path_buffer));
}

/* creates unique filename and starts recording */
void rec_new_file(void)
{
    audio_new_file(rec_create_filename(path_buffer));
}

/* used in trigger_listerner and recording_screen */
static unsigned int last_seconds = 0;

/**
 * Callback function so that the peak meter code can send an event
 * to this application. This function can be passed to
 * peak_meter_set_trigger_listener in order to activate the trigger.
 */
static void trigger_listener(int trigger_status)
{
    switch (trigger_status)
    {
        case TRIG_GO:
            if((audio_status() & AUDIO_STATUS_RECORD) != AUDIO_STATUS_RECORD)
            {
                rec_record();
                /* give control to mpeg thread so that it can start
                   recording */
                yield(); yield(); yield();
            }

            /* if we're already recording this is a retrigger */
            else
            {
                if((audio_status() & AUDIO_STATUS_PAUSE) &&
                       (global_settings.rec_trigger_type == 1))
                    audio_resume_recording();
                /* New file on trig start*/
                else if (global_settings.rec_trigger_type != 2)
                {
                    rec_new_file();
                    /* tell recording_screen to reset the time */
                    last_seconds = 0;
                }
            }
            break;

        /* A _change_ to TRIG_READY means the current recording has stopped */
        case TRIG_READY:
            if(audio_status() & AUDIO_STATUS_RECORD)
            {
                switch(global_settings.rec_trigger_type)
                {
                    case 0: /* Stop */
#if CONFIG_CODEC == SWCODEC
                        audio_stop_recording();
#else
                        audio_stop();
#endif
                        break;

                    case 1: /* Pause */
                        audio_pause_recording();
                        break;

                    case 2: /* New file on trig stop*/
                        rec_new_file();
                        /* tell recording_screen to reset the time */
                        last_seconds = 0;
                        break;
                 }

                if (global_settings.rec_trigger_mode != TRIG_MODE_REARM)
                {
                    peak_meter_set_trigger_listener(NULL);
                    peak_meter_trigger(false);
                }
            }
            break;
    }
}

bool recording_screen(bool no_source)
{
    long button;
    bool done = false;
    char buf[32];
    char buf2[32];
    int w, h;
    int update_countdown = 1;
    bool have_recorded = false;
    unsigned int seconds;
    int hours, minutes;
    char filename[13];
    bool been_in_usb_mode = false;
    int last_audio_stat = -1;
    int audio_stat;
#ifdef HAVE_FMRADIO_IN
    /* Radio is left on if:
     *   1) Is was on at the start and the initial source is FM Radio
     *   2) 1) and the source was never changed to something else
     */
    int radio_status = (global_settings.rec_source != AUDIO_SRC_FMRADIO) ?
                            FMRADIO_OFF : get_radio_status();
#endif
    int talk_menu = global_settings.talk_menu;
#if CONFIG_LED == LED_REAL
    bool led_state = false;
    int led_countdown = 2;
#endif
#ifdef HAVE_AGC
    bool peak_read = false;
    bool peak_valid = false;
    int peak_l, peak_r;
    int balance = 0;
    bool display_agc[NB_SCREENS];
#endif
    int line[NB_SCREENS];
    int i;
    int filename_offset[NB_SCREENS];
    int pm_y[NB_SCREENS];
    int trig_xpos[NB_SCREENS];
    int trig_ypos[NB_SCREENS];
    int trig_width[NB_SCREENS];

    static const unsigned char *byte_units[] = {
        ID2P(LANG_BYTE),
        ID2P(LANG_KILOBYTE),
        ID2P(LANG_MEGABYTE),
        ID2P(LANG_GIGABYTE)
    };

    struct audio_recording_options rec_options;

    global_settings.recscreen_on = true;
    cursor = 0;
#if (CONFIG_LED == LED_REAL) && !defined(SIMULATOR)
    ata_set_led_enabled(false);
#endif

#if CONFIG_CODEC == SWCODEC
    /* recording_menu gets messed up: so reset talk_menu */
    talk_menu = global_settings.talk_menu;
    global_settings.talk_menu = 0;
    /* audio_init_recording stops anything playing when it takes the audio
       buffer */
#else
    /* Yes, we use the D/A for monitoring */
    peak_meter_enabled = true;
    peak_meter_playback(true);
#endif

    audio_init_recording(0);
    sound_set_volume(global_settings.volume);

#ifdef HAVE_AGC
    peak_meter_get_peakhold(&peak_l, &peak_r);
#endif

    rec_init_recording_options(&rec_options);
    rec_set_recording_options(&rec_options);

    set_gain();
    settings_apply_trigger();

#ifdef HAVE_AGC
    agc_preset_str[0] = str(LANG_OFF);
    agc_preset_str[1] = str(LANG_AGC_SAFETY);
    agc_preset_str[2] = str(LANG_AGC_LIVE);
    agc_preset_str[3] = str(LANG_AGC_DJSET);
    agc_preset_str[4] = str(LANG_AGC_MEDIUM);
    agc_preset_str[5] = str(LANG_AGC_VOICE);
    if (global_settings.rec_source == AUDIO_SRC_MIC) {
        agc_preset = global_settings.rec_agc_preset_mic;
        agc_maxgain = global_settings.rec_agc_maxgain_mic;
    }
    else {
        agc_preset = global_settings.rec_agc_preset_line;
        agc_maxgain = global_settings.rec_agc_maxgain_line;
    }
#endif

    FOR_NB_SCREENS(i)
    {
        screens[i].setfont(FONT_SYSFIXED);
        screens[i].getstringsize("M", &w, &h);
        screens[i].setmargins(global_settings.invert_cursor ? 0 : w, 8);
        filename_offset[i] = ((screens[i].height >= 80) ? 1 : 0);
        pm_y[i] = 8 + h * (2 + filename_offset[i]);
    }
    
    if(rec_create_directory() > 0)
        have_recorded = true;
        
    if (!remote_display_on)
    {
        screens[1].clear_display();
        snprintf(buf, sizeof(buf), str(LANG_REMOTE_LCD_ON));
        screens[1].puts((screens[1].width/w - strlen(buf))/2 + 1, 
                            screens[1].height/(h*2) + 1, buf);
        screens[1].update();
        gui_syncsplash(0, true, str(LANG_REMOTE_LCD_OFF));
    }

    while(!done)
    {
        audio_stat = audio_status();
        
#if CONFIG_LED == LED_REAL

        /*
         * Flash the LED while waiting to record.  Turn it on while
         * recording.
         */
        if(audio_stat & AUDIO_STATUS_RECORD)
        {
            if (audio_stat & AUDIO_STATUS_PAUSE)
            {
                if (--led_countdown <= 0)
                {
                    led_state = !led_state;
                    led(led_state);
                    led_countdown = 2;
                }
            }
            else
            {
                /* trigger is on in status TRIG_READY (no check needed) */
                led(true);
            }
        }
        else
        {
            int trigStat = peak_meter_trigger_status();
            /*
             * other trigger stati than trig_off and trig_steady
             * already imply that we are recording.
             */
            if (trigStat == TRIG_STEADY)
            {
                if (--led_countdown <= 0)
                {
                    led_state = !led_state;
                    led(led_state);
                    led_countdown = 2;
                }
            }
            else
            {
                /* trigger is on in status TRIG_READY (no check needed) */
                led(false);
            }
        }
#endif /* CONFIG_LED */

        /* Wait for a button a while (HZ/10) drawing the peak meter */
        button = peak_meter_draw_get_btn(0, pm_y, h * PM_HEIGHT, screen_update);

        if (last_audio_stat != audio_stat)
        {
            if (audio_stat == AUDIO_STATUS_RECORD)
            {
                have_recorded = true;
            }
            last_audio_stat = audio_stat;
        }

        switch(button)
        {
            case ACTION_REC_LCD:
                if (remote_display_on)
                {
                    remote_display_on = false;
                    screen_update = 1;
                    screens[1].clear_display();
                    snprintf(buf, sizeof(buf), str(LANG_REMOTE_LCD_ON));
                    screens[1].puts((screens[1].width/w - strlen(buf))/2 + 1, 
                                          screens[1].height/(h*2) + 1, buf);
                    screens[1].update();
                    gui_syncsplash(0, true, str(LANG_REMOTE_LCD_OFF));
                }
                else
                {
                    remote_display_on = true;
                    screen_update = NB_SCREENS;
                }
                break;

            case ACTION_STD_CANCEL:
                /* turn off the trigger */
                peak_meter_trigger(false);
                peak_meter_set_trigger_listener(NULL);

                if(audio_stat & AUDIO_STATUS_RECORD)
                {
                    audio_stop_recording();
                }
                else
                {
#if CONFIG_CODEC != SWCODEC
                    peak_meter_playback(true);
                    peak_meter_enabled = false;
#endif                    
                    done = true;
                }
                update_countdown = 1; /* Update immediately */
                break;

            case ACTION_REC_PAUSE:
            case ACTION_REC_NEWFILE:
                /* Only act if the mpeg is stopped */
                if(!(audio_stat & AUDIO_STATUS_RECORD))
                {
                    /* is this manual or triggered recording? */
                    if ((global_settings.rec_trigger_mode == TRIG_MODE_OFF) ||
                        (peak_meter_trigger_status() != TRIG_OFF))
                    {
                        /* manual recording */
                        have_recorded = true;
                        rec_record();
                        last_seconds = 0;
                        if (talk_menu)
                        {   /* no voice possible here, but a beep */
                            audio_beep(HZ/2); /* longer beep on start */
                        }
                    }
                    /* this is triggered recording */
                    else
                    {
                        /* we don't start recording now, but enable the
                           trigger and let the callback function
                           trigger_listener control when the recording starts */
                        peak_meter_trigger(true);
                        peak_meter_set_trigger_listener(&trigger_listener);
                    }
                }
                else
                {
                    /*if new file button pressed, start new file */
                    if (button == ACTION_REC_NEWFILE)
                    {
                        rec_new_file();
                        last_seconds = 0;
                    }
                    else
                    /* if pause button pressed, pause or resume */
                    {
                        if(audio_stat & AUDIO_STATUS_PAUSE)
                        {
                            audio_resume_recording();
                            if (talk_menu)
                            {   /* no voice possible here, but a beep */
                                audio_beep(HZ/4); /* short beep on resume */
                            }
                        }
                        else
                        {
                            audio_pause_recording();
                        }
                    }
                }
                update_countdown = 1; /* Update immediately */
                break;

            case ACTION_STD_PREV:
                cursor--;
                adjust_cursor();
                update_countdown = 1; /* Update immediately */
                break;

            case ACTION_STD_NEXT:
                cursor++;
                adjust_cursor();
                update_countdown = 1; /* Update immediately */
                break;
       
            case ACTION_SETTINGS_INC:
                switch(cursor)
                {
                    case 0:
                        if(global_settings.volume <
                           sound_max(SOUND_VOLUME))
                            global_settings.volume++;
                        sound_set_volume(global_settings.volume);
                        break;
                    case 1:
                        if(global_settings.rec_source == AUDIO_SRC_MIC)
                        {
                            if(global_settings.rec_mic_gain <
                               sound_max(SOUND_MIC_GAIN))
                            global_settings.rec_mic_gain++;
                        }
                        else
                        {
                            if(global_settings.rec_left_gain <
                               sound_max(SOUND_LEFT_GAIN))
                                global_settings.rec_left_gain++;
                            if(global_settings.rec_right_gain <
                               sound_max(SOUND_RIGHT_GAIN))
                                global_settings.rec_right_gain++;
                        }
                        break;
                    case 2:
                        if(global_settings.rec_left_gain <
                           sound_max(SOUND_LEFT_GAIN))
                            global_settings.rec_left_gain++;
                        break;
                    case 3:
                        if(global_settings.rec_right_gain <
                           sound_max(SOUND_RIGHT_GAIN))
                            global_settings.rec_right_gain++;
                        break;
#ifdef HAVE_AGC
                    case 4:
                        agc_preset = MIN(agc_preset + 1, AGC_MODE_SIZE);
                        agc_enable = (agc_preset != 0);
                        if (global_settings.rec_source == AUDIO_SRC_MIC) {
                            global_settings.rec_agc_preset_mic = agc_preset;
                            agc_maxgain = global_settings.rec_agc_maxgain_mic;
                        } else {
                            global_settings.rec_agc_preset_line = agc_preset;
                            agc_maxgain = global_settings.rec_agc_maxgain_line;
                        }
                        break;
                    case 5:
                        if (global_settings.rec_source == AUDIO_SRC_MIC)
                        {
                            agc_maxgain = MIN(agc_maxgain + 1,
                                              sound_max(SOUND_MIC_GAIN));
                            global_settings.rec_agc_maxgain_mic = agc_maxgain;
                        }
                        else
                        {
                            agc_maxgain = MIN(agc_maxgain + 1,
                                              sound_max(SOUND_LEFT_GAIN));
                            global_settings.rec_agc_maxgain_line = agc_maxgain;
                        }
                        break;
#endif
                }
                set_gain();
                update_countdown = 1; /* Update immediately */
                break;
                
            case ACTION_SETTINGS_DEC:
                switch(cursor)
                {
                    case 0:
                        if(global_settings.volume >
                           sound_min(SOUND_VOLUME))
                            global_settings.volume--;
                        sound_set_volume(global_settings.volume);
                        break;
                    case 1:
                        if(global_settings.rec_source == AUDIO_SRC_MIC)
                        {
                            if(global_settings.rec_mic_gain >
                               sound_min(SOUND_MIC_GAIN))
                                global_settings.rec_mic_gain--;
                        }
                        else
                        {
                            if(global_settings.rec_left_gain >
                               sound_min(SOUND_LEFT_GAIN))
                                global_settings.rec_left_gain--;
                            if(global_settings.rec_right_gain >
                               sound_min(SOUND_RIGHT_GAIN))
                                global_settings.rec_right_gain--;
                        }
                        break;
                    case 2:
                        if(global_settings.rec_left_gain >
                           sound_min(SOUND_LEFT_GAIN))
                            global_settings.rec_left_gain--;
                        break;
                    case 3:
                        if(global_settings.rec_right_gain >
                           sound_min(SOUND_RIGHT_GAIN))
                            global_settings.rec_right_gain--;
                        break;
#ifdef HAVE_AGC
                    case 4:
                        agc_preset = MAX(agc_preset - 1, 0);
                        agc_enable = (agc_preset != 0);
                        if (global_settings.rec_source == AUDIO_SRC_MIC) {
                            global_settings.rec_agc_preset_mic = agc_preset;
                            agc_maxgain = global_settings.rec_agc_maxgain_mic;
                        } else {
                            global_settings.rec_agc_preset_line = agc_preset;
                            agc_maxgain = global_settings.rec_agc_maxgain_line;
                        }
                        break;
                    case 5:
                        if (global_settings.rec_source == AUDIO_SRC_MIC)
                        {
                            agc_maxgain = MAX(agc_maxgain - 1,
                                              sound_min(SOUND_MIC_GAIN));
                            global_settings.rec_agc_maxgain_mic = agc_maxgain;
                        }
                        else
                        {
                            agc_maxgain = MAX(agc_maxgain - 1,
                                              sound_min(SOUND_LEFT_GAIN));
                            global_settings.rec_agc_maxgain_line = agc_maxgain;
                        }
                        break;
#endif
                }
                set_gain();
                update_countdown = 1; /* Update immediately */
                break;
                
            case ACTION_STD_MENU:
                if(audio_stat != AUDIO_STATUS_RECORD)
                {
#ifdef HAVE_FMRADIO_IN
                    const int prev_rec_source = global_settings.rec_source;
#endif

#if CONFIG_LED == LED_REAL
                    /* led is restored at begin of loop / end of function */
                    led(false);
#endif
                    if (recording_menu(no_source))
                    {
                        done = true;
                        been_in_usb_mode = true;
#ifdef HAVE_FMRADIO_IN
                        radio_status = FMRADIO_OFF;
#endif
                    }
                    else
                    {
                        settings_save();

#ifdef HAVE_FMRADIO_IN
                        /* If input changes away from FM Radio, radio will
                           remain off when recording screen closes. */
                        if (global_settings.rec_source != prev_rec_source
                            && prev_rec_source == AUDIO_SRC_FMRADIO)
                            radio_status = FMRADIO_OFF;
#endif

#if CONFIG_CODEC == SWCODEC
                        /* reinit after submenu exit */
                        audio_close_recording();
                        audio_init_recording(0);
#endif
                        rec_init_recording_options(&rec_options);
                        rec_set_recording_options(&rec_options);

                        if(rec_create_directory() > 0)
                            have_recorded = true;
#ifdef HAVE_AGC
                        if (global_settings.rec_source == AUDIO_SRC_MIC) {
                            agc_preset = global_settings.rec_agc_preset_mic;
                            agc_maxgain = global_settings.rec_agc_maxgain_mic;
                        }
                        else {
                            agc_preset = global_settings.rec_agc_preset_line;
                            agc_maxgain = global_settings.rec_agc_maxgain_line;
                        }
#endif

                        adjust_cursor();
                        set_gain();
                        update_countdown = 1; /* Update immediately */

                        FOR_NB_SCREENS(i)
                        {
                            screens[i].setfont(FONT_SYSFIXED);
                            screens[i].setmargins(
                                global_settings.invert_cursor ? 0 : w, 8);
                        }
                    }
                }
                break;

#if CONFIG_KEYPAD == RECORDER_PAD
            case ACTION_REC_F2:
                if(audio_stat != AUDIO_STATUS_RECORD)
                {
#if CONFIG_LED == LED_REAL
                    /* led is restored at begin of loop / end of function */
                    led(false);
#endif
                    if (f2_rec_screen())
                    {
                        have_recorded = true;
                        done = true;
                    }
                    else
                        update_countdown = 1; /* Update immediately */
                }
                break;

            case ACTION_REC_F3:
                if(audio_stat & AUDIO_STATUS_RECORD)
                {
                    rec_new_file();
                    last_seconds = 0;
                }
                else
                {
                    if(audio_stat != AUDIO_STATUS_RECORD)
                    {
#if CONFIG_LED == LED_REAL
                        /* led is restored at begin of loop / end of function */
                        led(false);
#endif
                        if (f3_rec_screen())
                        {
                            have_recorded = true;
                            done = true;
                        }
                        else
                            update_countdown = 1; /* Update immediately */
                    }
                }
                break;
#endif /*  CONFIG_KEYPAD == RECORDER_PAD */

            case SYS_USB_CONNECTED:
                /* Only accept USB connection when not recording */
                if(audio_stat != AUDIO_STATUS_RECORD)
                {
                    default_event_handler(SYS_USB_CONNECTED);
                    done = true;
                    been_in_usb_mode = true;
#ifdef HAVE_FMRADIO_IN
                    radio_status = FMRADIO_OFF;
#endif
                }
                break;
                
            default:
                default_event_handler(button);
                break;
        }

#ifdef HAVE_AGC
        peak_read = !peak_read;
        if (peak_read) { /* every 2nd run of loop */
            peak_time++;
            peak_valid = read_peak_levels(&peak_l, &peak_r, &balance);
        }

        /* Handle AGC every 200ms when enabled and peak data is valid */
        if (peak_read && agc_enable && peak_valid)
            auto_gain_control(&peak_l, &peak_r, &balance);
#endif

            FOR_NB_SCREENS(i)       
                screens[i].setfont(FONT_SYSFIXED);

        seconds = audio_recorded_time() / HZ;
        
        update_countdown--;
        if(update_countdown == 0 || seconds > last_seconds)
        {
            unsigned int dseconds, dhours, dminutes;
            unsigned long num_recorded_bytes, dsize, dmb;
            int pos = 0;

            update_countdown = 5;
            last_seconds = seconds;

            dseconds = rec_timesplit_seconds();
            dsize = rec_sizesplit_bytes();
            num_recorded_bytes = audio_num_recorded_bytes();
            
            for(i = 0; i < screen_update; i++)
                screens[i].clear_display(); 

            if ((global_settings.rec_sizesplit) && (global_settings.rec_split_method))
            {
                dmb = dsize/1024/1024;
                snprintf(buf, sizeof(buf), "%s %dMB",
                             str(LANG_SYSFONT_SPLIT_SIZE), dmb);
            }
            else
            {
                hours = seconds / 3600;
                minutes = (seconds - (hours * 3600)) / 60;
                snprintf(buf, sizeof(buf), "%s %02d:%02d:%02d",
                         str(LANG_SYSFONT_RECORDING_TIME),
                         hours, minutes, seconds%60);
            }
            
            for(i = 0; i < screen_update; i++)
                screens[i].puts(0, 0, buf); 

            if(audio_stat & AUDIO_STATUS_PRERECORD)
            {
                snprintf(buf, sizeof(buf), "%s...", str(LANG_SYSFONT_RECORD_PRERECORD));
            }
            else
            {
                /* Display the split interval if the record timesplit
                   is active */
                if ((global_settings.rec_timesplit) && !(global_settings.rec_split_method))
                {
                    /* Display the record timesplit interval rather
                       than the file size if the record timer is
                       active */
                    dhours = dseconds / 3600;
                    dminutes = (dseconds - (dhours * 3600)) / 60;
                    snprintf(buf, sizeof(buf), "%s %02d:%02d",
                             str(LANG_SYSFONT_RECORD_TIMESPLIT_REC),
                             dhours, dminutes);
                }
                else
                {
                    output_dyn_value(buf2, sizeof buf2,
                                     num_recorded_bytes,
                                     byte_units, true);
                    snprintf(buf, sizeof(buf), "%s %s",
                             str(LANG_SYSFONT_RECORDING_SIZE), buf2);
                }
            }
            for(i = 0; i < screen_update; i++)
                screens[i].puts(0, 1, buf);

            for(i = 0; i < screen_update; i++)
            {
                if (filename_offset[i] > 0)
                {
                    if (audio_stat & AUDIO_STATUS_RECORD)
                    {
                        strncpy(filename, path_buffer +
                                    strlen(path_buffer) - 12, 13);
                        filename[12]='\0';
                    }
                    else
                        strcpy(filename, "");

                    snprintf(buf, sizeof(buf), "%s %s",
                        str(LANG_SYSFONT_RECORDING_FILENAME), filename);
                    screens[i].puts(0, 2, buf);         
                }
            }

            /* We will do file splitting regardless, either at the end of
               a split interval, or when the filesize approaches the 2GB
               FAT file size (compatibility) limit. */
            if ((audio_stat && !(global_settings.rec_split_method) 
                 && global_settings.rec_timesplit && (seconds >= dseconds))
                 || (audio_stat && global_settings.rec_split_method
                 && global_settings.rec_sizesplit && (num_recorded_bytes >= dsize))
                 || (num_recorded_bytes >= MAX_FILE_SIZE))
            {
                if (!(global_settings.rec_split_type)
                     || (num_recorded_bytes >= MAX_FILE_SIZE))
                {
                    rec_new_file();
                    last_seconds = 0;
                }
                else
                {
                    peak_meter_trigger(false);
                    peak_meter_set_trigger_listener(NULL);
                    audio_stop_recording();
                }
                update_countdown = 1;
            }

            snprintf(buf, sizeof(buf), "%s: %s", str(LANG_SYSFONT_VOLUME),
                     fmt_gain(SOUND_VOLUME,
                              global_settings.volume,
                              buf2, sizeof(buf2)));
            
            if (global_settings.invert_cursor && (pos++ == cursor))
            {
                for(i = 0; i < screen_update; i++)
                    screens[i].puts_style_offset(0, filename_offset[i] +
                                           PM_HEIGHT + 2, buf, STYLE_INVERT,0);
            }
            else
            {
                for(i = 0; i < screen_update; i++)
                    screens[i].puts(0, filename_offset[i] + PM_HEIGHT + 2, buf);
            }                

            if(global_settings.rec_source == AUDIO_SRC_MIC)
            {
                /* Draw MIC recording gain */
                snprintf(buf, sizeof(buf), "%s:%s", str(LANG_SYSFONT_RECORDING_GAIN),
                         fmt_gain(SOUND_MIC_GAIN,
                                  global_settings.rec_mic_gain,
                                  buf2, sizeof(buf2)));
                if(global_settings.invert_cursor && ((1==cursor)||(2==cursor)))
                {
                    for(i = 0; i < screen_update; i++)
                        screens[i].puts_style_offset(0, filename_offset[i] +
                                            PM_HEIGHT + 3, buf, STYLE_INVERT,0);
                }
                else
                {
                    for(i = 0; i < screen_update; i++)
                        screens[i].puts(0, filename_offset[i] +
                                            PM_HEIGHT + 3, buf);
                }
            }
            else if(global_settings.rec_source == AUDIO_SRC_LINEIN
#ifdef HAVE_FMRADIO_IN
                    || global_settings.rec_source == AUDIO_SRC_FMRADIO
#endif
                    )
            {
                /* Draw LINE or FMRADIO recording gain */
                snprintf(buf, sizeof(buf), "%s:%s",
                         str(LANG_SYSFONT_RECORDING_LEFT),
                         fmt_gain(SOUND_LEFT_GAIN,
                                  global_settings.rec_left_gain,
                                  buf2, sizeof(buf2)));
                if(global_settings.invert_cursor && ((1==cursor)||(2==cursor)))
                {
                    for(i = 0; i < screen_update; i++)
                        screens[i].puts_style_offset(0, filename_offset[i] + 
                                           PM_HEIGHT + 3, buf, STYLE_INVERT,0);
                }
                else
                {
                     for(i = 0; i < screen_update; i++)
                         screens[i].puts(0, filename_offset[i] +
                                             PM_HEIGHT + 3, buf);
                }                

                snprintf(buf, sizeof(buf), "%s:%s",
                         str(LANG_SYSFONT_RECORDING_RIGHT),
                         fmt_gain(SOUND_RIGHT_GAIN,
                                  global_settings.rec_right_gain,
                                  buf2, sizeof(buf2)));
                if(global_settings.invert_cursor && ((1==cursor)||(3==cursor)))
                {
                    for(i = 0; i < screen_update; i++)
                        screens[i].puts_style_offset(0, filename_offset[i] + 
                                            PM_HEIGHT + 4, buf, STYLE_INVERT,0);
                }
                else
                {
                    for(i = 0; i < screen_update; i++)
                        screens[i].puts(0, filename_offset[i] + 
                                            PM_HEIGHT + 4, buf);
                }                
            }

            FOR_NB_SCREENS(i)
            {
                switch (global_settings.rec_source)
                {
                case AUDIO_SRC_LINEIN:
#ifdef HAVE_FMRADIO_IN
                case AUDIO_SRC_FMRADIO:
#endif
                    line[i] = 5;
                    break;
                case AUDIO_SRC_MIC:
                    line[i] = 4;
                    break;
#ifdef HAVE_SPDIF_IN
                case AUDIO_SRC_SPDIF:
                    line[i] = 3;
                    break;
#endif
                default:
                    line[i] = 5; /* to prevent uninitialisation
                                    warnings for line[0] */
                    break;
                } /* end switch */
#ifdef HAVE_AGC
                if (screens[i].height < h * (2 + filename_offset[i] + PM_HEIGHT + line[i]))
                {
                    line[i] -= 1;
                    display_agc[i] = false;
                }
                else 
                    display_agc[i] = true;
 
                if ((cursor==4) || (cursor==5))
                    display_agc[i] = true;
             }

            /************** AGC test info ******************
            snprintf(buf, sizeof(buf), "D:%d U:%d",
                     (agc_droptime+2)/5, (agc_risetime+2)/5);
            lcd_putsxy(1, LCD_HEIGHT - 8, buf);
            snprintf(buf, sizeof(buf), "B:%d",
                     (agc_baltime+2)/5);
            lcd_putsxy(LCD_WIDTH/2 + 3, LCD_HEIGHT - 8, buf);
            ***********************************************/

            if (cursor == 5)
                snprintf(buf, sizeof(buf), "%s: %s",
                         str(LANG_RECORDING_AGC_MAXGAIN),
                         fmt_gain(SOUND_LEFT_GAIN,
                                  agc_maxgain, buf2, sizeof(buf2)));
            else if (agc_preset == 0)
                snprintf(buf, sizeof(buf), "%s: %s",
                         str(LANG_RECORDING_AGC_PRESET),
                         agc_preset_str[agc_preset]);
            else if (global_settings.rec_source == AUDIO_SRC_MIC)
                snprintf(buf, sizeof(buf), "%s: %s%s",
                         str(LANG_RECORDING_AGC_PRESET),
                         agc_preset_str[agc_preset],
                         fmt_gain(SOUND_LEFT_GAIN,
                             agc_maxgain -
                             global_settings.rec_mic_gain,
                             buf2, sizeof(buf2)));
            else
                snprintf(buf, sizeof(buf), "%s: %s%s",
                         str(LANG_RECORDING_AGC_PRESET),
                         agc_preset_str[agc_preset],
                         fmt_gain(SOUND_LEFT_GAIN,
                             agc_maxgain -
                            (global_settings.rec_left_gain +
                             global_settings.rec_right_gain)/2,
                             buf2, sizeof(buf2)));                         

            if(global_settings.invert_cursor && ((cursor==4) || (cursor==5)))
            {
                for(i = 0; i < screen_update; i++)
                    screens[i].puts_style_offset(0, filename_offset[i] + 
                                        PM_HEIGHT + line[i], buf, STYLE_INVERT,0);
            }
            else if (global_settings.rec_source == AUDIO_SRC_MIC
                    || global_settings.rec_source == AUDIO_SRC_LINEIN
#ifdef HAVE_FMRADIO_IN
                    || global_settings.rec_source == AUDIO_SRC_FMRADIO
#endif
                    )    
            {
                for(i = 0; i < screen_update; i++) {
                    if (display_agc[i]) {
                        screens[i].puts(0, filename_offset[i] + 
                                        PM_HEIGHT + line[i], buf);
                    }
                }
            }
            
            if (global_settings.rec_source == AUDIO_SRC_MIC)
            {
                if(agc_maxgain < (global_settings.rec_mic_gain))
                    change_recording_gain(false, true, true);
            }
            else
            {
                if(agc_maxgain < (global_settings.rec_left_gain))
                    change_recording_gain(false, true, false);
                if(agc_maxgain < (global_settings.rec_right_gain))
                    change_recording_gain(false, false, true);
            }
#else
            }
#endif /* HAVE_AGC */

            if(!global_settings.invert_cursor) {
                switch(cursor)
                {
                    case 1:
                        for(i = 0; i < screen_update; i++)
                            screen_put_cursorxy(&screens[i], 0, 
                                                    filename_offset[i] +
                                                    PM_HEIGHT + 3, true);

                        if(global_settings.rec_source != AUDIO_SRC_MIC)
                        {
                            for(i = 0; i < screen_update; i++)
                                screen_put_cursorxy(&screens[i], 0, 
                                                        filename_offset[i] +
                                                        PM_HEIGHT + 4, true);
                        }
                    break;
                    case 2:
                        for(i = 0; i < screen_update; i++)
                            screen_put_cursorxy(&screens[i], 0, 
                                                    filename_offset[i] + 
                                                    PM_HEIGHT + 3, true);
                    break;
                    case 3:
                        for(i = 0; i < screen_update; i++)
                            screen_put_cursorxy(&screens[i], 0, 
                                                    filename_offset[i] + 
                                                    PM_HEIGHT + 4, true);
                    break;
#ifdef HAVE_AGC
                    case 4:
                    case 5:
                        for(i = 0; i < screen_update; i++)
                            screen_put_cursorxy(&screens[i], 0,
                                                filename_offset[i] + 
                                                PM_HEIGHT + line[i], true);
                        break;
#endif /* HAVE_AGC */
                    default:
                        for(i = 0; i < screen_update; i++)
                            screen_put_cursorxy(&screens[i], 0, 
                                                    filename_offset[i] + 
                                                    PM_HEIGHT + 2, true);
                }
            }

#ifdef HAVE_AGC
            hist_time++;
#endif

            for(i = 0; i < screen_update; i++)
            {
                gui_statusbar_draw(&(statusbars.statusbars[i]), true);
                peak_meter_screen(&screens[i], 0, pm_y[i], h*PM_HEIGHT);
                screens[i].update();                
            }

            /* draw the trigger status */
            FOR_NB_SCREENS(i)
            {
                trig_width[i] = ((screens[i].height < 64) ||
                                ((screens[i].height < 72) && (PM_HEIGHT > 1))) ?
                                  screens[i].width - 14 * w : screens[i].width;
                trig_xpos[i] = screens[i].width - trig_width[i];
                trig_ypos[i] =  ((screens[i].height < 72) && (PM_HEIGHT > 1)) ?
                                  h*2 :
                                  h*(1 + filename_offset[i] + PM_HEIGHT + line[i]
#ifdef HAVE_AGC
                               + 1
#endif
                               );
            }

            if (peak_meter_trigger_status() != TRIG_OFF)
            {
                peak_meter_draw_trig(trig_xpos, trig_ypos, trig_width,
                                         screen_update);
                for(i = 0; i < screen_update; i++){
                    screens[i].update_rect(trig_xpos[i], trig_ypos[i],
                                           trig_width[i] + 2, TRIG_HEIGHT);
                }
            }
        }

        if(audio_stat & AUDIO_STATUS_ERROR)
        {
            done = true;
        }
    } /* end while(!done) */

    audio_stat = audio_status();
    if (audio_stat & AUDIO_STATUS_ERROR)
    {
        gui_syncsplash(0, true, str(LANG_SYSFONT_DISK_FULL));
        gui_syncstatusbar_draw(&statusbars, true);
        
        FOR_NB_SCREENS(i)
            screens[i].update();
        
        audio_error_clear();

        while(1)
        {
             if (action_userabort(TIMEOUT_NOBLOCK))
                break;
        }
    }
    
#if CONFIG_CODEC == SWCODEC
    audio_stop_recording(); 
    audio_close_recording();

#ifdef HAVE_FMRADIO_IN
    if (radio_status != FMRADIO_OFF)
        /* Restore radio playback - radio_status should be unchanged if started
           through fm radio screen (barring usb connect) */
        rec_set_source(AUDIO_SRC_FMRADIO, (radio_status & FMRADIO_PAUSED) ?
                            SRCF_FMRADIO_PAUSED : SRCF_FMRADIO_PLAYING);
    else
#endif
        /* Go back to playback mode */
        rec_set_source(AUDIO_SRC_PLAYBACK, SRCF_PLAYBACK);

    /* restore talk_menu setting */
    global_settings.talk_menu = talk_menu;
#else /* !SWCODEC */
    audio_init_playback();
#endif /* CONFIG_CODEC == SWCODEC */

    /* make sure the trigger is really turned off */
    peak_meter_trigger(false);
    peak_meter_set_trigger_listener(NULL);

    global_settings.recscreen_on = false;
    sound_settings_apply();

    FOR_NB_SCREENS(i)
        screens[i].setfont(FONT_UI);

    if (have_recorded)
        reload_directory();

#if (CONFIG_LED == LED_REAL) && !defined(SIMULATOR)
    ata_set_led_enabled(true);
#endif
    return been_in_usb_mode;
} /* recording_screen */

#if CONFIG_KEYPAD == RECORDER_PAD
bool f2_rec_screen(void)
{
    static const char* const freq_str[6] =
    {
        "44.1kHz",
        "48kHz",
        "32kHz",
        "22.05kHz",
        "24kHz",
        "16kHz"
    };

    bool exit = false;
    bool used = false;
    int w, h, i;
    char buf[32];
    int button;
    struct audio_recording_options rec_options;

    FOR_NB_SCREENS(i)
    {
        screens[i].setfont(FONT_SYSFIXED);
        screens[i].getstringsize("A",&w,&h);
    }

    while (!exit) {
        const char* ptr=NULL;

        FOR_NB_SCREENS(i)
        {
            screens[i].clear_display();

            /* Recording quality */
            screens[i].putsxy(0, LCD_HEIGHT/2 - h*2,
                str(LANG_SYSFONT_RECORDING_QUALITY));
        }
        
            snprintf(buf, sizeof(buf), "%d", global_settings.rec_quality);
        FOR_NB_SCREENS(i)
        {
            screens[i].putsxy(0, LCD_HEIGHT/2-h, buf);
            screens[i].mono_bitmap(bitmap_icons_7x8[Icon_FastBackward], 
                        LCD_WIDTH/2 - 16, LCD_HEIGHT/2 - 4, 7, 8);
        }

        /* Frequency */
        snprintf(buf, sizeof buf, "%s:", str(LANG_SYSFONT_RECORDING_FREQUENCY));
        ptr = freq_str[global_settings.rec_frequency];
        FOR_NB_SCREENS(i)
        {
            screens[i].getstringsize(buf,&w,&h);
            screens[i].putsxy((LCD_WIDTH-w)/2, LCD_HEIGHT - h*2, buf);
            screens[i].getstringsize(ptr, &w, &h);
            screens[i].putsxy((LCD_WIDTH-w)/2, LCD_HEIGHT - h, ptr);
            screens[i].mono_bitmap(bitmap_icons_7x8[Icon_DownArrow],
                            LCD_WIDTH/2 - 3, LCD_HEIGHT - h*3, 7, 8);
        }

        /* Channel mode */
        switch ( global_settings.rec_channels ) {
            case 0:
                ptr = str(LANG_SYSFONT_CHANNEL_STEREO);
                break;

            case 1:
                ptr = str(LANG_SYSFONT_CHANNEL_MONO);
                break;
        }

        FOR_NB_SCREENS(i)
        {
            screens[i].getstringsize(str(LANG_SYSFONT_RECORDING_CHANNELS), &w, &h);
            screens[i].putsxy(LCD_WIDTH - w, LCD_HEIGHT/2 - h*2,
                       str(LANG_SYSFONT_RECORDING_CHANNELS));
            screens[i].getstringsize(str(LANG_SYSFONT_F2_MODE), &w, &h);
            screens[i].putsxy(LCD_WIDTH - w, LCD_HEIGHT/2 - h,
                       str(LANG_SYSFONT_F2_MODE));
            screens[i].getstringsize(ptr, &w, &h);
            screens[i].putsxy(LCD_WIDTH - w, LCD_HEIGHT/2, ptr);
            screens[i].mono_bitmap(bitmap_icons_7x8[Icon_FastForward], 
                        LCD_WIDTH/2 + 8, LCD_HEIGHT/2 - 4, 7, 8);

            screens[i].update();
        }

        button = button_get(true);
        switch (button) {
            case BUTTON_LEFT:
            case BUTTON_F2 | BUTTON_LEFT:
                global_settings.rec_quality++;
                if(global_settings.rec_quality > 7)
                    global_settings.rec_quality = 0;
                used = true;
                break;

            case BUTTON_DOWN:
            case BUTTON_F2 | BUTTON_DOWN:
                global_settings.rec_frequency++;
                if(global_settings.rec_frequency > 5)
                    global_settings.rec_frequency = 0;
                used = true;
                break;

            case BUTTON_RIGHT:
            case BUTTON_F2 | BUTTON_RIGHT:
                global_settings.rec_channels++;
                if(global_settings.rec_channels > 1)
                    global_settings.rec_channels = 0;
                used = true;
                break;

            case BUTTON_F2 | BUTTON_REL:
                if ( used )
                    exit = true;
                used = true;
                break;

            case BUTTON_F2 | BUTTON_REPEAT:
                used = true;
                break;

            default:
                if(default_event_handler(button) == SYS_USB_CONNECTED)
                    return true;
                break;
        }
    }

    rec_init_recording_options(&rec_options);
    rec_set_recording_options(&rec_options);

    set_gain();
    
    settings_save();
    FOR_NB_SCREENS(i)
        screens[i].setfont(FONT_UI);

    return false;
}

bool f3_rec_screen(void)
{
    bool exit = false;
    bool used = false;
    int w, h, i;
    int button;
    char *src_str[] =
    {
        str(LANG_SYSFONT_RECORDING_SRC_MIC),
        str(LANG_SYSFONT_RECORDING_SRC_LINE),
        str(LANG_SYSFONT_RECORDING_SRC_DIGITAL)
    };
    struct audio_recording_options rec_options;

    FOR_NB_SCREENS(i)
    {
        screens[i].setfont(FONT_SYSFIXED);
        screens[i].getstringsize("A",&w,&h);
    }
    
    while (!exit) {
        char* ptr=NULL;
        ptr = src_str[global_settings.rec_source];
        FOR_NB_SCREENS(i)
        {
            screens[i].clear_display();

            /* Recording source */
            screens[i].putsxy(0, LCD_HEIGHT/2 - h*2,
                str(LANG_SYSFONT_RECORDING_SOURCE));
 
            screens[i].getstringsize(ptr, &w, &h);
            screens[i].putsxy(0, LCD_HEIGHT/2-h, ptr);
            screens[i].mono_bitmap(bitmap_icons_7x8[Icon_FastBackward], 
                            LCD_WIDTH/2 - 16, LCD_HEIGHT/2 - 4, 7, 8);
        }

        /* trigger setup */
        ptr = str(LANG_SYSFONT_RECORD_TRIGGER);
        FOR_NB_SCREENS(i)
        {
            screens[i].getstringsize(ptr,&w,&h);
            screens[i].putsxy((LCD_WIDTH-w)/2, LCD_HEIGHT - h*2, ptr);
            screens[i].mono_bitmap(bitmap_icons_7x8[Icon_DownArrow],
                        LCD_WIDTH/2 - 3, LCD_HEIGHT - h*3, 7, 8);

            screens[i].update();
        }

        button = button_get(true);
        switch (button) {
            case BUTTON_DOWN:
            case BUTTON_F3 | BUTTON_DOWN:
#ifndef SIMULATOR
                rectrigger();
                settings_apply_trigger();
#endif
                exit = true;
                break;

            case BUTTON_LEFT:
            case BUTTON_F3 | BUTTON_LEFT:
                global_settings.rec_source++;
                if(global_settings.rec_source > AUDIO_SRC_MAX)
                    global_settings.rec_source = 0;
                used = true;
                break;

            case BUTTON_F3 | BUTTON_REL:
                if ( used )
                    exit = true;
                used = true;
                break;

            case BUTTON_F3 | BUTTON_REPEAT:
                used = true;
                break;

            default:
                if(default_event_handler(button) == SYS_USB_CONNECTED)
                    return true;
                break;
        }
    }

    rec_init_recording_options(&rec_options);
    rec_set_recording_options(&rec_options);

    set_gain();

    settings_save();
    FOR_NB_SCREENS(i)
        screens[i].setfont(FONT_UI);

    return false;
}
#endif /* CONFIG_KEYPAD == RECORDER_PAD */

#if CONFIG_CODEC == SWCODEC
void audio_beep(int duration)
{
    /* dummy */
    (void)duration;
}

#ifdef SIMULATOR
/* stubs for recording sim */
void audio_init_recording(unsigned int buffer_offset)
{
    buffer_offset = buffer_offset;
}

void audio_close_recording(void)
{
}

unsigned long audio_recorded_time(void)
{
    return 123;
}

unsigned long audio_num_recorded_bytes(void)
{
    return 5 * 1024 * 1024;
}

#if CONFIG_CODEC == SWCODEC
void rec_set_source(int source, unsigned flags)
{
    source = source;
    flags = flags;
}

#ifdef HAVE_SPDIF_IN
#ifdef HAVE_SPDIF_POWER
void audio_set_spdif_power_setting(bool on)
{
    on = on;
}

bool audio_get_spdif_power_setting(void)
{
    return true;
}
#endif /* HAVE_SPDIF_POWER */
#endif /* HAVE_SPDIF_IN */
#endif /* CONFIG_CODEC == SWCODEC */

void audio_set_recording_options(struct audio_recording_options *options)
{
    options = options;
}

void audio_set_recording_gain(int left, int right, int type)
{
    left = left;
    right = right;
    type = type;
}

void audio_stop_recording(void)
{
}

void audio_pause_recording(void)
{
}

void audio_resume_recording(void)
{
}

void pcm_calculate_rec_peaks(int *left, int *right)
{
    if (left)
        *left = 0;
    if (right)
        *right = 0;
}

void audio_record(const char *filename)
{
    filename = filename;
}

void audio_new_file(const char *filename)
{
    filename = filename;
}

unsigned long pcm_rec_status(void)
{
    return 0;
}

#endif /* #ifdef SIMULATOR */
#endif /* #ifdef CONFIG_CODEC == SWCODEC */


#endif /* HAVE_RECORDING */
