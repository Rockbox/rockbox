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
#include "playback.h"
#include "enc_config.h"
#if defined(HAVE_SPDIF_IN) || defined(HAVE_SPDIF_OUT)
#include "spdif.h"
#endif
#endif /* CONFIG_CODEC == SWCODEC */
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
/* This array holds the record timer interval lengths, in seconds */
static const unsigned long rec_timer_seconds[] =
{
    0,        /* 0 means OFF */
    5*60,     /* 00:05 */
    10*60,    /* 00:10 */
    15*60,    /* 00:15 */
    30*60,    /* 00:30 */
    60*60,    /* 01:00 */
    74*60,    /* 74:00 */
    80*60,    /* 80:00 */
    2*60*60,  /* 02:00 */
    4*60*60,  /* 04:00 */
    6*60*60,  /* 06:00 */
    8*60*60,  /* 08:00 */
    10L*60*60, /* 10:00 */
    12L*60*60, /* 12:00 */
    18L*60*60, /* 18:00 */
    24L*60*60  /* 24:00 */
};

static unsigned int rec_timesplit_seconds(void)
{
    return rec_timer_seconds[global_settings.rec_timesplit];
}

/* This array holds the record size interval lengths, in bytes */
static const unsigned long rec_size_bytes[] =
{
    0,               /* 0 means OFF */
    5*1024*1024,     /* 5MB */
    10*1024*1024,    /* 10MB */
    15*1024*1024,    /* 15MB */
    32*1024*1024,    /* 32MB */
    64*1024*1024,    /* 64MB */
    75*1024*1024,    /* 75MB */
    100*1024*1024,   /* 100MB */
    128*1024*1024,   /* 128MB */
    256*1024*1024,   /* 256MB */
    512*1024*1024,   /* 512MB */
    650*1024*1024,   /* 650MB */
    700*1024*1024,   /* 700MB */
    1024*1024*1024,  /* 1GB */
    1536*1024*1024,  /* 1.5GB */
    1792*1024*1024,  /* 1.75GB  */
};

static unsigned long rec_sizesplit_bytes(void)
{
    return rec_size_bytes[global_settings.rec_sizesplit];
}
/*
 * Time strings used for the trigger durations.
 * Keep synchronous to trigger_times in settings_apply_trigger
 */
const struct opt_items trig_durations[TRIG_DURATION_COUNT] =
{
#define TS(x) { (unsigned char *)(#x "s"), TALK_ID(x, UNIT_SEC) }
#define TM(x) { (unsigned char *)(#x "min"), TALK_ID(x, UNIT_MIN) }
    TS(0), TS(1), TS(2), TS(5),
    TS(10), TS(15), TS(20), TS(25), TS(30),
    TM(1), TM(2), TM(5), TM(10)
#undef TS
#undef TM
};

void settings_apply_trigger(void)
{
    /* Keep synchronous to trig_durations and trig_durations_conf*/
    static const long trigger_times[TRIG_DURATION_COUNT] = {
        0, HZ, 2*HZ, 5*HZ,
        10*HZ, 15*HZ, 20*HZ, 25*HZ, 30*HZ,
        60*HZ, 2*60*HZ, 5*60*HZ, 10*60*HZ
    };

    peak_meter_define_trigger(
        global_settings.rec_start_thres,
        trigger_times[global_settings.rec_start_duration],
        MIN(trigger_times[global_settings.rec_start_duration] / 2, 2*HZ),
        global_settings.rec_stop_thres,
        trigger_times[global_settings.rec_stop_postrec],
        trigger_times[global_settings.rec_stop_gap]
    );
}
/* recording screen status flags */
enum rec_status_flags
{
    RCSTAT_IN_RECSCREEN      = 0x00000001,
    RCSTAT_BEEN_IN_USB_MODE  = 0x00000002,
    RCSTAT_CREATED_DIRECTORY = 0x00000004,
    RCSTAT_HAVE_RECORDED     = 0x00000008,
};

static int rec_status = 0;

bool in_recording_screen(void)
{   
    return (rec_status & RCSTAT_IN_RECSCREEN) != 0;
} 

#define PM_HEIGHT ((LCD_HEIGHT >= 72) ? 2 : 1)

