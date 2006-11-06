/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2005 by Linus Nielsen Feltzing
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#ifndef AUDIO_H
#define AUDIO_H

#include <stdbool.h>
#include <sys/types.h>
/* These must always be included with audio.h for this to compile under
   cetain conditions. Do it here or else spread the complication around to
   many files. */
#if CONFIG_CODEC == SWCODEC
#include "pcm_sampr.h"
#include "pcm_playback.h"
#ifdef HAVE_RECORDING
#include "pcm_record.h"
#include "id3.h"
#include "enc_base.h"
#endif /* HAVE_RECORDING */
#endif /* CONFIG_CODEC == SWCODEC */


#ifdef SIMULATOR
#define audio_play(x) sim_audio_play(x)
#endif

#define AUDIO_STATUS_PLAY 1
#define AUDIO_STATUS_PAUSE 2
#define AUDIO_STATUS_RECORD 4
#define AUDIO_STATUS_PRERECORD 8
#define AUDIO_STATUS_ERROR 16

#define AUDIOERR_DISK_FULL 1

#define AUDIO_GAIN_LINEIN    0
#define AUDIO_GAIN_MIC       1


struct audio_debug
{
        int audiobuflen;
        int audiobuf_write;
        int audiobuf_swapwrite;
        int audiobuf_read;

        int last_dma_chunk_size;

        bool dma_on;
        bool playing;
        bool play_pending;
        bool is_playing;
        bool filling;
        bool dma_underrun;

        int unplayed_space;
        int playable_space;
        int unswapped_space;

        int low_watermark_level;
        int lowest_watermark_level;
};

void audio_init(void);
void audio_play(long offset);
void audio_stop(void);
void audio_pause(void);
void audio_resume(void);
void audio_next(void);
void audio_prev(void);
int audio_status(void);
#if CONFIG_CODEC == SWCODEC
int audio_track_count(void); /* SWCODEC only */
long audio_filebufused(void); /* SWCODEC only */
void audio_pre_ff_rewind(void); /* SWCODEC only */
#endif /* CONFIG_CODEC == SWCODEC */
void audio_ff_rewind(long newtime);
void audio_flush_and_reload_tracks(void);
struct mp3entry* audio_current_track(void);
struct mp3entry* audio_next_track(void);
bool audio_has_changed_track(void);
void audio_get_debugdata(struct audio_debug *dbgdata);
void audio_set_crossfade(int type);
void audio_set_buffer_margin(int seconds);
unsigned int audio_error(void);
void audio_error_clear(void);
int audio_get_file_pos(void);
void audio_beep(int duration);
void audio_init_playback(void);
unsigned char *audio_get_buffer(bool talk_buf, size_t *buffer_size);

/* channel modes */
enum rec_channel_modes
{
    __CHN_MODE_START_INDEX = -1,

    CHN_MODE_STEREO,
    CHN_MODE_MONO,

    CHN_NUM_MODES
};

#if CONFIG_CODEC == SWCODEC
/* channel mode capability bits */
#define CHN_CAP_STEREO  (1 << CHN_MODE_STEREO)
#define CHN_CAP_MONO    (1 << CHN_MODE_MONO)
#define CHN_CAP_ALL     (CHN_CAP_STEREO | CHN_CAP_MONO)
#endif /* CONFIG_CODEC == SWCODEC */

/* audio sources */
enum audio_sources
{
    AUDIO_SRC_PLAYBACK = -1,    /* for audio playback (default) */
    AUDIO_SRC_MIC,              /* monitor mic */
    AUDIO_SRC_LINEIN,           /* monitor line in */
#ifdef HAVE_SPDIF_IN
    AUDIO_SRC_SPDIF,            /* monitor spdif */
#endif
#if defined(HAVE_FMRADIO_IN) || defined(CONFIG_TUNER)
    AUDIO_SRC_FMRADIO,          /* monitor fm radio */
#endif
    /* define new audio sources above this line */
    AUDIO_SOURCE_LIST_END,
    /* AUDIO_SRC_FMRADIO must be declared #ifdef CONFIG_TUNER but is not in
       the list of recordable sources. HAVE_FMRADIO_IN implies CONFIG_TUNER. */
#if defined(HAVE_FMRADIO_IN) || !defined(CONFIG_TUNER)
    AUDIO_NUM_SOURCES = AUDIO_SOURCE_LIST_END,
#else
    AUDIO_NUM_SOURCES = AUDIO_SOURCE_LIST_END-1,
#endif
    AUDIO_SRC_MAX = AUDIO_NUM_SOURCES-1
};

