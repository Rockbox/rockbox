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
#define AUDIO_GAIN_DECIMATOR 2   /* for UDA1380 */


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
int audio_track_count(void); /* SWCODEC only */
void audio_pre_ff_rewind(void); /* SWCODEC only */
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

/* audio recording functions */
void audio_init_recording(unsigned int buffer_offset);
void audio_close_recording(void);
void audio_record(const char *filename);
void audio_stop_recording(void);
void audio_pause_recording(void);
void audio_resume_recording(void);
void audio_new_file(const char *filename);
void audio_set_recording_options(int frequency, int quality,
                                int source, int channel_mode,
                                bool editable, int prerecord_time);
void audio_set_recording_gain(int left, int right, int type);
unsigned long audio_recorded_time(void);
unsigned long audio_num_recorded_bytes(void);
void audio_set_spdif_power_setting(bool on);
unsigned long audio_get_spdif_sample_rate(void);



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
