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
#include <stdbool.h>
#include <stdlib.h>
#include "config.h"

#if CONFIG_CODEC != SWCODEC

#include "debug.h"
#include "panic.h"
#include "id3.h"
#include "mpeg.h"
#include "audio.h"
#include "ata.h"
#include "string.h"
#include <kernel.h>
#include "thread.h"
#include "errno.h"
#include "mp3data.h"
#include "buffer.h"
#include "mp3_playback.h"
#include "sound.h"
#include "bitswap.h"
#ifndef SIMULATOR
#include "i2c.h"
#include "mas.h"
#include "dac.h"
#include "system.h"
#include "usb.h"
#include "file.h"
#include "hwcompat.h"
#endif /* !SIMULATOR */
#ifdef HAVE_LCD_BITMAP
#include "lcd.h"
#endif

#ifndef SIMULATOR
extern unsigned long mas_version_code;
#endif

#if CONFIG_CODEC == MAS3587F
extern enum /* from mp3_playback.c */
{
    MPEG_DECODER,
    MPEG_ENCODER
} mpeg_mode;
#endif /* CONFIG_CODEC == MAS3587F */

extern char* playlist_peek(int steps);
extern bool playlist_check(int steps);
extern int playlist_next(int steps);
extern int playlist_amount(void);
extern int playlist_update_resume_info(const struct mp3entry* id3);

#define MPEG_PLAY                  1
#define MPEG_STOP                  2
#define MPEG_PAUSE                 3
#define MPEG_RESUME                4
#define MPEG_NEXT                  5
#define MPEG_PREV                  6
#define MPEG_FF_REWIND             7
#define MPEG_FLUSH_RELOAD          8
#define MPEG_RECORD                9
#define MPEG_INIT_RECORDING       10
#define MPEG_INIT_PLAYBACK        11
#define MPEG_NEW_FILE             12
#define MPEG_PAUSE_RECORDING      13
#define MPEG_RESUME_RECORDING     14
#define MPEG_NEED_DATA           100
#define MPEG_TRACK_CHANGE        101
#define MPEG_SAVE_DATA           102
#define MPEG_STOP_DONE           103
#define MPEG_PRERECORDING_TICK   104

/* indicator for MPEG_NEED_DATA */
#define GENERATE_UNBUFFER_EVENTS 1

/* list of tracks in memory */
#define MAX_TRACK_ENTRIES (1<<4) /* Must be power of 2 */
#define MAX_TRACK_ENTRIES_MASK (MAX_TRACK_ENTRIES - 1)

struct trackdata
{
    struct mp3entry id3;
    int mempos;
    int load_ahead_index;
};

static struct trackdata trackdata[MAX_TRACK_ENTRIES];

static unsigned int current_track_counter = 0;
static unsigned int last_track_counter = 0;

/* Play time of the previous track */
unsigned long prev_track_elapsed;

#ifndef SIMULATOR
static int track_read_idx = 0;
static int track_write_idx = 0;
#endif /* !SIMULATOR */

/* Cuesheet callback */
static bool (*cuesheet_callback)(const char *filename) = NULL;

static const char mpeg_thread_name[] = "mpeg";
static unsigned int mpeg_errno;

static bool playing = false;    /* We are playing an MP3 stream */
static bool is_playing = false; /* We are (attempting to) playing MP3 files */
static bool paused;             /* playback is paused */

#ifdef SIMULATOR
static char mpeg_stack[DEFAULT_STACK_SIZE];
static struct mp3entry taginfo;

#else /* !SIMULATOR */
static struct event_queue mpeg_queue;
static long mpeg_stack[(DEFAULT_STACK_SIZE + 0x1000)/sizeof(long)];

static int audiobuflen;
static int audiobuf_write;
static int audiobuf_swapwrite;
static int audiobuf_read;

static int mpeg_file;

static bool play_pending; /* We are about to start playing */
static bool play_pending_track_change;  /* When starting play we're starting a new file */
static bool filling;      /* We are filling the buffer with data from disk */
static bool dma_underrun; /* True when the DMA has stopped because of
                             slow disk reading (read error, shaking) */
static bool mpeg_stop_done;

static int last_dma_tick = 0;
static int last_dma_chunk_size;

static long low_watermark;          /* Dynamic low watermark level */
static long low_watermark_margin = 0;   /* Extra time in seconds for watermark */
static long lowest_watermark_level; /* Debug value to observe the buffer
                                       usage */
#if CONFIG_CODEC == MAS3587F
static char recording_filename[MAX_PATH]; /* argument to thread */
static char delayed_filename[MAX_PATH];   /* internal copy of above */

static char xing_buffer[MAX_XING_HEADER_SIZE];

static bool init_recording_done;
static bool init_playback_done;
static bool prerecording;        /* True if prerecording is enabled */
static bool is_prerecording;     /* True if we are prerecording */
static bool is_recording;        /* We are recording */

static enum {
    NOT_SAVING = 0,  /* reasons to save data, sorted by importance */
    BUFFER_FULL,
    NEW_FILE,
    STOP_RECORDING
} saving_status;

static int rec_frequency_index; /* For create_xing_header() calls */
static int rec_version_index;   /* For create_xing_header() calls */

struct prerecord_info {
    int mempos;
    unsigned long framecount;
};

static struct prerecord_info prerecord_buffer[MPEG_MAX_PRERECORD_SECONDS];
static int prerecord_index;       /* Current index in the prerecord buffer */
static int prerecording_max_seconds;     /* Max number of seconds to store */
static int prerecord_count;   /* Number of seconds in the prerecord buffer */
static int prerecord_timeout; /* The tick count of the next prerecord data
                                 store */

unsigned long record_start_time; /* Value of current_tick when recording
                                    was started */
unsigned long pause_start_time;  /* Value of current_tick when pause was
                                    started */
static unsigned long last_rec_time;
static unsigned long num_rec_bytes;
static unsigned long last_rec_bytes;
static unsigned long frame_count_start;
static unsigned long frame_count_end;
static unsigned long saved_header = 0;

/* Shadow MAS registers */
unsigned long shadow_encoder_control = 0;
#endif /* CONFIG_CODEC == MAS3587F */

#if (CONFIG_CODEC == MAS3587F) || (CONFIG_CODEC == MAS3539F)
unsigned long shadow_io_control_main = 0;
unsigned long shadow_soft_mute = 0;
unsigned shadow_codec_reg0;
#endif /* (CONFIG_CODEC == MAS3587F) || (CONFIG_CODEC == MAS3539F) */

#ifdef HAVE_RECORDING
const unsigned char empty_id3_header[] =
{
    'I', 'D', '3', 0x03, 0x00, 0x00,
    0x00, 0x00, 0x1f, 0x76 /* Size is 4096 minus 10 bytes for the header */
};
#endif /* HAVE_RECORDING */


static int get_unplayed_space(void);
static int get_playable_space(void);
static int get_unswapped_space(void);
#endif /* !SIMULATOR */

#if (CONFIG_CODEC == MAS3587F) && !defined(SIMULATOR)
static void init_recording(void);
static void prepend_header(void);
static void update_header(void);
static void start_prerecording(void);
static void start_recording(void);
static void stop_recording(void);
static int get_unsaved_space(void);
static void pause_recording(void);
static void resume_recording(void);
#endif /* (CONFIG_CODEC == MAS3587F) && !defined(SIMULATOR) */


#ifndef SIMULATOR
static int num_tracks_in_memory(void)
{
    return (track_write_idx - track_read_idx) & MAX_TRACK_ENTRIES_MASK;
}

#ifdef DEBUG_TAGS
static void debug_tags(void)
{
    int i;

    for(i = 0;i < MAX_TRACK_ENTRIES;i++)
    {
        DEBUGF("%d - %s\n", i, trackdata[i].id3.path);
    }
    DEBUGF("read: %d, write :%d\n", track_read_idx, track_write_idx);
    DEBUGF("num_tracks_in_memory: %d\n", num_tracks_in_memory());
}
#else /* !DEBUG_TAGS */
#define debug_tags()
#endif /* !DEBUG_TAGS */

static void remove_current_tag(void)
{
    if(num_tracks_in_memory() > 0)
    {
        /* First move the index, so nobody tries to access the tag */
        track_read_idx = (track_read_idx+1) & MAX_TRACK_ENTRIES_MASK;
        debug_tags();
    }
    else
    {
        DEBUGF("remove_current_tag: no tracks to remove\n");
    }
}

static void remove_all_non_current_tags(void)
{
    track_write_idx = (track_read_idx+1) & MAX_TRACK_ENTRIES_MASK;
    debug_tags();
}

static void remove_all_tags(void)
{
    track_write_idx = track_read_idx;

    debug_tags();
}

static struct trackdata *get_trackdata(int offset)
{
    if(offset >= num_tracks_in_memory())
        return NULL;
    else
       return &trackdata[(track_read_idx + offset) & MAX_TRACK_ENTRIES_MASK];
}
#endif /* !SIMULATOR */

/***********************************************************************/
/* audio event handling */

#define MAX_EVENT_HANDLERS 10
struct event_handlers_table
{
    AUDIO_EVENT_HANDLER handler;
    unsigned short mask;
};
static struct event_handlers_table event_handlers[MAX_EVENT_HANDLERS];
static int event_handlers_count = 0;

void audio_register_event_handler(AUDIO_EVENT_HANDLER handler, unsigned short mask)
{
    if (event_handlers_count < MAX_EVENT_HANDLERS)
    {
        event_handlers[event_handlers_count].handler = handler;
        event_handlers[event_handlers_count].mask = mask;
        event_handlers_count++;
    }
}

/* dispatch calls each handler in the order registered and returns after some
   handler actually handles the event (the event is assumed to no longer be valid
   after this, due to the handler changing some condition); returns true if someone
   handled the event, which is expected to cause the caller to skip its own handling
   of the event */
#ifndef SIMULATOR
static bool audio_dispatch_event(unsigned short event, unsigned long data)
{
    int i = 0;
    for(i=0; i < event_handlers_count; i++)
    {
        if ( event_handlers[i].mask & event )
        {
            int rc = event_handlers[i].handler(event, data);
            if ( rc == AUDIO_EVENT_RC_HANDLED )
                return true;
        }
    }
    return false;
}
#endif