#ifdef HAVE_RECORDING
/* parameters for audio_set_recording_options */
struct audio_recording_options
{
    int  rec_source;
    int  rec_frequency;
    int  rec_channels;
    int  rec_prerecord_time;
#if CONFIG_CODEC == SWCODEC
    int  rec_source_flags;  /* for rec_set_source */
    struct encoder_config enc_config;
#else
    int  rec_quality;
    bool rec_editable;
#endif
};

/* audio recording functions */
void audio_init_recording(unsigned int buffer_offset);
void audio_close_recording(void);
void audio_record(const char *filename);
void audio_stop_recording(void);
void audio_pause_recording(void);
void audio_resume_recording(void);
void audio_new_file(const char *filename);
void audio_set_recording_options(struct audio_recording_options *options);
void audio_set_recording_gain(int left, int right, int type);
unsigned long audio_recorded_time(void);
unsigned long audio_num_recorded_bytes(void);

#if CONFIG_CODEC == SWCODEC
/* SWCODEC recoring functions */
/* playback.c */
bool audio_load_encoder(int afmt);
void audio_remove_encoder(void);
unsigned char *audio_get_recording_buffer(size_t *buffer_size);
#endif /* CONFIG_CODEC == SWCODEC */
#endif /* HAVE_RECORDING */

#ifdef HAVE_SPDIF_IN
#ifdef HAVE_SPDIF_POWER
void audio_set_spdif_power_setting(bool on);
bool audio_get_spdif_power_setting(void);
#endif
/* returns index into rec_master_sampr_list */
int audio_get_spdif_sample_rate(void);
/* > 0: monitor EBUin, 0: Monitor IISrecv, <0: reset only */
void audio_spdif_set_monitor(int monitor_spdif);
#endif /* HAVE_SPDIF_IN */

unsigned long audio_prev_elapsed(void);


/***********************************************************************/
/* audio event handling */

/* subscribe to one or more audio event(s) by OR'ing together the desired */
/* event IDs (defined below); a handler is called with a solitary event ID */
/* (so switch() is okay) and possibly some useful data (depending on the */
/* event); a handler must return one of the return codes defined below */

typedef int (*AUDIO_EVENT_HANDLER)(unsigned short event, unsigned long data);

void audio_register_event_handler(AUDIO_EVENT_HANDLER handler, unsigned short mask);

/***********************************************************************/
/* handler return codes */

#define AUDIO_EVENT_RC_IGNORED      200 
    /* indicates that no action was taken or the event was not recognized */

#define AUDIO_EVENT_RC_HANDLED      201 
    /* indicates that the event was handled and some action was taken which renders 
    the original event invalid; USE WITH CARE!; this return code aborts all further 
    processing of the given event */

/***********************************************************************/
/* audio event IDs */

#define AUDIO_EVENT_POS_REPORT      (1<<0)  
    /* sends a periodic song position report to handlers; a report is sent on
    each kernal tick; the number of ticks per second is defined by HZ; on each 
    report the current song position is passed in 'data'; if a handler takes an 
    action that changes the song or the song position it must return 
    AUDIO_EVENT_RC_HANDLED which suppresses the event for any remaining handlers */

#define AUDIO_EVENT_END_OF_TRACK    (1<<1) 
    /* generated when the end of the currently playing track is reached; no
    data is passed; if the handler implements some alternate end-of-track
    processing it should return AUDIO_EVENT_RC_HANDLED which suppresses the
    event for any remaining handlers as well as the normal end-of-track 
    processing */

#endif
