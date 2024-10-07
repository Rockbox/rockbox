/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2005-2007 Miika Pekkarinen
 * Copyright (C) 2007-2008 Nicolas Pennequin
 * Copyright (C) 2011      Michael Sevakis
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
#include "config.h"
#include "system.h"
#include "kernel.h"
#include "panic.h"
#include "core_alloc.h"
#include "sound.h"
#include "codecs.h"
#include "codec_thread.h"
#include "voice_thread.h"
#include "metadata.h"
#include "cuesheet.h"
#include "buffering.h"
#include "talk.h"
#include "playlist.h"
#include "abrepeat.h"
#ifdef HAVE_PLAY_FREQ
#include "pcm_mixer.h"
#endif
#include "pcmbuf.h"
#include "audio_thread.h"
#include "playback.h"
#include "storage.h"
#include "misc.h"
#include "settings.h"
#include "audiohw.h"

#ifdef HAVE_TAGCACHE
#include "tagcache.h"
#endif

#ifdef HAVE_ALBUMART
#include "albumart.h"
#endif

#ifdef HAVE_PLAY_FREQ
#include "pcm_mixer.h"
#endif

#if defined(SIMULATOR) || defined(SDLAPP)
#include <strings.h>  /* For strncasecmp() */
#endif

/* TODO: The audio thread really is doing multitasking of acting like a
         consumer and producer of tracks. It may be advantageous to better
         logically separate the two functions. I won't go that far just yet. */

/* Internal support for voice playback */
#define PLAYBACK_VOICE

#if CONFIG_PLATFORM & PLATFORM_NATIVE
/* Application builds don't support direct code loading */
#define HAVE_CODEC_BUFFERING
#endif

/* Amount of guess-space to allow for codecs that must hunt and peck
 * for their correct seek target, 32k seems a good size */
#define AUDIO_REBUFFER_GUESS_SIZE    (1024*32)

/* Define LOGF_ENABLE to enable logf output in this file */
#if 0
#define LOGF_ENABLE
#endif
#include "logf.h"

/* Macros to enable logf for queues
   logging on SYS_TIMEOUT can be disabled */
#ifdef SIMULATOR
/* Define this for logf output of all queuing except SYS_TIMEOUT */
#define PLAYBACK_LOGQUEUES
/* Define this to logf SYS_TIMEOUT messages */
/*#define PLAYBACK_LOGQUEUES_SYS_TIMEOUT*/
#endif

#ifdef PLAYBACK_LOGQUEUES
#define LOGFQUEUE logf
#else
#define LOGFQUEUE(...)
#endif

#ifdef PLAYBACK_LOGQUEUES_SYS_TIMEOUT
#define LOGFQUEUE_SYS_TIMEOUT logf
#else
#define LOGFQUEUE_SYS_TIMEOUT(...)
#endif

/* Variables are commented with the threads that use them:
 * A=audio, C=codec, O=other. A suffix of "-" indicates that the variable is
 * read but not updated on that thread. Audio is the only user unless otherwise
 * specified.
 */

/** Miscellaneous **/
extern unsigned int audio_thread_id;   /* from audio_thread.c */
extern struct event_queue audio_queue; /* from audio_thread.c */
extern bool audio_is_initialized;      /* from audio_thread.c */
extern struct codec_api ci;            /* from codecs.c */

/** Possible arrangements of the main buffer **/
static enum audio_buffer_state
{
    AUDIOBUF_STATE_TRASHED     = -1,     /* trashed; must be reset */
    AUDIOBUF_STATE_INITIALIZED =  0,     /* voice+audio OR audio-only */
} buffer_state = AUDIOBUF_STATE_TRASHED; /* (A,O) */

/** Main state control **/
static bool ff_rw_mode SHAREDBSS_ATTR = false; /* Pre-ff-rewind mode (A,O-) */

static enum play_status
{
    PLAY_STOPPED = 0,
    PLAY_PLAYING = AUDIO_STATUS_PLAY,
    PLAY_PAUSED  = AUDIO_STATUS_PLAY | AUDIO_STATUS_PAUSE,
} play_status = PLAY_STOPPED;

/* Sizeable things that only need exist during playback and not when stopped */
static struct audio_scratch_memory
{
    struct mp3entry codec_id3; /* (A,C) */
    struct mp3entry unbuffered_id3;
    struct cuesheet *curr_cue; /* Will follow this structure */
} * audio_scratch_memory = NULL;

/* These are used to store the current, next and optionally the peek-ahead
 * mp3entry's - this guarantees that the pointer returned by audio_current/
 * next_track will be valid for the full duration of the currently playing
 * track */
enum audio_id3_types
{
    /* These are allocated statically */
    PLAYING_ID3 = 0,
    NEXTTRACK_ID3,
#ifdef AUDIO_FAST_SKIP_PREVIEW
    /* The real playing metadata must has to be protected since it contains
       critical info for other features */
    PLAYING_PEEK_ID3,
#endif
    ID3_TYPE_NUM_STATIC,
    /* These go in the scratch memory */
    UNBUFFERED_ID3 = ID3_TYPE_NUM_STATIC,
    CODEC_ID3,
};
static struct mp3entry static_id3_entries[ID3_TYPE_NUM_STATIC]; /* (A,O) */

struct audio_resume_info
{
    unsigned long elapsed;
    unsigned long offset;
};

/* Peeking functions can yield and mess us up */
static struct mutex id3_mutex SHAREDBSS_ATTR; /* (A,O)*/

/** For album art support **/
#define MAX_MULTIPLE_AA SKINNABLE_SCREENS_COUNT
#ifdef HAVE_ALBUMART

static int albumart_mode = -1;

static struct albumart_slot
{
    struct dim dim;     /* Holds width, height of the albumart */
    int used;           /* Counter; increments if something uses it */
} albumart_slots[MAX_MULTIPLE_AA]; /* (A,O) */

static char last_folder_aa_path[MAX_PATH] = "\0";
static int last_folder_aa_hid[MAX_MULTIPLE_AA] = {0};

#define FOREACH_ALBUMART(i) for (int i = 0; i < MAX_MULTIPLE_AA; i++)
#endif /* HAVE_ALBUMART */


/** Information used for tracking buffer fills **/

/* Buffer and thread state tracking */
static enum filling_state
{
    STATE_IDLE = 0, /* audio is stopped: nothing to do */
    STATE_FILLING,  /* adding tracks to the buffer */
    STATE_FULL,     /* can't add any more tracks */
    STATE_END_OF_PLAYLIST, /* all remaining tracks have been added */
    STATE_FINISHED, /* all remaining tracks are fully buffered */
    STATE_ENDING,   /* audio playback is ending */
    STATE_ENDED,    /* audio playback is done */
    STATE_STOPPED,  /* buffering is stopped explicitly */
} filling = STATE_IDLE;

/* Track info - holds information about each track in the buffer */
#ifdef HAVE_ALBUMART
#define TRACK_INFO_AA       MAX_MULTIPLE_AA
#else
#define TRACK_INFO_AA       0
#endif

#ifdef HAVE_CODEC_BUFFERING
#define TRACK_INFO_CODEC    1
#else
#define TRACK_INFO_CODEC    0
#endif

#define TRACK_INFO_HANDLES  (3 + TRACK_INFO_AA + TRACK_INFO_CODEC)

struct track_info
{
    int self_hid;                   /* handle for the info on buffer */

    /* In per-track allocated order: */
    union {
    int handle[TRACK_INFO_HANDLES]; /* array mirror for efficient wipe/close */
    struct {
    int id3_hid;                    /* Metadata handle ID */
    int cuesheet_hid;               /* Parsed cuesheet handle ID */
#ifdef HAVE_ALBUMART
    int aa_hid[MAX_MULTIPLE_AA];    /* Album art handle IDs */
#endif
#ifdef HAVE_CODEC_BUFFERING
    int codec_hid;                  /* Buffered codec handle ID */
#endif
    int audio_hid;                  /* Main audio data handle ID */
    }; };
};

/* On-buffer info format; includes links */
struct track_buf_info
{
    int               link[2];   /* prev/next handles */
    struct track_info info;
};

#define FOR_EACH_TRACK_INFO_HANDLE(i) \
    for (int i = 0; i < TRACK_INFO_HANDLES; i++)

static struct
{
    /* TODO: perhaps cache -1/+1 delta handles if speed ever matters much
       because those lookups are common; also could cache a few recent
       acccesses */
    int first_hid;          /* handle of first track in list */
    int current_hid;        /* handle of track delta 0 */
    int last_hid;           /* handle of last track in list */
    int in_progress_hid;    /* track in process of loading */
    unsigned int count;     /* number of tracks in list */
} track_list; /* (A, O-) */


/* Playlist steps from playlist position to next track to be buffered */
static int playlist_peek_offset = 0;

#ifdef HAVE_DISK_STORAGE
/* Buffer margin A.K.A. anti-skip buffer (in seconds) */
static size_t buffer_margin = 5;
#endif

/* Values returned for track loading */
enum track_load_status
{
    LOAD_TRACK_ERR_START_CODEC   = -6,
    LOAD_TRACK_ERR_FINISH_FAILED = -5,
    LOAD_TRACK_ERR_FINISH_FULL   = -4,
    LOAD_TRACK_ERR_BUSY          = -3,
    LOAD_TRACK_ERR_NO_MORE       = -2,
    LOAD_TRACK_ERR_FAILED        = -1,
    LOAD_TRACK_OK                =  0,
    LOAD_TRACK_READY             =  1,
};

/** Track change controls **/

/* What sort of skip is pending globally? */
static enum track_skip_type
{
    /* Relative to what user is intended to see: */
    /* Codec: +0, Track List: +0, Playlist: +0 */
    TRACK_SKIP_NONE = 0,          /* no track skip */
    /* Codec: +1, Track List: +1, Playlist: +0 */
    TRACK_SKIP_AUTO,              /* codec-initiated skip */
    /* Codec: +1, Track List: +1, Playlist: +1 */
    TRACK_SKIP_AUTO_NEW_PLAYLIST, /* codec-initiated skip is new playlist */
    /* Codec: xx, Track List: +0, Playlist: +0 */
    TRACK_SKIP_AUTO_END_PLAYLIST, /* codec-initiated end of playlist */
    /* Manual skip: Never pends */
    TRACK_SKIP_MANUAL,            /* manual track skip */
    /* Manual skip: Never pends */
    TRACK_SKIP_DIR_CHANGE,        /* manual directory skip */
} skip_pending = TRACK_SKIP_NONE;

/* Note about TRACK_SKIP_AUTO_NEW_PLAYLIST:
   Fixing playlist code to be able to peek into the first song of
   the next playlist would fix any issues and this wouldn't need
   to be a special case since pre-advancing the playlist would be
   unneeded - it could be much more like TRACK_SKIP_AUTO and all
   actions that require reversal during an in-progress transition
   would work as expected */

/* Used to indicate status for the events. Must be separate to satisfy all
   clients so the correct metadata is read when sending the change events. */
static unsigned int track_event_flags = TEF_NONE; /* (A, O-) */

/* Pending manual track skip offset */
static int skip_offset = 0; /* (A, O) */

static bool track_skip_is_manual = false;

/* Track change notification */
static struct
{
    unsigned int in;  /* Number of pcmbuf posts (audio isr) */
    unsigned int out; /* Number of times audio has read the difference */
} track_change = { 0, 0 };

/** Codec status **/
/* Did the codec notify us it finished while we were paused or while still
   in an automatic transition?

   If paused, it is necessary to defer a codec-initiated skip until resuming
   or else the track will move forward while not playing audio!

   If in-progress, skips should not build-up ahead of where the WPS is when
   really short tracks finish decoding.

   If it is forgotten, it will be missed altogether and playback will just sit
   there looking stupid and comatose until the user does something */
static bool codec_skip_pending = false;
static int  codec_skip_status;
static bool codec_seeking = false;          /* Codec seeking ack expected? */
static unsigned int position_key = 0;

/* Forward declarations */
enum audio_start_playback_flags
{
    AUDIO_START_RESTART = 0x1, /* "Restart" playback (flush _all_ tracks) */
    AUDIO_START_NEWBUF  = 0x2, /* Mark the audiobuffer as invalid */
};

static void audio_start_playback(const struct audio_resume_info *resume_info,
                                 unsigned int flags);
static void audio_stop_playback(void);
static void buffer_event_buffer_low_callback(unsigned short id, void *data, void *user_data);
static void buffer_event_rebuffer_callback(unsigned short id, void *data);
static void buffer_event_finished_callback(unsigned short id, void *data);
void audio_pcmbuf_sync_position(void);


/**************************************/

/** --- MP3Entry --- **/

/* Does the mp3entry have enough info for us to use it? */
static struct mp3entry * valid_mp3entry(const struct mp3entry *id3)
{
    return id3 && (id3->length != 0 || id3->filesize != 0) &&
           id3->codectype != AFMT_UNKNOWN ? (struct mp3entry *)id3 : NULL;
}

/* Return a pointer to an mp3entry on the buffer, as it is */
static struct mp3entry * bufgetid3(int handle_id)
{
    if (handle_id < 0)
        return NULL;

    struct mp3entry *id3;
    ssize_t ret = bufgetdata(handle_id, 0, (void *)&id3);

    if (ret != sizeof(struct mp3entry))
        return NULL;

    return id3;
}

/* Read an mp3entry from the buffer, adjusted */
static bool bufreadid3(int handle_id, struct mp3entry *id3out)
{
    struct mp3entry *id3 = bufgetid3(handle_id);

    if (id3)
    {
        copy_mp3entry(id3out, id3);
        return true;
    }

    return false;
}

/* Lock the id3 mutex */
static void id3_mutex_lock(void)
{
    mutex_lock(&id3_mutex);
}

/* Unlock the id3 mutex */
static void id3_mutex_unlock(void)
{
    mutex_unlock(&id3_mutex);
}

/* Return one of the collection of mp3entry pointers - collect them all here */
static inline struct mp3entry * id3_get(enum audio_id3_types id3_num)
{
    switch (id3_num)
    {
    case UNBUFFERED_ID3:
        return &audio_scratch_memory->unbuffered_id3;
    case CODEC_ID3:
        return &audio_scratch_memory->codec_id3;
    default:
        return &static_id3_entries[id3_num];
    }
}

/* Copy an mp3entry into one of the mp3 entries */
static void id3_write(enum audio_id3_types id3_num,
                      const struct mp3entry *id3_src)
{
    struct mp3entry *dest_id3 = id3_get(id3_num);

    if (id3_src)
        copy_mp3entry(dest_id3, id3_src);
    else
        wipe_mp3entry(dest_id3);
}

/* Call id3_write "safely" because peek aheads can yield, even if the fast
   preview isn't enabled */
static void id3_write_locked(enum audio_id3_types id3_num,
                             const struct mp3entry *id3_src)
{
    id3_mutex_lock();
    id3_write(id3_num, id3_src);
    id3_mutex_unlock();
}


/** --- Track info --- **/

#ifdef HAVE_CODEC_BUFFERING
static void track_info_close_handle(int *hidp)
{
    bufclose(*hidp);
    *hidp = ERR_HANDLE_NOT_FOUND;
}
#endif /* HAVE_CODEC_BUFFERING */

/* Invalidate all members to initial values - does not close handles or sync */
static void track_info_wipe(struct track_info *infop)
{
    /* don't touch ->self_hid */

    FOR_EACH_TRACK_INFO_HANDLE(i)
        infop->handle[i] = ERR_HANDLE_NOT_FOUND;
}

/** --- Track list --- **/

/* Clear tracks in the list, optionally preserving the current track -
   returns 'false' if the operation was changed */
enum track_clear_action
{
    TRACK_LIST_CLEAR_ALL = 0, /* Clear all tracks */
    TRACK_LIST_KEEP_CURRENT,  /* Keep current only; clear before + after */
    TRACK_LIST_KEEP_NEW       /* Keep current and those that follow */
};