/***********************************************************************/

static void set_elapsed(struct mp3entry* id3)
{
    if ( id3->vbr ) {
        if ( id3->has_toc ) {
            /* calculate elapsed time using TOC */
            int i;
            unsigned int remainder, plen, relpos, nextpos;

            /* find wich percent we're at */
            for (i=0; i<100; i++ )
            {
                if ( id3->offset < id3->toc[i] * (id3->filesize / 256) )
                {
                    break;
                }
            }
            
            i--;
            if (i < 0)
                i = 0;

            relpos = id3->toc[i];

            if (i < 99)
            {
                nextpos = id3->toc[i+1];
            }
            else
            {
                nextpos = 256; 
            }

            remainder = id3->offset - (relpos * (id3->filesize / 256));

            /* set time for this percent (divide before multiply to prevent
               overflow on long files. loss of precision is negligible on
               short files) */
            id3->elapsed = i * (id3->length / 100);

            /* calculate remainder time */
            plen = (nextpos - relpos) * (id3->filesize / 256);
            id3->elapsed += (((remainder * 100) / plen) *
                             (id3->length / 10000));
        }
        else {
            /* no TOC exists. set a rough estimate using average bitrate */
            int tpk = id3->length / (id3->filesize / 1024);
            id3->elapsed = id3->offset / 1024 * tpk;
        }
    }
    else
        /* constant bitrate, use exact calculation */
        id3->elapsed = id3->offset / (id3->bitrate / 8);
}

int audio_get_file_pos(void)
{
    int pos = -1;
    struct mp3entry *id3 = audio_current_track();
    
    if (id3->vbr)
    {
        if (id3->has_toc)
        {
            /* Use the TOC to find the new position */
            unsigned int percent, remainder;
            int curtoc, nexttoc, plen;
                        
            percent = (id3->elapsed*100)/id3->length;
            if (percent > 99)
                percent = 99;
                        
            curtoc = id3->toc[percent];
                        
            if (percent < 99)
                nexttoc = id3->toc[percent+1];
            else
                nexttoc = 256;
                        
            pos = (id3->filesize/256)*curtoc;
                        
            /* Use the remainder to get a more accurate position */
            remainder   = (id3->elapsed*100)%id3->length;
            remainder   = (remainder*100)/id3->length;
            plen        = (nexttoc - curtoc)*(id3->filesize/256);
            pos     += (plen/100)*remainder;
        }
        else
        {
            /* No TOC exists, estimate the new position */
            pos = (id3->filesize / (id3->length / 1000)) *
                (id3->elapsed / 1000);
        }
    }
    else if (id3->bitrate)
        pos = id3->elapsed * (id3->bitrate / 8);
    else
    {
        return -1;
    }

    if (pos >= (int)(id3->filesize - id3->id3v1len))
    {
        /* Don't seek right to the end of the file so that we can
           transition properly to the next song */
        pos = id3->filesize - id3->id3v1len - 1;
    }
    else if (pos < (int)id3->first_frame_offset)
    {
        /* skip past id3v2 tag and other leading garbage */
        pos = id3->first_frame_offset;
    }
    return pos;    
}

unsigned long mpeg_get_last_header(void)
{
#ifdef SIMULATOR
    return 0;
#else /* !SIMULATOR */
    unsigned long tmp[2];

    /* Read the frame data from the MAS and reconstruct it with the
       frame sync and all */
    mas_readmem(MAS_BANK_D0, MAS_D0_MPEG_STATUS_1, tmp, 2);
    return 0xffe00000 | ((tmp[0] & 0x7c00) << 6) | (tmp[1] & 0xffff);
#endif /* !SIMULATOR */
}

void audio_set_cuesheet_callback(bool (*handler)(const char *filename))
{
    cuesheet_callback = handler;
}

#ifndef SIMULATOR
/* Send callback events to notify about removing old tracks. */
static void generate_unbuffer_events(void)
{
    int i;
    int numentries = MAX_TRACK_ENTRIES - num_tracks_in_memory();
    int cur_idx = track_write_idx;

    for (i = 0; i < numentries; i++)
    {
        /* Send an event to notify that track has finished. */
        send_event(PLAYBACK_EVENT_TRACK_FINISH, &trackdata[cur_idx].id3);
        cur_idx = (cur_idx + 1) & MAX_TRACK_ENTRIES_MASK;
    }
}

/* Send callback events to notify about new tracks. */
static void generate_postbuffer_events(void)
{
    int i;
    int numentries = num_tracks_in_memory();
    int cur_idx = track_read_idx;

    for (i = 0; i < numentries; i++)
    {
        send_event(PLAYBACK_EVENT_TRACK_BUFFER, &trackdata[cur_idx].id3);
        cur_idx = (cur_idx + 1) & MAX_TRACK_ENTRIES_MASK;
    }
}

static void recalculate_watermark(int bitrate)
{
    int bytes_per_sec;
    int time = ata_spinup_time;

    /* A bitrate of 0 probably means empty VBR header. We play safe
       and set a high threshold */
    if(bitrate == 0)
        bitrate = 320;
    
    bytes_per_sec = bitrate * 1000 / 8;
    
    if(time)
    {
        /* No drive spins up faster than 3.5s */
        if(time < 350)
            time = 350;

        time = time * 3;
        low_watermark = ((low_watermark_margin * HZ + time) *
                         bytes_per_sec) / HZ;
    }
    else
    {
        low_watermark = MPEG_LOW_WATER;
    }
}

#ifndef HAVE_FLASH_STORAGE
void audio_set_buffer_margin(int seconds)
{
    low_watermark_margin = seconds;
}
#endif

void audio_get_debugdata(struct audio_debug *dbgdata)
{
    dbgdata->audiobuflen = audiobuflen;
    dbgdata->audiobuf_write = audiobuf_write;
    dbgdata->audiobuf_swapwrite = audiobuf_swapwrite;
    dbgdata->audiobuf_read = audiobuf_read;

    dbgdata->last_dma_chunk_size = last_dma_chunk_size;

#if CONFIG_CPU == SH7034
    dbgdata->dma_on = (SCR0 & 0x80) != 0;
#endif
    dbgdata->playing = playing;
    dbgdata->play_pending = play_pending;
    dbgdata->is_playing = is_playing;
    dbgdata->filling = filling;
    dbgdata->dma_underrun = dma_underrun;

    dbgdata->unplayed_space = get_unplayed_space();
    dbgdata->playable_space = get_playable_space();
    dbgdata->unswapped_space = get_unswapped_space();

    dbgdata->low_watermark_level = low_watermark;
    dbgdata->lowest_watermark_level = lowest_watermark_level;
}

#ifdef DEBUG
static void dbg_timer_start(void)
{
    /* We are using timer 2 */
    
    TSTR &= ~0x04; /* Stop the timer */
    TSNC &= ~0x04; /* No synchronization */
    TMDR &= ~0x44; /* Operate normally */

    TCNT2 = 0;   /* Start counting at 0 */
    TCR2 = 0x03; /* Sysclock/8 */

    TSTR |= 0x04; /* Start timer 2 */
}

static int dbg_cnt2us(unsigned int cnt)
{
    return (cnt * 10000) / (FREQ/800);
}
#endif /* DEBUG */

static int get_unplayed_space(void)
{
    int space = audiobuf_write - audiobuf_read;
    if (space < 0)
        space += audiobuflen;
    return space;
}

static int get_playable_space(void)
{
    int space = audiobuf_swapwrite - audiobuf_read;
    if (space < 0)
        space += audiobuflen;
    return space;
}

static int get_unplayed_space_current_song(void)
{
    int space;

    if (num_tracks_in_memory() > 1)
    {
        space = get_trackdata(1)->mempos - audiobuf_read;
    }
    else
    {
        space = audiobuf_write - audiobuf_read;
    }

    if (space < 0)
        space += audiobuflen;

    return space;
}

static int get_unswapped_space(void)
{
    int space = audiobuf_write - audiobuf_swapwrite;
    if (space < 0)
        space += audiobuflen;
    return space;
}

#if CONFIG_CODEC == MAS3587F
static int get_unsaved_space(void)
{
    int space = audiobuf_write - audiobuf_read;
    if (space < 0)
        space += audiobuflen;
    return space;
}

static void drain_dma_buffer(void)
{
    while (PBDRH & 0x40)
    {
        xor_b(0x08, &PADRH);

        while (PBDRH & 0x80);

        xor_b(0x08, &PADRH);

        while (!(PBDRH & 0x80));
    }
}

#ifdef DEBUG
static long timing_info_index = 0;
static long timing_info[1024];
#endif /* DEBUG */

void rec_tick (void) __attribute__ ((section (".icode")));
void rec_tick(void)
{
    int i;
    int delay;
    char data;

    if(is_recording && (PBDRH & 0x40))
    {
#ifdef DEBUG
        timing_info[timing_info_index++] = current_tick;
        TCNT2 = 0;
#endif /* DEBUG */
        /* Note: Although this loop is run in interrupt context, further
         * optimisation will do no good. The MAS would then deliver bad
         * frames occasionally, as observed in extended experiments. */
        i = 0;
        while (PBDRH & 0x40)      /* We try to read as long as EOD is high */
        {
            xor_b(0x08, &PADRH);  /* Set PR active, independent of polarity */

            delay = 100;
            while (PBDRH & 0x80)  /* Wait until /RTW becomes active */
            {
                if (--delay <= 0) /* Bail out if we have to wait too long */
                {                 /* i.e. the MAS doesn't want to talk to us */
                    xor_b(0x08, &PADRH);         /* Set PR inactive */
                    goto transfer_end;           /* and get out of here */
                }
            }

            data = *(unsigned char *)0x04000000; /* read data byte */
                
            xor_b(0x08, &PADRH);                 /* Set PR inactive */

            audiobuf[audiobuf_write++] = data;

            if (audiobuf_write >= audiobuflen)
                audiobuf_write = 0;

            i++;
        }
      transfer_end:

#ifdef DEBUG
        timing_info[timing_info_index++] = TCNT2 + (i << 16);
        timing_info_index &= 0x3ff;
#endif /* DEBUG */

        num_rec_bytes += i;

        if(is_prerecording)
        {
            if(TIME_AFTER(current_tick, prerecord_timeout))
            {
                prerecord_timeout = current_tick + HZ;
                queue_post(&mpeg_queue, MPEG_PRERECORDING_TICK, 0);
            }
        }
        else
        {
            /* Signal to save the data if we are running out of buffer
               space */
            if (audiobuflen - get_unsaved_space() < MPEG_RECORDING_LOW_WATER
                && saving_status == NOT_SAVING)
            {
                saving_status = BUFFER_FULL;
                queue_post(&mpeg_queue, MPEG_SAVE_DATA, 0);
            }
        }
    }
}
#endif /* CONFIG_CODEC == MAS3587F */