#if CONFIG_KEYPAD == RECORDER_PAD
static bool f2_rec_screen(void);
static bool f3_rec_screen(void);
#endif

#define MAX_FILE_SIZE 0x7F800000 /* 2 GB - 4 MB */

#ifndef HAVE_REMOTE_LCD
static const int screen_update = NB_SCREENS;
#else
static int screen_update = NB_SCREENS;
static bool remote_display_on = true;
#endif

/** File name creation **/
#if CONFIG_RTC == 0
/* current file number to assist in creating unique numbered filenames
   without actually having to create the file on disk */
static int file_number = -1;
#endif /* CONFIG_RTC */

#if CONFIG_CODEC == SWCODEC

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

static const char* agc_preset_str[] =
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
static const short agc_th_hi[AGC_MODE_SIZE] =
{ 23197, 14637, 21156, 18428, 18426 };
/* autogain low level thresholds (-14dB, -11dB, -6dB, -7dB, -8dB) */
static const short agc_th_lo[AGC_MODE_SIZE] =
{ 6538, 9235, 16422, 14636, 13045 };
/* autogain threshold times [1/5s] or [200ms] */
static const short agc_tdrop[AGC_MODE_SIZE] =
{ 900,  225, 150, 60, 8 };
static const short agc_trise[AGC_MODE_SIZE] =
{ 9000, 750, 400, 150, 20 };
static const short agc_tbal[AGC_MODE_SIZE] =
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
        /* AUDIO_SRC_LINEIN, AUDIO_SRC_FMRADIO, AUDIO_SRC_SPDIF */
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
static bool read_peak_levels(int *peak_l, int *peak_r, int *balance)
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
static bool agc_gain_is_max(bool left, bool right)
{
    /* range -128...+108 [0.5dB] */
    short gain_current_l;
    short gain_current_r;

    if (agc_preset == 0)
        return false;

    switch (global_settings.rec_source)
    {
    HAVE_LINE_REC_(case AUDIO_SRC_LINEIN:)
    HAVE_FMRADIO_REC_(case AUDIO_SRC_FMRADIO:)
        gain_current_l = global_settings.rec_left_gain;
        gain_current_r = global_settings.rec_right_gain;
        break;
    case AUDIO_SRC_MIC:
    default:
        gain_current_l = global_settings.rec_mic_gain;
        gain_current_r = global_settings.rec_mic_gain;
    }

    return ((left && (gain_current_l >= agc_maxgain)) ||
            (right && (gain_current_r >= agc_maxgain)));
}

static void change_recording_gain(bool increment, bool left, bool right)
{
    int factor = (increment ? 1 : -1);

    switch (global_settings.rec_source)
    {
    HAVE_LINE_REC_(case AUDIO_SRC_LINEIN:)
    HAVE_FMRADIO_REC_(case AUDIO_SRC_FMRADIO:)
        if (left) global_settings.rec_left_gain += factor;
        if (right) global_settings.rec_right_gain += factor;
        break;
    case AUDIO_SRC_MIC:
         global_settings.rec_mic_gain += factor;
    }
}

/* 
 * Handle automatic gain control (AGC).
 * Change recording gain if peak_x levels are above or below
 * target volume for specified timeouts.
 */
static void auto_gain_control(int *peak_l, int *peak_r, int *balance)
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

static char *fmt_gain(int snd, int val, char *str, int len)
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