/* Initialize the track list */
static void INIT_ATTR track_list_init(void)
{
    track_list.first_hid = 0;
    track_list.current_hid = 0;
    track_list.last_hid = 0;
    track_list.in_progress_hid = 0;
    track_list.count = 0;
}

/* Return number of items allocated in the list */
static inline unsigned int track_list_count(void)
{
    return track_list.count;
}

/* Return true if the list is empty */
static inline bool track_list_empty(void)
{
    return track_list.count == 0;
}

/* Returns a pointer to the track info data on the buffer */
static struct track_buf_info * track_buf_info_get(int hid)
{
    void *p;
    ssize_t size = bufgetdata(hid, sizeof (struct track_buf_info), &p);
    return size == (ssize_t)sizeof (struct track_buf_info) ? p : NULL;
}

/* Synchronize the buffer object with the cached track info */
static bool track_info_sync(const struct track_info *infop)
{
    struct track_buf_info *tbip = track_buf_info_get(infop->self_hid);
    if (!tbip)
        return false;

    tbip->info = *infop;
    return true;
}

/* Return track info a given offset from the info referenced by hid and
 * place a copy into *infop, if provided */
static struct track_buf_info *
    track_list_get_info_from(int hid, int offset, struct track_info *infop)
{
    int sgn = SGN(offset);
    struct track_buf_info *tbip;

    while (1)
    {
        if (!(tbip = track_buf_info_get(hid)))
            break;

        if (!offset)
            break;

        if ((hid = tbip->link[(unsigned)(sgn + 1) / 2]) <= 0)
        {
            tbip = NULL;
            break;
        }

        offset -= sgn;
    }

    if (infop)
    {
        if (tbip)
        {
            *infop = tbip->info;
        }
        else
        {
            track_info_wipe(infop);
            infop->self_hid = ERR_HANDLE_NOT_FOUND;
        }
    }

    return tbip;
}

/* Commit the track info to the buffer updated with the provided source info */
static bool track_list_commit_buf_info(struct track_buf_info *tbip,
                                       const struct track_info *src_infop)
{
    /* Leaves the list unmodified if anything fails */
    if (tbip->link[1] != ERR_HANDLE_NOT_FOUND)
        return false;

    int hid = tbip->info.self_hid;
    int last_hid = track_list.last_hid;
    struct track_buf_info *last_tbip = NULL;

    if (last_hid > 0 && !(last_tbip = track_buf_info_get(last_hid)))
        return false;

    tbip->info = *src_infop;

    /* Insert last */
    tbip->link[0] = last_hid;
    tbip->link[1] = 0; /* "In list" */

    if (last_tbip)
    {
        last_tbip->link[1] = hid;
    }
    else
    {
        track_list.first_hid   = hid;
        track_list.current_hid = hid;
    }

    track_list.last_hid = hid;
    track_list.count++;
    return true;
}

#ifdef HAVE_ALBUMART
static inline void clear_cached_aa_handles(int* aa_handles)
{
    if (last_folder_aa_path[0] == 0)
        return;

    FOREACH_ALBUMART(i)
    {
        if (aa_handles[i] == last_folder_aa_hid[i])
            aa_handles[i] = 0;
    }
}
#endif //HAVE_ALBUMART

/* Free the track buffer entry and possibly remove it from the list if it
   was succesfully added at some point */
static void track_list_free_buf_info(struct track_buf_info *tbip)
{
    int hid = tbip->info.self_hid;
    int next_hid = tbip->link[1];

    if (next_hid != ERR_HANDLE_NOT_FOUND)
    {
        int prev_hid = tbip->link[0];
        struct track_buf_info *prev_tbip = NULL;
        struct track_buf_info *next_tbip = NULL;

        if ((prev_hid > 0 && !(prev_tbip = track_buf_info_get(prev_hid))) ||
            (next_hid > 0 && !(next_tbip = track_buf_info_get(next_hid))))
        {
            return;
        }

        /* If this one is the current track; new current track is next one,
           if any, else, the previous */
        if (hid == track_list.current_hid)
            track_list.current_hid = next_hid > 0 ? next_hid : prev_hid;

        /* Fixup list links */
        if (prev_tbip)
            prev_tbip->link[1] = next_hid;
        else
            track_list.first_hid = next_hid;

        if (next_tbip)
            next_tbip->link[0] = prev_hid;
        else
            track_list.last_hid = prev_hid;

        track_list.count--;
    }

    /* No movement allowed during bufclose calls */
    buf_pin_handle(hid, true);

#ifdef HAVE_ALBUMART
    clear_cached_aa_handles(tbip->info.aa_hid);
#endif
    FOR_EACH_TRACK_INFO_HANDLE(i)
        bufclose(tbip->info.handle[i]);

    /* Finally, the handle itself */
    bufclose(hid);
}

/* Return current track plus an offset */
static bool track_list_current(int offset, struct track_info *infop)
{
    return !!track_list_get_info_from(track_list.current_hid, offset, infop);
}

/* Return current based upon what's intended that the user sees - not
   necessarily where decoding is taking place */
static bool track_list_user_current(int offset, struct track_info *infop)
{
    if (skip_pending == TRACK_SKIP_AUTO ||
        skip_pending == TRACK_SKIP_AUTO_NEW_PLAYLIST)
    {
        offset--;
    }

    return !!track_list_get_info_from(track_list.current_hid, offset, infop);
}

/* Advance current track by an offset, return false if result is out of
   bounds */
static bool track_list_advance_current(int offset, struct track_info *infop)
{
    struct track_buf_info *new_bufinfop =
        track_list_get_info_from(track_list.current_hid, offset, infop);

    if (!new_bufinfop)
        return false;

    track_list.current_hid = new_bufinfop->info.self_hid;
    return true;
}

/* Return the info of the last allocation plus an offset, NULL if result is
   out of bounds */
static bool track_list_last(int offset, struct track_info *infop)
{
    return !!track_list_get_info_from(track_list.last_hid, offset, infop);
}

/* Allocate a new struct track_info on the buffer; does not add to list */
static bool track_list_alloc_info(struct track_info *infop)
{
    int hid = bufalloc(NULL, sizeof (struct track_buf_info), TYPE_RAW_ATOMIC);

    track_info_wipe(infop);

    struct track_buf_info *tbip = track_buf_info_get(hid);
    if (!tbip)
    {
        infop->self_hid = ERR_HANDLE_NOT_FOUND;
        bufclose(hid);
        return false;
    }

    infop->self_hid = hid;

    tbip->link[0] = 0;
    tbip->link[1] = ERR_HANDLE_NOT_FOUND; /* "Not in list" */
    tbip->info.self_hid = hid;
    track_info_wipe(&tbip->info);

    return true;
}

/* Actually commit the track info to the track list */
static bool track_list_commit_info(const struct track_info *infop)
{
    struct track_buf_info *tbip = track_buf_info_get(infop->self_hid);
    if (!tbip)
        return false;

    return track_list_commit_buf_info(tbip, infop);
}

/* Free the track entry and possibly remove it from the list if it was
   succesfully added at some point */
static void track_list_free_info(struct track_info *infop)
{
    struct track_buf_info *tbip = track_buf_info_get(infop->self_hid);
    if (!tbip)
        return;

    track_list_free_buf_info(tbip);
}

/* Close all open handles in the range except the for the current track
   if preserving that */
static void track_list_clear(unsigned int action)
{
    logf("%s:action=%u", __func__, action);

    /* Don't care now since rebuffering is imminent */
    buf_set_watermark(0);

    if (action != TRACK_LIST_CLEAR_ALL)
    {
        struct track_info info;
        if (!track_list_current(0, &info) || info.id3_hid < 0)
            action = TRACK_LIST_CLEAR_ALL; /* Nothing worthwhile keeping */
    }

    int hid         = track_list.first_hid;
    int current_hid = track_list.current_hid;
    int last_hid    = action == TRACK_LIST_KEEP_NEW ? current_hid : 0;

    while (hid != last_hid)
    {
        struct track_buf_info *tbip = track_buf_info_get(hid);
        if (!tbip)
            break;

        int next_hid = tbip->link[1];

        if (action != TRACK_LIST_KEEP_CURRENT || hid != current_hid)
        {
            /* If this is the in-progress load, abort it */
            if (hid == track_list.in_progress_hid)
                track_list.in_progress_hid = 0;

            track_list_free_buf_info(tbip);
        }

        hid = next_hid;
    }
}


/** --- Audio buffer -- **/

/* What size is needed for the scratch buffer? */
static size_t scratch_mem_size(void)
{
    size_t size = sizeof (struct audio_scratch_memory);

    if (global_settings.cuesheet)
        size += sizeof (struct cuesheet);

    return size;
}

/* Initialize the memory area where data is stored that is only used when
   playing audio and anything depending upon it */
static void scratch_mem_init(void *mem)
{
    audio_scratch_memory = (struct audio_scratch_memory *)mem;
    id3_write_locked(UNBUFFERED_ID3, NULL);
    id3_write(CODEC_ID3, NULL);
    ci.id3 = id3_get(CODEC_ID3);
    audio_scratch_memory->curr_cue = NULL;

    if (global_settings.cuesheet)
    {
        audio_scratch_memory->curr_cue =
            SKIPBYTES((struct cuesheet *)audio_scratch_memory,
                      sizeof (struct audio_scratch_memory));
    }
}

static int audiobuf_handle;
#define AUDIO_BUFFER_RESERVE (256*1024)
static size_t filebuflen;


size_t audio_buffer_size(void)
{
    if (audiobuf_handle > 0)
        return filebuflen - AUDIO_BUFFER_RESERVE;
    return 0;
}

size_t audio_buffer_available(void)
{
    size_t size = 0;
    size_t core_size = core_available();
    if (audiobuf_handle > 0) /* if allocated return what we can give */
        size = filebuflen - AUDIO_BUFFER_RESERVE - 128;
    return MAX(core_size, size);
}

#ifdef HAVE_ALBUMART
static void clear_last_folder_album_art(void)
{
    if(last_folder_aa_path[0] == 0)
        return;

    last_folder_aa_path[0] = 0;
    FOREACH_ALBUMART(i)
    {
        bufclose(last_folder_aa_hid[i]);
        last_folder_aa_hid[i] = 0;
    }
}
#endif

/* Set up the audio buffer for playback
 * filebuflen must be pre-initialized with the maximum size */
static void audio_reset_buffer_noalloc(
    void *filebuf)
{
    /*
     * Layout audio buffer as follows:
     * [|SCRATCH|BUFFERING|PCM]
     */
    logf("%s()", __func__);

    /* If the setup of anything allocated before the file buffer is
       changed, do check the adjustments after the buffer_alloc call
       as it will likely be affected and need sliding over */
    size_t allocsize;
    /* Subtract whatever the pcm buffer says it used plus the guard
       buffer */
    allocsize = pcmbuf_init(filebuf + filebuflen);

    /* Make sure filebuflen is a pointer sized multiple after
       adjustment */
    allocsize = ALIGN_UP(allocsize, sizeof (intptr_t));
    if (allocsize > filebuflen)
        goto bufpanic;

    filebuflen -= allocsize;

    /* Scratch memory */
    allocsize = scratch_mem_size();
    if (allocsize > filebuflen)
        goto bufpanic;

    scratch_mem_init(filebuf);
    filebuf += allocsize;
    filebuflen -= allocsize;

#ifdef HAVE_ALBUMART
    clear_last_folder_album_art();
#endif

    buffering_reset(filebuf, filebuflen);

    buffer_state = AUDIOBUF_STATE_INITIALIZED;

#if defined(ROCKBOX_HAS_LOGF) && defined(LOGF_ENABLE)
    /* Make sure everything adds up - yes, some info is a bit redundant but
       aids viewing and the summation of certain variables should add up to
       the location of others. */
    {
        logf("fbuf:   %08X", (unsigned)filebuf);
        logf("fbufe:  %08X", (unsigned)(filebuf + filebuflen));
        logf("sbuf:   %08X", (unsigned)audio_scratch_memory);
        logf("sbufe:  %08X", (unsigned)(audio_scratch_memory + allocsize));
    }
#endif

    return;

bufpanic:
    panicf("%s(): EOM (%zu > %zu)", __func__, allocsize, filebuflen);
}

/* Buffer must not move. */
static int shrink_callback(int handle, unsigned hints, void* start, size_t old_size)
{
    struct queue_event ev;
    static const long filter_list[][2] =
    {
        /* codec messages */
        { Q_AUDIO_PLAY, Q_AUDIO_PLAY },
    };

    static struct audio_resume_info resume;

    bool give_up = false;

    /* filebuflen is, at this point, the buffering.c buffer size,
     * i.e. the audiobuf except voice, scratch mem, pcm, ... */
    ssize_t extradata_size = old_size - filebuflen;
    /* check what buflib requests */
    size_t wanted_size = (hints & BUFLIB_SHRINK_SIZE_MASK);
    ssize_t size = (ssize_t)old_size - wanted_size;

    if ((size - extradata_size) < AUDIO_BUFFER_RESERVE)
    {
        /* check if buflib needs the memory really hard. if yes we give
         * up playback for now, otherwise refuse to shrink to keep at least
         * 256K for the buffering */
        if ((hints & BUFLIB_SHRINK_POS_MASK) == BUFLIB_SHRINK_POS_MASK)
            give_up = true;
        else
            return BUFLIB_CB_CANNOT_SHRINK;
    }


    /* TODO: Do it without stopping playback, if possible */
    struct mp3entry *id3 = audio_current_track();
    unsigned long elapsed = id3->elapsed;
    unsigned long offset = id3->offset;
    /* resume if playing */
    bool playing = (audio_status() == AUDIO_STATUS_PLAY);
    /* There's one problem with stoping and resuming: If it happens in a too
     * frequent fashion, the codecs lose the resume postion and playback
     * begins from the beginning.
     * To work around use queue_post() to effectively delay the resume in case
     * we're called another time. However this has another problem: id3->offset
     * gets zero since playback is stopped. Therefore, try to peek at the
     * queue_post from the last call to get the correct offset. This also
     * lets us conviniently remove the queue event so Q_AUDIO_PLAY is only
     * processed once. */
    bool play_queued = queue_peek_ex(&audio_queue, &ev, QPEEK_REMOVE_EVENTS,
                                     filter_list);

    if (playing && (elapsed > 0 || offset > 0))
    {
        if (play_queued)
            resume = *(struct audio_resume_info *)ev.data;

        /* current id3->elapsed/offset are king */
        if (elapsed > 0)
            resume.elapsed = elapsed;

        if (offset > 0)
            resume.offset = offset;
    }

    /* don't call audio_hard_stop() as it frees this handle */
    if (thread_self() == audio_thread_id)
    {   /* inline case Q_AUDIO_STOP (audio_hard_stop() response
         * if we're in the audio thread */
        audio_stop_playback();
        queue_clear(&audio_queue);
    }
    else
        audio_queue_send(Q_AUDIO_STOP, 1);
#ifdef PLAYBACK_VOICE
    voice_stop();
#endif

    /* we should be free to change the buffer now */
    if (give_up)
    {
        buffer_state = AUDIOBUF_STATE_TRASHED;
        audiobuf_handle = core_free(audiobuf_handle);
        return BUFLIB_CB_OK;
    }
    /* set final buffer size before calling audio_reset_buffer_noalloc()
     * (now it's the total size, the call will subtract voice etc) */
    filebuflen = size;
    switch (hints & BUFLIB_SHRINK_POS_MASK)
    {
        case BUFLIB_SHRINK_POS_BACK:
            core_shrink(handle, start, size);
            audio_reset_buffer_noalloc(start);
            break;
        case BUFLIB_SHRINK_POS_FRONT:
            core_shrink(handle, start + wanted_size, size);
            audio_reset_buffer_noalloc(start + wanted_size);
            break;
    }
    if (playing || play_queued)
    {
        /* post, to make subsequent calls not break the resume position */
        audio_queue_post(Q_AUDIO_PLAY, (intptr_t)&resume);
    }

    return BUFLIB_CB_OK;
}