void playback_tick(void)
{
    struct trackdata *ptd = get_trackdata(0);
    if(ptd)
    {
        ptd->id3.elapsed += (current_tick - last_dma_tick) * 1000 / HZ;
        last_dma_tick = current_tick;
        audio_dispatch_event(AUDIO_EVENT_POS_REPORT,
                             (unsigned long)ptd->id3.elapsed);
    }
}

static void reset_mp3_buffer(void)
{
    audiobuf_read = 0;
    audiobuf_write = 0;
    audiobuf_swapwrite = 0;
    lowest_watermark_level = audiobuflen;
}

 /* DMA transfer end interrupt callback */
static void transfer_end(unsigned char** ppbuf, size_t* psize)
{
    if(playing && !paused)
    {
        int unplayed_space_left;
        int space_until_end_of_buffer;
        int track_offset = 1;
        struct trackdata *track;

        audiobuf_read += last_dma_chunk_size;
        if(audiobuf_read >= audiobuflen)
            audiobuf_read = 0;
    
        /* First, check if we are on a track boundary */
        if (num_tracks_in_memory() > 1)
        {
            if (audiobuf_read == get_trackdata(track_offset)->mempos)
            {
                if ( ! audio_dispatch_event(AUDIO_EVENT_END_OF_TRACK, 0) )
                {
                    queue_post(&mpeg_queue, MPEG_TRACK_CHANGE, 0);
                    track_offset++;
                }
            }
        }
        
        unplayed_space_left = get_unplayed_space();
        
        space_until_end_of_buffer = audiobuflen - audiobuf_read;
        
        if(!filling && unplayed_space_left < low_watermark)
        {
            filling = true;
            queue_post(&mpeg_queue, MPEG_NEED_DATA, GENERATE_UNBUFFER_EVENTS);
        }
        
        if(unplayed_space_left)
        {
            last_dma_chunk_size = MIN(0x2000, unplayed_space_left);
            last_dma_chunk_size = MIN(last_dma_chunk_size,
                                      space_until_end_of_buffer);

            /* several tracks loaded? */
            track = get_trackdata(track_offset);
            if(track)
            {
                /* will we move across the track boundary? */
                if (( audiobuf_read < track->mempos ) &&
                    ((audiobuf_read+last_dma_chunk_size) >
                     track->mempos ))
                {
                    /* Make sure that we end exactly on the boundary */
                    last_dma_chunk_size = track->mempos - audiobuf_read;
                }
            }

            *psize = last_dma_chunk_size & 0xffff;
            *ppbuf = audiobuf + audiobuf_read;
            track = get_trackdata(0);
            if(track)
                track->id3.offset += last_dma_chunk_size;

            /* Update the watermark debug level */
            if(unplayed_space_left < lowest_watermark_level)
                lowest_watermark_level = unplayed_space_left;
        }
        else
        {
            /* Check if the end of data is because of a hard disk error.
               If there is an open file handle, we are still playing music.
               If not, the last file has been loaded, and the file handle is
               closed. */
            if(mpeg_file >= 0)
            {
                /* Update the watermark debug level */
                if(unplayed_space_left < lowest_watermark_level)
                    lowest_watermark_level = unplayed_space_left;
                
                DEBUGF("DMA underrun.\n");
                dma_underrun = true;
            }
            else
            {
                if ( ! audio_dispatch_event(AUDIO_EVENT_END_OF_TRACK, 0) )
                {
                    DEBUGF("No more MP3 data. Stopping.\n");
                    queue_post(&mpeg_queue, MPEG_TRACK_CHANGE, 0);
                    playing = false;
                }
            }
            *psize = 0; /* no more transfer */
        }
    }
}

static struct trackdata *add_track_to_tag_list(const char *filename)
{
    struct trackdata *track;
    
    if(num_tracks_in_memory() >= MAX_TRACK_ENTRIES)
    {
        DEBUGF("Tag memory is full\n");
        return NULL;
    }

    track = &trackdata[track_write_idx];
    
    /* grab id3 tag of new file and
       remember where in memory it starts */
    if(mp3info(&track->id3, filename))
    {
        DEBUGF("Bad mp3\n");
        return NULL;
    }
    track->mempos = audiobuf_write;
    track->id3.elapsed = 0;
#ifdef HAVE_LCD_BITMAP
    if (track->id3.title)
        lcd_getstringsize(track->id3.title, NULL, NULL);
    if (track->id3.artist)
        lcd_getstringsize(track->id3.artist, NULL, NULL);
    if (track->id3.album)
        lcd_getstringsize(track->id3.album, NULL, NULL);
#endif
    if (cuesheet_callback)
        if (cuesheet_callback(filename))
            track->id3.cuesheet_type = 1;

    track_write_idx = (track_write_idx+1) & MAX_TRACK_ENTRIES_MASK;
    debug_tags();
    return track;
}

static int new_file(int steps)
{
    int max_steps = playlist_amount();
    int start = 0;
    int i;
    struct trackdata *track;

    /* Find out how many steps to advance. The load_ahead_index field tells
       us how many playlist entries it had to skip to get to a valid one.
       We add those together to find out where to start. */
    if(steps > 0 && num_tracks_in_memory() > 1)
    {
        /* Begin with the song after the currently playing one */
        i = 1;
        while((track = get_trackdata(i++)))
        {
            start += track->load_ahead_index;
        }
    }
    
    do {
        char *trackname;

        trackname = playlist_peek( start + steps );
        if ( !trackname )
            return -1;
        
        DEBUGF("Loading %s\n", trackname);
        
        mpeg_file = open(trackname, O_RDONLY);
        if(mpeg_file < 0) {
            DEBUGF("Couldn't open file: %s\n",trackname);
            if(steps < 0)
               steps--;
            else
               steps++;
        }
        else
        {
            struct trackdata *track = add_track_to_tag_list(trackname);

            if(!track)
            {
                /* Bad mp3 file */
                if(steps < 0)
                    steps--;
                else
                    steps++;
                close(mpeg_file);
                mpeg_file = -1;
            }
            else
            {
                /* skip past id3v2 tag */
                lseek(mpeg_file, 
                      track->id3.first_frame_offset,
                      SEEK_SET);
                track->id3.index = steps;
                track->load_ahead_index = steps;
                track->id3.offset = 0;

                if(track->id3.vbr)
                    /* Average bitrate * 1.5 */
                    recalculate_watermark(
                        (track->id3.bitrate * 3) / 2);
                else
                    recalculate_watermark(
                        track->id3.bitrate);
                    
            }
        }

        /* Bail out if no file could be opened */
        if(abs(steps) > max_steps)
           return -1;
    } while ( mpeg_file < 0 );

    return 0;
}

static void stop_playing(void)
{
    struct trackdata *track;

    /* Stop the current stream */
    mp3_play_stop();
    playing = false;
    filling = false;

    track = get_trackdata(0);
    if (track != NULL)
        prev_track_elapsed = track->id3.elapsed;

    if(mpeg_file >= 0)
        close(mpeg_file);
    mpeg_file = -1;
    remove_all_tags();
    generate_unbuffer_events();
    reset_mp3_buffer();
}

static void end_current_track(void) {
    struct trackdata *track;

    play_pending = false;
    playing = false;
    mp3_play_pause(false);

    track = get_trackdata(0);
    if (track != NULL)
        prev_track_elapsed = track->id3.elapsed;

    reset_mp3_buffer();
    remove_all_tags();
    generate_unbuffer_events();

    if(mpeg_file >= 0)
        close(mpeg_file);
}

/* Is this a really the end of playback or is a new playlist starting */
static void check_playlist_end(int direction)
{
    /* Use the largest possible step size to account for skipped tracks */
    int steps = playlist_amount();

    if (direction < 0)
        steps = -steps;

    if (playlist_next(steps) < 0)
        is_playing = false;
}

static void update_playlist(void)
{
    if (num_tracks_in_memory() > 0)
    {
        struct trackdata *track = get_trackdata(0);
        track->id3.index = playlist_next(track->id3.index);
    }
    else
    {
        /* End of playlist? */
        check_playlist_end(1);
    }

    playlist_update_resume_info(audio_current_track());
}

static void track_change(void)
{
    DEBUGF("Track change\n");

    struct trackdata *track = get_trackdata(0);
    prev_track_elapsed = track->id3.elapsed;

#if (CONFIG_CODEC == MAS3587F) || (CONFIG_CODEC == MAS3539F)
    /* Reset the AVC */
    sound_set_avc(-1);
#endif /* (CONFIG_CODEC == MAS3587F) || (CONFIG_CODEC == MAS3539F) */

    if (num_tracks_in_memory() > 0)
    {
        remove_current_tag();
        send_event(PLAYBACK_EVENT_TRACK_CHANGE, audio_current_track());
        update_playlist();
    }

    current_track_counter++;
}

unsigned long audio_prev_elapsed(void)
{
    return prev_track_elapsed;
}

#ifdef DEBUG
void hexdump(const unsigned char *buf, int len)
{
    int i;

    for(i = 0;i < len;i++)
    {
        if(i && (i & 15) == 0)
        {
            DEBUGF("\n");
        }
        DEBUGF("%02x ", buf[i]);
    }
    DEBUGF("\n");
}
#endif /* DEBUG */