static void adjust_cursor(void)
{
    int max_cursor;
    
    if(cursor < 0)
        cursor = 0;

#ifdef HAVE_AGC
    switch(global_settings.rec_source)
    {
    case REC_SRC_MIC:
        if(cursor == 2)
            cursor = 4;
        else if(cursor == 3)
            cursor = 1;
    HAVE_LINE_REC_(case AUDIO_SRC_LINEIN:)
    HAVE_FMRADIO_REC_(case AUDIO_SRC_FMRADIO:)
        max_cursor = 5;
        break;
    default:
        max_cursor = 0;
        break;
    }
#else /* !HAVE_AGC */
    switch(global_settings.rec_source)
    {
    case AUDIO_SRC_MIC:
        max_cursor = 1;
        break;
    HAVE_LINE_REC_(case AUDIO_SRC_LINEIN:)
    HAVE_FMRADIO_REC_(case AUDIO_SRC_FMRADIO:)
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

/* the list below must match enum audio_sources in audio.h */
static const char* const prestr[] =
{
    HAVE_MIC_IN_([AUDIO_SRC_MIC]          = "R_MIC_",)
    HAVE_LINE_REC_([AUDIO_SRC_LINEIN]     = "R_LINE_",)
    HAVE_SPDIF_IN_([AUDIO_SRC_SPDIF]      = "R_SPDIF_",)
    HAVE_FMRADIO_REC_([AUDIO_SRC_FMRADIO] = "R_FM_",)
};

char *rec_create_filename(char *buffer)
{
    char ext[16];
    const char *pref = "R_";

    /* Directory existence and writeablility should have already been
     * verified - do not pass NULL pointers to pcmrec */

    if((unsigned)global_settings.rec_source < AUDIO_NUM_SOURCES)
    {
        pref = prestr[global_settings.rec_source];
    }

    strcpy(buffer, global_settings.rec_directory);
    
    snprintf(ext, sizeof(ext), ".%s",
             REC_FILE_ENDING(global_settings.rec_format));

#if CONFIG_RTC == 0
    return create_numbered_filename(buffer, buffer, pref, ext, 4,
                                    &file_number);
#else
    /* We'll wait at least up to the start of the next second so no duplicate
       names are created */
    return create_datetime_filename(buffer, buffer, pref, ext, true);
#endif
}

#if CONFIG_RTC == 0
/* Hit disk to get a starting filename for the type */
static void rec_init_filename(void)
{
    file_number = -1;
    rec_create_filename(path_buffer);
    file_number--;
}
#endif

int rec_create_directory(void)
{
    int rc = 0;
    const char * const folder = global_settings.rec_directory;

    if (strcmp(folder, "/") && !dir_exists(folder))
    {
        rc = mkdir(folder);

        if(rc < 0)
        {
            while (action_userabort(HZ) == false)
            {
                gui_syncsplash(0, "%s %s", 
                               str(LANG_REC_DIR_NOT_WRITABLE), 
                               str(LANG_OFF_ABORT));
            }
        }
        else
        {
            rec_status |= RCSTAT_CREATED_DIRECTORY;
            rc = 1;
        }
    }

    return rc;
}

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

#if CONFIG_CODEC == SWCODEC && !defined (SIMULATOR)
void rec_set_source(int source, unsigned flags)
{
    /* Set audio input source, power up/down devices */
    audio_set_input_source(source, flags);

    /* Set peakmeters for recording or reset to playback */
    peak_meter_playback((flags & SRCF_RECORDING) == 0);
    peak_meter_enabled = true;
}
#endif /* CONFIG_CODEC == SWCODEC && !defined (SIMULATOR) */

void rec_set_recording_options(struct audio_recording_options *options)
{
#if CONFIG_CODEC != SWCODEC
    if (global_settings.rec_prerecord_time)
    {
        talk_buffer_steal(); /* will use the mp3 buffer */
    }
#else /* == SWCODEC */
    rec_set_source(options->rec_source,
                   options->rec_source_flags | SRCF_RECORDING);
#endif /* CONFIG_CODEC != SWCODEC */

    audio_set_recording_options(options);
}

void rec_command(enum recording_command cmd)
{
    switch(cmd)
    {
        case RECORDING_CMD_STOP:
            pm_activate_clipcount(false);
            audio_stop_recording();
            break;
        case RECORDING_CMD_START:
            /* steal mp3 buffer, create unique filename and start recording */
            pm_reset_clipcount();
            pm_activate_clipcount(true);
#if CONFIG_CODEC != SWCODEC
            talk_buffer_steal(); /* we use the mp3 buffer */
#endif
            audio_record(rec_create_filename(path_buffer));
            break;
        case RECORDING_CMD_START_NEWFILE:
            /* create unique filename and start recording*/
            pm_reset_clipcount();
            pm_activate_clipcount(true); /* just to be sure */
            audio_new_file(rec_create_filename(path_buffer));
            break;
        case RECORDING_CMD_PAUSE:
            pm_activate_clipcount(false);
            audio_pause_recording();
            break;
        case RECORDING_CMD_RESUME:
            pm_activate_clipcount(true);
            audio_resume_recording();
            break;
    }
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
            if(!(audio_status() & AUDIO_STATUS_RECORD))
            {
                rec_status |= RCSTAT_HAVE_RECORDED;
                rec_command(RECORDING_CMD_START);
#if CONFIG_CODEC != SWCODEC
                /* give control to mpeg thread so that it can start
                   recording */
                yield(); yield(); yield();
#endif
            }

            /* if we're already recording this is a retrigger */
            else
            {
                if((audio_status() & AUDIO_STATUS_PAUSE) &&
                       (global_settings.rec_trigger_type == 1))
                {
                    rec_command(RECORDING_CMD_RESUME);
                }
                /* New file on trig start*/
                else if (global_settings.rec_trigger_type != 2)
                {
                    rec_command(RECORDING_CMD_START_NEWFILE);
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
                        rec_command(RECORDING_CMD_STOP);
                        break;

                    case 1: /* Pause */
                        rec_command(RECORDING_CMD_PAUSE);
                        break;

                    case 2: /* New file on trig stop*/
                        rec_command(RECORDING_CMD_START_NEWFILE);
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

bool recording_start_automatic = false;

bool recording_screen(bool no_source)
{
    long button;
    bool done = false;
    char buf[32];
    char buf2[32];
    int w, h;
    int update_countdown = 1;
    unsigned int seconds;
    int hours, minutes;
    char filename[13];
    int last_audio_stat = -1;
    int audio_stat;
#if CONFIG_CODEC == SWCODEC
    int warning_counter = 0;
    #define WARNING_PERIOD 7
#endif
#ifdef HAVE_FMRADIO_REC
    /* Radio is left on if:
     *   1) Is was on at the start and the initial source is FM Radio
     *   2) 1) and the source was never changed to something else
     */
    int radio_status = (global_settings.rec_source != AUDIO_SRC_FMRADIO) ?
                            FMRADIO_OFF : get_radio_status();
#endif
#if (CONFIG_LED == LED_REAL)
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
    /* pm_x = offset pm to put clipcount in front.
       Use lcd_getstringsize() when not using SYSFONT */
    int pm_x = global_settings.peak_meter_clipcounter ? 30 : 0;

    static const unsigned char *byte_units[] = {
        ID2P(LANG_BYTE),
        ID2P(LANG_KILOBYTE),
        ID2P(LANG_MEGABYTE),
        ID2P(LANG_GIGABYTE)
    };

    int style = STYLE_INVERT;
#ifdef HAVE_LCD_COLOR
    if (global_settings.cursor_style == 2) {
        style |= STYLE_COLORBAR;
    }
    else if (global_settings.cursor_style == 3) {
        style |= STYLE_GRADIENT | 1;
    }
#endif

    struct audio_recording_options rec_options;
    rec_status = RCSTAT_IN_RECSCREEN;

    if (rec_create_directory() < 0)
    {
        rec_status = 0;
        return false;
    }

    cursor = 0;
#if (CONFIG_LED == LED_REAL) && !defined(SIMULATOR)
    ata_set_led_enabled(false);
#endif

#if CONFIG_CODEC == SWCODEC
    /* recording_menu gets messed up: so prevent manus talking */
    talk_disable(true);
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

#if CONFIG_RTC == 0
    /* Create new filename for recording start */
    rec_init_filename();
#endif

    pm_reset_clipcount();
    pm_activate_clipcount(false);
    settings_apply_trigger();

#ifdef HAVE_AGC
    agc_preset_str[0] = str(LANG_SYSFONT_OFF);
    agc_preset_str[1] = str(LANG_SYSFONT_AGC_SAFETY);
    agc_preset_str[2] = str(LANG_SYSFONT_AGC_LIVE);
    agc_preset_str[3] = str(LANG_SYSFONT_AGC_DJSET);
    agc_preset_str[4] = str(LANG_SYSFONT_AGC_MEDIUM);
    agc_preset_str[5] = str(LANG_SYSFONT_AGC_VOICE);
    if (global_settings.rec_source == AUDIO_SRC_MIC) {
        agc_preset = global_settings.rec_agc_preset_mic;
        agc_maxgain = global_settings.rec_agc_maxgain_mic;
    }
    else {
        agc_preset = global_settings.rec_agc_preset_line;
        agc_maxgain = global_settings.rec_agc_maxgain_line;
    }
#endif /* HAVE_AGC */

    FOR_NB_SCREENS(i)
    {
        screens[i].setfont(FONT_SYSFIXED);
        screens[i].getstringsize("M", &w, &h);
        screens[i].setmargins(global_settings.cursor_style ? 0 : w, 8);
        filename_offset[i] = ((screens[i].height >= 80) ? 1 : 0);
        pm_y[i] = 8 + h * (2 + filename_offset[i]);
    }

#ifdef HAVE_REMOTE_LCD
    if (!remote_display_on)
    {
        screens[1].clear_display();
        snprintf(buf, sizeof(buf), str(LANG_REMOTE_LCD_ON));
        screens[1].puts((screens[1].width/w - strlen(buf))/2 + 1, 
                            screens[1].height/(h*2) + 1, buf);
        screens[1].update();
        gui_syncsplash(0, str(LANG_REMOTE_LCD_OFF));
    }
#endif

    while(!done)
    {
        audio_stat = audio_status();
        
#if (CONFIG_LED == LED_REAL)

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
        button = peak_meter_draw_get_btn(pm_x, pm_y, h * PM_HEIGHT, screen_update);

        if (last_audio_stat != audio_stat)
        {
            if (audio_stat & AUDIO_STATUS_RECORD)
            {
                rec_status |= RCSTAT_HAVE_RECORDED;
            }
            last_audio_stat = audio_stat;
        }


        if (recording_start_automatic)
        {
            /* simulate a button press */
            button = ACTION_REC_PAUSE;
            recording_start_automatic = false;
        }

        switch(button)
        {
#ifdef HAVE_REMOTE_LCD
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
                    gui_syncsplash(0, str(LANG_REMOTE_LCD_OFF));
                }
                else
                {
                    remote_display_on = true;
                    screen_update = NB_SCREENS;
                }
                break;
#endif
            case ACTION_STD_CANCEL:
                /* turn off the trigger */
                peak_meter_trigger(false);
                peak_meter_set_trigger_listener(NULL);

                if(audio_stat & AUDIO_STATUS_RECORD)
                {
                    rec_command(RECORDING_CMD_STOP);
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
                        rec_status |= RCSTAT_HAVE_RECORDED;
                        rec_command(RECORDING_CMD_START);
                        last_seconds = 0;
                        if (global_settings.talk_menu)
                        {   
                            /* no voice possible here, but a beep */
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
                        rec_command(RECORDING_CMD_START_NEWFILE);
                        last_seconds = 0;
                    }
                    else
                    /* if pause button pressed, pause or resume */
                    {
                        if(audio_stat & AUDIO_STATUS_PAUSE)
                        {
                            rec_command(RECORDING_CMD_RESUME);
                            if (global_settings.talk_menu)
                            {   
                                /* no voice possible here, but a beep */
                                audio_beep(HZ/4); /* short beep on resume */
                            }
                        }
                        else
                        {
                            rec_command(RECORDING_CMD_PAUSE);
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
            case ACTION_SETTINGS_INCREPEAT:
                switch(cursor)
                {
                    case 0:
                        global_settings.volume++;
                        setvol();
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
#endif /* HAVE_AGC */
                }
                set_gain();
                update_countdown = 1; /* Update immediately */
                break;
                
            case ACTION_SETTINGS_DEC:
            case ACTION_SETTINGS_DECREPEAT:
                switch(cursor)
                {
                    case 0:
                        global_settings.volume--;
                        setvol();
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
#endif /* HAVE_AGC */
                }
                set_gain();
                update_countdown = 1; /* Update immediately */
                break;
                
            case ACTION_STD_MENU:
#if CONFIG_CODEC == SWCODEC
                if(!(audio_stat & AUDIO_STATUS_RECORD))
#else
                if(audio_stat != AUDIO_STATUS_RECORD)
#endif
                {
#ifdef HAVE_FMRADIO_REC
                    const int prev_rec_source = global_settings.rec_source;
#endif

#if (CONFIG_LED == LED_REAL)
                    /* led is restored at begin of loop / end of function */
                    led(false);
#endif
                    if (recording_menu(no_source))
                    {
                        done = true;
                        rec_status |= RCSTAT_BEEN_IN_USB_MODE;
#ifdef HAVE_FMRADIO_REC
                        radio_status = FMRADIO_OFF;
#endif
                    }
                    else
                    {
#ifdef HAVE_FMRADIO_REC
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

                        if(rec_create_directory() < 0)
                        {
                            goto rec_abort;
                        }

#if CONFIG_CODEC == SWCODEC && CONFIG_RTC == 0
                        /* If format changed, a new number is required */
                        rec_init_filename();
#endif

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
                                global_settings.cursor_style ? 0 : w, 8);
                        }
                    }
                }
                break;

#if CONFIG_KEYPAD == RECORDER_PAD
            case ACTION_REC_F2:
                if(audio_stat != AUDIO_STATUS_RECORD)
                {
#if (CONFIG_LED == LED_REAL)
                    /* led is restored at begin of loop / end of function */
                    led(false);
#endif
                    if (f2_rec_screen())
                    {
                        rec_status |= RCSTAT_HAVE_RECORDED;
                        done = true;
                    }
                    else
                        update_countdown = 1; /* Update immediately */
                }
                break;

            case ACTION_REC_F3:
                if(audio_stat & AUDIO_STATUS_RECORD)
                {
                    rec_command(RECORDING_CMD_START_NEWFILE);
                    last_seconds = 0;
                }
                else
                {
#if (CONFIG_LED == LED_REAL)
                    /* led is restored at begin of loop / end of function */
                    led(false);
#endif
                    if (f3_rec_screen())
                    {
                        rec_status |= RCSTAT_HAVE_RECORDED;
                        done = true;
                    }
                    else
                        update_countdown = 1; /* Update immediately */
                }
                break;
#endif /*  CONFIG_KEYPAD == RECORDER_PAD */

            case SYS_USB_CONNECTED:
                /* Only accept USB connection when not recording */
                if(!(audio_stat & AUDIO_STATUS_RECORD))
                {
                    default_event_handler(SYS_USB_CONNECTED);
                    done = true;
                    rec_status |= RCSTAT_BEEN_IN_USB_MODE;
#ifdef HAVE_FMRADIO_REC
                    radio_status = FMRADIO_OFF;
#endif
                }
                break;
                
            default:
                default_event_handler(button);
                break;
        } /* end switch */

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

#if CONFIG_CODEC == SWCODEC
            if ((audio_stat & AUDIO_STATUS_WARNING)
                && (warning_counter++ % WARNING_PERIOD) < WARNING_PERIOD/2)
            {
                /* Switch back and forth displaying warning on first available
                   line to ensure visibility - the motion should also help
                   draw attention */
                /* Don't use language string unless agreed upon to make this
                   method permanent - could do something in the statusbar */
                snprintf(buf, sizeof(buf), "Warning: %08X",
                         pcm_rec_get_warnings());
            }
            else
#endif /* CONFIG_CODEC == SWCODEC */
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
                    *filename = '\0';
                    if (audio_stat & AUDIO_STATUS_RECORD)
                    {
                        strncpy(filename, path_buffer +
                                    strlen(path_buffer) - 12, 13);
                        filename[12]='\0';
                    }

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
                    rec_command(RECORDING_CMD_START_NEWFILE);
                    last_seconds = 0;
                }
                else
                {
                    peak_meter_trigger(false);
                    peak_meter_set_trigger_listener(NULL);
                    rec_command(RECORDING_CMD_STOP);
                }
                update_countdown = 1;
            }

            /* draw the clipcounter just in front of the peakmeter */
            if(global_settings.peak_meter_clipcounter)
            {
                char clpstr[32];
                snprintf(clpstr, 32, "%4d", pm_get_clipcount());
                for(i = 0; i < screen_update; i++)
                {
                    if(PM_HEIGHT > 1)
                        screens[i].puts(0, 2 + filename_offset[i],
                                                str(LANG_SYSFONT_PM_CLIPCOUNT));
                    screens[i].puts(0, 1 + PM_HEIGHT + filename_offset[i],
                                                                        clpstr);
                }
            }

            snprintf(buf, sizeof(buf), "%s: %s", str(LANG_SYSFONT_VOLUME),
                     fmt_gain(SOUND_VOLUME,
                              global_settings.volume,
                              buf2, sizeof(buf2)));
            
            if (global_settings.cursor_style && (pos++ == cursor))
            {
                for(i = 0; i < screen_update; i++)
                    screens[i].puts_style_offset(0, filename_offset[i] +
                                           PM_HEIGHT + 2, buf, style,0);
            }
            else
            {
                for(i = 0; i < screen_update; i++)
                    screens[i].puts(0, filename_offset[i] + PM_HEIGHT + 2, buf);
            }                

            if(global_settings.rec_source == AUDIO_SRC_MIC)
            {
                /* Draw MIC recording gain */
                snprintf(buf, sizeof(buf), "%s:%s", str(LANG_SYSFONT_GAIN),
                         fmt_gain(SOUND_MIC_GAIN,
                                  global_settings.rec_mic_gain,
                                  buf2, sizeof(buf2)));
                if(global_settings.cursor_style && ((1==cursor)||(2==cursor)))
                {
                    for(i = 0; i < screen_update; i++)
                        screens[i].puts_style_offset(0, filename_offset[i] +
                                            PM_HEIGHT + 3, buf, style,0);
                }
                else
                {
                    for(i = 0; i < screen_update; i++)
                        screens[i].puts(0, filename_offset[i] +
                                            PM_HEIGHT + 3, buf);
                }
            }
            else if(0
                HAVE_LINE_REC_( || global_settings.rec_source == AUDIO_SRC_LINEIN)
                HAVE_FMRADIO_REC_( || global_settings.rec_source == AUDIO_SRC_FMRADIO)
            )
            {
                /* Draw LINE or FMRADIO recording gain */
                snprintf(buf, sizeof(buf), "%s:%s",
                         str(LANG_SYSFONT_RECORDING_LEFT),
                         fmt_gain(SOUND_LEFT_GAIN,
                                  global_settings.rec_left_gain,
                                  buf2, sizeof(buf2)));
                if(global_settings.cursor_style && ((1==cursor)||(2==cursor)))
                {
                    for(i = 0; i < screen_update; i++)
                        screens[i].puts_style_offset(0, filename_offset[i] + 
                                           PM_HEIGHT + 3, buf, style,0);
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
                if(global_settings.cursor_style && ((1==cursor)||(3==cursor)))
                {
                    for(i = 0; i < screen_update; i++)
                        screens[i].puts_style_offset(0, filename_offset[i] + 
                                            PM_HEIGHT + 4, buf, style,0);
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
                HAVE_LINE_REC_(case AUDIO_SRC_LINEIN:)
                HAVE_FMRADIO_REC_(case AUDIO_SRC_FMRADIO:)
                    line[i] = 5;
                    break;

                case AUDIO_SRC_MIC:
                    line[i] = 4;
                    break;
#ifdef HAVE_SPDIF_REC
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
                         str(LANG_SYSFONT_RECORDING_AGC_MAXGAIN),
                         fmt_gain(SOUND_LEFT_GAIN,
                                  agc_maxgain, buf2, sizeof(buf2)));
            else if (agc_preset == 0)
                snprintf(buf, sizeof(buf), "%s: %s",
                         str(LANG_SYSFONT_RECORDING_AGC_PRESET),
                         agc_preset_str[agc_preset]);
            else if (global_settings.rec_source == AUDIO_SRC_MIC)
                snprintf(buf, sizeof(buf), "%s: %s%s",
                         str(LANG_SYSFONT_RECORDING_AGC_PRESET),
                         agc_preset_str[agc_preset],
                         fmt_gain(SOUND_LEFT_GAIN,
                             agc_maxgain -
                             global_settings.rec_mic_gain,
                             buf2, sizeof(buf2)));
            else
                snprintf(buf, sizeof(buf), "%s: %s%s",
                         str(LANG_SYSFONT_RECORDING_AGC_PRESET),
                         agc_preset_str[agc_preset],
                         fmt_gain(SOUND_LEFT_GAIN,
                             agc_maxgain -
                            (global_settings.rec_left_gain +
                             global_settings.rec_right_gain)/2,
                             buf2, sizeof(buf2)));                         

            if(global_settings.cursor_style && ((cursor==4) || (cursor==5)))
            {
                for(i = 0; i < screen_update; i++)
                    screens[i].puts_style_offset(0, filename_offset[i] + 
                                        PM_HEIGHT + line[i], buf, style,0);
            }
            else if (
                global_settings.rec_source == AUDIO_SRC_MIC
                HAVE_LINE_REC_(|| global_settings.rec_source == AUDIO_SRC_LINEIN)
                HAVE_FMRADIO_REC_(|| global_settings.rec_source == AUDIO_SRC_FMRADIO)
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
#else  /* !HAVE_AGC */
            }
#endif /* HAVE_AGC */

            if(!global_settings.cursor_style) {
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
                peak_meter_screen(&screens[i], pm_x, pm_y[i], h*PM_HEIGHT);
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
        gui_syncsplash(0, str(LANG_SYSFONT_DISK_FULL));
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

rec_abort:

#if CONFIG_CODEC == SWCODEC
    rec_command(RECORDING_CMD_STOP);
    audio_close_recording();

#ifdef HAVE_FMRADIO_REC
    if (radio_status != FMRADIO_OFF)
        /* Restore radio playback - radio_status should be unchanged if started
           through fm radio screen (barring usb connect) */
        rec_set_source(AUDIO_SRC_FMRADIO, (radio_status & FMRADIO_PAUSED) ?
                            SRCF_FMRADIO_PAUSED : SRCF_FMRADIO_PLAYING);
    else
#endif
        /* Go back to playback mode */
        rec_set_source(AUDIO_SRC_PLAYBACK, SRCF_PLAYBACK);

    /* restore talking */
    talk_disable(false);
#else /* !SWCODEC */
    audio_init_playback();
#endif /* CONFIG_CODEC == SWCODEC */

    /* make sure the trigger is really turned off */
    peak_meter_trigger(false);
    peak_meter_set_trigger_listener(NULL);

    rec_status &= ~RCSTAT_IN_RECSCREEN;
    sound_settings_apply();

    FOR_NB_SCREENS(i)
        screens[i].setfont(FONT_UI);

    /* if the directory was created or recording happened, make sure the
       browser is updated */
    if (rec_status & (RCSTAT_CREATED_DIRECTORY | RCSTAT_HAVE_RECORDED))
        reload_directory();

#if (CONFIG_LED == LED_REAL) && !defined(SIMULATOR)
    ata_set_led_enabled(true);
#endif

	settings_save();

    return (rec_status & RCSTAT_BEEN_IN_USB_MODE) != 0;
} /* recording_screen */

#if CONFIG_KEYPAD == RECORDER_PAD
static bool f2_rec_screen(void)
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
            screens[i].getstringsize(str(LANG_SYSFONT_CHANNELS), &w, &h);
            screens[i].putsxy(LCD_WIDTH - w, LCD_HEIGHT/2 - h*2,
                       str(LANG_SYSFONT_CHANNELS));
            screens[i].getstringsize(str(LANG_SYSFONT_MODE), &w, &h);
            screens[i].putsxy(LCD_WIDTH - w, LCD_HEIGHT/2 - h,
                       str(LANG_SYSFONT_MODE));
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

static bool f3_rec_screen(void)
{
    bool exit = false;
    bool used = false;
    int w, h, i;
    int button;
    char *src_str[] =
    {
        str(LANG_SYSFONT_RECORDING_SRC_MIC),
        str(LANG_SYSFONT_LINE_IN),
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

unsigned long pcm_rec_get_warnings(void)
{
    return 0;
}

unsigned long audio_recorded_time(void)
{
    return 123;
}

unsigned long audio_num_recorded_bytes(void)
{
    return 5 * 1024 * 1024;
}

void rec_set_source(int source, unsigned flags)
{
    source = source;
    flags = flags;
}

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

void audio_record(const char *filename)
{
    filename = filename;
}

void audio_new_file(const char *filename)
{
    filename = filename;
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

#endif /* #ifdef SIMULATOR */
#endif /* #ifdef CONFIG_CODEC == SWCODEC */

#endif /* HAVE_RECORDING */