static struct buflib_callbacks ops = {
    .move_callback = NULL,
    .shrink_callback = shrink_callback,
};

static void audio_reset_buffer(void)
{
    if (audiobuf_handle > 0)
    {
        core_free(audiobuf_handle);
        audiobuf_handle = 0;
    }
    if (core_allocatable() < pcmbuf_size_reqd())
        talk_buffer_set_policy(TALK_BUFFER_LOOSE); /* back off voice buffer */
    audiobuf_handle = core_alloc_maximum(&filebuflen, &ops);

    if (audiobuf_handle > 0)
        audio_reset_buffer_noalloc(core_get_data(audiobuf_handle));
    else
    /* someone is abusing core_alloc_maximum(). Fix this evil guy instead of
     * trying to handle OOM without hope */
        panicf("%s(): OOM!\n", __func__);
}

/* Set the buffer margin to begin rebuffering when 'seconds' from empty */
static void audio_update_filebuf_watermark(int seconds)
{
    size_t bytes = 0;

#ifdef HAVE_DISK_STORAGE
    int spinup = storage_spinup_time();

    if (seconds == 0)
    {
        /* By current setting */
        seconds = buffer_margin;
    }
    else
    {
        /* New setting */
        buffer_margin = seconds;

        if (buf_get_watermark() == 0)
        {
            /* Write a watermark only if the audio thread already did so for
               itself or it will fail to set the event and the watermark - if
               it hasn't yet, it will use the new setting when it does */
            return;
        }
    }

    if (spinup)
        seconds += (spinup / HZ) + 1;
    else
        seconds += 5;

    seconds += buffer_margin;
#else
    /* flash storage */
    seconds = 1;
#endif

    /* Watermark is a function of the bitrate of the last track in the buffer */
    struct track_info info;
    struct mp3entry *id3 = NULL;

    if (track_list_last(0, &info))
        id3 = valid_mp3entry(bufgetid3(info.id3_hid));

    if (id3)
    {
        if (!rbcodec_format_is_atomic(id3->codectype))
        {
            bytes = id3->bitrate * (1000/8) * seconds;
        }
        else
        {
            /* Bitrate has no meaning to buffering margin for atomic audio -
               rebuffer when it's the only track left unless it's the only
               track that fits, in which case we should avoid constant buffer
               low events */
            if (track_list_count() > 1)
                bytes = buf_filesize(info.audio_hid) + 1;
        }
    }
    else
    {
        /* Then set the minimum - this should not occur anyway */
        logf("fwmark: No id3 for last track (f=%d:c=%d:l=%d)",
             track_list.first_hid, track_list.current_hid, track_list.last_hid);
    }

    /* Actually setting zero disables the notification and we use that
       to detect that it has been reset */
    buf_set_watermark(MAX(bytes, 1));
    logf("fwmark: %zu", bytes);
}


/** -- Track change notification -- **/

/* Check the pcmbuf track changes and return write the message into the event
   if there are any */
static inline bool audio_pcmbuf_track_change_scan(void)
{
    if (track_change.out != track_change.in)
    {
        track_change.out++;
        return true;
    }

    return false;
}

/* Clear outstanding track change posts */
static inline void audio_pcmbuf_track_change_clear(void)
{
    track_change.out = track_change.in;
}

/* Post a track change notification - called by audio ISR */
static inline void audio_pcmbuf_track_change_post(void)
{
    track_change.in++;
}


/** --- Helper functions --- **/

/* Removes messages that might end up in the queue before or while processing
   a manual track change. Responding to them would be harmful since they
   belong to a previous track's playback period. Anything that would generate
   the stale messages must first be put into a state where it will not do so.
 */
static void audio_clear_track_notifications(void)
{
    static const long filter_list[][2] =
    {
        /* codec messages */
        { Q_AUDIO_CODEC_SEEK_COMPLETE, Q_AUDIO_CODEC_COMPLETE },
        /* track change messages */
        { Q_AUDIO_TRACK_CHANGED, Q_AUDIO_TRACK_CHANGED },
    };

    const int filter_count = ARRAYLEN(filter_list) - 1;

    /* Remove any pcmbuf notifications */
    pcmbuf_monitor_track_change(false);
    audio_pcmbuf_track_change_clear();

    /* Scrub the audio queue of the old mold */
    while (queue_peek_ex(&audio_queue, NULL,
                         filter_count | QPEEK_REMOVE_EVENTS,
                         filter_list))
    {
        yield(); /* Not strictly needed, per se, ad infinitum, ra, ra */
    }
}

/* Takes actions based upon track load status codes */
static void audio_handle_track_load_status(int trackstat)
{
    switch (trackstat)
    {
    case LOAD_TRACK_ERR_NO_MORE:
        if (track_list_count() > 0)
            break;

    case LOAD_TRACK_ERR_START_CODEC:
        audio_queue_post(Q_AUDIO_CODEC_COMPLETE, CODEC_ERROR);
        break;

    default:
        break;
    }
}

/* Send track events that use a struct track_event for data */
static void send_track_event(unsigned int id, unsigned int flags,
                             struct mp3entry *id3)
{
    if (id3 == id3_get(PLAYING_ID3))
        flags |= TEF_CURRENT;

    send_event(id, &(struct track_event){ .flags = flags, .id3 = id3 });
}

/* Announce the end of playing the current track */
static void audio_playlist_track_finish(void)
{
    struct mp3entry *ply_id3 = id3_get(PLAYING_ID3);
    struct mp3entry *id3 = valid_mp3entry(ply_id3);

    playlist_update_resume_info(filling == STATE_ENDED ? NULL : id3);

    if (id3)
    {
        send_track_event(PLAYBACK_EVENT_TRACK_FINISH,
                         track_event_flags, id3);
    }
}

/* Announce the beginning of the new track */
static void audio_playlist_track_change(void)
{
    struct mp3entry *id3 = valid_mp3entry(id3_get(PLAYING_ID3));

    if (id3)
    {
        send_track_event(PLAYBACK_EVENT_TRACK_CHANGE,
                         track_event_flags, id3);
    }

    position_key = pcmbuf_get_position_key();

    playlist_update_resume_info(id3);
}

/* Change the data for the next track and send the event */
static void audio_update_and_announce_next_track(const struct mp3entry *id3_next)
{
    id3_write_locked(NEXTTRACK_ID3, id3_next);
    send_track_event(PLAYBACK_EVENT_NEXTTRACKID3_AVAILABLE,
                     0,  id3_get(NEXTTRACK_ID3));
}

/* Bring the user current mp3entry up to date and set a new offset for the
   buffered metadata */
static void playing_id3_sync(struct track_info *user_infop, struct audio_resume_info *resume_info, bool skip_resume_adjustments)
{
    id3_mutex_lock();

    struct mp3entry *id3 = bufgetid3(user_infop->id3_hid);

    pcm_play_lock();

    if (id3)
    {
        if (resume_info)
        {
            id3->elapsed = resume_info->elapsed;
            id3->offset = resume_info->offset;
        }
        id3->skip_resume_adjustments = skip_resume_adjustments;
    }

    id3_write(PLAYING_ID3, id3);

    if (!resume_info && id3)
    {
        id3->offset = 0;
        id3->elapsed = 0;
        id3->skip_resume_adjustments = false;
    }

    pcm_play_unlock();

    id3_mutex_unlock();
}

/* Wipe-out track metadata - current is optional */
static void wipe_track_metadata(bool current)
{
    id3_mutex_lock();

    if (current)
        id3_write(PLAYING_ID3, NULL);

    id3_write(NEXTTRACK_ID3, NULL);
    id3_write(UNBUFFERED_ID3, NULL);

    id3_mutex_unlock();
}

/* Called when buffering is completed on the last track handle */
static void filling_is_finished(void)
{
    logf("last track finished buffering");

    /* There's no more to load or watch for */
    buf_set_watermark(0);
    filling = STATE_FINISHED;
}

/* Stop the codec decoding or waiting for its data to be ready - returns
   'false' if the codec ended up stopped */
static bool halt_decoding_track(bool stop)
{
    /* If it was waiting for us to clear the buffer to make a rebuffer
       happen, it should cease otherwise codec_stop could deadlock waiting
       for the codec to go to its main loop - codec's request will now
       force-fail */
    bool retval = false;

    buf_signal_handle(ci.audio_hid, true);

    if (stop)
        codec_stop();
    else
        retval = codec_pause();

    audio_clear_track_notifications();

    /* We now know it's idle and not waiting for buffered data */
    buf_signal_handle(ci.audio_hid, false);

    codec_skip_pending = false;
    codec_seeking = false;

    return retval;
}

/* Wait for any in-progress fade to complete */
static void audio_wait_fade_complete(void)
{
    /* Just loop until it's done */
    while (pcmbuf_fading())
        sleep(0);
}

/* End the ff/rw mode */
static void audio_ff_rewind_end(void)
{
    /* A seamless seek (not calling audio_pre_ff_rewind) skips this
       section */
    if (ff_rw_mode)
    {
        ff_rw_mode = false;

        if (codec_seeking)
        {
            /* Clear the buffer */
            pcmbuf_play_stop();
            audio_pcmbuf_sync_position();
        }

        if (play_status != PLAY_PAUSED)
        {
            /* Seeking-while-playing, resume PCM playback */
            pcmbuf_pause(false);
        }
    }
}

/* Complete the codec seek */
static void audio_complete_codec_seek(void)
{
    /* If a seek completed while paused, 'paused' is true.
     * If seeking from seek mode, 'ff_rw_mode' is true. */
    if (codec_seeking)
    {
        audio_ff_rewind_end();
        codec_seeking = false; /* set _after_ the call! */
    }
    /* else it's waiting and we must repond */
}

/* Get the current cuesheet pointer */
static inline struct cuesheet * get_current_cuesheet(void)
{
    return audio_scratch_memory->curr_cue;
}

/* Read the cuesheet from the buffer */
static void buf_read_cuesheet(int handle_id)
{
    struct cuesheet *cue = get_current_cuesheet();

    if (!cue || handle_id < 0)
        return;

    bufread(handle_id, sizeof (struct cuesheet), cue);
}

/* Backend to peek/current/next track metadata interface functions -
   fill in the mp3entry with as much information as we may obtain about
   the track at the specified offset from the user current track -
   returns false if no information exists with us */
static bool audio_get_track_metadata(int offset, struct mp3entry *id3)
{
    if (play_status == PLAY_STOPPED)
        return false;

    if (id3->path[0] != '\0')
        return true; /* Already filled */

    struct track_info info;

    if (!track_list_user_current(offset, &info))
    {
        struct mp3entry *ub_id3 = id3_get(UNBUFFERED_ID3);

        if (offset > 0 && track_list_user_current(offset - 1, NULL))
        {
            /* Try the unbuffered id3 since we're moving forward */
            if (ub_id3->path[0] != '\0')
            {
                copy_mp3entry(id3, ub_id3);
                return true;
            }
        }
    }
    else if (bufreadid3(info.id3_hid, id3))
    {
        id3->cuesheet = NULL;
        return true;
    }

    /* We didn't find the ID3 metadata, so we fill it with the little info we
       have and return that */

    char path[MAX_PATH+1];
    if (playlist_peek(offset, path, sizeof (path)))
    {
#if defined(HAVE_TC_RAMCACHE) && defined(HAVE_DIRCACHE)
        /* Try to get it from the database */
        if (!tagcache_fill_tags(id3, path))
#endif
        {
            /* By now, filename is the only source of info */
            fill_metadata_from_path(id3, path);
        }

        return true;
    }

    wipe_mp3entry(id3);

    return false;
}

/* Get resume rewind adjusted progress from the ID3 */
static void resume_rewind_adjust_progress(const struct mp3entry *id3,
                                          unsigned long *elapsed,
                                          unsigned long *offset)
{
    unsigned int rewind = MAX(global_settings.resume_rewind, 0);
    unsigned long d_e = rewind*1000;
    *elapsed = id3->elapsed - MIN(id3->elapsed, d_e);
    unsigned long d_o = rewind * id3->bitrate * (1000/8);
    *offset = id3->offset - MIN(id3->offset, d_o);
}

/* Get the codec into ram and initialize it - keep it if it's ready */
static bool audio_init_codec(struct track_info *track_infop,
                             struct mp3entry *track_id3)
{
    int codt_loaded = get_audio_base_codec_type(codec_loaded());
    int hid = ERR_HANDLE_NOT_FOUND;

    if (codt_loaded != AFMT_UNKNOWN)
    {
        int codt = get_audio_base_codec_type(track_id3->codectype);

        if (codt == codt_loaded)
        {
            /* Codec is the same base type */
            logf("Reusing prev. codec: %d", track_id3->codectype);
#ifdef HAVE_CODEC_BUFFERING
            /* Close any buffered codec (we could have skipped directly to a
               format transistion that is the same format as the current track
               and the buffered one is no longer needed) */
            track_info_close_handle(&track_infop->codec_hid);
            track_info_sync(track_infop);
#endif /* HAVE_CODEC_BUFFERING */
            return true;
        }
        else
        {
            /* New codec - first make sure the old one's gone */
            logf("Removing prev. codec: %d", codt_loaded);
            codec_unload();
        }
    }

    logf("New codec: %d/%d", track_id3->codectype, codec_loaded());

#ifdef HAVE_CODEC_BUFFERING
    /* Codec thread will close the handle even if it fails and will load from
       storage if hid is not valid or the buffer load fails */
    hid = track_infop->codec_hid;
    track_infop->codec_hid = ERR_HANDLE_NOT_FOUND;
    track_info_sync(track_infop);
#endif

    return codec_load(hid, track_id3->codectype);
    (void)track_infop; /* When codec buffering isn't supported */
}

#ifdef HAVE_TAGCACHE
/* Check settings for whether the file should be autoresumed */
enum { AUTORESUMABLE_UNKNOWN = 0, AUTORESUMABLE_TRUE, AUTORESUMABLE_FALSE };
static bool autoresumable(struct mp3entry *id3)
{
    char *path;
    size_t len;
    bool is_resumable;

    if (id3->autoresumable)             /* result cached? */
        return id3->autoresumable == AUTORESUMABLE_TRUE;

    is_resumable = false;

    if (*id3->path)
    {
        for (path = global_settings.autoresume_paths;
             *path;                     /* search terms left? */
             path++)
        {
            if (*path == ':')           /* Skip empty search patterns */
                continue;

            len = strcspn(path, ":");

            /* Note: At this point, len is always > 0 */

            if (strncasecmp(id3->path, path, len) == 0)
            {
                /* Full directory-name matches only.  Trailing '/' in
                   search path OK. */
                if (id3->path[len] == '/' || id3->path[len - 1] == '/')
                {
                    is_resumable = true;
                    break;
                }
            }
            path += len - 1;
        }
    }

    /* cache result */
    id3->autoresumable =
        is_resumable ? AUTORESUMABLE_TRUE : AUTORESUMABLE_FALSE;

    logf("autoresumable: %s is%s resumable",
         id3->path, is_resumable ? "" : " not");

    return is_resumable;
}
#endif  /* HAVE_TAGCACHE */