static void start_playback_if_ready(void)
{
    int playable_space;

    playable_space = audiobuf_swapwrite - audiobuf_read;
    if(playable_space < 0)
        playable_space += audiobuflen;
    
    /* See if we have started playing yet. If not, do it. */
    if(play_pending || dma_underrun)
    {
        /* If the filling has stopped, and we still haven't reached
           the watermark, the file must be smaller than the
           watermark. We must still play it. */
        if((playable_space >= MPEG_PLAY_PENDING_THRESHOLD) ||
           !filling || dma_underrun)
        {
            DEBUGF("P\n");
            if (play_pending) /* don't do this when recovering from DMA underrun */
            {
                generate_postbuffer_events(); /* signal first track as buffered */
                if (play_pending_track_change)
                {
                    play_pending_track_change = false;
                    send_event(PLAYBACK_EVENT_TRACK_CHANGE, audio_current_track());
                }
                play_pending = false;
            }
            playing = true;

            last_dma_chunk_size = MIN(0x2000, get_unplayed_space_current_song());
            mp3_play_data(audiobuf + audiobuf_read, last_dma_chunk_size, transfer_end);
            dma_underrun = false;

            if (!paused)
            {
                last_dma_tick = current_tick;
                mp3_play_pause(true);
            }

            /* Tell ourselves that we need more data */
            queue_post(&mpeg_queue, MPEG_NEED_DATA, 0);
        }
    }
}

static bool swap_one_chunk(void)
{
    int free_space_left;
    int amount_to_swap;

    free_space_left = get_unswapped_space();

    if(free_space_left == 0 && !play_pending)
        return false;

    /* Swap in larger chunks when the user is waiting for the playback
       to start, or when there is dangerously little playable data left */
    if(play_pending)
        amount_to_swap = MIN(MPEG_PLAY_PENDING_SWAPSIZE, free_space_left);
    else
    {
        if(get_playable_space() < low_watermark)
            amount_to_swap = MIN(MPEG_LOW_WATER_SWAP_CHUNKSIZE,
                                 free_space_left);
        else
            amount_to_swap = MIN(MPEG_SWAP_CHUNKSIZE, free_space_left);
    }
    
    if(audiobuf_write < audiobuf_swapwrite)
        amount_to_swap = MIN(audiobuflen - audiobuf_swapwrite,
                             amount_to_swap);
    else
        amount_to_swap = MIN(audiobuf_write - audiobuf_swapwrite,
                             amount_to_swap);

    bitswap(audiobuf + audiobuf_swapwrite, amount_to_swap);

    audiobuf_swapwrite += amount_to_swap;
    if(audiobuf_swapwrite >= audiobuflen)
    {
        audiobuf_swapwrite = 0;
    }

    return true;
}

static void mpeg_thread(void)
{
    static int pause_tick = 0;
    static unsigned int pause_track = 0;
    struct queue_event ev;
    int len;
    int free_space_left;
    int unplayed_space_left;
    int amount_to_read;
    int t1, t2;
    int start_offset;
#if CONFIG_CODEC == MAS3587F
    int amount_to_save;
    int save_endpos = 0;
    int rc;
    int level;
    long offset;
#endif /* CONFIG_CODEC == MAS3587F */

    is_playing = false;
    play_pending = false;
    playing = false;
    mpeg_file = -1;

    while(1)
    {
#if CONFIG_CODEC == MAS3587F
        if(mpeg_mode == MPEG_DECODER)
        {
#endif /* CONFIG_CODEC == MAS3587F */
        yield();

        /* Swap if necessary, and don't block on the queue_wait() */
        if(swap_one_chunk())
        {
            queue_wait_w_tmo(&mpeg_queue, &ev, 0);
        }
        else if (playing)
        {
            /* periodically update resume info */
            queue_wait_w_tmo(&mpeg_queue, &ev, HZ/2);
        }
        else
        {
            DEBUGF("S R:%x W:%x SW:%x\n",
                   audiobuf_read, audiobuf_write, audiobuf_swapwrite);
            queue_wait(&mpeg_queue, &ev);
        }

        start_playback_if_ready();
        
        switch(ev.id)
        {
            case MPEG_PLAY:
                DEBUGF("MPEG_PLAY\n");

#if CONFIG_TUNER
                /* Silence the A/D input, it may be on because the radio
                   may be playing */
                mas_codec_writereg(6, 0x0000);
#endif /* CONFIG_TUNER */

                /* Stop the current stream */
                paused = false;
                end_current_track();

                if ( new_file(0) == -1 )
                {
                    is_playing = false;
                    track_change();
                    break;
                }

                start_offset = (int)ev.data;

                /* mid-song resume? */
                if (start_offset) {
                    struct mp3entry* id3 = &get_trackdata(0)->id3;
                    lseek(mpeg_file, start_offset, SEEK_SET);
                    id3->offset = start_offset;
                    set_elapsed(id3);
                }
                else {
                    /* skip past id3v2 tag */
                    lseek(mpeg_file, 
                          get_trackdata(0)->id3.first_frame_offset, 
                          SEEK_SET);

                }

                /* Make it read more data */
                filling = true;
                queue_post(&mpeg_queue, MPEG_NEED_DATA, 0);

                /* Tell the file loading code that we want to start playing
                   as soon as we have some data */
                play_pending = true;
                play_pending_track_change = true;

                update_playlist();
                current_track_counter++;
                break;

            case MPEG_STOP:
                DEBUGF("MPEG_STOP\n");
                is_playing = false;
                paused = false;

                if (playing)
                    playlist_update_resume_info(audio_current_track());

                stop_playing();
                mpeg_stop_done = true;
                break;

            case MPEG_PAUSE:
                DEBUGF("MPEG_PAUSE\n");
                /* Stop the current stream */
                if (playing)
                    playlist_update_resume_info(audio_current_track());
                paused = true;
                playing = false;
                pause_tick = current_tick;
                pause_track = current_track_counter;
                mp3_play_pause(false);
                break;

            case MPEG_RESUME:
                DEBUGF("MPEG_RESUME\n");
                /* Continue the current stream */
                paused = false;
                if (!play_pending)
                {
                    playing = true;
                    if ( current_track_counter == pause_track )
                        last_dma_tick += current_tick - pause_tick;
                    else
                        last_dma_tick = current_tick;                        
                    pause_tick = 0;
                    mp3_play_pause(true);
                }
                break;

            case MPEG_NEXT:
                DEBUGF("MPEG_NEXT\n");
                /* is next track in ram? */
                if ( num_tracks_in_memory() > 1 ) {
                    int unplayed_space_left, unswapped_space_left;

                    /* stop the current stream */
                    play_pending = false;
                    playing = false;
                    mp3_play_pause(false);

                    track_change();
                    audiobuf_read = get_trackdata(0)->mempos;
                    last_dma_chunk_size = MIN(0x2000, get_unplayed_space_current_song());
                    mp3_play_data(audiobuf + audiobuf_read, last_dma_chunk_size, transfer_end);
                    dma_underrun = false;
                    last_dma_tick = current_tick;

                    unplayed_space_left  = get_unplayed_space();
                    unswapped_space_left = get_unswapped_space();

                    /* should we start reading more data? */
                    if(!filling && (unplayed_space_left < low_watermark)) {
                        filling = true;
                        queue_post(&mpeg_queue, MPEG_NEED_DATA, GENERATE_UNBUFFER_EVENTS);
                        play_pending = true;
                    } else if(unswapped_space_left &&
                              unswapped_space_left > unplayed_space_left) {
                        /* Stop swapping the data from the previous file */
                        audiobuf_swapwrite = audiobuf_read;
                        play_pending = true;
                    } else {
                        playing = true;
                        if (!paused)
                            mp3_play_pause(true);
                    }
                }
                else {
                    if (!playlist_check(1))
                        break;

                    /* stop the current stream */
                    end_current_track();

                    if (new_file(1) < 0) {
                        DEBUGF("No more files to play\n");
                        filling = false;

                        check_playlist_end(1);
                        current_track_counter++;
                    } else {
                        /* Make it read more data */
                        filling = true;
                        queue_post(&mpeg_queue, MPEG_NEED_DATA, 0);
                        
                        /* Tell the file loading code that we want
                           to start playing as soon as we have some data */
                        play_pending = true;
                        play_pending_track_change = true;

                        update_playlist();
                        current_track_counter++;
                    }
                }
                break;

            case MPEG_PREV: {
                DEBUGF("MPEG_PREV\n");

                if (!playlist_check(-1))
                    break;
                
                /* stop the current stream */
                end_current_track();

                /* Open the next file */
                if (new_file(-1) < 0) {
                    DEBUGF("No more files to play\n");
                    filling = false;

                    check_playlist_end(-1);
                    current_track_counter++;
                } else {
                    /* Make it read more data */
                    filling = true;
                    queue_post(&mpeg_queue, MPEG_NEED_DATA, 0);

                    /* Tell the file loading code that we want to
                       start playing as soon as we have some data */
                    play_pending = true;
                    play_pending_track_change = true;

                    update_playlist();
                    current_track_counter++;
                }
                break;
            }

            case MPEG_FF_REWIND: {
                struct mp3entry *id3  = audio_current_track();
                unsigned int oldtime  = id3->elapsed;
                unsigned int newtime  = (unsigned int)ev.data;
                int curpos, newpos, diffpos;
                DEBUGF("MPEG_FF_REWIND\n");

                id3->elapsed = newtime;

                newpos = audio_get_file_pos();
                if(newpos < 0)
                {
                    id3->elapsed = oldtime;
                    break;
                }
                
                if (mpeg_file >= 0)
                    curpos = lseek(mpeg_file, 0, SEEK_CUR);
                else
                    curpos = id3->filesize;

                if (num_tracks_in_memory() > 1)
                {
                    /* We have started loading other tracks that need to be
                       accounted for */
                    struct trackdata *track;
                    int i = 0;

                    while((track = get_trackdata(i++)))
                    {
                        curpos += track->id3.filesize;
                    }
                }

                diffpos = curpos - newpos;

                if(!filling && diffpos >= 0 && diffpos < audiobuflen)
                {
                    int unplayed_space_left, unswapped_space_left;

                    /* We are changing to a position that's already in
                       memory, so we just move the DMA read pointer. */
                    audiobuf_read = audiobuf_write - diffpos;
                    if (audiobuf_read < 0)
                    {
                        audiobuf_read += audiobuflen;
                    }

                    unplayed_space_left  = get_unplayed_space();
                    unswapped_space_left = get_unswapped_space();

                    /* If unswapped_space_left is larger than
                       unplayed_space_left, it means that the swapwrite pointer
                       hasn't yet advanced up to the new location of the read
                       pointer. We just move it, there is no need to swap
                       data that won't be played anyway. */
                    
                    if (unswapped_space_left > unplayed_space_left)
                    {
                        DEBUGF("Moved swapwrite\n");
                        audiobuf_swapwrite = audiobuf_read;
                        play_pending = true;
                    }

                    if (mpeg_file>=0 && unplayed_space_left < low_watermark)
                    {
                        /* We need to load more data before starting */
                        filling = true;
                        queue_post(&mpeg_queue, MPEG_NEED_DATA, GENERATE_UNBUFFER_EVENTS);
                        play_pending = true;
                    }
                    else
                    {
                        /* resume will start at new position */
                        last_dma_chunk_size = 
                            MIN(0x2000, get_unplayed_space_current_song());
                        mp3_play_data(audiobuf + audiobuf_read, 
                            last_dma_chunk_size, transfer_end);
                        dma_underrun = false;
                    }
                }
                else
                {
                    /* Move to the new position in the file and start
                       loading data */
                    reset_mp3_buffer();

                    if (num_tracks_in_memory() > 1)
                    {
                        /* We have to reload the current track */
                        close(mpeg_file);
                        remove_all_non_current_tags();
                        generate_unbuffer_events();
                        mpeg_file = -1;
                    }

                    if (mpeg_file < 0)
                    {
                        mpeg_file = open(id3->path, O_RDONLY);
                        if (mpeg_file < 0)
                        {
                            id3->elapsed = oldtime;
                            break;
                        }
                    }

                    if(-1 == lseek(mpeg_file, newpos, SEEK_SET))
                    {
                        id3->elapsed = oldtime;
                        break;
                    }

                    filling = true;
                    queue_post(&mpeg_queue, MPEG_NEED_DATA, 0);

                    /* Tell the file loading code that we want to start playing
                       as soon as we have some data */
                    play_pending = true;
                }

                id3->offset = newpos;

                break;
            }

            case MPEG_FLUSH_RELOAD: {
                int  numtracks    = num_tracks_in_memory();
                bool reload_track = false;

                if (numtracks > 1)
                {
                    /* Reset the buffer */
                    audiobuf_write = get_trackdata(1)->mempos;

                    /* Reset swapwrite unless we're still swapping current
                       track */
                    if (get_unplayed_space() <= get_playable_space())
                        audiobuf_swapwrite = audiobuf_write;

                    close(mpeg_file);
                    remove_all_non_current_tags();
                    generate_unbuffer_events();
                    mpeg_file = -1;
                    reload_track = true;
                }
                else if (numtracks == 1 && mpeg_file < 0)
                {
                    reload_track = true;
                }

                if(reload_track && new_file(1) >= 0)
                {
                    /* Tell ourselves that we want more data */
                    filling = true;
                    queue_post(&mpeg_queue, MPEG_NEED_DATA, 0);
                }

                break;
            }

            case MPEG_NEED_DATA:
                free_space_left = audiobuf_read - audiobuf_write;

                /* We interpret 0 as "empty buffer" */
                if(free_space_left <= 0)
                    free_space_left += audiobuflen;

                unplayed_space_left = audiobuflen - free_space_left;

                /* Make sure that we don't fill the entire buffer */
                free_space_left -= MPEG_HIGH_WATER;
                
                if (ev.data == GENERATE_UNBUFFER_EVENTS)
                    generate_unbuffer_events();

                /* do we have any more buffer space to fill? */
                if(free_space_left <= 0)
                {
                    DEBUGF("0\n");
                    filling = false;
                    generate_postbuffer_events();
                    ata_sleep();
                    break;
                }

                /* Read small chunks while we are below the low water mark */
                if(unplayed_space_left < low_watermark)
                    amount_to_read = MIN(MPEG_LOW_WATER_CHUNKSIZE,
                                         free_space_left);
                else
                    amount_to_read = free_space_left;

                /* Don't read more than until the end of the buffer */
                amount_to_read = MIN(audiobuflen - audiobuf_write,
                                     amount_to_read);
#ifdef HAVE_MMC /* MMC is slow, so don't read too large chunks */
                amount_to_read = MIN(0x40000, amount_to_read);
#elif MEM == 8
                amount_to_read = MIN(0x100000, amount_to_read);
#endif

                /* Read as much mpeg data as we can fit in the buffer */
                if(mpeg_file >= 0)
                {
                    DEBUGF("R\n");
                    t1 = current_tick;
                    len = read(mpeg_file, audiobuf + audiobuf_write,
                               amount_to_read);
                    if(len > 0)
                    {
                        t2 = current_tick;
                        DEBUGF("time: %d\n", t2 - t1);
                        DEBUGF("R: %x\n", len);

                        /* Now make sure that we don't feed the MAS with ID3V1
                           data */
                        if (len < amount_to_read)
                        {
                            int i;
                            static const unsigned char tag[] = "TAG";
                            int taglen = 128;
                            int tagptr = audiobuf_write + len - 128;
                            
                            /* Really rare case: entire potential tag wasn't
                               read in this call AND audiobuf_write < 128 */
                            if (tagptr < 0)
                                tagptr += audiobuflen;
                            
                            for(i = 0;i < 3;i++)
                            {
                                if(tagptr >= audiobuflen)
                                    tagptr -= audiobuflen;

                                if(audiobuf[tagptr] != tag[i])
                                {
                                    taglen = 0;
                                    break;
                                }

                                tagptr++;
                            }

                            if(taglen)
                            {
                                /* Skip id3v1 tag */
                                DEBUGF("Skipping ID3v1 tag\n");
                                len -= taglen;

                                /* In the very rare case when the entire tag
                                   wasn't read in this read() len will be < 0.
                                   Take care of this when changing the write
                                   pointer. */
                            }
                        }

                        audiobuf_write += len;

                        if (audiobuf_write < 0)
                            audiobuf_write += audiobuflen;

                        if(audiobuf_write >= audiobuflen)
                        {
                            audiobuf_write = 0;
                            DEBUGF("W\n");
                        }

                        /* Tell ourselves that we want more data */
                        queue_post(&mpeg_queue, MPEG_NEED_DATA, 0);
                    }
                    else
                    {
                        if(len < 0)
                        {
                            DEBUGF("MPEG read error\n");
                        }
                        
                        close(mpeg_file);
                        mpeg_file = -1;
                    
                        if(new_file(1) < 0)
                        {
                            /* No more data to play */
                            DEBUGF("No more files to play\n");
                            filling = false;
                        }
                        else
                        {
                            /* Tell ourselves that we want more data */
                            queue_post(&mpeg_queue, MPEG_NEED_DATA, 0);
                        }
                    }
                }
                break;
                
            case MPEG_TRACK_CHANGE:
                track_change();
                break;

#ifndef USB_NONE
            case SYS_USB_CONNECTED:
                is_playing = false;
                paused = false;
                stop_playing();

                /* Tell the USB thread that we are safe */
                DEBUGF("mpeg_thread got SYS_USB_CONNECTED\n");
                usb_acknowledge(SYS_USB_CONNECTED_ACK);

                /* Wait until the USB cable is extracted again */
                usb_wait_for_disconnect(&mpeg_queue);
                break;
#endif /* !USB_NONE */
                
#if CONFIG_CODEC == MAS3587F
            case MPEG_INIT_RECORDING:
                init_recording();
                init_recording_done = true;
                break;
#endif /* CONFIG_CODEC == MAS3587F */

            case SYS_TIMEOUT:
                if (playing)
                    playlist_update_resume_info(audio_current_track());
                break;
        }
#if CONFIG_CODEC == MAS3587F
        }
        else
        {
            queue_wait(&mpeg_queue, &ev);
            switch(ev.id)
            {
                case MPEG_RECORD:
                    if (is_prerecording)
                    {
                        int startpos;

                        /* Go back prerecord_count seconds in the buffer */
                        startpos = prerecord_index - prerecord_count;
                        if(startpos < 0)
                            startpos += prerecording_max_seconds;

                        /* Read the position data from the prerecord buffer */
                        frame_count_start = prerecord_buffer[startpos].framecount;
                        startpos = prerecord_buffer[startpos].mempos;

                        DEBUGF("Start looking at address %x (%x)\n",
                               audiobuf+startpos, startpos);

                        saved_header = mpeg_get_last_header();
                        
                        mem_find_next_frame(startpos, &offset, 1800,
                                            saved_header);

                        audiobuf_read = startpos + offset;
                        if(audiobuf_read >= audiobuflen)
                            audiobuf_read -= audiobuflen;

                        DEBUGF("New audiobuf_read address: %x (%x)\n",
                               audiobuf+audiobuf_read, audiobuf_read);

                        level = disable_irq_save();
                        num_rec_bytes = get_unsaved_space();
                        restore_irq(level);
                    }
                    else
                    {
                        frame_count_start = 0;
                        num_rec_bytes = 0;
                        audiobuf_read = MPEG_RESERVED_HEADER_SPACE;
                        audiobuf_write = MPEG_RESERVED_HEADER_SPACE;
                    }

                    prepend_header();
                    DEBUGF("Recording...\n");
                    start_recording();

                    /* Wait until at least one frame is encoded and get the
                       frame header, for later use by the Xing header
                       generation */
                    sleep(HZ/5);
                    saved_header = mpeg_get_last_header();

                    /* delayed until buffer is saved, don't open yet */
                    strcpy(delayed_filename, recording_filename);
                    mpeg_file = -1; 

                    break;
                    
                case MPEG_STOP:
                    DEBUGF("MPEG_STOP\n");

                    stop_recording();

                    /* Save the remaining data in the buffer */
                    save_endpos = audiobuf_write;
                    saving_status = STOP_RECORDING;
                    queue_post(&mpeg_queue, MPEG_SAVE_DATA, 0);
                    break;

                case MPEG_STOP_DONE:
                    DEBUGF("MPEG_STOP_DONE\n");

                    if (mpeg_file >= 0)
                        close(mpeg_file);
                    mpeg_file = -1;

                    update_header();
#ifdef DEBUG1
                    {
                        int i;
                        for(i = 0;i < 512;i++)
                        {
                            DEBUGF("%d - %d us (%d bytes)\n",
                                   timing_info[i*2],
                                   (timing_info[i*2+1] & 0xffff) *
                                   10000 / 13824,
                                   timing_info[i*2+1] >> 16);
                        }
                    }
#endif /* DEBUG1 */

                    if (prerecording)
                    {
                        start_prerecording();
                    }
                    mpeg_stop_done = true;
                    break;

                case MPEG_NEW_FILE:
                    /* Bail out when a more important save is happening */
                    if (saving_status > NEW_FILE)
                        break;

                    /* Make sure we have at least one complete frame
                       in the buffer. If we haven't recorded a single
                       frame within 200ms, the MAS is probably not recording
                       anything, and we bail out. */
                    amount_to_save = get_unsaved_space();
                    if (amount_to_save < 1800)
                    {
                        sleep(HZ/5);
                        amount_to_save = get_unsaved_space();
                    }

                    mas_readmem(MAS_BANK_D0, MAS_D0_MPEG_FRAME_COUNT,
                                &frame_count_end, 1);
 
                    last_rec_time = current_tick - record_start_time;
                    record_start_time = current_tick;
                    if (paused)
                        pause_start_time = record_start_time;

                    /* capture all values at one point */
                    level = disable_irq_save();
                    save_endpos = audiobuf_write;
                    last_rec_bytes = num_rec_bytes;
                    num_rec_bytes = 0;
                    restore_irq(level);

                    if (amount_to_save >= 1800)
                    {
                        /* Now find a frame boundary to split at */
                        save_endpos -= 1800;
                        if (save_endpos < 0)
                            save_endpos += audiobuflen;

                        rc = mem_find_next_frame(save_endpos, &offset, 1800,
                                                 saved_header);
                        if (!rc) /* No header found, save whole buffer */
                            offset = 1800;

                        save_endpos += offset;
                        if (save_endpos >= audiobuflen)
                            save_endpos -= audiobuflen;

                        last_rec_bytes += offset - 1800;
                        level = disable_irq_save();
                        num_rec_bytes += 1800 - offset;
                        restore_irq(level);
                    }

                    saving_status = NEW_FILE;
                    queue_post(&mpeg_queue, MPEG_SAVE_DATA, 0);
                    break;

                case MPEG_SAVE_DATA:    
                    if (saving_status == BUFFER_FULL)
                        save_endpos = audiobuf_write;

                    if (mpeg_file < 0) /* delayed file open */
                    {
                        mpeg_file = open(delayed_filename, O_WRONLY|O_CREAT);

                        if (mpeg_file < 0)
                            panicf("recfile: %d", mpeg_file);
                    }

                    amount_to_save = save_endpos - audiobuf_read;
                    if (amount_to_save < 0)
                        amount_to_save += audiobuflen;

                    amount_to_save = MIN(amount_to_save,
                                         audiobuflen - audiobuf_read);
#ifdef HAVE_MMC /* MMC is slow, so don't save too large chunks at once */
                    amount_to_save = MIN(0x40000, amount_to_save);
#elif MEM == 8
                    amount_to_save = MIN(0x100000, amount_to_save);
#endif
                    rc = write(mpeg_file, audiobuf + audiobuf_read,
                               amount_to_save);
                    if (rc < 0)
                    {
                        if (errno == ENOSPC)
                        {
                            mpeg_errno = AUDIOERR_DISK_FULL;
                            stop_recording();
                            queue_post(&mpeg_queue, MPEG_STOP_DONE, 0);
                            /* will close the file */
                            break;
                        }
                        else
                            panicf("rec wrt: %d", rc);
                    }

                    audiobuf_read += amount_to_save;
                    if (audiobuf_read >= audiobuflen)
                        audiobuf_read = 0;

                    if (audiobuf_read == save_endpos) /* all saved */
                    {
                        switch (saving_status)
                        {
                            case BUFFER_FULL:
                                rc = fsync(mpeg_file);
                                if (rc < 0)
                                    panicf("rec fls: %d", rc);
                                ata_sleep();
                                break;

                            case NEW_FILE:
                                /* Close the current file */
                                rc = close(mpeg_file);
                                if (rc < 0)
                                    panicf("rec cls: %d", rc);
                                mpeg_file = -1;
                                update_header();
                                ata_sleep();

                                /* copy new filename */
                                strcpy(delayed_filename, recording_filename);
                                prepend_header();
                                frame_count_start = frame_count_end;
                                break;

                            case STOP_RECORDING:
                                queue_post(&mpeg_queue, MPEG_STOP_DONE, 0);
                                /* will close the file */
                                break;

                            default:
                                break;
                        }
                        saving_status = NOT_SAVING;
                    }
                    else /* tell ourselves to save the next chunk */
                        queue_post(&mpeg_queue, MPEG_SAVE_DATA, 0);

                    break;
                    
                case MPEG_PRERECORDING_TICK:
                    if(!is_prerecording)
                        break;

                    /* Store the write pointer every second */
                    prerecord_buffer[prerecord_index].mempos = audiobuf_write;
                    mas_readmem(MAS_BANK_D0, MAS_D0_MPEG_FRAME_COUNT,
                                &prerecord_buffer[prerecord_index].framecount, 1);

                    /* Wrap if necessary */
                    if(++prerecord_index == prerecording_max_seconds)
                        prerecord_index = 0;

                    /* Update the number of seconds recorded */
                    if(prerecord_count < prerecording_max_seconds)
                        prerecord_count++;
                    break;

                case MPEG_INIT_PLAYBACK:
                    /* Stop the prerecording */ 
                    stop_recording();
                    reset_mp3_buffer();
                    mp3_play_init();
                    init_playback_done = true;
                    break;
                    
                case MPEG_PAUSE_RECORDING:
                    pause_recording();
                    break;
                   
                case MPEG_RESUME_RECORDING:
                    resume_recording();
                    break;
                   
                case SYS_USB_CONNECTED:
                    /* We can safely go to USB mode if no recording
                       is taking place */
                    if((!is_recording || is_prerecording) && mpeg_stop_done)
                    {
                        /* Even if we aren't recording, we still call this
                           function, to put the MAS in monitoring mode,
                           to save power. */
                        stop_recording();
                
                        /* Tell the USB thread that we are safe */
                        DEBUGF("mpeg_thread got SYS_USB_CONNECTED\n");
                        usb_acknowledge(SYS_USB_CONNECTED_ACK);
                    
                        /* Wait until the USB cable is extracted again */
                        usb_wait_for_disconnect(&mpeg_queue);
                    }
                    break;
            }
        }
#endif /* CONFIG_CODEC == MAS3587F */
    }
}
#endif /* !SIMULATOR */