/* Start the codec for the current track scheduled to be decoded */
static bool audio_start_codec(bool auto_skip)
{
    struct track_info info;
    track_list_current(0, &info);

    struct mp3entry *cur_id3 = valid_mp3entry(bufgetid3(info.id3_hid));

    if (!cur_id3)
        return false;

    buf_pin_handle(info.id3_hid, true);

    if (!audio_init_codec(&info, cur_id3))
    {
        buf_pin_handle(info.id3_hid, false);
        return false;
    }

    #ifdef HAVE_TAGCACHE
    bool autoresume_enable = !cur_id3->skip_resume_adjustments && global_settings.autoresume_enable;

    if (autoresume_enable && !(cur_id3->elapsed || cur_id3->offset))
    {
        /* Resume all manually selected tracks */
        bool resume = !auto_skip;

        /* Send the "buffer" event to obtain the resume position for the codec */
        send_track_event(PLAYBACK_EVENT_TRACK_BUFFER, 0, cur_id3);

        if (!resume)
        {
            /* Automatic skip - do further tests to see if we should just
               ignore any autoresume position */
            int autoresume_automatic = global_settings.autoresume_automatic;

            switch (autoresume_automatic)
            {
            case AUTORESUME_NEXTTRACK_ALWAYS:
                /* Just resume unconditionally */
                resume = true;
                break;
            case AUTORESUME_NEXTTRACK_NEVER:
                /* Force-rewind it */
                break;
            default:
                /* Not "never resume" - pass resume filter? */
                resume = autoresumable(cur_id3);
            }
        }

        if (!resume)
        {
            cur_id3->elapsed = 0;
            cur_id3->offset = 0;
        }

        logf("%s: Set resume for %s to %lu %lX", __func__,
             cur_id3->title, cur_id3->elapsed, cur_id3->offset);
    }
#endif /* HAVE_TAGCACHE */

    /* Rewind the required amount - if an autoresume was done, this also rewinds
       that by the setting's amount

       It would be best to have bookkeeping about whether or not the track
       sounded or not since skipping to it or else skipping to it while paused
       and back again will cause accumulation of silent rewinds - that's not
       our job to track directly nor could it be in any reasonable way
     */
    if (!cur_id3->skip_resume_adjustments)
    {
        resume_rewind_adjust_progress(cur_id3, &cur_id3->elapsed, &cur_id3->offset);
        cur_id3->skip_resume_adjustments = true;
    }

    /* Update the codec API with the metadata and track info */
    id3_write(CODEC_ID3, cur_id3);

    ci.audio_hid = info.audio_hid;
    ci.filesize = buf_filesize(info.audio_hid);
    buf_set_base_handle(info.audio_hid);

    /* All required data is now available for the codec */
    codec_go();

#ifdef HAVE_TAGCACHE
    if (!autoresume_enable || cur_id3->elapsed || cur_id3->offset)
#endif
    {
        /* Send the "buffer" event now */
        send_track_event(PLAYBACK_EVENT_TRACK_BUFFER, 0, cur_id3);
    }

    buf_pin_handle(info.id3_hid, false);
    return true;

    (void)auto_skip; /* ifndef HAVE_TAGCACHE */
}


/** --- Audio thread --- **/

/* Load and parse a cuesheet for the file - returns false if the buffer
   is full */
static bool audio_load_cuesheet(struct track_info *infop,
                                struct mp3entry *track_id3)
{
    struct cuesheet *cue = get_current_cuesheet();
    track_id3->cuesheet = NULL;

    if (cue && infop->cuesheet_hid == ERR_HANDLE_NOT_FOUND)
    {
        /* If error other than a full buffer, then mark it "unsupported" to
           avoid reloading attempt */
        int hid = ERR_UNSUPPORTED_TYPE;
        struct cuesheet_file cue_file;

        if (look_for_cuesheet_file(track_id3, &cue_file))
        {
            hid = bufalloc(NULL, sizeof (struct cuesheet), TYPE_CUESHEET);

            if (hid >= 0)
            {
                void *cuesheet = NULL;
                bufgetdata(hid, sizeof (struct cuesheet), &cuesheet);

                if (parse_cuesheet(&cue_file, (struct cuesheet *)cuesheet))
                {
                    /* Indicate cuesheet is present (while track remains
                       buffered) */
                    track_id3->cuesheet = cue;
                }
                else
                {
                    bufclose(hid);
                    hid = ERR_UNSUPPORTED_TYPE;
                }
            }
        }

        if (hid == ERR_BUFFER_FULL)
        {
            logf("buffer is full for now (%s)", __func__);
            return false;
        }
        else
        {
            if (hid < 0)
                logf("Cuesheet loading failed");

            infop->cuesheet_hid = hid;
        }
    }

    return true;
}

#ifdef HAVE_ALBUMART

void set_albumart_mode(int setting)
{
    if (albumart_mode != -1 &&
        albumart_mode != setting)
        playback_update_aa_dims();
    albumart_mode = setting;
}

static int load_album_art_from_path(char *path, struct bufopen_bitmap_data *user_data, bool is_current_track, int i)
{
    user_data->embedded_albumart = NULL;

    bool same_path = strcmp(last_folder_aa_path, path) == 0;
    if (same_path && last_folder_aa_hid[i] != 0)
        return last_folder_aa_hid[i];

    // To simplify caching logic a bit we keep track only for first AA path
    // If other album arts use different path (like dimension specific arts) just skip caching for them
    bool is_cacheable = i == 0 && (is_current_track || last_folder_aa_path[0] == 0);
    if (!same_path && is_cacheable)
    {
        clear_last_folder_album_art();
        strcpy(last_folder_aa_path, path);
    }
    int hid = bufopen(path, 0, TYPE_BITMAP, user_data);
    if (hid != ERR_BUFFER_FULL && (same_path || is_cacheable))
        last_folder_aa_hid[i] = hid;
    return hid;
}

/* Load any album art for the file - returns false if the buffer is full */
static int audio_load_albumart(struct track_info *infop,
                                struct mp3entry *track_id3, bool is_current_track)
{
    FOREACH_ALBUMART(i)
    {
        struct bufopen_bitmap_data user_data;
        int *aa_hid = &infop->aa_hid[i];
        int hid = ERR_UNSUPPORTED_TYPE;
        bool checked_image_file = false;

        /* albumart_slots may change during a yield of bufopen,
         * but that's no problem */
        if (*aa_hid >= 0 || *aa_hid == ERR_UNSUPPORTED_TYPE ||
            !albumart_slots[i].used)
            continue;

        memset(&user_data, 0, sizeof(user_data));
        user_data.dim = &albumart_slots[i].dim;

        char path[MAX_PATH];
        if(global_settings.album_art == AA_PREFER_IMAGE_FILE)
        {
            if (find_albumart(track_id3, path, sizeof(path),
                          &albumart_slots[i].dim))
            {
                hid = load_album_art_from_path(path, &user_data, is_current_track, i);
            }
            checked_image_file = true;
        }

        /* We can only decode jpeg for embedded AA */
        if (global_settings.album_art != AA_OFF &&
            hid < 0 && hid != ERR_BUFFER_FULL &&
            track_id3->has_embedded_albumart && track_id3->albumart.type == AA_TYPE_JPG)
        {
            if (is_current_track)
                clear_last_folder_album_art();
            user_data.embedded_albumart = &track_id3->albumart;
            hid = bufopen(track_id3->path, 0, TYPE_BITMAP, &user_data);
        }

        if (global_settings.album_art != AA_OFF && !checked_image_file &&
            hid < 0 && hid != ERR_BUFFER_FULL)
        {
            /* No embedded AA or it couldn't be loaded - try other sources */
            if (find_albumart(track_id3, path, sizeof(path),
                              &albumart_slots[i].dim))
            {
                hid = load_album_art_from_path(path, &user_data, is_current_track, i);
            }
        }

        if (hid == ERR_BUFFER_FULL)
        {
            logf("buffer is full for now (%s)", __func__);
            return ERR_BUFFER_FULL;
        }
        else if (hid == ERR_BITMAP_TOO_LARGE){
            logf("image is too large to fit in buffer (%s)", __func__);
            return ERR_BITMAP_TOO_LARGE;
        }
        else
        {
            /* If error other than a full buffer, then mark it "unsupported"
               to avoid reloading attempt */
            if (hid < 0)
            {
                logf("Album art loading failed");
                hid = ERR_UNSUPPORTED_TYPE;
            }
            else
            {
                logf("Loaded album art:%dx%d", user_data.dim->width,
                     user_data.dim->height);
            }

            *aa_hid = hid;
        }
    }

    return true;
}
#endif /* HAVE_ALBUMART */

#ifdef HAVE_CODEC_BUFFERING
/* Load a codec for the file onto the buffer - assumes we're working from the
   currently loading track - not called for the current track */
static bool audio_buffer_codec(struct track_info *track_infop,
                               struct mp3entry *track_id3)
{
    /* This will not be the current track -> it cannot be the first and the
       current track cannot be ahead of buffering -> there is a previous
       track entry which is either current or ahead of the current */
    struct track_info prev_info;
    track_list_last(-1, &prev_info);

    struct mp3entry *prev_id3 = bufgetid3(prev_info.id3_hid);

    /* If the previous codec is the same as this one, there is no need to
       put another copy of it on the file buffer (in other words, only
       buffer codecs at format transitions) */
    if (prev_id3)
    {
        int codt = get_audio_base_codec_type(track_id3->codectype);
        int prev_codt = get_audio_base_codec_type(prev_id3->codectype);

        if (codt == prev_codt)
        {
            logf("Reusing prev. codec: %d", prev_id3->codectype);
            return true;
        }
    }
    /* else just load it (harmless) */

    /* Load the codec onto the buffer if possible */
    const char *codec_fn = get_codec_filename(track_id3->codectype);
    if (!codec_fn)
        return false;

    char codec_path[MAX_PATH+1]; /* Full path to codec */
    codec_get_full_path(codec_path, codec_fn);

    track_infop->codec_hid = bufopen(codec_path, 0, TYPE_CODEC, NULL);

    if (track_infop->codec_hid > 0)
    {
        logf("Buffered codec: %d", track_infop->codec_hid);
        return true;
    }

    return false;
}
#endif /* HAVE_CODEC_BUFFERING */

/* Load metadata for the next track (with bufopen). The rest of the track
   loading will be handled by audio_finish_load_track once the metadata has
   been actually loaded by the buffering thread.

   Each track is arranged in the buffer as follows:
        <id3|[cuesheet|][album art|][codec|]audio>

   The next will not be loaded until the previous succeeds if the buffer was
   full at the time. To put any metadata after audio would make those handles
   unmovable.
*/
static int audio_load_track(void)
{
    struct track_info info;

    if (track_list.in_progress_hid > 0)
    {
        /* There must be an info pointer if the in-progress id3 is even there */
        if (track_list_last(0, &info) && info.self_hid == track_list.in_progress_hid)
        {
            if (filling == STATE_FILLING)
            {
                /* Haven't finished the metadata but the notification is
                   anticipated to come soon */
                logf("%s:in progress:id=%d", __func__, info.self_hid);
                return LOAD_TRACK_OK;
            }
            else if (filling == STATE_FULL)
            {
                /* Buffer was full trying to complete the load after the
                   metadata finished, so attempt to continue - older handles
                   should have been cleared already */
                logf("%s:finished:id=%d", __func__, info.self_hid);
                filling = STATE_FILLING;
                buffer_event_finished_callback(BUFFER_EVENT_FINISHED, &info.id3_hid);
                return LOAD_TRACK_OK;
            }
        }

        /* Some old, stray buffering message */
        logf("%s:busy:id=%d", __func__, info.self_hid);
        return LOAD_TRACK_ERR_BUSY;
    }

    filling = STATE_FILLING;

    if (!track_list_alloc_info(&info))
    {
        /* List is full so stop buffering tracks - however, attempt to obtain
           metadata as the unbuffered id3 */
        logf("buffer full:alloc");
        filling = STATE_FULL;
    }

    playlist_peek_offset++;

    logf("Buffering track:f=%d:c=%d:l=%d:pk=%d",
         track_list.first_hid, track_list.current_hid, track_list.last_hid,
         playlist_peek_offset);

    /* Get track name from current playlist read position */
    int fd = -1;
    char path_buf[MAX_PATH + 1];
    const char *path;

    while (1)
    {
        path = playlist_peek(playlist_peek_offset,
                             path_buf,
                             sizeof (path_buf));

        if (!path)
            break;

        /* Test for broken playlists by probing for the files */
        if (file_exists(path))
        {
            fd = open(path, O_RDONLY);
            if (fd >= 0)
                break;
        }
        logf("Open failed %s", path);

        /* only skip if failed track has a successor in playlist */
        if (!playlist_peek(playlist_peek_offset + 1, NULL, 0))
            break;

        /* Skip invalid entry from playlist */
        playlist_skip_entry(NULL, playlist_peek_offset);

        /* Sync the playlist if it isn't finished */
        if (playlist_peek(playlist_peek_offset, NULL, 0))
            playlist_next(0);
    }

    if (!path)
    {
        /* No track - exhausted the playlist entries */
        logf("End-of-playlist");
        id3_write_locked(UNBUFFERED_ID3, NULL);

        if (filling != STATE_FULL)
            track_list_free_info(&info); /* Free this entry */

        playlist_peek_offset--;          /* Maintain at last index */

        /* We can end up here after the real last track signals its completion
           and miss the transition to STATE_FINISHED esp. if dropping the last
           songs of a playlist late in their load (2nd stage) */
        if (track_list_last(0, &info) && buf_handle_remaining(info.audio_hid) == 0)
            filling_is_finished();
        else
            filling = STATE_END_OF_PLAYLIST;

        return LOAD_TRACK_ERR_NO_MORE;
    }

    /* Successfully opened the file - get track metadata */
    if (filling == STATE_FULL ||
        (info.id3_hid = bufopen(path, 0, TYPE_ID3, NULL)) < 0)
    {
        /* Buffer or track list is full */
        struct mp3entry *ub_id3;

        playlist_peek_offset--;

        /* Load the metadata for the first unbuffered track */
        ub_id3 = id3_get(UNBUFFERED_ID3);

        if (fd >= 0)
        {
            id3_mutex_lock();
            get_metadata(ub_id3, fd, path);
            id3_mutex_unlock();
        }

        if (filling != STATE_FULL)
        {
            track_list_free_info(&info);
            filling = STATE_FULL;
        }

        logf("%s: buffer is full for now (%u tracks)", __func__,
             track_list_count());
    }
    else
    {
        if (!track_list_commit_info(&info))
        {
            track_list_free_info(&info);
            track_list.in_progress_hid = 0;
            if (fd >= 0)
                close(fd);
            return LOAD_TRACK_ERR_FAILED;
        }

        /* Successful load initiation */
        track_list.in_progress_hid = info.self_hid;
    }
    if (fd >= 0)
        close(fd);
    return LOAD_TRACK_OK;
}

#ifdef HAVE_PLAY_FREQ
static bool audio_auto_change_frequency(struct mp3entry *id3, bool play);
#endif

/* Second part of the track loading: We now have the metadata available, so we
   can load the codec, the album art and finally the audio data.
   This is called on the audio thread after the buffering thread calls the
   buffering_handle_finished_callback callback. */