struct mp3entry* audio_current_track()
{
#ifdef SIMULATOR
    return &taginfo;
#else /* !SIMULATOR */
    if(num_tracks_in_memory())
        return &get_trackdata(0)->id3;
    else
        return NULL;
#endif /* !SIMULATOR */
}

struct mp3entry* audio_next_track()
{
#ifdef SIMULATOR
    return &taginfo;
#else /* !SIMULATOR */
    if(num_tracks_in_memory() > 1)
        return &get_trackdata(1)->id3;
    else
        return NULL;
#endif /* !SIMULATOR */
}

bool audio_has_changed_track(void)
{
    if(last_track_counter != current_track_counter)
    {
        last_track_counter = current_track_counter;
        return true;
    }
    return false;
}

#if CONFIG_CODEC == MAS3587F
#ifndef SIMULATOR
void audio_init_playback(void)
{
    init_playback_done = false;
    queue_post(&mpeg_queue, MPEG_INIT_PLAYBACK, 0);

    while(!init_playback_done)
        sleep(1);
}


/****************************************************************************
 * Recording functions
 ***************************************************************************/
void audio_init_recording(unsigned int buffer_offset)
{
    buffer_offset = buffer_offset;
    init_recording_done = false;
    queue_post(&mpeg_queue, MPEG_INIT_RECORDING, 0);

    while(!init_recording_done)
        sleep(1);
}