static int audio_finish_load_track(struct track_info *infop)
{
    int trackstat = LOAD_TRACK_OK;

    if (infop->self_hid != track_list.in_progress_hid)
    {
        /* We must not be here if not! */
        logf("%s:wrong track:hids=%d!=%d", __func__, infop->self_hid,
             track_list.in_progress_hid);
        return LOAD_TRACK_ERR_BUSY;
    }

    /* The current track for decoding (there is always one if the list is
       populated) */
    struct track_info cur_info;
    track_list_current(0, &cur_info);

    struct mp3entry *track_id3 = valid_mp3entry(bufgetid3(infop->id3_hid));

    if (!track_id3)
    {
        /* This is an error condition. Track cannot be played without valid
           metadata; skip the track. */
        logf("No metadata");
        trackstat = LOAD_TRACK_ERR_FINISH_FAILED;
        goto audio_finish_load_track_exit;
    }

    struct track_info user_cur;

#ifdef HAVE_PLAY_FREQ
    track_list_user_current(0, &user_cur);
    bool is_current_user = infop->self_hid == user_cur.self_hid;
    if (audio_auto_change_frequency(track_id3, is_current_user))
    {
        // frequency switch requires full re-buffering, so stop buffering
        filling = STATE_STOPPED;
        logf("buffering stopped (current_track: %b, current_user: %b)", infop->self_hid == cur_info.self_hid, is_current_user);
        if (is_current_user)
            // audio_finish_load_track_exit not needed as playback restart is already initiated
            return trackstat;

        goto audio_finish_load_track_exit;
    }
#endif

    /* Try to load a cuesheet for the track */
    if (!audio_load_cuesheet(infop, track_id3))
    {
        /* No space for cuesheet on buffer, not an error */
        filling = STATE_FULL;
        goto audio_finish_load_track_exit;
    }

#ifdef HAVE_ALBUMART
    /* Try to load album art for the track */
    int retval = audio_load_albumart(infop, track_id3, infop->self_hid == cur_info.self_hid);
    if (retval == ERR_BITMAP_TOO_LARGE)
    {
        /* No space for album art on buffer because the file is larger than the buffer.
        Ignore the file and keep buffering */
    } else if (retval == ERR_BUFFER_FULL)
    {
        /* No space for album art on buffer, not an error */
        filling = STATE_FULL;
        goto audio_finish_load_track_exit;
    }
#endif

    /* All handles available to external routines are ready - audio and codec
       information is private */

    track_list_user_current(0, &user_cur);
    if (infop->self_hid == user_cur.self_hid)
    {
        /* Send only when the track handles could not all be opened ahead of
           time for the user's current track - otherwise everything is ready
           by the time PLAYBACK_EVENT_TRACK_CHANGE is sent */
        send_track_event(PLAYBACK_EVENT_CUR_TRACK_READY, 0,
                         id3_get(PLAYING_ID3));
    }

#ifdef HAVE_CODEC_BUFFERING
    /* Try to buffer a codec for the track */
    if (infop->self_hid != cur_info.self_hid
        && !audio_buffer_codec(infop, track_id3))
    {
        if (infop->codec_hid == ERR_BUFFER_FULL)
        {
            /* No space for codec on buffer, not an error */
            filling = STATE_FULL;
            logf("%s:STATE_FULL", __func__);
        }
        else
        {
            /* This is an error condition, either no codec was found, or
               reading the codec file failed part way through, either way,
               skip the track */
            logf("No codec for: %s", track_id3->path);
            trackstat = LOAD_TRACK_ERR_FINISH_FAILED;
        }

        goto audio_finish_load_track_exit;
    }
#endif /* HAVE_CODEC_BUFFERING */

    /** Finally, load the audio **/
    off_t file_offset = 0;

    /* Adjust for resume rewind so we know what to buffer - starting the codec
       calls it again, so we don't save it (and they shouldn't accumulate) */
    unsigned long elapsed, offset;
    resume_rewind_adjust_progress(track_id3, &elapsed, &offset);

    enum data_type audiotype = rbcodec_format_is_atomic(track_id3->codectype) ?
                                      TYPE_ATOMIC_AUDIO : TYPE_PACKET_AUDIO;

    if (audiotype == TYPE_ATOMIC_AUDIO)
        logf("Loading atomic %d", track_id3->codectype);

    if (format_buffers_with_offset(track_id3->codectype))
    {
        /* This format can begin buffering from any point */
        file_offset = offset;
    }

    logf("load track: %s", track_id3->path);

    if (file_offset > AUDIO_REBUFFER_GUESS_SIZE)
    {
        /* We can buffer later in the file, adjust the hunt-and-peck margin */
        file_offset -= AUDIO_REBUFFER_GUESS_SIZE;
    }
    else
    {
        /* No offset given or it is very minimal - begin at the first frame
           according to the metadata */
        file_offset = track_id3->first_frame_offset;
    }

    int hid = bufopen(track_id3->path, file_offset, audiotype, NULL);

    if (hid >= 0)
    {
        infop->audio_hid = hid;

        /*
         * Fix up elapsed time and offset if past the end
         */
        if (track_id3->elapsed > track_id3->length)
            track_id3->elapsed = 0;

        if ((off_t)track_id3->offset >= buf_filesize(infop->audio_hid))
            track_id3->offset = 0;

        logf("%s: set resume for %s to %lu %lX", __func__,
             track_id3->title, track_id3->elapsed, track_id3->offset);

        if (infop->self_hid == cur_info.self_hid)
        {
            /* This is the current track to decode - should be started now */
            trackstat = LOAD_TRACK_READY;
        }
    }
    else
    {
        /* Buffer could be full but not properly so if this is the only
           track! */
        if (hid == ERR_BUFFER_FULL && audio_track_count() > 1)
        {
            filling = STATE_FULL;
            logf("Buffer is full for now (%s)", __func__);
        }
        else
        {
            /* Nothing to play if no audio handle - skip this */
            logf("Could not add audio data handle");
            trackstat = LOAD_TRACK_ERR_FINISH_FAILED;
        }
    }

audio_finish_load_track_exit:
    if (trackstat >= LOAD_TRACK_OK && !track_info_sync(infop))
    {
        logf("Track info sync failed");
        trackstat = LOAD_TRACK_ERR_FINISH_FAILED;
    }

    if (trackstat < LOAD_TRACK_OK)
    {
        playlist_skip_entry(NULL, playlist_peek_offset);
        track_list_free_info(infop);

        if (playlist_peek(playlist_peek_offset, NULL, 0))
            playlist_next(0);

        playlist_peek_offset--;
    }

    if (filling != STATE_FULL && filling != STATE_STOPPED)
    {
        /* Load next track - error or not */
        track_list.in_progress_hid = 0;
        LOGFQUEUE("audio > audio Q_AUDIO_FILL_BUFFER");
        audio_queue_post(Q_AUDIO_FILL_BUFFER, 0);
    }
    else
    {
        /* Full */
        trackstat = LOAD_TRACK_ERR_FINISH_FULL;
    }

    return trackstat;
}

/* Start a new track load */
static int audio_fill_file_buffer(void)
{
    if (play_status == PLAY_STOPPED)
        return LOAD_TRACK_ERR_FAILED;

    trigger_cpu_boost();

    /* Must reset the buffer before use if trashed or voice only - voice
       file size shouldn't have changed so we can go straight from
       AUDIOBUF_STATE_VOICED_ONLY to AUDIOBUF_STATE_INITIALIZED */
    if (buffer_state != AUDIOBUF_STATE_INITIALIZED ||
        !pcmbuf_is_same_size())
    {
        audio_reset_buffer();
    }

    logf("Starting buffer fill");

    int trackstat = audio_load_track();

    if (trackstat >= LOAD_TRACK_OK)
    {
        struct track_info info, user_cur;
        track_list_current(0, &info);
        track_list_user_current(0, &user_cur);

        if (info.self_hid == user_cur.self_hid)
            playlist_next(0);

        if (filling == STATE_FULL && !track_list_user_current(1, NULL))
        {
            /* There are no user tracks on the buffer after this therefore
               this is the next track */
            audio_update_and_announce_next_track(id3_get(UNBUFFERED_ID3));
        }
    }

    return trackstat;
}

/* Discard unwanted tracks and start refill from after the specified playlist
   offset */
static int audio_reset_and_rebuffer(
    enum track_clear_action action, int peek_offset)
{
    logf("Forcing rebuffer: 0x%X, %d", action, peek_offset);

    id3_write_locked(UNBUFFERED_ID3, NULL);

    /* Remove unwanted tracks - caller must have ensured codec isn't using
       any */
    track_list_clear(action);

    /* Refill at specified position (-1 starts at index offset 0) */
    playlist_peek_offset = peek_offset;

    /* Fill the buffer */
    return audio_fill_file_buffer();
}

/* Handle buffering events
   (Q_AUDIO_BUFFERING) */
static void audio_on_buffering(int event)
{
    enum track_clear_action action;
    int peek_offset;

    if (track_list_empty())
        return;

    switch (event)
    {
    case BUFFER_EVENT_BUFFER_LOW:
        if (filling != STATE_FULL && filling != STATE_END_OF_PLAYLIST)
            return; /* Should be nothing left to fill */

        /* Clear old tracks and continue buffering where it left off */
        action = TRACK_LIST_KEEP_NEW;
        peek_offset = playlist_peek_offset;
        break;

    case BUFFER_EVENT_REBUFFER:
        /* Remove all but the currently decoding track and redo buffering
           after that */
        action = TRACK_LIST_KEEP_CURRENT;
        peek_offset = (skip_pending == TRACK_SKIP_AUTO) ? 1 : 0;
        break;

    default:
        return;
    }

    switch (skip_pending)
    {
    case TRACK_SKIP_NONE:
    case TRACK_SKIP_AUTO:
    case TRACK_SKIP_AUTO_NEW_PLAYLIST:
        audio_reset_and_rebuffer(action, peek_offset);
        break;

    case TRACK_SKIP_AUTO_END_PLAYLIST:
        /* Already finished */
        break;

    default:
        /* Invalid */
        logf("Buffering call, inv. state: %d", (int)skip_pending);
    }
}

/* Handle starting the next track load
   (Q_AUDIO_FILL_BUFFER) */
static void audio_on_fill_buffer(void)
{
    audio_handle_track_load_status(audio_fill_file_buffer());
}

/* Handle posted load track finish event
   (Q_AUDIO_FINISH_LOAD_TRACK) */
static void audio_on_finish_load_track(int id3_hid)
{
    struct track_info info, user_cur;

    if (!buf_is_handle(id3_hid) || !track_list_last(0, &info))
        return;

    track_list_user_current(1, &user_cur);
    if (info.self_hid == user_cur.self_hid)
    {
        /* Just loaded the metadata right after the current position */
        audio_update_and_announce_next_track(bufgetid3(info.id3_hid));
    }

    if (audio_finish_load_track(&info) != LOAD_TRACK_READY)
        return; /* Not current track */

    track_list_user_current(0, &user_cur);
    bool is_user_current = info.self_hid == user_cur.self_hid;

    if (is_user_current)
    {
        /* Copy cuesheet */
        buf_read_cuesheet(info.cuesheet_hid);
    }

    if (audio_start_codec(track_event_flags & TEF_AUTO_SKIP))
    {
        if (is_user_current)
        {
            /* Be sure all tagtree info is synchronized; it will be needed for the
               track finish event - the sync will happen when finalizing a track
               change otherwise */
            bool was_valid = valid_mp3entry(id3_get(PLAYING_ID3));

            playing_id3_sync(&info, NULL, true);

            if (!was_valid)
            {
                /* Playing id3 hadn't been updated yet because no valid track
                   was yet available - treat like the first track */
                audio_playlist_track_change();
            }
        }
    }
    else
    {
        audio_handle_track_load_status(LOAD_TRACK_ERR_START_CODEC);
    }
}

/* Called when handles other than metadata handles have finished buffering
   (Q_AUDIO_HANDLE_FINISHED) */
static void audio_on_handle_finished(int hid)
{
    /* Right now, only audio handles should end up calling this */
    if (filling == STATE_END_OF_PLAYLIST)
    {
        struct track_info info;

        /* Really we don't know which order the handles will actually complete
           to zero bytes remaining since another thread is doing it - be sure
           it's the right one */
        if (track_list_last(0, &info) && info.audio_hid == hid)
        {
            /* This was the last track in the playlist and we now have all the
               data we need */
            filling_is_finished();
        }
    }
}

static inline char* single_mode_get_id3_tag(struct mp3entry *id3)
{
    struct mp3entry *valid_id3 = valid_mp3entry(id3);
    if (valid_id3 == NULL)
        return NULL;

    switch (global_settings.single_mode)
    {
        case SINGLE_MODE_ALBUM:
            return valid_id3->album;
        case SINGLE_MODE_ALBUM_ARTIST:
            return valid_id3->albumartist;
        case SINGLE_MODE_ARTIST:
            return valid_id3->artist;
        case SINGLE_MODE_COMPOSER:
            return valid_id3->composer;
        case SINGLE_MODE_GROUPING:
            return valid_id3->grouping;
        case SINGLE_MODE_GENRE:
            return valid_id3->genre_string;
    }

    return NULL;
}

static bool single_mode_do_pause(int id3_hid)
{
    if (global_settings.single_mode != SINGLE_MODE_OFF && global_settings.party_mode == 0 &&
        ((skip_pending == TRACK_SKIP_AUTO) || (skip_pending == TRACK_SKIP_AUTO_NEW_PLAYLIST))) {

        if (global_settings.single_mode == SINGLE_MODE_TRACK)
            return true;

        char *previous_tag = single_mode_get_id3_tag(id3_get(PLAYING_ID3));
        char *new_tag = single_mode_get_id3_tag(bufgetid3(id3_hid));
        return previous_tag == NULL ||
               new_tag == NULL ||
               strcmp(previous_tag, new_tag) != 0;
    }
    return false;
}

/* Called to make an outstanding track skip the current track and to send the
   transition events */
static void audio_finalise_track_change(void)
{
    switch (skip_pending)
    {
    case TRACK_SKIP_NONE: /* Manual skip */
        break;

    case TRACK_SKIP_AUTO:
    case TRACK_SKIP_AUTO_NEW_PLAYLIST:
    {
        int playlist_delta = skip_pending == TRACK_SKIP_AUTO ? 1 : 0;
        audio_playlist_track_finish();

        if (!playlist_peek(playlist_delta, NULL, 0))
        {
            /* Track ended up rejected - push things ahead like the codec blew
               it (because it was never started and now we're here where it
               should have been decoding the next track by now) - next, a
               directory change or end of playback will most likely happen */
            skip_pending = TRACK_SKIP_NONE;
            audio_handle_track_load_status(LOAD_TRACK_ERR_START_CODEC);
            return;
        }

        if (!playlist_delta)
            break;

        playlist_peek_offset -= playlist_delta;
        if (playlist_next(playlist_delta) >= 0)
            break;
            /* What!? Disappear? Hopeless bleak despair */
        }
        /* Fallthrough */
    case TRACK_SKIP_AUTO_END_PLAYLIST:
    default:            /* Invalid */
        filling = STATE_ENDED;
        audio_stop_playback();
        return;
    }

    struct track_info info;
    bool have_info = track_list_current(0, &info);
    struct mp3entry *track_id3 = NULL;

    /* Update the current cuesheet if any and enabled */
    if (have_info)
    {
        buf_read_cuesheet(info.cuesheet_hid);
        track_id3 = bufgetid3(info.id3_hid);

        if (single_mode_do_pause(info.id3_hid))
        {
            play_status = PLAY_PAUSED;
            pcmbuf_pause(true);
        }
    }
    /* Sync the next track information */
    have_info = track_list_current(1, &info);

    id3_mutex_lock();

    id3_write(PLAYING_ID3, track_id3);

    /* The skip is technically over */
    skip_pending = TRACK_SKIP_NONE;

    id3_write(NEXTTRACK_ID3, have_info ? bufgetid3(info.id3_hid) :
                                         id3_get(UNBUFFERED_ID3));

    id3_mutex_unlock();

    audio_playlist_track_change();

#ifdef HAVE_PLAY_FREQ
    if (filling == STATE_STOPPED)
        audio_auto_change_frequency(track_id3, true);
#endif
}

/* Actually begin a transition and take care of the codec change - may complete
   it now or ask pcmbuf for notification depending on the type */
static void audio_begin_track_change(enum pcm_track_change_type type,
                                     int trackstat)
{
    /* Even if the new track is bad, the old track must be finished off */
    pcmbuf_start_track_change(type);

    track_skip_is_manual = (type == TRACK_CHANGE_MANUAL);

    if (track_skip_is_manual)
    {
        /* Manual track change happens now */
        audio_finalise_track_change();
        pcmbuf_sync_position_update();

        if (play_status == PLAY_STOPPED)
            return; /* Stopped us */
    }

    if (trackstat >= LOAD_TRACK_OK)
    {
        struct track_info info;
        if (track_list_current(0, &info))
        {
            if (info.audio_hid < 0)
                return;

            /* Everything needed for the codec is ready - start it */
            if (audio_start_codec(!track_skip_is_manual))
            {
                if (track_skip_is_manual)
                    playing_id3_sync(&info, NULL, true);
                return;
            }
        }

        trackstat = LOAD_TRACK_ERR_START_CODEC;
    }

    audio_handle_track_load_status(trackstat);
}

/* Transition to end-of-playlist state and begin wait for PCM to finish */
static void audio_monitor_end_of_playlist(void)
{
    skip_pending = TRACK_SKIP_AUTO_END_PLAYLIST;
    filling = STATE_ENDING;
    pcmbuf_start_track_change(TRACK_CHANGE_END_OF_DATA);
}

/* Does this track have an entry allocated? */
static bool audio_can_change_track(int *trackstat, int *id3_hid)
{
    struct track_info info;
    bool have_track = track_list_advance_current(1, &info);
    *id3_hid = info.id3_hid;
    if (!have_track || info.audio_hid < 0)
    {
        bool end_of_playlist = false;

        if (have_track)
        {
            if (filling == STATE_STOPPED)
            {
                audio_begin_track_change(TRACK_CHANGE_END_OF_DATA, *trackstat);
                return false;
            }

            /* Track load is not complete - it might have stopped on a
               full buffer without reaching the audio handle or we just
               arrived at it early

               If this type is atomic and we couldn't get the audio,
               perhaps it would need to wrap to make the allocation and
               handles are in the way - to maximize the liklihood it can
               be allocated, clear all handles to reset the buffer and
               its indexes to 0 - for packet audio, this should not be an
               issue and a pointless full reload of all the track's
               metadata may be avoided */

            struct mp3entry *track_id3 = bufgetid3(info.id3_hid);

            if (track_id3 && !rbcodec_format_is_atomic(track_id3->codectype))
            {
                /* Continue filling after this track */
                audio_reset_and_rebuffer(TRACK_LIST_KEEP_CURRENT, 1);
                return true;
            }
            /* else rebuffer at this track; status applies to the track we
               want */
        }
        else if (!playlist_peek(1, NULL, 0))
        {
            /* Play sequence is complete - directory change or other playlist
               resequencing - the playlist must now be advanced in order to
               continue since a peek ahead to the next track is not possible */
            skip_pending = TRACK_SKIP_AUTO_NEW_PLAYLIST;
            end_of_playlist = playlist_next(1) < 0;
        }

        if (!end_of_playlist)
        {
            *trackstat = audio_reset_and_rebuffer(TRACK_LIST_CLEAR_ALL,
                    skip_pending == TRACK_SKIP_AUTO ? 0 : -1);

            if (*trackstat == LOAD_TRACK_ERR_NO_MORE)
            {
                /* Failed to find anything after all - do playlist switchover
                   instead */
                skip_pending = TRACK_SKIP_AUTO_NEW_PLAYLIST;
                end_of_playlist = playlist_next(1) < 0;
            }
        }

        if (end_of_playlist)
        {
            audio_monitor_end_of_playlist();
            return false;
        }
    }
    return true;
}

/* Codec has completed decoding the track
   (usually Q_AUDIO_CODEC_COMPLETE) */
static void audio_on_codec_complete(int status)
{
    logf("%s(%d)", __func__, status);

    if (play_status == PLAY_STOPPED)
        return;

    /* If it didn't notify us first, don't expect "seek complete" message
       since the codec can't post it now - do things like it would have
       done */
    audio_complete_codec_seek();

    if (play_status == PLAY_PAUSED || skip_pending != TRACK_SKIP_NONE)
    {
        /* Old-hay on the ip-skay - codec has completed decoding

           Paused: We're not sounding it, so just remember that it happened
                   and the resume will begin the transition

           Skipping: There was already a skip in progress, remember it and
                     allow no further progress until the PCM from the previous
                     song has finished

           This function will be reentered upon completing the existing
           transition in order to do the one that was just tried (below)
         */
        codec_skip_pending = true;
        codec_skip_status = status;

        /* PCM buffer must know; audio could still be filling and hasn't
           yet reached the play watermark */
        pcmbuf_start_track_change(TRACK_CHANGE_AUTO_PILEUP);
        return;
    }

    codec_skip_pending = false;

    int trackstat = LOAD_TRACK_OK;

    track_event_flags = TEF_AUTO_SKIP;
    skip_pending = TRACK_SKIP_AUTO;

    int id3_hid = 0;
    if (audio_can_change_track(&trackstat, &id3_hid))
    {
        audio_begin_track_change(
                single_mode_do_pause(id3_hid)
                ? TRACK_CHANGE_END_OF_DATA
                : TRACK_CHANGE_AUTO, trackstat);
    }
}

/* Called when codec completes seek operation
   (usually Q_AUDIO_CODEC_SEEK_COMPLETE) */
static void audio_on_codec_seek_complete(void)
{
    logf("%s()", __func__);
    audio_complete_codec_seek();
    codec_go();
}

/* Called when PCM track change has completed
   (Q_AUDIO_TRACK_CHANGED) */
static void audio_on_track_changed(void)
{
    /* Finish whatever is pending so that the WPS is in sync */
    audio_finalise_track_change();

    if (codec_skip_pending)
    {
        /* Codec got ahead completing a short track - complete the
           codec's skip and begin the next */
        codec_skip_pending = false;
        audio_on_codec_complete(codec_skip_status);
    }
}

/* Begin playback from an idle state, transition to a new playlist or
   invalidate the buffer and resume (if playing).
   (usually Q_AUDIO_PLAY, Q_AUDIO_REMAKE_AUDIO_BUFFER) */
static void audio_start_playback(const struct audio_resume_info *resume_info,
                                 unsigned int flags)
{
    static struct audio_resume_info resume = { 0, 0 };
    enum play_status old_status = play_status;

    bool skip_resume_adjustments = false;
    if (resume_info)
    {
        resume.elapsed = resume_info->elapsed;
        resume.offset = resume_info->offset;
    }
    else
    {
        resume.elapsed = 0;
        resume.offset = 0;
    }

    if (flags & AUDIO_START_NEWBUF)
    {
        /* Mark the buffer dirty - if not playing, it will be reset next
           time */
        if (buffer_state == AUDIOBUF_STATE_INITIALIZED)
            buffer_state = AUDIOBUF_STATE_TRASHED;
    }

    if (old_status != PLAY_STOPPED)
    {
        logf("%s(%lu, %lu): skipping", __func__, resume.elapsed,
             resume.offset);

        halt_decoding_track(true);

        track_event_flags = TEF_NONE;
        ff_rw_mode = false;

        if (flags & AUDIO_START_RESTART)
        {
            /* Clear out some stuff to resume the current track where it
               left off */
            pcmbuf_play_stop();

            resume.elapsed = id3_get(PLAYING_ID3)->elapsed;
            resume.offset = id3_get(PLAYING_ID3)->offset;
            skip_resume_adjustments = id3_get(PLAYING_ID3)->skip_resume_adjustments;

            track_list_clear(TRACK_LIST_CLEAR_ALL);
            pcmbuf_update_frequency();
        }
        else
        {
            /* This is more-or-less treated as manual track transition */
            /* Save resume information for current track */
            audio_playlist_track_finish();
            track_list_clear(TRACK_LIST_CLEAR_ALL);

            /* Indicate manual track change */
            pcmbuf_start_track_change(TRACK_CHANGE_MANUAL);
            wipe_track_metadata(true);
        }
        pcmbuf_update_frequency();

        /* Set after track finish event in case skip was in progress */
        skip_pending = TRACK_SKIP_NONE;
    }
    else
    {
        if (flags & AUDIO_START_RESTART)
            return; /* Must already be playing */

        /* Cold playback start from a stopped state */
        logf("%s(%lu, %lu): starting", __func__, resume.elapsed,
             resume.offset);

        /* Set audio parameters */
#if INPUT_SRC_CAPS != 0
        audio_set_input_source(AUDIO_SRC_PLAYBACK, SRCF_PLAYBACK);
        audio_set_output_source(AUDIO_SRC_PLAYBACK);
#endif
#ifndef PLATFORM_HAS_VOLUME_CHANGE
        sound_set_volume(global_settings.volume);
#endif
        pcmbuf_update_frequency();

        /* Be sure channel is audible */
        pcmbuf_fade(false, true);

        /* Update our state */
        play_status = PLAY_PLAYING;
    }

    /* Codec's position should be available as soon as it knows it */
    position_key = pcmbuf_get_position_key();
    pcmbuf_sync_position_update();

    /* Start fill from beginning of playlist */
    playlist_peek_offset = -1;
    buf_set_base_handle(-1);

    /* Officially playing */
    queue_reply(&audio_queue, 1);

    /* Add these now - finish event for the first id3 will most likely be sent
       immediately */
    add_event(BUFFER_EVENT_REBUFFER, buffer_event_rebuffer_callback);
    add_event(BUFFER_EVENT_FINISHED, buffer_event_finished_callback);

    if (old_status == PLAY_STOPPED)
    {
        /* Send coldstart event */
        send_event(PLAYBACK_EVENT_START_PLAYBACK, NULL);
    }

    /* Fill the buffer */
    int trackstat = audio_fill_file_buffer();

    if (trackstat >= LOAD_TRACK_OK)
    {
        /* This is the currently playing track - get metadata, stat */
        struct track_info info;
        track_list_current(0, &info);
        playing_id3_sync(&info, &resume, skip_resume_adjustments);

        if (valid_mp3entry(id3_get(PLAYING_ID3)))
        {
            /* Only if actually changing tracks... */
            if (!(flags & AUDIO_START_RESTART))
                audio_playlist_track_change();
        }
    }
    else
    {
        /* Found nothing playable */
        audio_handle_track_load_status(trackstat);
    }
}

/* Stop playback and enter an idle state
   (usually Q_AUDIO_STOP) */
static void audio_stop_playback(void)
{
    logf("%s()", __func__);

    if (play_status == PLAY_STOPPED)
        return;

    bool do_fade = global_settings.fade_on_stop && filling != STATE_ENDED;

    pcmbuf_fade(do_fade, false);

    /* Wait for fade-out */
    audio_wait_fade_complete();

    /* Stop the codec and unload it */
    halt_decoding_track(true);
    pcmbuf_play_stop();
    codec_unload();

    /* Save resume information  - "filling" might have been set to
       "STATE_ENDED" by caller in order to facilitate end of playlist */
    audio_playlist_track_finish();

    skip_pending = TRACK_SKIP_NONE;
    track_event_flags = TEF_NONE;
    track_skip_is_manual = false;

    /* Close all tracks and mark them NULL */
    remove_event(BUFFER_EVENT_REBUFFER, buffer_event_rebuffer_callback);
    remove_event(BUFFER_EVENT_FINISHED, buffer_event_finished_callback);
    remove_event_ex(BUFFER_EVENT_BUFFER_LOW, buffer_event_buffer_low_callback, NULL);

    track_list_clear(TRACK_LIST_CLEAR_ALL);

    /* Update our state */
    ff_rw_mode = false;
    play_status = PLAY_STOPPED;

    wipe_track_metadata(true);
#ifdef HAVE_ALBUMART
    clear_last_folder_album_art();
#endif
    /* Go idle */
    filling = STATE_IDLE;
    cancel_cpu_boost();
}

/* Pause the playback of the current track
   (Q_AUDIO_PAUSE) */
static void audio_on_pause(bool pause)
{
    logf("%s(%s)", __func__, pause ? "true" : "false");

    if (play_status == PLAY_STOPPED || pause == (play_status == PLAY_PAUSED))
        return;

    play_status = pause ? PLAY_PAUSED : PLAY_PLAYING;

    if (!pause && codec_skip_pending)
    {
        /* Actually do the skip that is due - resets the status flag */
        audio_on_codec_complete(codec_skip_status);
    }

    bool do_fade = global_settings.fade_on_stop;

    pcmbuf_fade(do_fade, !pause);

    if (!ff_rw_mode && !(do_fade && pause))
    {
        /* Not in ff/rw mode - can actually change the audio state now */
        pcmbuf_pause(pause);
    }
}

/* Skip a certain number of tracks forwards or backwards
   (Q_AUDIO_SKIP) */
static void audio_on_skip(void)
{
    id3_mutex_lock();

    /* Eat the delta to keep it synced, even if not playing */
    int toskip = skip_offset;
    skip_offset = 0;

    logf("%s(): %d", __func__, toskip);

    id3_mutex_unlock();

    if (play_status == PLAY_STOPPED)
        return;

    /* Force codec to abort this track */
    halt_decoding_track(true);

    /* Kill the ff/rw halt */
    ff_rw_mode = false;

    /* Manual skip */
    track_event_flags = TEF_NONE;

    if (toskip == 1 && global_settings.repeat_mode == REPEAT_ONE)
    {
        audio_reset_and_rebuffer(TRACK_LIST_KEEP_CURRENT, 1);
    }

    /* If there was an auto skip in progress, there will be residual
       advancement of the playlist and/or track list so compensation will be
       required in order to end up in the right spot */
    int track_list_delta = toskip;
    int playlist_delta = toskip;

    if (skip_pending != TRACK_SKIP_NONE)
    {
        if (skip_pending != TRACK_SKIP_AUTO_END_PLAYLIST)
            track_list_delta--;

        if (skip_pending == TRACK_SKIP_AUTO_NEW_PLAYLIST)
            playlist_delta--;
    }

    audio_playlist_track_finish();
    skip_pending = TRACK_SKIP_NONE;

    /* Update the playlist current track now */
    int pl_retval;
    while ((pl_retval = playlist_next(playlist_delta)) < 0)
    {
        if (pl_retval < -1)
        {
            /* Some variety of fatal error while updating playlist */
            filling = STATE_ENDED;
            audio_stop_playback();
            return;
        }

        /* Manual skip out of range (because the playlist wasn't updated
           yet by us and so the check in audio_skip returned 'ok') - bring
           back into range */
        int d = toskip < 0 ? 1 : -1;

        while (!playlist_check(playlist_delta))
        {
            if (playlist_delta == d)
            {
                /* Had to move the opposite direction to correct, which is
                   wrong - this is the end */
                filling = STATE_ENDED;
                audio_stop_playback();
                return;
            }

            playlist_delta += d;
            track_list_delta += d;
        }
    }
    track_skip_is_manual = false;
    /* Adjust things by how much the playlist was manually moved */
    playlist_peek_offset -= playlist_delta;

    int trackstat = LOAD_TRACK_OK;

    struct track_info info;
    if (!track_list_advance_current(track_list_delta, &info)
        || info.audio_hid < 0)
    {
        /* We don't know the next track thus we know we don't have it */
        trackstat = audio_reset_and_rebuffer(TRACK_LIST_CLEAR_ALL, -1);
    }

    audio_begin_track_change(TRACK_CHANGE_MANUAL, trackstat);
}

/* Skip to the next/previous directory
   (Q_AUDIO_DIR_SKIP) */
static void audio_on_dir_skip(int direction)
{
    logf("%s(%d)", __func__, direction);

    id3_mutex_lock();
    skip_offset = 0;
    id3_mutex_unlock();

    if (play_status == PLAY_STOPPED)
        return;

    /* Force codec to abort this track */
    halt_decoding_track(true);

    /* Kill the ff/rw halt */
    ff_rw_mode = false;

    /* Manual skip */
    track_event_flags = TEF_NONE;

    audio_playlist_track_finish();

    /* Unless automatic and gapless, skips do not pend */
    skip_pending = TRACK_SKIP_NONE;

    /* Regardless of the return value we need to rebuffer. If it fails the old
       playlist will resume, else the next dir will start playing. */
    playlist_next_dir(direction);

    wipe_track_metadata(false);

    int trackstat = audio_reset_and_rebuffer(TRACK_LIST_CLEAR_ALL, -1);

    if (trackstat == LOAD_TRACK_ERR_NO_MORE)
    {
        /* The day the music died - finish-off whatever is playing and call it
           quits */
        audio_monitor_end_of_playlist();
        return;
    }

    audio_begin_track_change(TRACK_CHANGE_MANUAL, trackstat);
}