static void init_recording(void)
{
    unsigned long val;
    int rc;

    /* Disable IRQ6 */
    IPRB &= 0xff0f;

    stop_playing();
    is_playing = false;
    paused = false;

    /* Init the recording variables */
    is_recording = false;
    is_prerecording = false;

    mpeg_stop_done = true;
    
    mas_reset();
    
    /* Enable the audio CODEC and the DSP core, max analog voltage range */
    rc = mas_direct_config_write(MAS_CONTROL, 0x8c00);
    if(rc < 0)
        panicf("mas_ctrl_w: %d", rc);

    /* Stop the current application */
    val = 0;
    mas_writemem(MAS_BANK_D0, MAS_D0_APP_SELECT, &val, 1);
    do
    {
        mas_readmem(MAS_BANK_D0, MAS_D0_APP_RUNNING, &val, 1);
    } while(val);

    /* Perform black magic as described by the data sheet */
    if((mas_version_code & 0x0fff) == 0x0102)
    {       
        DEBUGF("Performing MAS black magic for B2 version\n");
        mas_writereg(0xa3, 0x98);
        mas_writereg(0x94, 0xfffff);
        val = 0;
        mas_writemem(MAS_BANK_D1, 0, &val, 1);
        mas_writereg(0xa3, 0x90);
    }

    /* Enable A/D Converters */
    shadow_codec_reg0 = 0xcccd;
    mas_codec_writereg(0x0, shadow_codec_reg0);

    /* Copy left channel to right (mono mode) */
    mas_codec_writereg(8, 0x8000);
    
    /* ADC scale 0%, DSP scale 100%
       We use the DSP output for monitoring, because it works with all
       sources including S/PDIF */
    mas_codec_writereg(6, 0x0000);
    mas_codec_writereg(7, 0x4000);

    /* No mute */
    shadow_soft_mute = 0;
    mas_writemem(MAS_BANK_D0, MAS_D0_SOFT_MUTE, &shadow_soft_mute, 1);
    
#ifdef HAVE_SPDIF_OUT
    val = 0x09; /* Disable SDO and SDI, low impedance S/PDIF outputs */
#else
    val = 0x2d; /* Disable SDO and SDI, disable S/PDIF output */
#endif
    mas_writemem(MAS_BANK_D0, MAS_D0_INTERFACE_CONTROL, &val, 1);

    /* Set Demand mode, monitoring OFF and validate all settings */
    shadow_io_control_main = 0x125;
    mas_writemem(MAS_BANK_D0, MAS_D0_IO_CONTROL_MAIN, &shadow_io_control_main, 1);

    /* Start the encoder application */
    val = 0x40;
    mas_writemem(MAS_BANK_D0, MAS_D0_APP_SELECT, &val, 1);
    do
    {
        mas_readmem(MAS_BANK_D0, MAS_D0_APP_RUNNING, &val, 1);
    } while(!(val & 0x40));

    /* We have started the recording application with monitoring OFF.
       This is because we want to record at least one frame to fill the DMA
       buffer, because the silly MAS will not negate EOD until at least one
       DMA transfer has taken place.
       Now let's wait for some data to be encoded. */
    sleep(HZ/5);
    
    /* Now set it to Monitoring mode as default, saves power */
    shadow_io_control_main = 0x525;
    mas_writemem(MAS_BANK_D0, MAS_D0_IO_CONTROL_MAIN, &shadow_io_control_main, 1);

    /* Wait until the DSP has accepted the settings */
    do
    {
        mas_readmem(MAS_BANK_D0, MAS_D0_IO_CONTROL_MAIN, &val,1);
    } while(val & 1);

    drain_dma_buffer();
    mpeg_mode = MPEG_ENCODER;

    DEBUGF("MAS Recording application started\n");

    /* At this point, all settings are the reset MAS defaults, next thing is to
       call mpeg_set_recording_options(). */
}

void audio_record(const char *filename)
{
    mpeg_errno = 0;
    
    strncpy(recording_filename, filename, MAX_PATH - 1);
    recording_filename[MAX_PATH - 1] = 0;

    queue_post(&mpeg_queue, MPEG_RECORD, 0);
}

void audio_pause_recording(void)
{
    queue_post(&mpeg_queue, MPEG_PAUSE_RECORDING, 0);
}

void audio_resume_recording(void)
{
    queue_post(&mpeg_queue, MPEG_RESUME_RECORDING, 0);
}

static void prepend_header(void)
{
    int startpos;
    unsigned i;

    /* Make room for header */
    audiobuf_read -= MPEG_RESERVED_HEADER_SPACE;
    if(audiobuf_read < 0)
    {
        /* Clear the bottom half */
        memset(audiobuf, 0, audiobuf_read + MPEG_RESERVED_HEADER_SPACE);

        /* And the top half */
        audiobuf_read += audiobuflen;
        memset(audiobuf + audiobuf_read, 0, audiobuflen - audiobuf_read);
    }
    else
    {
        memset(audiobuf + audiobuf_read, 0, MPEG_RESERVED_HEADER_SPACE);
    }
    /* Copy the empty ID3 header */
    startpos = audiobuf_read;
    for(i = 0; i < sizeof(empty_id3_header); i++)
    {
        audiobuf[startpos++] = empty_id3_header[i];
        if(startpos == audiobuflen)
            startpos = 0;
    }
}

static void update_header(void)
{
    int fd, framelen;
    unsigned long frames;

    if (last_rec_bytes > 0)
    {
        /* Create the Xing header */
        fd = open(delayed_filename, O_RDWR);
        if (fd < 0)
            panicf("rec upd: %d (%s)", fd, recording_filename);

        frames = frame_count_end - frame_count_start;
        /* If the number of recorded frames has reached 0x7ffff,
           we can no longer trust it */
        if (frame_count_end == 0x7ffff)
            frames = 0;

        /* saved_header is saved right before stopping the MAS */
        framelen = create_xing_header(fd, 0, last_rec_bytes, xing_buffer,
                                      frames, last_rec_time * (1000/HZ),
                                      saved_header, NULL, false);

        lseek(fd, MPEG_RESERVED_HEADER_SPACE - framelen, SEEK_SET);
        write(fd, xing_buffer, framelen);
        close(fd);
    }
}

static void start_prerecording(void)
{
    unsigned long val;

    DEBUGF("Starting prerecording\n");
    
    prerecord_index = 0;
    prerecord_count = 0;
    prerecord_timeout = current_tick + HZ;
    memset(prerecord_buffer, 0, sizeof(prerecord_buffer));
    reset_mp3_buffer();
    
    is_prerecording = true;

    /* Stop monitoring and start the encoder */
    shadow_io_control_main &= ~(1 << 10);
    mas_writemem(MAS_BANK_D0, MAS_D0_IO_CONTROL_MAIN, &shadow_io_control_main, 1);
    DEBUGF("mas_writemem(MAS_BANK_D0, IO_CONTROL_MAIN, %x)\n", shadow_io_control_main);

    /* Wait until the DSP has accepted the settings */
    do
    {
        mas_readmem(MAS_BANK_D0, MAS_D0_IO_CONTROL_MAIN, &val,1);
    } while(val & 1);
    
    is_recording = true;
    saving_status = NOT_SAVING;

    demand_irq_enable(true);
}

static void start_recording(void)
{
    unsigned long val;

    if(is_prerecording)
    {
        /* This will make the IRQ handler start recording
           for real, i.e send MPEG_SAVE_DATA messages when
           the buffer is full */
        is_prerecording = false;
    }
    else
    {
        /* If prerecording is off, we need to stop the monitoring
           and start the encoder */
        shadow_io_control_main &= ~(1 << 10);
        mas_writemem(MAS_BANK_D0, MAS_D0_IO_CONTROL_MAIN, &shadow_io_control_main, 1);
        DEBUGF("mas_writemem(MAS_BANK_D0, IO_CONTROL_MAIN, %x)\n", shadow_io_control_main);

        /* Wait until the DSP has accepted the settings */
        do
        {
            mas_readmem(MAS_BANK_D0, MAS_D0_IO_CONTROL_MAIN, &val,1);
        } while(val & 1);
    }
    
    is_recording = true;
    saving_status = NOT_SAVING;
    paused = false;

    /* Store the current time */
    if(prerecording)
        record_start_time = current_tick - prerecord_count * HZ;
    else
        record_start_time = current_tick;

    pause_start_time = 0;

    demand_irq_enable(true);
}

static void pause_recording(void)
{
    pause_start_time = current_tick;

    /* Set the pause bit */
    shadow_soft_mute |= 2;
    mas_writemem(MAS_BANK_D0, MAS_D0_SOFT_MUTE, &shadow_soft_mute, 1);

    paused = true;
}

static void resume_recording(void)
{
    paused = false;
    
    /* Clear the pause bit */
    shadow_soft_mute &= ~2;
    mas_writemem(MAS_BANK_D0,  MAS_D0_SOFT_MUTE, &shadow_soft_mute, 1);

    /* Compensate for the time we have been paused */
    if(pause_start_time)
    {
       record_start_time =
          current_tick - (pause_start_time - record_start_time);
       pause_start_time = 0;
    }
}

static void stop_recording(void)
{
    unsigned long val;

    /* Let it finish the last frame */
    if(!paused)
       pause_recording();
    sleep(HZ/5);
    
    demand_irq_enable(false);

    is_recording = false;
    is_prerecording = false;

    last_rec_bytes = num_rec_bytes;
    mas_readmem(MAS_BANK_D0, MAS_D0_MPEG_FRAME_COUNT, &frame_count_end, 1);
    last_rec_time = current_tick - record_start_time;

    /* Start monitoring */
    shadow_io_control_main |= (1 << 10);
    mas_writemem(MAS_BANK_D0, MAS_D0_IO_CONTROL_MAIN, &shadow_io_control_main, 1);
    DEBUGF("mas_writemem(MAS_BANK_D0, IO_CONTROL_MAIN, %x)\n", shadow_io_control_main);
    
    /* Wait until the DSP has accepted the settings */
    do
    {
        mas_readmem(MAS_BANK_D0, MAS_D0_IO_CONTROL_MAIN, &val,1);
    } while(val & 1);

    resume_recording();
}

void audio_set_recording_options(struct audio_recording_options *options)
{
    bool is_mpeg1;

    is_mpeg1 = (options->rec_frequency < 3)?true:false;

    rec_version_index = is_mpeg1?3:2;
    rec_frequency_index = options->rec_frequency % 3;

    shadow_encoder_control = (options->rec_quality << 17) |
        (rec_frequency_index << 10) |
        ((is_mpeg1?1:0) << 9) |
        (((options->rec_channels * 2 + 1) & 3) << 6) |
        (1 << 5) /* MS-stereo */ |
        (1 << 2) /* Is an original */;
    mas_writemem(MAS_BANK_D0, MAS_D0_ENCODER_CONTROL, &shadow_encoder_control,1);

    DEBUGF("mas_writemem(MAS_BANK_D0, ENCODER_CONTROL, %x)\n", shadow_encoder_control);

    shadow_soft_mute = options->rec_editable?4:0;
    mas_writemem(MAS_BANK_D0, MAS_D0_SOFT_MUTE, &shadow_soft_mute,1);

    DEBUGF("mas_writemem(MAS_BANK_D0, SOFT_MUTE, %x)\n", shadow_soft_mute);

    shadow_io_control_main = ((1 << 10) | /* Monitoring ON */
        ((options->rec_source < 2)?1:2) << 8) | /* Input select */
        (1 << 5) | /* SDO strobe invert */
        ((is_mpeg1?0:1) << 3) |
        (1 << 2) | /* Inverted SIBC clock signal */
        1; /* Validate */
    mas_writemem(MAS_BANK_D0, MAS_D0_IO_CONTROL_MAIN, &shadow_io_control_main,1);

    DEBUGF("mas_writemem(MAS_BANK_D0, IO_CONTROL_MAIN, %x)\n", shadow_io_control_main);

    if(options->rec_source == AUDIO_SRC_MIC)
    {
        /* Copy left channel to right (mono mode) */
        mas_codec_writereg(8, 0x8000);
    }
    else
    {
        /* Stereo input mode */
        mas_codec_writereg(8, 0);
    }

    prerecording_max_seconds = options->rec_prerecord_time;
    if(prerecording_max_seconds)
    {
        prerecording = true;
        start_prerecording();
    }
    else
    {
        prerecording = false;
        is_prerecording = false;
        is_recording = false;
    }
}