/* Enter seek mode in order to start a seek
   (Q_AUDIO_PRE_FF_REWIND) */
static void audio_on_pre_ff_rewind(void)
{
    logf("%s()", __func__);

    if (play_status == PLAY_STOPPED || ff_rw_mode)
        return;

    ff_rw_mode = true;

    audio_wait_fade_complete();

    if (play_status == PLAY_PAUSED)
        return;

    pcmbuf_pause(true);
}

/* Seek the playback of the current track to the specified time
   (Q_AUDIO_FF_REWIND) */
static void audio_on_ff_rewind(long time)
{
    logf("%s(%ld)", __func__, time);

    if (play_status == PLAY_STOPPED)
        return;

    enum track_skip_type pending = skip_pending;

    switch (pending)
    {
    case TRACK_SKIP_NONE:              /* The usual case */
    case TRACK_SKIP_AUTO:              /* Have to back it out (fun!) */
    case TRACK_SKIP_AUTO_END_PLAYLIST: /* Still have the last codec used */
    {
        struct mp3entry *id3 = id3_get(PLAYING_ID3);
        struct mp3entry *ci_id3 = id3_get(CODEC_ID3);

        track_event_flags = TEF_NONE;

        if (time < 0)
        {
            time = id3->length + time;
            if (time < 0)
                time = 0;
        }
        /* Send event before clobbering the time if rewinding. */
        if (time == 0)
        {
            send_track_event(PLAYBACK_EVENT_TRACK_FINISH,
                             track_event_flags | TEF_REWIND, id3);
        }

        id3->elapsed = time;
        queue_reply(&audio_queue, 1);

        bool haltres = halt_decoding_track(pending == TRACK_SKIP_AUTO);

        /* Need this set in case ff/rw mode + error but _after_ the codec
           halt that will reset it */
        codec_seeking = true;

        /* If in transition, key will have changed - sync to it */
        position_key = pcmbuf_get_position_key();

        if (pending == TRACK_SKIP_AUTO && !track_list_advance_current(-1, NULL))
        {
            /* Not in list - must rebuffer at the current playlist index */
            if (audio_reset_and_rebuffer(TRACK_LIST_CLEAR_ALL, -1)
                    < LOAD_TRACK_OK)
            {
                /* Codec is stopped */
                break;
            }
        }

        /* Set after audio_fill_file_buffer to disable playing id3 clobber if
           rebuffer is needed */
        skip_pending = TRACK_SKIP_NONE;

        struct track_info cur_info;
        track_list_current(0, &cur_info);

        bool finish_load = cur_info.audio_hid < 0;
        if (finish_load)
        {
            // track is not yet loaded so simply update resume details for upcoming finish_load_track and quit
            playing_id3_sync(&cur_info, &(struct audio_resume_info){ time, 0 }, true);
            return;
        }

        if (pending == TRACK_SKIP_AUTO)
        {
            if (!bufreadid3(cur_info.id3_hid, ci_id3) ||
                !audio_init_codec(&cur_info, ci_id3))
            {
                /* We should have still been able to get it - skip it and move
                   onto the next one - like it or not this track is broken */
                break;
            }

            /* Set the codec API to the correct metadata and track info */
            ci.audio_hid = cur_info.audio_hid;
            ci.filesize = buf_filesize(cur_info.audio_hid);
            buf_set_base_handle(cur_info.audio_hid);
        }

        if (!haltres)
        {
            /* If codec must be (re)started, reset the resume info so that
               it doesn't execute resume procedures */
            ci_id3->elapsed = 0;
            ci_id3->offset = 0;
        }

        codec_seek(time);
        return;
        }

    case TRACK_SKIP_AUTO_NEW_PLAYLIST:
    {
        /* We cannot do this because the playlist must be reversed by one
           and it doesn't always return the same song when going backwards
           across boundaries as forwards (either because of randomization
           or inconsistency in deciding what the previous track should be),
           therefore the whole operation would often end up as nonsense -
           lock out seeking for a couple seconds */

        /* Sure as heck cancel seek mode too! */
        audio_ff_rewind_end();
        return;
        }

    default:
        /* Won't see this */
        return;
    }

    if (play_status == PLAY_STOPPED)
    {
        /* Playback ended because of an error completing a track load */
        return;
    }

    /* Always fake it as a codec start error which will handle mode
       cancellations and skip to the next track */
    audio_handle_track_load_status(LOAD_TRACK_ERR_START_CODEC);
}

/* Invalidates all but currently playing track
   (Q_AUDIO_FLUSH) */
static void audio_on_audio_flush(void)
{
    logf("%s", __func__);

    if (track_list_empty())
        return; /* Nothing to flush out */

    switch (skip_pending)
    {
    case TRACK_SKIP_NONE:
    case TRACK_SKIP_AUTO_END_PLAYLIST:
        /* Remove all but the currently playing track from the list and
           refill after that */
        track_list_clear(TRACK_LIST_KEEP_CURRENT);
        playlist_peek_offset = 0;
        id3_write_locked(UNBUFFERED_ID3, NULL);
        audio_update_and_announce_next_track(NULL);

        /* Ignore return since it's about the next track, not this one */
        audio_fill_file_buffer();

        if (skip_pending == TRACK_SKIP_NONE)
            break;

        /* There's now a track after this one now - convert to auto skip -
           no skip should pend right now because multiple flush messages can
           be fired which would cause a restart in the below cases */
        skip_pending = TRACK_SKIP_NONE;
        audio_clear_track_notifications();
        audio_queue_post(Q_AUDIO_CODEC_COMPLETE, CODEC_OK);
        break;

    case TRACK_SKIP_AUTO:
    case TRACK_SKIP_AUTO_NEW_PLAYLIST:
        /* Precisely removing what it already decoded for the next track is
           not possible so a restart is required in order to continue the
           currently playing track without the now invalid future track
           playing */
        audio_start_playback(NULL, AUDIO_START_RESTART);
        break;

    default: /* Nothing else is a state */
        break;
    }
}

/* Called by audio thread when playback is started */
void audio_playback_handler(struct queue_event *ev)
{
    while (1)
    {
        switch (ev->id)
        {
        /** Codec and track change messages **/
        case Q_AUDIO_CODEC_COMPLETE:
            /* Codec is done processing track and has gone idle */
            LOGFQUEUE("playback < Q_AUDIO_CODEC_COMPLETE: %ld",
                      (long)ev->data);
            audio_on_codec_complete(ev->data);
            break;

        case Q_AUDIO_CODEC_SEEK_COMPLETE:
            /* Codec is done seeking */
            LOGFQUEUE("playback < Q_AUDIO_SEEK_COMPLETE");
            audio_on_codec_seek_complete();
            break;

        case Q_AUDIO_TRACK_CHANGED:
            /* PCM track change done */
            LOGFQUEUE("playback < Q_AUDIO_TRACK_CHANGED");
            audio_on_track_changed();
            break;

        /** Control messages **/
        case Q_AUDIO_PLAY:
            LOGFQUEUE("playback < Q_AUDIO_PLAY");
            audio_start_playback((struct audio_resume_info *)ev->data, 0);
            break;

#ifdef HAVE_RECORDING
        /* So we can go straight from playback to recording */
        case Q_AUDIO_INIT_RECORDING:
#endif
        case SYS_USB_CONNECTED:
        case Q_AUDIO_STOP:
            LOGFQUEUE("playback < Q_AUDIO_STOP");
            audio_stop_playback();
            if (ev->data != 0)
                queue_clear(&audio_queue);
            return; /* no more playback */

        case Q_AUDIO_PAUSE:
            LOGFQUEUE("playback < Q_AUDIO_PAUSE");
            audio_on_pause(ev->data);
            break;

        case Q_AUDIO_SKIP:
            LOGFQUEUE("playback < Q_AUDIO_SKIP");
            audio_on_skip();
            break;

        case Q_AUDIO_DIR_SKIP:
            LOGFQUEUE("playback < Q_AUDIO_DIR_SKIP");
            audio_on_dir_skip(ev->data);
            break;

        case Q_AUDIO_PRE_FF_REWIND:
            LOGFQUEUE("playback < Q_AUDIO_PRE_FF_REWIND");
            audio_on_pre_ff_rewind();
            break;

        case Q_AUDIO_FF_REWIND:
            LOGFQUEUE("playback < Q_AUDIO_FF_REWIND");
            audio_on_ff_rewind(ev->data);
            break;

        case Q_AUDIO_FLUSH:
            LOGFQUEUE("playback < Q_AUDIO_FLUSH: %d", (int)ev->data);
            audio_on_audio_flush();
            break;

        /** Buffering messages **/
        case Q_AUDIO_BUFFERING:
            /* some buffering event */
            LOGFQUEUE("playback < Q_AUDIO_BUFFERING: %d", (int)ev->data);
            audio_on_buffering(ev->data);
            break;

        case Q_AUDIO_FILL_BUFFER:
            /* continue buffering next track */
            LOGFQUEUE("playback < Q_AUDIO_FILL_BUFFER");
            audio_on_fill_buffer();
            break;

        case Q_AUDIO_FINISH_LOAD_TRACK:
            /* metadata is buffered */
            LOGFQUEUE("playback < Q_AUDIO_FINISH_LOAD_TRACK");
            audio_on_finish_load_track(ev->data);
            break;

        case Q_AUDIO_HANDLE_FINISHED:
            /* some other type is buffered */
            LOGFQUEUE("playback < Q_AUDIO_HANDLE_FINISHED");
            audio_on_handle_finished(ev->data);
            break;

        /** Miscellaneous messages **/
        case Q_AUDIO_REMAKE_AUDIO_BUFFER:
            /* buffer needs to be reinitialized */
            LOGFQUEUE("playback < Q_AUDIO_REMAKE_AUDIO_BUFFER");
            audio_start_playback(NULL, AUDIO_START_RESTART | AUDIO_START_NEWBUF);
            if (play_status == PLAY_STOPPED)
                return; /* just need to change buffer state */
            break;

#ifdef HAVE_DISK_STORAGE
        case Q_AUDIO_UPDATE_WATERMARK:
            /* buffering watermark needs updating */
            LOGFQUEUE("playback < Q_AUDIO_UPDATE_WATERMARK: %d",
                      (int)ev->data);
            audio_update_filebuf_watermark(ev->data);
            if (play_status == PLAY_STOPPED)
                return; /* just need to update setting */
            break;
#endif /* HAVE_DISK_STORAGE */

        case SYS_TIMEOUT:
            LOGFQUEUE_SYS_TIMEOUT("playback < SYS_TIMEOUT");
            break;

        default:
            /* LOGFQUEUE("audio < default : %08lX", ev->id); */
            break;
        } /* end switch */

        switch (filling)
        {
        /* Active states */
        case STATE_FULL:
        case STATE_END_OF_PLAYLIST:
            if (buf_get_watermark() == 0)
            {
                /* End of buffering for now, let's calculate the watermark,
                   register for a low buffer event and unboost */
                audio_update_filebuf_watermark(0);
                add_event_ex(BUFFER_EVENT_BUFFER_LOW, true,
                        buffer_event_buffer_low_callback, NULL);
            }
            /* Fall-through */
        case STATE_FINISHED:
        case STATE_STOPPED:
            /* All data was buffered */
            cancel_cpu_boost();
            /* Fall-through */
        case STATE_FILLING:
        case STATE_ENDING:
            if (audio_pcmbuf_track_change_scan())
            {
                /* Transfer notification to audio queue event */
                ev->id = Q_AUDIO_TRACK_CHANGED;
                ev->data = 1;
            }
            else
            {
                /* If doing auto skip, poll pcmbuf track notifications a bit
                   faster to promply detect the transition */
                queue_wait_w_tmo(&audio_queue, ev,
                    skip_pending == TRACK_SKIP_NONE ? HZ/2 : HZ/10);
            }
            break;

        /* Idle states */
        default:
            queue_wait(&audio_queue, ev);
        }
    } /* end while */
}


/* --- Buffering callbacks --- */

/* Called when fullness is below the watermark level */
static void buffer_event_buffer_low_callback(unsigned short id, void *ev_data, void *user_data)
{
    logf("low buffer callback");
    LOGFQUEUE("buffering > audio Q_AUDIO_BUFFERING: buffer low");
    audio_queue_post(Q_AUDIO_BUFFERING, BUFFER_EVENT_BUFFER_LOW);
    (void)id;
    (void)ev_data;
    (void)user_data;
}

/* Called when handles must be discarded in order to buffer new data */
static void buffer_event_rebuffer_callback(unsigned short id, void *ev_data)
{
    logf("rebuffer callback");
    LOGFQUEUE("buffering > audio Q_AUDIO_BUFFERING: rebuffer");
    audio_queue_post(Q_AUDIO_BUFFERING, BUFFER_EVENT_REBUFFER);
    (void)id;
    (void)ev_data;
}

/* A handle has completed buffering and all required data is available */
static void buffer_event_finished_callback(unsigned short id, void *ev_data)
{
    (void)id;
    int hid = *(const int *)ev_data;
    int htype = buf_handle_data_type(hid);

    logf("handle %d finished buffering (type:%d)", hid, htype);

    /* Limit queue traffic */
    switch (htype)
    {
    case TYPE_ID3:
        /* The metadata handle for the last loaded track has been buffered.
           We can ask the audio thread to load the rest of the track's data. */
        LOGFQUEUE("buffering > audio Q_AUDIO_FINISH_LOAD_TRACK: %d", hid);
        audio_queue_post(Q_AUDIO_FINISH_LOAD_TRACK, hid);
        break;

    case TYPE_PACKET_AUDIO:
    case TYPE_ATOMIC_AUDIO:
        LOGFQUEUE("buffering > audio Q_AUDIO_HANDLE_FINISHED: %d", hid);
        audio_queue_post(Q_AUDIO_HANDLE_FINISHED, hid);
        break;

    default:
        /* Don't care to know about these */
        break;
    }
}


/** -- Codec callbacks -- **/

/* Update elapsed time for next PCM insert */
void audio_codec_update_elapsed(unsigned long elapsed)
{
    ab_position_report(elapsed);

    /* Save in codec's id3 where it is used at next pcm insert */
    id3_get(CODEC_ID3)->elapsed = elapsed;
}

/* Update offset for next PCM insert */
void audio_codec_update_offset(size_t offset)
{
    /* Save in codec's id3 where it is used at next pcm insert */
    id3_get(CODEC_ID3)->offset = offset;
}

/* Codec has finished running */
void audio_codec_complete(int status)
{
#ifdef AB_REPEAT_ENABLE
    if (status >= CODEC_OK)
    {
        /* Normal automatic skip */
        ab_end_of_track_report();
    }
#endif

    LOGFQUEUE("codec > audio Q_AUDIO_CODEC_COMPLETE: %d", status);
    audio_queue_post(Q_AUDIO_CODEC_COMPLETE, status);
}

/* Codec has finished seeking */
void audio_codec_seek_complete(void)
{
    LOGFQUEUE("codec > audio Q_AUDIO_CODEC_SEEK_COMPLETE");
    audio_queue_post(Q_AUDIO_CODEC_SEEK_COMPLETE, 0);
}


/** --- Pcmbuf callbacks --- **/

/* Update the elapsed and offset from the information cached during the
   PCM buffer insert */