/* If use_mic is true, the left gain is used */
void audio_set_recording_gain(int left, int right, int type)
{
    /* Enable both left and right A/D */
    shadow_codec_reg0 = (left << 12) |
                        (right << 8) |
                        (left << 4) |
                        (type==AUDIO_GAIN_MIC?0x0008:0) | /* Connect left A/D to mic */
                        0x0007;
    mas_codec_writereg(0x0, shadow_codec_reg0);
}

#if CONFIG_TUNER & S1A0903X01
/* Get the (unpitched) MAS PLL frequency, for avoiding FM interference with the
 * Samsung tuner. Zero means unknown. Currently handles recording from analog
 * input only. */
int mpeg_get_mas_pllfreq(void)
{
    if (mpeg_mode != MPEG_ENCODER)
        return 0;

    if (rec_frequency_index == 0)  /* 44.1 kHz / 22.05 kHz */
        return 22579000;
    else
        return 24576000;
}
#endif /* CONFIG_TUNER & S1A0903X01 */

/* try to make some kind of beep, also in recording mode */
void audio_beep(int duration)
{
    long starttick = current_tick;
    do
    {  /* toggle bit 0 of codec register 0, toggling the DAC off & on.
        * While this is still audible even without an external signal,
        * it doesn't affect the (pre-)recording. */
        mas_codec_writereg(0, shadow_codec_reg0 ^ 1);
        mas_codec_writereg(0, shadow_codec_reg0);
        yield();
    }
    while (current_tick - starttick < duration);
}

void audio_new_file(const char *filename)
{
    mpeg_errno = 0;

    strncpy(recording_filename, filename, MAX_PATH - 1);
    recording_filename[MAX_PATH - 1] = 0;

    queue_post(&mpeg_queue, MPEG_NEW_FILE, 0);
}

unsigned long audio_recorded_time(void)
{
    if(is_prerecording)
        return prerecord_count * HZ;
    
    if(is_recording)
    {
       if(paused)
          return pause_start_time - record_start_time;
       else
          return current_tick - record_start_time;
    }

    return 0;
}

unsigned long audio_num_recorded_bytes(void)
{
    int num_bytes;
    int index;
    
    if(is_recording)
    {
        if(is_prerecording)
        {
            index = prerecord_index - prerecord_count;
            if(index < 0)
                index += prerecording_max_seconds;
            
            num_bytes = audiobuf_write - prerecord_buffer[index].mempos;
            if(num_bytes < 0)
                num_bytes += audiobuflen;
            
            return num_bytes;;
        }
        else
            return num_rec_bytes;
    }
    else
        return 0;
}

#else /* SIMULATOR */

/* dummies coming up */

void audio_init_playback(void)
{
    /* a dummy */
}
unsigned long audio_recorded_time(void)
{
    /* a dummy */
    return 0;
}
void audio_beep(int duration)
{
    /* a dummy */
    (void)duration;
}
void audio_pause_recording(void)
{
    /* a dummy */
}
void audio_resume_recording(void)
{
    /* a dummy */
}
unsigned long audio_num_recorded_bytes(void)
{
    /* a dummy */
    return 0;
}
void audio_record(const char *filename)
{
    /* a dummy */
    (void)filename;
}
void audio_new_file(const char *filename)
{
    /* a dummy */
    (void)filename;
}

void audio_set_recording_gain(int left, int right, int type)
{
    /* a dummy */
    (void)left;
    (void)right;
    (void)type;
}
void audio_init_recording(unsigned int buffer_offset)
{
    /* a dummy */
    (void)buffer_offset;
}
void audio_set_recording_options(struct audio_recording_options *options)
{
    /* a dummy */
    (void)options;
}
#endif /* SIMULATOR */
#endif /* CONFIG_CODEC == MAS3587F */

void audio_play(long offset)
{
#ifdef SIMULATOR
    char* trackname;
    int steps=0;

    is_playing = true;
    
    do {
        trackname = playlist_peek( steps );
        if (!trackname)
            break;
        if(mp3info(&taginfo, trackname)) {
            /* bad mp3, move on */
            if(++steps > playlist_amount())
                break;
            continue;
        }
#ifdef HAVE_MPEG_PLAY
        real_mpeg_play(trackname);
#endif
        playlist_next(steps);
        taginfo.offset = offset;
        set_elapsed(&taginfo);
        is_playing = true;
        playing = true;
        break;
    } while(1);
#else /* !SIMULATOR */
    is_playing = true;

    queue_post(&mpeg_queue, MPEG_PLAY, offset);
#endif /* !SIMULATOR */

    mpeg_errno = 0;
}

void audio_stop(void)
{
#ifndef SIMULATOR
    if (playing)
    {
        struct trackdata *track = get_trackdata(0);
        prev_track_elapsed = track->id3.elapsed;
    }
    mpeg_stop_done = false;
    queue_post(&mpeg_queue, MPEG_STOP, 0);
    while(!mpeg_stop_done)
        yield();
#else /* SIMULATOR */
    paused = false;
    is_playing = false;
    playing = false;
#endif /* SIMULATOR */
}

/* dummy */
void audio_stop_recording(void)
{
    audio_stop(); 
}

void audio_pause(void)
{
#ifndef SIMULATOR
    queue_post(&mpeg_queue, MPEG_PAUSE, 0);
#else /* SIMULATOR */
    is_playing = true;
    playing = false;
    paused = true;
#endif /* SIMULATOR */
}

void audio_resume(void)
{
#ifndef SIMULATOR
    queue_post(&mpeg_queue, MPEG_RESUME, 0);
#else /* SIMULATOR */
    is_playing = true;
    playing = true;
    paused = false;
#endif /* SIMULATOR */
}

void audio_next(void)
{
#ifndef SIMULATOR
    queue_remove_from_head(&mpeg_queue, MPEG_NEED_DATA);
    queue_post(&mpeg_queue, MPEG_NEXT, 0);
#else /* SIMULATOR */
    char* file;
    int steps = 1;
    int index;

    do {
        file = playlist_peek(steps);
        if(!file)
            break;
        if(mp3info(&taginfo, file)) {
            if(++steps > playlist_amount())
                break;
            continue;
        }
        index = playlist_next(steps);
        taginfo.index = index;
        current_track_counter++;
        is_playing = true;
        playing = true;
        break;
    } while(1);
#endif /* SIMULATOR */
}

void audio_prev(void)
{
#ifndef SIMULATOR
    queue_remove_from_head(&mpeg_queue, MPEG_NEED_DATA);
    queue_post(&mpeg_queue, MPEG_PREV, 0);
#else /* SIMULATOR */
    char* file;
    int steps = -1;
    int index;

    do {
        file = playlist_peek(steps);
        if(!file)
            break;
        if(mp3info(&taginfo, file)) {
            steps--;
            continue;
        }
        index = playlist_next(steps);
        taginfo.index = index;
        current_track_counter++;
        is_playing = true;
        playing = true;
        break;
    } while(1);
#endif /* SIMULATOR */
}

void audio_ff_rewind(long newtime)
{
#ifndef SIMULATOR
    queue_post(&mpeg_queue, MPEG_FF_REWIND, newtime);
#else /* SIMULATOR */
    (void)newtime;
#endif /* SIMULATOR */
}

void audio_flush_and_reload_tracks(void)
{
#ifndef SIMULATOR
    queue_post(&mpeg_queue, MPEG_FLUSH_RELOAD, 0);
#endif /* !SIMULATOR*/
}

int audio_status(void)
{
    int ret = 0;

    if(is_playing)
        ret |= AUDIO_STATUS_PLAY;

    if(paused)
        ret |= AUDIO_STATUS_PAUSE;
    
#if (CONFIG_CODEC == MAS3587F) && !defined(SIMULATOR)
    if(is_recording && !is_prerecording)
        ret |= AUDIO_STATUS_RECORD;

    if(is_prerecording)
        ret |= AUDIO_STATUS_PRERECORD;
#endif /* CONFIG_CODEC == MAS3587F */

    if(mpeg_errno)
        ret |= AUDIO_STATUS_ERROR;
    
    return ret;
}

unsigned int audio_error(void)
{
    return mpeg_errno;
}

void audio_error_clear(void)
{
    mpeg_errno = 0;
}

#ifdef SIMULATOR
static void mpeg_thread(void)
{
    struct mp3entry* id3;
    while ( 1 ) {
        if (is_playing) {
            id3 = audio_current_track();
            if (!paused)
            {
                id3->elapsed+=1000;
                id3->offset+=1000;
            }
            if (id3->elapsed>=id3->length)
                audio_next();
        }
        sleep(HZ);
    }
}
#endif /* SIMULATOR */

void audio_init(void)
{
    mpeg_errno = 0;

#ifndef SIMULATOR
    audiobuflen = audiobufend - audiobuf;
    queue_init(&mpeg_queue, true);
#endif /* !SIMULATOR */
    create_thread(mpeg_thread, mpeg_stack,
                  sizeof(mpeg_stack), 0, mpeg_thread_name
                  IF_PRIO(, PRIORITY_SYSTEM)
		          IF_COP(, CPU));

    memset(trackdata, sizeof(trackdata), 0);

#if (CONFIG_CODEC == MAS3587F) && !defined(SIMULATOR)
    if (HW_MASK & PR_ACTIVE_HIGH)
        and_b(~0x08, &PADRH);
    else
        or_b(0x08, &PADRH);
#endif /* CONFIG_CODEC == MAS3587F */

#ifdef DEBUG
    dbg_timer_start();
    dbg_cnt2us(0);
#endif /* DEBUG */
}

#endif /* CONFIG_CODEC != SWCODEC */