void audio_pcmbuf_position_callback(unsigned long elapsed, off_t offset,
                                    unsigned int key)
{
    if (key == position_key)
    {
        struct mp3entry *id3 = id3_get(PLAYING_ID3);
        id3->elapsed = elapsed;
        id3->offset = offset;
    }
}

/* Synchronize position info to the codec's */
void audio_pcmbuf_sync_position(void)
{
    audio_pcmbuf_position_callback(ci.id3->elapsed, ci.id3->offset,
                                   pcmbuf_get_position_key());
}

/* Post message from pcmbuf that the end of the previous track has just
 * been played */
void audio_pcmbuf_track_change(bool pcmbuf)
{
    if (pcmbuf)
    {
        /* Notify of the change in special-purpose semaphore object */
        LOGFQUEUE("pcmbuf > pcmbuf Q_AUDIO_TRACK_CHANGED");
        audio_pcmbuf_track_change_post();
    }
    else
    {
        /* Safe to post directly to the queue */
        LOGFQUEUE("pcmbuf > audio Q_AUDIO_TRACK_CHANGED");
        audio_queue_post(Q_AUDIO_TRACK_CHANGED, 0);
    }
}

/* May pcmbuf start PCM playback when the buffer is full enough? */
bool audio_pcmbuf_may_play(void)
{
    return play_status == PLAY_PLAYING && !ff_rw_mode;
}


/** -- External interfaces -- **/

/* Get a copy of the id3 data for the for current track + offset + skip delta */
bool audio_peek_track(struct mp3entry *id3, int offset)
{
    bool retval = false;

    id3_mutex_lock();

    if (play_status != PLAY_STOPPED)
    {
        id3->path[0] = '\0'; /* Null path means it should be filled now */
        retval = audio_get_track_metadata(offset + skip_offset, id3) &&
                id3->path[0] != '\0';
    }

    id3_mutex_unlock();

    return retval;
}

/* Return the mp3entry for the currently playing track */
struct mp3entry * audio_current_track(void)
{
    struct mp3entry *id3;

    id3_mutex_lock();

#ifdef AUDIO_FAST_SKIP_PREVIEW
    if (skip_offset != 0)
    {
        /* This is a peekahead */
        id3 = id3_get(PLAYING_PEEK_ID3);
        audio_peek_track(id3, 0);
    }
    else
#endif
    {
        /* Normal case */
        id3 = id3_get(PLAYING_ID3);
        audio_get_track_metadata(0, id3);
    }

    id3_mutex_unlock();

    return id3;
}

/* Obtains the mp3entry for the next track from the current */
struct mp3entry * audio_next_track(void)
{
    struct mp3entry *id3 = id3_get(NEXTTRACK_ID3);

    id3_mutex_lock();

#ifdef AUDIO_FAST_SKIP_PREVIEW
    if (skip_offset != 0)
    {
        /* This is a peekahead */
        if (!audio_peek_track(id3, 1))
            id3 = NULL;
    }
    else
#endif
    {
        /* Normal case */
        if (!audio_get_track_metadata(1, id3))
            id3 = NULL;
    }

    id3_mutex_unlock();

    return id3;
}

/* Start playback at the specified elapsed time or offset */
void audio_play(unsigned long elapsed, unsigned long offset)
{
    logf("audio_play");

#ifdef PLAYBACK_VOICE
    /* Truncate any existing voice output so we don't have spelling
     * etc. over the first part of the played track */
    talk_force_shutup();
#endif

    LOGFQUEUE("audio >| audio Q_AUDIO_PLAY: %lu %lX", elapsed, offset);
    audio_queue_send(Q_AUDIO_PLAY,
                     (intptr_t)&(struct audio_resume_info){ elapsed, offset });
}

/* Stop playback if playing */
void audio_stop(void)
{
    LOGFQUEUE("audio >| audio Q_AUDIO_STOP");
    audio_queue_send(Q_AUDIO_STOP, 0);
}

/* Pause playback if playing */
void audio_pause(void)
{
    LOGFQUEUE("audio >| audio Q_AUDIO_PAUSE");
    audio_queue_send(Q_AUDIO_PAUSE, true);
}

/* This sends a stop message and the audio thread will dump all its
   subsequent messages */
void audio_hard_stop(void)
{
    /* Stop playback */
    LOGFQUEUE("audio >| audio Q_AUDIO_STOP: 1");
    audio_queue_send(Q_AUDIO_STOP, 1);
#ifdef PLAYBACK_VOICE
    voice_stop();
#endif
    audiobuf_handle = core_free(audiobuf_handle);
}

/* Resume playback if paused */
void audio_resume(void)
{
    LOGFQUEUE("audio >| audio Q_AUDIO_PAUSE resume");
    audio_queue_send(Q_AUDIO_PAUSE, false);
}

/* Internal function used by REPEAT_ONE extern playlist.c */
bool audio_pending_track_skip_is_manual(void)
{
    logf("Track change is: %s", track_skip_is_manual ? "Manual": "Auto");
    return track_skip_is_manual;
}

/* Skip the specified number of tracks forward or backward from the current */
void audio_skip(int offset)
{
    id3_mutex_lock();

    /* If offset has to be backed-out to stay in range, no skip is done */
    int accum = skip_offset + offset;

    while (offset != 0 && !playlist_check(accum))
    {
        offset += offset < 0 ? 1 : -1;
        accum = skip_offset + offset;
    }

    if (offset != 0)
    {
        /* Accumulate net manual skip count since the audio thread last
           processed one */
        skip_offset = accum;

        system_sound_play(SOUND_TRACK_SKIP);

        LOGFQUEUE("audio > audio Q_AUDIO_SKIP %d", offset);

#ifdef AUDIO_FAST_SKIP_PREVIEW
        /* Do this before posting so that the audio thread can correct us
           when things settle down - additionally, if audio gets a message
           and the delta is zero, the Q_AUDIO_SKIP handler (audio_on_skip)
           handler a skip event with the correct info but doesn't skip */
        send_event(PLAYBACK_EVENT_TRACK_SKIP, NULL);
#endif /* AUDIO_FAST_SKIP_PREVIEW */

        /* Playback only needs the final state even if more than one is
           processed because it wasn't removed in time */
        queue_remove_from_head(&audio_queue, Q_AUDIO_SKIP);
        audio_queue_post(Q_AUDIO_SKIP, 0);
    }
    else
    {
        /* No more tracks */
        system_sound_play(SOUND_TRACK_NO_MORE);
    }

    id3_mutex_unlock();
}

/* Skip one track forward from the current */
void audio_next(void)
{
    audio_skip(1);
}

/* Skip one track backward from the current */
void audio_prev(void)
{
    audio_skip(-1);
}

/* Move one directory forward */
void audio_next_dir(void)
{
    LOGFQUEUE("audio > audio Q_AUDIO_DIR_SKIP 1");
    audio_queue_post(Q_AUDIO_DIR_SKIP, 1);
}

/* Move one directory backward */
void audio_prev_dir(void)
{
    LOGFQUEUE("audio > audio Q_AUDIO_DIR_SKIP -1");
    audio_queue_post(Q_AUDIO_DIR_SKIP, -1);
}

/* Pause playback in order to start a seek that flushes the old audio */
void audio_pre_ff_rewind(void)
{
    LOGFQUEUE("audio > audio Q_AUDIO_PRE_FF_REWIND");
    audio_queue_post(Q_AUDIO_PRE_FF_REWIND, 0);
}

/* Seek to the new time in the current track */
void audio_ff_rewind(long time)
{
    LOGFQUEUE("audio > audio Q_AUDIO_FF_REWIND");
    audio_queue_post(Q_AUDIO_FF_REWIND, time);
}

/* Clear all but the currently playing track then rebuffer */
void audio_flush_and_reload_tracks(void)
{
    LOGFQUEUE("audio > audio Q_AUDIO_FLUSH");
    audio_queue_post(Q_AUDIO_FLUSH, 0);
}

/** --- Miscellaneous public interfaces --- **/

#ifdef HAVE_ALBUMART
/* Return which album art handle is current for the user in the given slot */
int playback_current_aa_hid(int slot)
{
    if ((unsigned)slot < MAX_MULTIPLE_AA)
    {
        struct track_info user_cur;
        bool have_info = track_list_user_current(skip_offset, &user_cur);

        if (!have_info && abs(skip_offset) <= 1)
        {
            /* Give the actual position a go */
            have_info = track_list_user_current(0, &user_cur);
        }

        if (have_info)
            return user_cur.aa_hid[slot];
    }

    return ERR_HANDLE_NOT_FOUND;
}

/* Find an album art slot that doesn't match the dimensions of another that
   is already claimed - increment the use count if it is */
int playback_claim_aa_slot(struct dim *dim)
{
    /* First try to find a slot already having the size to reuse it since we
       don't want albumart of the same size buffered multiple times */
    FOREACH_ALBUMART(i)
    {
        struct albumart_slot *slot = &albumart_slots[i];

        if (slot->dim.width == dim->width &&
            slot->dim.height == dim->height)
        {
            slot->used++;
            return i;
        }
    }

    /* Size is new, find a free slot */
    FOREACH_ALBUMART(i)
    {
        if (!albumart_slots[i].used)
        {
            albumart_slots[i].used++;
            albumart_slots[i].dim = *dim;
            return i;
        }
    }

    /* Sorry, no free slot */
    return -1;
}

/* Invalidate the albumart_slot - decrement the use count if > 0 */
void playback_release_aa_slot(int slot)
{
    if ((unsigned)slot < MAX_MULTIPLE_AA)
    {
        struct albumart_slot *aa_slot = &albumart_slots[slot];

        if (aa_slot->used > 0)
            aa_slot->used--;
    }
}

void playback_update_aa_dims(void)
{
    LOGFQUEUE("audio >| audio Q_AUDIO_REMAKE_AUDIO_BUFFER");
    audio_queue_send(Q_AUDIO_REMAKE_AUDIO_BUFFER, 0);
}
#endif /* HAVE_ALBUMART */

/* Return file byte offset */
int audio_get_file_pos(void)
{
    return id3_get(PLAYING_ID3)->offset;
}

/* Return total file buffer length after accounting for the talk buf */
size_t audio_get_filebuflen(void)
{
    return buf_length();
}

/* How many tracks exist on the buffer - full or partial */
unsigned int audio_track_count(void)
    __attribute__((alias("track_list_count")));

/* Return total ringbuffer space occupied - ridx to widx */
long audio_filebufused(void)
{
    return buf_used();
}


/** -- Settings -- **/

/* Enable or disable cuesheet support and allocate/don't allocate the
   extra associated resources */
void audio_set_cuesheet(bool enable)
{
    if (play_status == PLAY_STOPPED || !enable != !get_current_cuesheet())
    {
        LOGFQUEUE("audio >| audio Q_AUDIO_REMAKE_AUDIO_BUFFER");
        audio_queue_send(Q_AUDIO_REMAKE_AUDIO_BUFFER, 0);
    }
}

#ifdef HAVE_DISK_STORAGE
/* Set the audio antiskip buffer margin in SECONDS */
void audio_set_buffer_margin(int seconds)
{
    logf("buffer margin: %u", (unsigned) seconds);
    LOGFQUEUE("audio > audio Q_AUDIO_UPDATE_WATERMARK: %u",(unsigned) seconds);
    audio_queue_post(Q_AUDIO_UPDATE_WATERMARK, (unsigned) seconds); /*SECONDS*/
}
#endif /* HAVE_DISK_STORAGE */

#ifdef HAVE_CROSSFADE
/* Take necessary steps to enable or disable the crossfade setting */
void audio_set_crossfade(int enable)
{
    /* Tell it the next setting to use */
    pcmbuf_request_crossfade_enable(enable);

    /* Return if size hasn't changed or this is too early to determine
       which in the second case there's no way we could be playing
       anything at all */
    if (!pcmbuf_is_same_size())
    {
        LOGFQUEUE("audio >| audio Q_AUDIO_REMAKE_AUDIO_BUFFER");
        audio_queue_send(Q_AUDIO_REMAKE_AUDIO_BUFFER, 0);
    }
}
#endif /* HAVE_CROSSFADE */

#ifdef HAVE_PLAY_FREQ
static unsigned long audio_guess_frequency(struct mp3entry *id3)
{
    switch (id3->frequency)
    {
#if HAVE_PLAY_FREQ >= 48
    case 44100:
        return SAMPR_44;
    case 48000:
        return SAMPR_48;
#endif
#if HAVE_PLAY_FREQ >= 96
    case 88200:
        return SAMPR_88;
    case 96000:
        return SAMPR_96;
#endif
#if HAVE_PLAY_FREQ >= 192
    case 176400:
        return SAMPR_176;
    case 192000:
        return SAMPR_192;
#endif
    default:
        return (id3->frequency % 4000) ? SAMPR_44 : SAMPR_48;
    }
}

static bool audio_auto_change_frequency(struct mp3entry *id3, bool play)
{
    if (!valid_mp3entry(id3))
        return false;
    unsigned long guessed_frequency = global_settings.play_frequency == 0 ? audio_guess_frequency(id3) : 0;
    if (guessed_frequency && mixer_get_frequency() != guessed_frequency)
    {
        if (!play)
            return true;

#ifdef PLAYBACK_VOICE
        voice_stop();
#endif
        mixer_set_frequency(guessed_frequency);
        audio_queue_post(Q_AUDIO_REMAKE_AUDIO_BUFFER, 0);
        return true;
    }
    return false;
}

void audio_set_playback_frequency(unsigned int sample_rate_hz)
{
    /* sample_rate_hz == 0 is "automatic", and also a sentinel */
#if HAVE_PLAY_FREQ >= 192
    static const unsigned int play_sampr[] = {SAMPR_44, SAMPR_48, SAMPR_88, SAMPR_96, SAMPR_176, SAMPR_192, 0 };
#elif HAVE_PLAY_FREQ >= 96
    static const unsigned int play_sampr[] = {SAMPR_44, SAMPR_48, SAMPR_88, SAMPR_96, 0 };
#elif HAVE_PLAY_FREQ >= 48
    static const unsigned int play_sampr[] = {SAMPR_44, SAMPR_48, 0 };
#else
    #error "HAVE_PLAY_FREQ < 48 ??"
#endif
    const unsigned int *p_sampr = play_sampr;
    unsigned int sampr = 0;

    while (*p_sampr != 0)
    {
        if (*p_sampr == sample_rate_hz)
        {
            sampr = *p_sampr;
            break;
        }
        p_sampr++;
    }

    if (sampr == 0)
    {
        if (audio_status() & (AUDIO_STATUS_PLAY | AUDIO_STATUS_PAUSE))
        {
            sampr = audio_guess_frequency(audio_current_track());
        }
        else
        {
            logf("could not set sample rate to %u hz", sample_rate_hz);
            return; /* leave as is */
        }
    }

    if (sampr != mixer_get_frequency())
    {
        mixer_set_frequency(sampr);
        LOGFQUEUE("audio >| audio Q_AUDIO_REMAKE_AUDIO_BUFFER");
        audio_queue_send(Q_AUDIO_REMAKE_AUDIO_BUFFER, 0);
    }
}
#endif /* HAVE_PLAY_FREQ */

unsigned int playback_status(void)
{
    return play_status;
}

/** -- Startup -- **/
void INIT_ATTR playback_init(void)
{
    logf("playback: initializing");

    /* Initialize the track buffering system */
    mutex_init(&id3_mutex);
    track_list_init();
    buffering_init();
    pcmbuf_update_frequency();
#ifdef HAVE_CROSSFADE
    /* Set crossfade setting for next buffer init which should be about... */
    pcmbuf_request_crossfade_enable(global_settings.crossfade);
#endif
#ifdef HAVE_DISK_STORAGE
    audio_set_buffer_margin(global_settings.buffer_margin);
#endif
}
