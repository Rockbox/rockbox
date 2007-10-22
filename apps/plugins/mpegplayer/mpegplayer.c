/*
 * mpegplayer.c - based on :
 *        - mpeg2dec.c
 *        - m2psd.c (http://www.brouhaha.com/~eric/software/m2psd/)
 *
 * Copyright (C) 2000-2003 Michel Lespinasse <walken@zoy.org>
 * Copyright (C) 1999-2000 Aaron Holtzman <aholtzma@ess.engr.uvic.ca>
 *
 * m2psd: MPEG 2 Program Stream Demultiplexer
 * Copyright (C) 2003 Eric Smith <eric@brouhaha.com>
 *
 * This file is part of mpeg2dec, a free MPEG-2 video stream decoder.
 * See http://libmpeg2.sourceforge.net/ for updates.
 *
 * mpeg2dec is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * mpeg2dec is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/*

NOTES:

mpegplayer is structured as follows:

1) Video thread (running on the COP for PortalPlayer targets).
2) Audio thread (running on the main CPU to maintain consistency with
   the audio FIQ hander on PP).
3) The main thread which takes care of buffering.

Using the main thread for buffering wastes the 8KB main stack which is
in IRAM.  However, 8KB is not enough for the audio thread to run (it
needs somewhere between 8KB and 9KB), so we create a new thread in
order to`give it a larger stack and steal the core codec thread's
stack (9KB of precious IRAM).

The button loop (and hence pause/resume, main menu and, in the future,
seeking) is placed in the audio thread.  This keeps it on the main CPU
in PP targets and also allows buffering to continue in the background
whilst the main thread is filling the buffer.

A/V sync is not yet implemented but is planned to be achieved by
syncing the master clock with the audio, and then (as is currently
implemented), syncing video with the master clock.  This can happen in
the audio thread, along with resyncing after pause.

Seeking should probably happen in the main thread, as that's where the
buffering happens.

On PortalPlayer targets, the main CPU is not being fully utilised -
the bottleneck is the video decoding on the COP.  One way to improve
that might be to move the rendering of the frames (i.e. the
lcd_yuv_blit() call) from the COP back to the main CPU.  Ideas and
patches for that are welcome!

Notes about MPEG files:

MPEG System Clock is 27MHz - i.e. 27000000 ticks/second.

FPS is represented in terms of a frame period - this is always an
integer number of 27MHz ticks.

e.g. 29.97fps (30000/1001) NTSC video has an exact frame period of
900900 27MHz ticks.

In libmpeg2, info->sequence->frame_period contains the frame_period.

Working with Rockbox's 100Hz tick, the common frame rates would need
to be as follows:

FPS     | 27Mhz   | 100Hz          | 44.1KHz   | 48KHz
--------|-----------------------------------------------------------
10*     | 2700000 | 10             | 4410      | 4800
12*     | 2250000 |  8.3333        | 3675      | 4000
15*     | 1800000 |  6.6667        | 2940      | 3200
23.9760 | 1126125 |  4.170833333   | 1839.3375 | 2002
24      | 1125000 |  4.166667      | 1837.5    | 2000
25      | 1080000 |  4             | 1764      | 1920
29.9700 |  900900 |  3.336667      | 1471,47   | 1601.6
30      |  900000 |  3.333333      | 1470      | 1600


*Unofficial framerates

*/


#include "mpeg2dec_config.h"

#include "plugin.h"
#include "gray.h"
#include "helper.h"

#include "mpeg2.h"
#include "mpeg_settings.h"
#include "video_out.h"
#include "../../codecs/libmad/mad.h"

PLUGIN_HEADER
PLUGIN_IRAM_DECLARE

/* button definitions */
#if (CONFIG_KEYPAD == IRIVER_H100_PAD) || (CONFIG_KEYPAD == IRIVER_H300_PAD)
#define MPEG_MENU       BUTTON_MODE
#define MPEG_STOP       BUTTON_OFF
#define MPEG_PAUSE      BUTTON_ON
#define MPEG_VOLDOWN    BUTTON_DOWN
#define MPEG_VOLUP      BUTTON_UP

#elif (CONFIG_KEYPAD == IPOD_4G_PAD) || (CONFIG_KEYPAD == IPOD_3G_PAD) || \
      (CONFIG_KEYPAD == IPOD_1G2G_PAD)
#define MPEG_MENU       BUTTON_MENU
#define MPEG_PAUSE      (BUTTON_PLAY | BUTTON_REL)
#define MPEG_STOP       (BUTTON_PLAY | BUTTON_REPEAT)
#define MPEG_VOLDOWN    BUTTON_SCROLL_BACK
#define MPEG_VOLUP      BUTTON_SCROLL_FWD

#elif CONFIG_KEYPAD == IAUDIO_X5M5_PAD
#define MPEG_MENU       (BUTTON_REC | BUTTON_REL)
#define MPEG_STOP       BUTTON_POWER
#define MPEG_PAUSE      BUTTON_PLAY
#define MPEG_VOLDOWN    BUTTON_DOWN
#define MPEG_VOLUP      BUTTON_UP

#elif CONFIG_KEYPAD == GIGABEAT_PAD
#define MPEG_MENU       BUTTON_MENU
#define MPEG_STOP       BUTTON_POWER
#define MPEG_PAUSE      BUTTON_SELECT
#define MPEG_VOLDOWN    BUTTON_LEFT
#define MPEG_VOLUP      BUTTON_RIGHT
#define MPEG_VOLDOWN2   BUTTON_VOL_DOWN
#define MPEG_VOLUP2     BUTTON_VOL_UP

#elif CONFIG_KEYPAD == IRIVER_H10_PAD
#define MPEG_MENU       (BUTTON_REW | BUTTON_REL)
#define MPEG_STOP       BUTTON_POWER
#define MPEG_PAUSE      BUTTON_PLAY
#define MPEG_VOLDOWN    BUTTON_SCROLL_DOWN
#define MPEG_VOLUP      BUTTON_SCROLL_UP

#elif CONFIG_KEYPAD == SANSA_E200_PAD
#define MPEG_MENU       BUTTON_SELECT
#define MPEG_STOP       BUTTON_POWER
#define MPEG_PAUSE      BUTTON_UP
#define MPEG_VOLDOWN    BUTTON_SCROLL_UP
#define MPEG_VOLUP      BUTTON_SCROLL_DOWN

#elif CONFIG_KEYPAD == SANSA_C200_PAD
#define MPEG_MENU       BUTTON_SELECT
#define MPEG_STOP       BUTTON_POWER
#define MPEG_PAUSE      BUTTON_UP
#define MPEG_VOLDOWN    BUTTON_VOL_DOWN
#define MPEG_VOLUP      BUTTON_VOL_UP

#else
#error MPEGPLAYER: Unsupported keypad
#endif

struct plugin_api* rb;

CACHE_FUNCTION_WRAPPERS(rb);

extern void *mpeg_malloc(size_t size, mpeg2_alloc_t reason);
extern size_t mpeg_alloc_init(unsigned char *buf, size_t mallocsize,
                              size_t libmpeg2size);

static mpeg2dec_t * mpeg2dec NOCACHEBSS_ATTR;
static int total_offset NOCACHEBSS_ATTR = 0;
static int num_drawn  NOCACHEBSS_ATTR = 0;
static int count_start  NOCACHEBSS_ATTR = 0;

/* Streams */
typedef struct
{
    struct thread_entry *thread; /* Stream's thread */
    int      status;             /* Current stream status */
    struct   queue_event ev;     /* Event sent to steam */
    int      have_msg;           /* 1=event pending */
    int      replied;            /* 1=replied to last event */
    int      reply;              /* reply value */
    struct mutex msg_lock;    /* serialization for event senders */
    uint8_t* curr_packet;        /* Current stream packet beginning */
    uint8_t* curr_packet_end;    /* Current stream packet end */

    uint8_t* prev_packet;        /* Previous stream packet beginning */
    size_t prev_packet_length;   /* Lenth of previous packet */
    size_t buffer_remaining;     /* How much data is left in the buffer */
    uint32_t curr_pts;           /* Current presentation timestamp */
    uint32_t curr_time;          /* Current time in samples */
    uint32_t tagged;             /* curr_pts is valid */

    int id;
} Stream;

static Stream audio_str IBSS_ATTR;
static Stream video_str IBSS_ATTR;

/* Messages */
enum
{
    STREAM_PLAY,
    STREAM_PAUSE,
    STREAM_QUIT
};

/* Status */
enum
{
    STREAM_ERROR = -4,
    STREAM_STOPPED = -3,
    STREAM_TERMINATED = -2,
    STREAM_DONE = -1,
    STREAM_PLAYING = 0,
    STREAM_PAUSED,
    STREAM_BUFFERING
};

/* Returns true if a message is waiting */
static inline bool str_have_msg(Stream *str)
{
    return str->have_msg != 0;
}

/* Waits until a message is sent */
static void str_wait_msg(Stream *str)
{
    int spin_count = 0;

    while (str->have_msg == 0)
    {
        if (spin_count < 100)
        {
            rb->yield();
            spin_count++;
            continue;
        }

        rb->sleep(0);
    }
}

/* Returns a message waiting or blocks until one is available - removes the
   event */
static void str_get_msg(Stream *str, struct queue_event *ev)
{
    str_wait_msg(str);
    ev->id   = str->ev.id;
    ev->data = str->ev.data;
    str->have_msg = 0;
}

/* Peeks at the current message without blocking, returns the data but
   does not remove the event */
static bool str_look_msg(Stream *str, struct queue_event *ev)
{
    if (!str_have_msg(str))
        return false;

    ev->id = str->ev.id;
    ev->data = str->ev.data;
    return true;
}

/* Replies to the last message pulled - has no effect if last message has not
   been pulled or already replied */
static void str_reply_msg(Stream *str, int reply)
{
    if (str->replied == 1 || str->have_msg != 0)
        return;

    str->reply = reply;
    str->replied = 1;
}

/* Sends a message to a stream and waits for a reply */
static intptr_t str_send_msg(Stream *str, int id, intptr_t data)
{
    int spin_count = 0;
    intptr_t reply;

#if 0
    if (str->thread == rb->thread_get_current())
        return str->dispatch_fn(str, msg);
#endif

    /* Only one thread at a time, please */
    rb->mutex_lock(&str->msg_lock);

    str->ev.id = id;
    str->ev.data = data;
    str->reply = 0;
    str->replied = 0;
    str->have_msg = 1;

    while (str->replied == 0 && str->status != STREAM_TERMINATED)
    {
        if (spin_count < 100)
        {
            rb->yield();
            spin_count++;
            continue;
        }

        rb->sleep(0);
    }

    reply = str->reply;

    rb->mutex_unlock(&str->msg_lock);

    return reply;
}

/* NOTE: Putting the following variables in IRAM cause audio corruption
   on the ipod (reason unknown)
*/
static uint8_t *disk_buf_start IBSS_ATTR;  /* Start pointer */
static uint8_t *disk_buf_end IBSS_ATTR; /* End of buffer pointer less
                                           MPEG_GUARDBUF_SIZE. The
                                           guard space is used to wrap
                                           data at the buffer start to
                                           pass continuous data
                                           packets */
static uint8_t *disk_buf_tail IBSS_ATTR;  /* Location of last data + 1
                                             filled into the buffer */
static size_t   disk_buf_size IBSS_ATTR;  /* The total buffer length
                                             including the guard
                                             space */
static size_t file_remaining IBSS_ATTR;

#if NUM_CORES > 1
/* Some stream variables are shared between cores */
struct mutex stream_lock IBSS_ATTR;
static inline void init_stream_lock(void)
    { rb->mutex_init(&stream_lock); }
static inline void lock_stream(void)
    { rb->mutex_lock(&stream_lock); }
static inline void unlock_stream(void)
    { rb->mutex_unlock(&stream_lock); }
#else
/* No RMW issue here */
static inline void init_stream_lock(void)
    { }
static inline void lock_stream(void)
    { }
static inline void unlock_stream(void)
    { }
#endif

static int audio_sync_start IBSS_ATTR; /* If 0, the audio thread
                                          yields waiting on the video
                                          thread to synchronize with
                                          the stream */
static uint32_t audio_sync_time IBSS_ATTR; /* The time that the video
                                              thread has reached after
                                              synchronizing.  The
                                              audio thread now needs
                                              to advance to this
                                              time */
static int video_sync_start IBSS_ATTR; /* While 0, the video thread
                                          yields until the audio
                                          thread has reached the
                                          audio_sync_time */
static int video_thumb_print IBSS_ATTR; /* If 1, the video thread is
                                           only decoding one frame for
                                           use in the menu.  If 0,
                                           normal operation */
static int end_pts_time IBSS_ATTR; /* The movie end time as represented by
                                   the maximum audio PTS tag in the
                                   stream converted to half minutes */
static int start_pts_time IBSS_ATTR; /* The movie start time as represented by
                                   the first audio PTS tag in the
                                   stream converted to half minutes */
char *filename;                 /* hack for resume time storage */


/* Various buffers */
/* TODO: Can we reduce the PCM buffer size? */
#define PCMBUFFER_SIZE              ((512*1024)-PCMBUFFER_GUARD_SIZE)
#define PCMBUFFER_GUARD_SIZE        (1152*4 + sizeof (struct pcm_frame_header))
#define MPA_MAX_FRAME_SIZE          1729 /* Largest frame - MPEG1, Layer II, 384kbps, 32kHz, pad */
#define MPABUF_SIZE                 (64*1024 + ALIGN_UP(MPA_MAX_FRAME_SIZE + 2*MAD_BUFFER_GUARD, 4))
#define LIBMPEG2BUFFER_SIZE         (2*1024*1024)

/* 65536+6 is required since each PES has a 6 byte header with a 16 bit packet length field  */
#define MPEG_GUARDBUF_SIZE (65*1024) /* Keep a bit extra - excessive for now */
#define MPEG_LOW_WATERMARK (1024*1024)

static void pcm_playback_play_pause(bool play);

/* libmad related functions/definitions */
#define INPUT_CHUNK_SIZE 8192

struct mad_stream stream IBSS_ATTR;
struct mad_frame  frame IBSS_ATTR;
struct mad_synth  synth IBSS_ATTR;

unsigned char mad_main_data[MAD_BUFFER_MDLEN];  /* 2567 bytes */

/* There isn't enough room for this in IRAM on PortalPlayer, but there
   is for Coldfire. */

#ifdef CPU_COLDFIRE
static mad_fixed_t mad_frame_overlap[2][32][18] IBSS_ATTR;  /* 4608 bytes */
#else
static mad_fixed_t mad_frame_overlap[2][32][18] __attribute__((aligned(16)));  /* 4608 bytes */
#endif

static void init_mad(void* mad_frame_overlap)
{
    rb->memset(&stream, 0, sizeof(struct mad_stream));
    rb->memset(&frame, 0, sizeof(struct mad_frame));
    rb->memset(&synth, 0, sizeof(struct mad_synth));

    mad_stream_init(&stream);
    mad_frame_init(&frame);

    /* We do this so libmad doesn't try to call codec_calloc() */
    frame.overlap = mad_frame_overlap;

    rb->memset(mad_main_data, 0, sizeof(mad_main_data));
    stream.main_data = &mad_main_data;
}

/* MPEG related headers */

/* Macros for comparing memory bytes to a series of constant bytes in an
   efficient manner - evaluate to true if corresponding bytes match */
#if defined (CPU_ARM)
/* ARM must load 32-bit values at addres % 4 == 0 offsets but this data
   isn't aligned nescessarily, so just byte compare */
#define CMP_3_CONST(_a, _b) \
    ({                                      \
        int _x;                             \
        asm volatile (                      \
            "ldrb   %[x], [%[a], #0]  \r\n" \
            "eors   %[x], %[x], %[b0] \r\n" \
            "ldreqb %[x], [%[a], #1]  \r\n" \
            "eoreqs %[x], %[x], %[b1] \r\n" \
            "ldreqb %[x], [%[a], #2]  \r\n" \
            "eoreqs %[x], %[x], %[b2] \r\n" \
            : [x]"=&r"(_x)                  \
            : [a]"r"(_a),                   \
              [b0]"i"((_b)       >> 24),    \
              [b1]"i"((_b) << 8  >> 24),    \
              [b2]"i"((_b) << 16 >> 24)     \
        );                                  \
        _x == 0;                            \
    })
#define CMP_4_CONST(_a, _b) \
    ({                                      \
        int _x;                             \
        asm volatile (                      \
            "ldrb   %[x], [%[a], #0]  \r\n" \
            "eors   %[x], %[x], %[b0] \r\n" \
            "ldreqb %[x], [%[a], #1]  \r\n" \
            "eoreqs %[x], %[x], %[b1] \r\n" \
            "ldreqb %[x], [%[a], #2]  \r\n" \
            "eoreqs %[x], %[x], %[b2] \r\n" \
            "ldreqb %[x], [%[a], #3]  \r\n" \
            "eoreqs %[x], %[x], %[b3] \r\n" \
            : [x]"=&r"(_x)                  \
            : [a]"r"(_a),                   \
              [b0]"i"((_b)       >> 24),    \
              [b1]"i"((_b) <<  8 >> 24),    \
              [b2]"i"((_b) << 16 >> 24),    \
              [b3]"i"((_b) << 24 >> 24)     \
        );                                  \
        _x == 0;                            \
    })
#elif defined (CPU_COLDFIRE)
/* Coldfire can just load a 32 bit value at any offset but ASM is not the best way
   to integrate this with the C code */
#define CMP_3_CONST(a, b) \
    (((*(uint32_t *)(a) >> 8) ^ ((uint32_t)(b) >> 8)) == 0)
#define CMP_4_CONST(a, b) \
    ((*(uint32_t *)(a) ^ (b)) == 0)
#else
/* Don't know what this is - use bytewise comparisons */
#define CMP_3_CONST(a, b) \
    (( ((a)[0] ^ (((b) >> 24) & 0xff)) | \
       ((a)[1] ^ (((b) >> 16) & 0xff)) | \
       ((a)[2] ^ (((b) >> 8) & 0xff)) ) == 0)
#define CMP_4_CONST(a, b) \
    (( ((a)[0] ^ (((b) >> 24) & 0xff)) | \
       ((a)[1] ^ (((b) >> 16) & 0xff)) | \
       ((a)[2] ^ (((b) >> 8) & 0xff)) | \
       ((a)[3] ^ ((b) & 0xff)) ) == 0)
#endif

/* Codes for various header byte sequences - MSB represents lowest memory
   address */
#define PACKET_START_CODE_PREFIX    0x00000100ul
#define END_CODE                    0x000001b9ul
#define PACK_START_CODE             0x000001baul
#define SYSTEM_HEADER_START_CODE    0x000001bbul

/* p = base pointer, b0 - b4 = byte offsets from p */
/* We only care about the MS 32 bits of the 33 and so the ticks are 45kHz */
#define TS_FROM_HEADER(p, b0, b1, b2, b3, b4) \
    ((uint32_t)(((p)[b0] >> 1 << 29) | \
                ((p)[b1]      << 21) | \
                ((p)[b2] >> 1 << 14) | \
                ((p)[b3]      <<  6) | \
                ((p)[b4] >> 2      )))

/* This function synchronizes the mpeg stream.  The function returns
   true on error */
bool sync_data_stream(uint8_t **p)
{
    for (;;) 
    {
        while ( !CMP_4_CONST(*p, PACK_START_CODE) && (*p) < disk_buf_tail ) 
            (*p)++;
        if ( (*p) >= disk_buf_tail )
            break;
        uint8_t *p_save = (*p); 
        if ( ((*p)[4] & 0xc0) == 0x40 ) /* mpeg-2 */
            (*p) += 14 + ((*p)[13] & 7);
        else if ( ((*p)[4] & 0xf0) == 0x20 ) /* mpeg-1 */
            (*p) += 12;
        else
            (*p) += 5;
        if ( (*p) >= disk_buf_tail )
            break;
        if ( CMP_3_CONST(*p, PACKET_START_CODE_PREFIX) )
        {
            (*p) = p_save;
            break;
        }
        else
            (*p) = p_save+1;
    }

    if ( (*p) >= disk_buf_tail )
        return true;
    else
        return false;
}

/* This function demuxes the streams and gives the next stream data
   pointer.  Type 0 is normal operation.  Type 1 and 2 have been added
   for rapid seeks into the data stream.  Type 1 and 2 ignore the
   video_sync_start state (a signal to yield for refilling the
   buffer).  Type 1 will append more data to the buffer tail (minumal
   bufer size reads that are increased only as needed). */
static int get_next_data( Stream* str, uint8_t type )
{
    uint8_t *p;
    uint8_t *header;
    int stream;

    static int mpeg1_skip_table[16] =
        { 0, 0, 4, 9, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

    if ( (p=str->curr_packet_end) == NULL)
        p = disk_buf_start;

    while (1)
    {
        int length, bytes;

        /* Yield for buffer filling */
        if ( (type == 0) && (str->buffer_remaining < 120*1024) && (file_remaining > 0) )
          while ( (str->buffer_remaining < 512*1024) && (file_remaining > 0) )
            rb->yield();
        
        /* The packet start position (plus an arbitrary header length)
           has exceeded the amount of data in the buffer */
        if ( type == 1 && (p+50) >= disk_buf_tail ) 
        {
            DEBUGF("disk buffer overflow\n");
            return 1;
        }

        /* are we at the end of file? */
        {
            size_t tmp_length;
            if (p < str->prev_packet)
                tmp_length = (disk_buf_end - str->prev_packet) +
                  (p - disk_buf_start);
            else
                tmp_length = (p - str->prev_packet);
            if (0 == str->buffer_remaining-tmp_length-str->prev_packet_length)
            {
                str->curr_packet_end = str->curr_packet = NULL;
                break;      
            }
        }

        /* wrap the disk buffer */
        if (p >= disk_buf_end)
            p = disk_buf_start + (p - disk_buf_end);

        /* wrap packet header if needed */
        if ( (p+50) >= disk_buf_end ) 
            rb->memcpy(disk_buf_end, disk_buf_start, 50);

        /* Pack header, skip it */
        if (CMP_4_CONST(p, PACK_START_CODE))
        {
            if ((p[4] & 0xc0) == 0x40)      /* mpeg-2 */
            {
                p += 14 + (p[13] & 7);
            }
            else if ((p[4] & 0xf0) == 0x20) /* mpeg-1 */
            {
                p += 12;
            }
            else
            {
                rb->splash( 30, "Weird Pack header!" );
                p += 5;
            }
        }

        /* System header, parse and skip it - four bytes */
        if (CMP_4_CONST(p, SYSTEM_HEADER_START_CODE))
        {
            int header_length;

            p += 4;  /*skip start code*/
            header_length = *p++ << 8;
            header_length += *p++;

            p += header_length;

            if ( p >= disk_buf_end )
                p = disk_buf_start + (p - disk_buf_end);
        }
        
        /* Packet header, parse it */
        if (!CMP_3_CONST(p, PACKET_START_CODE_PREFIX))
        {
            /* Problem */
            rb->splash( HZ*3, "missing packet start code prefix : %X%X at %lX",
                        *p, *(p+2), (long unsigned int)(p-disk_buf_start) );

            /* not 64bit safe
            DEBUGF("end diff: %X,%X,%X,%X,%X,%X\n",(int)str->curr_packet_end,
                   (int)audio_str.curr_packet_end,(int)video_str.curr_packet_end,
                   (int)disk_buf_start,(int)disk_buf_end,(int)disk_buf_tail);
            */
            
            str->curr_packet_end = str->curr_packet = NULL;
            break;
        }

        /* We retrieve basic infos */
        stream = p[3];
        length = (p[4] << 8) | p[5];

        if (stream != str->id)
        {
            /* End of stream ? */
            if (stream == 0xB9)
            {
                str->curr_packet_end = str->curr_packet = NULL;
                break;
            }

            /* It's not the packet we're looking for, skip it */
            p += length + 6;
            continue;
        }

        /* Ok, it's our packet */
        str->curr_packet_end = p + length+6;
        header = p;

        if ((header[6] & 0xc0) == 0x80) /* mpeg2 */
        {
            length = 9 + header[8];

            /* header points to the mpeg2 pes header */
            if (header[7] & 0x80)
            {
                /* header has a pts */
                uint32_t pts = TS_FROM_HEADER(header, 9, 10, 11, 12, 13);

                if (stream >= 0xe0)
                {
                    /* video stream - header may have a dts as well */
                    uint32_t dts = (header[7] & 0x40) == 0 ?
                        pts : TS_FROM_HEADER(header, 14, 15, 16, 17, 18);

                    mpeg2_tag_picture (mpeg2dec, pts, dts);
                }
                else
                {
                    str->curr_pts = pts;
                    str->tagged = 1;
                }
            }
        }
        else                            /* mpeg1 */
        {
            int len_skip;
            uint8_t * ptsbuf;

            length = 7;

            while (header[length - 1] == 0xff)
            {
                length++;
                if (length > 23)
                {
                    rb->splash( 30, "Too much stuffing" );
                    DEBUGF("Too much stuffing" );
                    break;
                }
            }
            
            if ( (header[length - 1] & 0xc0) == 0x40 )
                length += 2;

            len_skip = length;
            length += mpeg1_skip_table[header[length - 1] >> 4];

            /* header points to the mpeg1 pes header */
            ptsbuf = header + len_skip;

            if ((ptsbuf[-1] & 0xe0) == 0x20)
            {
                /* header has a pts */
                uint32_t pts = TS_FROM_HEADER(ptsbuf, -1, 0, 1, 2, 3);

                if (stream >= 0xe0)
                {
                    /* video stream - header may have a dts as well */
                    uint32_t dts = (ptsbuf[-1] & 0xf0) != 0x30 ?
                        pts : TS_FROM_HEADER(ptsbuf, 4, 5, 6, 7, 18);

                    mpeg2_tag_picture (mpeg2dec, pts, dts);
                }
                else
                {
                    str->curr_pts = pts;
                    str->tagged = 1;
                }
            }
        }

        p += length;
        bytes = 6 + (header[4] << 8) + header[5] - length;

        if (bytes > 0)
        {
            str->curr_packet_end = p + bytes;

            if (str->curr_packet != NULL)
            {
                lock_stream();

                str->buffer_remaining -= str->prev_packet_length;
                if (str->curr_packet < str->prev_packet)
                  str->prev_packet_length = (disk_buf_end - str->prev_packet) +
                    (str->curr_packet - disk_buf_start);
                else
                  str->prev_packet_length = (str->curr_packet - str->prev_packet);

                unlock_stream();

                str->prev_packet = str->curr_packet;
            }

            str->curr_packet = p;

            if (str->curr_packet_end > disk_buf_end)
              rb->memcpy(disk_buf_end, disk_buf_start, 
                         str->curr_packet_end - disk_buf_end );
        }

        break;
    } /* end while */
    return 0;
}

/* Our clock rate in ticks/second - this won't be a constant for long */
#define CLOCK_RATE 44100

/* For simple lowpass filtering of sync variables */
#define AVERAGE(var, x, count) (((var) * (count-1) + (x)) / (count))
/* Convert 45kHz PTS/DTS ticks to our clock ticks */
#define TS_TO_TICKS(pts) ((uint64_t)CLOCK_RATE*(pts) / 45000)
/* Convert 27MHz ticks to our clock ticks */
#define TIME_TO_TICKS(stamp) ((uint64_t)CLOCK_RATE*(stamp) / 27000000)

/** MPEG audio stream buffer */
uint8_t* mpa_buffer NOCACHEBSS_ATTR;

static bool init_mpabuf(void)
{
    mpa_buffer = mpeg_malloc(MPABUF_SIZE,-2);
    return mpa_buffer != NULL;
}

#define PTS_QUEUE_LEN  (1 << 5) /* 32 should be way more than sufficient -
                                   if not, the case is handled */
#define PTS_QUEUE_MASK (PTS_QUEUE_LEN-1)
struct pts_queue_slot
{
    uint32_t pts;   /* Time stamp for packet          */
    ssize_t  size;  /* Number of bytes left in packet */
} pts_queue[PTS_QUEUE_LEN] __attribute__((aligned(16)));

 /* This starts out wr == rd but will never be emptied to zero during
    streaming again in order to support initializing the first packet's
    pts value without a special case */
static unsigned pts_queue_rd NOCACHEBSS_ATTR;
static unsigned pts_queue_wr NOCACHEBSS_ATTR;

/* Increments the queue head postion - should be used to preincrement */
static bool pts_queue_add_head(void)
{
    if (pts_queue_wr - pts_queue_rd >= PTS_QUEUE_LEN-1)
        return false;

    pts_queue_wr++;
    return true;
}

/* Increments the queue tail position - leaves one slot as current */
static bool pts_queue_remove_tail(void)
{
    if (pts_queue_wr - pts_queue_rd <= 1u)
        return false;

    pts_queue_rd++;
    return true;
}

/* Returns the "head" at the index just behind the write index */
static struct pts_queue_slot * pts_queue_head(void)
{
    return &pts_queue[(pts_queue_wr - 1) & PTS_QUEUE_MASK];
}

/* Returns a pointer to the current tail */
static struct pts_queue_slot * pts_queue_tail(void)
{
    return &pts_queue[pts_queue_rd & PTS_QUEUE_MASK];
}

/* Resets the pts queue - call when starting and seeking */
static void pts_queue_reset(void)
{
    struct pts_queue_slot *pts;
    pts_queue_rd = pts_queue_wr;
    pts = pts_queue_tail();
    pts->pts = 0;
    pts->size = 0;    
}

struct pcm_frame_header     /* Header added to pcm data every time a decoded
                               mpa frame is sent out */
{
    uint32_t size;          /* size of this frame - including header */
    uint32_t time;          /* timestamp for this frame - derived from PTS */
    unsigned char data[];   /* open array of audio data */
};

#define PCMBUF_PLAY_ALL         1l          /* Forces buffer to play back all data */
#define PCMBUF_PLAY_NONE        LONG_MAX    /* Keeps buffer from playing any data */
static volatile uint64_t        pcmbuf_read      IBSS_ATTR;
static volatile uint64_t        pcmbuf_written   IBSS_ATTR;
static volatile ssize_t         pcmbuf_threshold IBSS_ATTR;
static struct pcm_frame_header *pcm_buffer       IBSS_ATTR;
static struct pcm_frame_header *pcmbuf_end       IBSS_ATTR;
static struct pcm_frame_header * volatile pcmbuf_head IBSS_ATTR;
static struct pcm_frame_header * volatile pcmbuf_tail IBSS_ATTR;

static volatile uint32_t samplesplayed IBSS_ATTR; /* Our base clock */
static volatile uint32_t samplestart   IBSS_ATTR; /* Clock at playback start */
static volatile int32_t  sampleadjust  IBSS_ATTR; /* Clock drift adjustment */

static ssize_t pcmbuf_used(void)
{
    return (ssize_t)(pcmbuf_written - pcmbuf_read);
}

static bool init_pcmbuf(void)
{
    pcm_buffer = mpeg_malloc(PCMBUFFER_SIZE + PCMBUFFER_GUARD_SIZE, -2);

    if (pcm_buffer == NULL)
        return false;

    pcmbuf_head = pcm_buffer;
    pcmbuf_tail = pcm_buffer;
    pcmbuf_end  = SKIPBYTES(pcm_buffer, PCMBUFFER_SIZE);
    pcmbuf_read = 0;
    pcmbuf_written = 0;

    return true;
}

/* Advance a PCM buffer pointer by size bytes circularly */
static inline void pcm_advance_buffer(struct pcm_frame_header * volatile *p,
                                      size_t size)
{
    *p = SKIPBYTES(*p, size);
    if (*p >= pcmbuf_end)
        *p = pcm_buffer;
}

static void get_more(unsigned char** start, size_t* size)
{
    /* 25ms @ 44.1kHz */
    static unsigned char silence[4412] __attribute__((aligned (4))) = { 0 };
    size_t sz;

    if (pcmbuf_used() >= pcmbuf_threshold)
    {
        uint32_t time = pcmbuf_tail->time;
        sz = pcmbuf_tail->size;

        *start = (unsigned char *)pcmbuf_tail->data;

        pcm_advance_buffer(&pcmbuf_tail, sz);

        pcmbuf_read += sz;

        sz -= sizeof (*pcmbuf_tail);

        *size = sz;

        /* Drift the clock towards the audio timestamp values */
        sampleadjust = AVERAGE(sampleadjust, (int32_t)(time - samplesplayed), 8);

        /* Update master clock */
        samplesplayed += sz >> 2;
        return;
    }

    /* Keep clock going at all times */
    sz = sizeof (silence);
    *start = silence;
    *size  = sz;

    samplesplayed += sz >> 2;

    if (pcmbuf_read > pcmbuf_written)
        pcmbuf_read = pcmbuf_written;
}

/* Flushes the buffer - clock keeps counting */
static void pcm_playback_flush(void)
{
    bool was_playing = rb->pcm_is_playing();

    if (was_playing)
        rb->pcm_play_stop();

    pcmbuf_read = 0;
    pcmbuf_written = 0;
    pcmbuf_head = pcmbuf_tail;

    if (was_playing)
        rb->pcm_play_data(get_more, NULL, 0);
}

/* Seek the reference clock to the specified time - next audio data ready to
   go to DMA should be on the buffer with the same time index or else the PCM
   buffer should be empty */
static void pcm_playback_seek_time(uint32_t time)
{
    bool was_playing = rb->pcm_is_playing();

    if (was_playing)
        rb->pcm_play_stop();

    samplesplayed = time;
    samplestart   = time;
    sampleadjust  = 0;

    if (was_playing)
        rb->pcm_play_data(get_more, NULL, 0);
}

/* Start pcm playback with the reference clock set to the specified time */
static void pcm_playback_play(uint32_t time)
{
    pcm_playback_seek_time(time);

    if (!rb->pcm_is_playing())
        rb->pcm_play_data(get_more, NULL, 0);
}

/* Pauses playback - and the clock */
static void pcm_playback_play_pause(bool play)
{
    rb->pcm_play_pause(play);
}

/* Stops all playback and resets the clock */
static void pcm_playback_stop(void)
{
    if (rb->pcm_is_playing())
        rb->pcm_play_stop();

    pcm_playback_flush();

    sampleadjust  =
    samplestart   =
    samplesplayed = 0;
}

static uint32_t get_stream_time(void)
{
    return samplesplayed + sampleadjust - (rb->pcm_get_bytes_waiting() >> 2);
}

static uint32_t get_playback_time(void)
{
    return samplesplayed + sampleadjust -
            samplestart - (rb->pcm_get_bytes_waiting() >> 2);
}

static inline int32_t clip_sample(int32_t sample)
{
    if ((int16_t)sample != sample)
        sample = 0x7fff ^ (sample >> 31);

    return sample;
}

static int button_loop(void)
{
    int result;
    int vol, minvol, maxvol;
    int button;

    if (video_sync_start==1) {

    if (str_have_msg(&audio_str))
    {
        struct queue_event ev;
        str_get_msg(&audio_str, &ev);

        if (ev.id == STREAM_QUIT)
        {
            audio_str.status = STREAM_STOPPED;
            goto quit;
        }
        else
        {
            str_reply_msg(&audio_str, 0);
        }
    }

    button = rb->button_get(false);

    switch (button)
    {
        case MPEG_VOLUP:
        case MPEG_VOLUP|BUTTON_REPEAT:
#ifdef MPEG_VOLUP2
        case MPEG_VOLUP2:
        case MPEG_VOLUP2|BUTTON_REPEAT:
#endif
            vol = rb->global_settings->volume;
            maxvol = rb->sound_max(SOUND_VOLUME);

            if (vol < maxvol) {
                vol++;
                rb->sound_set(SOUND_VOLUME, vol);
                rb->global_settings->volume = vol;
            }
            break;

        case MPEG_VOLDOWN:
        case MPEG_VOLDOWN|BUTTON_REPEAT:
#ifdef MPEG_VOLDOWN2
        case MPEG_VOLDOWN2:
        case MPEG_VOLDOWN2|BUTTON_REPEAT:
#endif
            vol = rb->global_settings->volume;
            minvol = rb->sound_min(SOUND_VOLUME);

            if (vol > minvol) {
                vol--;
                rb->sound_set(SOUND_VOLUME, vol);
                rb->global_settings->volume = vol;
            }
            break;

        case MPEG_MENU:
            pcm_playback_play_pause(false);
            audio_str.status = STREAM_PAUSED;
            str_send_msg(&video_str, STREAM_PAUSE, 0);
#ifndef HAVE_LCD_COLOR
            gray_show(false);
#endif
            result = mpeg_menu();
            count_start = get_playback_time();
            num_drawn = 0;

#ifndef HAVE_LCD_COLOR
            gray_show(true);
#endif

            /* The menu can change the font, so restore */
            rb->lcd_setfont(FONT_SYSFIXED);

            switch (result)
            {
                case MPEG_MENU_QUIT:
                    settings.resume_time = (int)(get_stream_time()/CLOCK_RATE/
                                                 30-start_pts_time);
                    str_send_msg(&video_str, STREAM_QUIT, 0);
                    audio_str.status = STREAM_STOPPED;
                    break;
                default:
                  audio_str.status = STREAM_PLAYING;
                  str_send_msg(&video_str, STREAM_PLAY, 0);
                  pcm_playback_play_pause(true);
                  break;
            }
            break;

        case MPEG_STOP:
            settings.resume_time = (int)(get_stream_time()/CLOCK_RATE/
                                         30-start_pts_time);
            str_send_msg(&video_str, STREAM_QUIT, 0);
            audio_str.status = STREAM_STOPPED;
            break;

        case MPEG_PAUSE:
            settings.resume_time = (int)(get_stream_time()/CLOCK_RATE/
                                         30-start_pts_time);
            save_settings();
            str_send_msg(&video_str, STREAM_PAUSE, 0);
            audio_str.status = STREAM_PAUSED;
            pcm_playback_play_pause(false);

            button = BUTTON_NONE;
#ifdef HAVE_ADJUSTABLE_CPU_FREQ
            rb->cpu_boost(false);
#endif
            do {
                button = rb->button_get(true);
                if (button == MPEG_STOP) {
                    str_send_msg(&video_str, STREAM_QUIT, 0);
                    audio_str.status = STREAM_STOPPED;
                    goto quit;
                }
            } while (button != MPEG_PAUSE);

            str_send_msg(&video_str, STREAM_PLAY, 0);
            audio_str.status = STREAM_PLAYING;
            pcm_playback_play_pause(true);
#ifdef HAVE_ADJUSTABLE_CPU_FREQ
            rb->cpu_boost(true);
#endif
            break;

        default:
            if(rb->default_event_handler(button) == SYS_USB_CONNECTED) {
                str_send_msg(&video_str, STREAM_QUIT, 0);
                audio_str.status = STREAM_STOPPED;
            }
        }
    }
quit:
    return audio_str.status;
}

static void audio_thread(void)
{
    uint8_t *mpabuf = mpa_buffer;
    ssize_t mpabuf_used = 0;
    int mad_errors = 0;  /* A count of the errors in each frame */
    struct pts_queue_slot *pts;

    /* We need this here to init the EMAC for Coldfire targets */
    mad_synth_init(&synth);

    /* Init pts queue */
    pts_queue_reset();
    pts = pts_queue_tail();

    /* Keep buffer from playing */
    pcmbuf_threshold = PCMBUF_PLAY_NONE;

    /* Start clock */
    pcm_playback_play(0);

    /* Get first packet */
    get_next_data(&audio_str, 0 );

    /* skip audio packets here */
    while (audio_sync_start==0)
    {
      audio_str.status = STREAM_PLAYING;
      rb->yield();
    }

    if (audio_sync_time>10000)
    {
      while (TS_TO_TICKS(audio_str.curr_pts) < audio_sync_time - 10000)
      {
        get_next_data(&audio_str, 0 );
        rb->priority_yield();
      }
    }

    if (audio_str.curr_packet == NULL)
        goto done;

    /* This is the decoding loop. */
    while (1)
    {
        int mad_stat;
        size_t len;

        if (button_loop() == STREAM_STOPPED)
            goto audio_thread_quit;

        if (pts->size <= 0)
        {
            /* Carry any overshoot to the next size since we're technically
               -pts->size bytes into it already. If size is negative an audio
               frame was split accross packets. Old has to be saved before
               moving the tail. */
            if (pts_queue_remove_tail())
            {
                struct pts_queue_slot *old = pts;
                pts = pts_queue_tail();
                pts->size += old->size;
                old->size = 0;
            }
        }

        /** Buffering **/
        if (mpabuf_used >= MPA_MAX_FRAME_SIZE + MAD_BUFFER_GUARD)
        {
            /* Above low watermark - do nothing */
        }
        else if (audio_str.curr_packet != NULL)
        {
            do
            {
                /* Get data from next audio packet */
                len = audio_str.curr_packet_end - audio_str.curr_packet;

                if (audio_str.tagged)
                {
                    struct pts_queue_slot *stamp = pts;

                    if (pts_queue_add_head())
                    {
                        stamp = pts_queue_head();
                        stamp->pts = TS_TO_TICKS(audio_str.curr_pts);
                        /* pts->size should have been zeroed when slot was
                           freed */
                    }
                    /* else queue full - just count up from the last to make
                       it look like more data in the same packet */
                    stamp->size += len;
                    audio_str.tagged = 0;
                }
                else
                {
                    /* Add to the one just behind the head - this may be the
                       tail or the previouly added head - whether or not we'll
                       ever reach this is quite in question since audio always
                       seems to have every packet timestamped */
                    pts_queue_head()->size += len;
                }

                /* Slide any remainder over to beginning - avoid function
                   call overhead if no data remaining as well */
                if (mpabuf > mpa_buffer && mpabuf_used > 0)
                    rb->memmove(mpa_buffer, mpabuf, mpabuf_used);

                /* Splice this packet onto any remainder */
                rb->memcpy(mpa_buffer + mpabuf_used, audio_str.curr_packet,
                           len);

                mpabuf_used += len;
                mpabuf = mpa_buffer;

                /* Get data from next audio packet */
                get_next_data(&audio_str, 0 );
            }
            while (audio_str.curr_packet != NULL &&
                   mpabuf_used < MPA_MAX_FRAME_SIZE + MAD_BUFFER_GUARD);
        }
        else if (mpabuf_used <= 0)
        {
            /* Used up remainder of mpa buffer so quit */
            break;
        }

        /** Decoding **/
        mad_stream_buffer(&stream, mpabuf, mpabuf_used);

        mad_stat = mad_frame_decode(&frame, &stream);

        if (stream.next_frame == NULL)
        {
            /* What to do here? (This really is fatal) */
            DEBUGF("/* What to do here? */\n");
            break;
        }

        /* Next mad stream buffer is the next frame postion */
        mpabuf = (uint8_t *)stream.next_frame;

        /* Adjust sizes by the frame size */
        len = stream.next_frame - stream.this_frame;
        mpabuf_used -= len;
        pts->size -= len;

        if (mad_stat != 0)
        {
            if (stream.error == MAD_FLAG_INCOMPLETE
                || stream.error == MAD_ERROR_BUFLEN)
            {
                /* This makes the codec support partially corrupted files */
                if (++mad_errors > 30)
                    break;

                stream.error = 0;
                rb->priority_yield();
                continue;
            }
            else if (MAD_RECOVERABLE(stream.error))
            {
                stream.error = 0;
                rb->priority_yield();
                continue;
            }
            else
            {
                /* Some other unrecoverable error */
                DEBUGF("Unrecoverable error\n");
            }

            break;
        }

        mad_errors = 0; /* Clear errors */

        /* Generate the pcm samples */
        mad_synth_frame(&synth, &frame);

        /** Output **/

        /* TODO: Output through core dsp. We'll still use our own PCM buffer
           since the core pcm buffer has no timestamping or clock facilities */

        /* Add a frame of audio to the pcm buffer. Maximum is 1152 samples. */
        if (synth.pcm.length > 0)
        {
            int16_t *audio_data = (int16_t *)pcmbuf_head->data;
            size_t size = sizeof (*pcmbuf_head) + synth.pcm.length*4;
            size_t wait_for = size + 32*1024;

            /* Leave at least 32KB free (this will be the currently
               playing chunk) */
            while (pcmbuf_used() + wait_for > PCMBUFFER_SIZE)
            {
                if (str_have_msg(&audio_str))
                {
                    struct queue_event ev;
                    str_look_msg(&audio_str, &ev);

                    if (ev.id == STREAM_QUIT)
                        goto audio_thread_quit;
                }

                rb->priority_yield();
            }

            if (video_sync_start == 0 && 
                pts->pts+(uint32_t)synth.pcm.length<audio_sync_time) {
              synth.pcm.length = 0;
              size = 0;
              rb->yield();
            }

            /* TODO: This part will be replaced with dsp calls soon */
            if (MAD_NCHANNELS(&frame.header) == 2)
            {
                int32_t *left = &synth.pcm.samples[0][0];
                int32_t *right = &synth.pcm.samples[1][0];
                int i = synth.pcm.length;

                do
                {
                    /* libmad outputs s3.28 */
                    *audio_data++ = clip_sample(*left++ >> 13);
                    *audio_data++ = clip_sample(*right++ >> 13);
                }
                while (--i > 0);
            }
            else  /* mono */
            {
                int32_t *mono = &synth.pcm.samples[0][0];
                int i = synth.pcm.length;

                do
                {
                    int32_t s = clip_sample(*mono++ >> 13);
                    *audio_data++ = s;
                    *audio_data++ = s;
                }
                while (--i > 0);
            }
            /**/

            pcmbuf_head->time = pts->pts;
            pcmbuf_head->size = size;

            /* As long as we're on this timestamp, the time is just incremented
               by the number of samples */
            pts->pts += synth.pcm.length;

            pcm_advance_buffer(&pcmbuf_head, size);

            if (pcmbuf_threshold != PCMBUF_PLAY_ALL && pcmbuf_used() >= 64*1024)
            {
                /* We've reached our size treshold so start playing back the
                   audio in the buffer and set the buffer to play all data */
                audio_str.status = STREAM_PLAYING;
                pcmbuf_threshold = PCMBUF_PLAY_ALL;
                pcm_playback_seek_time(pcmbuf_tail->time);
                video_sync_start = 1;
            }

            /* Make this data available to DMA */
            pcmbuf_written += size;
        }

        rb->yield();
    } /* end decoding loop */

done:
    if (audio_str.status == STREAM_STOPPED)
        goto audio_thread_quit;

    /* Force any residue to play if audio ended before reaching the
       threshold */
    if (pcmbuf_threshold != PCMBUF_PLAY_ALL && pcmbuf_used() > 0)
    {
        pcm_playback_play(pcmbuf_tail->time);
        pcmbuf_threshold = PCMBUF_PLAY_ALL;
    }

    if (rb->pcm_is_playing() && !rb->pcm_is_paused())
    {
        /* Wait for audio to finish */
        while (pcmbuf_used() > 0)
        {
            if (button_loop() == STREAM_STOPPED)
                goto audio_thread_quit;
            rb->sleep(HZ/10);
        }
    }

    audio_str.status = STREAM_DONE;

    /* Process events until finished */
    while (button_loop() != STREAM_STOPPED)
        rb->sleep(HZ/4);

audio_thread_quit:
    pcm_playback_stop();

    audio_str.status = STREAM_TERMINATED;
}

/* End of libmad stuff */

/* The audio stack is stolen from the core codec thread (but not in uisim) */
#define AUDIO_STACKSIZE (9*1024)
uint32_t* audio_stack;

#ifndef SIMULATOR
static uint32_t codec_stack_copy[AUDIO_STACKSIZE / sizeof(uint32_t)];
#endif

/* TODO: Check if 4KB is appropriate - it works for my test streams,
   so maybe we can reduce it. */
#define VIDEO_STACKSIZE (4*1024)
static uint32_t video_stack[VIDEO_STACKSIZE / sizeof(uint32_t)] IBSS_ATTR;

static void video_thread(void)
{
    struct queue_event ev;
    const mpeg2_info_t * info;
    mpeg2_state_t state;
    char str[80];
    uint32_t curr_time = 0;
    uint32_t period = 0; /* Frame period in clock ticks */
    uint32_t eta_audio = UINT_MAX, eta_video = 0;
    int32_t eta_early = 0, eta_late = 0;
    int frame_drop_level = 0;
    int skip_level = 0;
    int num_skipped = 0;
    /* Used to decide when to display FPS */
    unsigned long last_showfps = *rb->current_tick - HZ;
    /* Used to decide whether or not to force a frame update */
    unsigned long last_render = last_showfps;

    mpeg2dec = mpeg2_init();
    if (mpeg2dec == NULL)
    {
        rb->splash(0, "mpeg2_init failed");
        /* Commit suicide */
        video_str.status = STREAM_TERMINATED;
        return;
    }

    /* Clear the display - this is mainly just to indicate that the
       video thread has started successfully. */
    if (!video_thumb_print)
    {
      rb->lcd_clear_display();
      rb->lcd_update();
    }

    /* Request the first packet data */
    get_next_data( &video_str, 0 );

    if (video_str.curr_packet == NULL)
        goto video_thread_quit;

    mpeg2_buffer (mpeg2dec, video_str.curr_packet, video_str.curr_packet_end);
    total_offset += video_str.curr_packet_end - video_str.curr_packet;

    info = mpeg2_info (mpeg2dec);

    while (1)
    {
        /* quickly check mailbox first */
        if (video_thumb_print)
        {
            if (video_str.status == STREAM_STOPPED)
                break;
        }
        else if (str_have_msg(&video_str))
        {
            while (1)
            {
                str_get_msg(&video_str, &ev);

                switch (ev.id)
                {
                case STREAM_QUIT:
                    video_str.status = STREAM_STOPPED;
                    goto video_thread_quit;
                case STREAM_PAUSE:
            #if NUM_CORES > 1
                    flush_icache();
            #endif
                    video_str.status = STREAM_PAUSED;
                    str_reply_msg(&video_str, 1);
                    continue;
                }

                break;
            }

            video_str.status = STREAM_PLAYING;
            str_reply_msg(&video_str, 1);
        }

        state = mpeg2_parse (mpeg2dec);
        rb->yield();

        /* Prevent idle poweroff */
        rb->reset_poweroff_timer();
        
        switch (state)
        {
        case STATE_BUFFER:
            /* Request next packet data */
            get_next_data( &video_str, 0 );

            mpeg2_buffer (mpeg2dec, video_str.curr_packet, video_str.curr_packet_end);
            total_offset += video_str.curr_packet_end - video_str.curr_packet;
            info = mpeg2_info (mpeg2dec);

            if (video_str.curr_packet == NULL)
            {
                /* No more data. */
                goto video_thread_quit;
            }
            continue;

        case STATE_SEQUENCE:
            /* New GOP, inform output of any changes */
            vo_setup(info->sequence);
            break;

        case STATE_PICTURE:
        {
            int skip = 0; /* Assume no skip */

            if (frame_drop_level >= 1 || skip_level > 0)
            {
                /* A frame will be dropped in the decoder */

                /* Frame type: I/P/B/D */
                int type = info->current_picture->flags & PIC_MASK_CODING_TYPE;

                switch (type)
                {
                case PIC_FLAG_CODING_TYPE_I:
                case PIC_FLAG_CODING_TYPE_D:
                    /* Level 5: Things are extremely late and all frames will be
                       dropped until the next key frame */
                    if (frame_drop_level >= 1)
                        frame_drop_level = 0; /* Key frame - reset drop level */
                    if (skip_level >= 5)
                    {
                        frame_drop_level = 1;
                        skip_level = 0; /* reset */
                    }
                    break;
                case PIC_FLAG_CODING_TYPE_P:
                    /* Level 4: Things are very late and all frames will be
                       dropped until the next key frame */
                    if (skip_level >= 4)
                    {
                        frame_drop_level = 1;
                        skip_level = 0; /* reset */
                    }
                    break;
                case PIC_FLAG_CODING_TYPE_B:
                    /* We want to drop something, so this B frame won't even
                       be decoded. Drawing can happen on the next frame if so
                       desired. Bring the level down as skips are done. */
                    skip = 1;
                    if (skip_level > 0)
                        skip_level--;
                }

                skip |= frame_drop_level;
            }

            mpeg2_skip(mpeg2dec, skip);
            break;  
            }

        case STATE_SLICE:
        case STATE_END:
        case STATE_INVALID_END:
        {
            int32_t offset;  /* Tick adjustment to keep sync */

            /* draw current picture */
            if (!info->display_fbuf)
                break;

            /* No limiting => no dropping - draw this frame */
            if (!settings.limitfps && (video_thumb_print == 0))
            {
                audio_sync_start = 1;
                video_sync_start = 1;
                goto picture_draw;
            }

            /* Get presentation times in audio samples - quite accurate
               enough - add previous frame duration if not stamped */
            curr_time = (info->display_picture->flags & PIC_FLAG_TAGS) ?
                TS_TO_TICKS(info->display_picture->tag) : (curr_time + period);

            period = TIME_TO_TICKS(info->sequence->frame_period);

            if ( (video_thumb_print == 1 || video_sync_start == 0) && 
                 ((int)(info->current_picture->flags & PIC_MASK_CODING_TYPE) 
                 == PIC_FLAG_CODING_TYPE_B))
              break;

            eta_video = curr_time;

            audio_sync_time = eta_video;
            audio_sync_start = 1;
            
            while (video_sync_start == 0)
                rb->yield();

            eta_audio = get_stream_time();

            /* How early/late are we? > 0 = late, < 0  early */
            offset = eta_audio - eta_video;

            if (!settings.skipframes)
            {
                /* Make no effort to determine whether this frame should be
                   drawn or not since no action can be taken to correct the
                   situation. We'll just wait if we're early and correct for
                   lateness as much as possible. */
                if (offset < 0)
                    offset = 0;

                eta_late = AVERAGE(eta_late, offset, 4);
                offset = eta_late;

                if ((uint32_t)offset > eta_video)
                    offset = eta_video;

                eta_video -= offset;
                goto picture_wait;
            }

            /** Possibly skip this frame **/

            /* Frameskipping has the following order of preference:
             *
             * Frame Type  Who      Notes/Rationale
             * B           decoder  arbitrarily drop - no decode or draw
             * Any         renderer arbitrarily drop - will be I/D/P
             * P           decoder  must wait for I/D-frame - choppy
             * I/D         decoder  must wait for I/D-frame - choppy
             *
             * If a frame can be drawn and it has been at least 1/2 second,
             * the image will be updated no matter how late it is just to
             * avoid looking stuck.
             */

            /* If we're late, set the eta to play the frame early so
               we may catch up. If early, especially because of a drop,
               mitigate a "snap" by moving back gradually. */
            if (offset >= 0) /* late or on time */
            {
                eta_early = 0; /* Not early now :( */

                eta_late = AVERAGE(eta_late, offset, 4);
                offset = eta_late;

                if ((uint32_t)offset > eta_video)
                    offset = eta_video;

                eta_video -= offset;
            }
            else
            {
                eta_late = 0; /* Not late now :) */

                if (offset > eta_early)
                {
                    /* Just dropped a frame and we're now early or we're
                       coming back from being early */
                    eta_early = offset;
                    if ((uint32_t)-offset > eta_video)
                        offset = -eta_video;

                    eta_video += offset;
                }
                else
                {
                    /* Just early with an offset, do exponential drift back */
                    if (eta_early != 0)
                    {
                        eta_early = AVERAGE(eta_early, 0, 8);
                        eta_video = ((uint32_t)-eta_early > eta_video) ?
                            0 : (eta_video + eta_early);
                    }

                    offset = eta_early;
                }
            }

            if (info->display_picture->flags & PIC_FLAG_SKIP)
            {
                /* This frame was set to skip so skip it after having updated
                   timing information */
                num_skipped++;
                eta_early = INT32_MIN;
                goto picture_skip;
            }

            if (skip_level == 3 && TIME_BEFORE(*rb->current_tick, last_render + HZ/2))
            {
                /* Render drop was set previously but nothing was dropped in the
                   decoder or it's been to long since drawing the last frame. */
                skip_level = 0;
                num_skipped++;
                eta_early = INT32_MIN;
                goto picture_skip;
            }

            /* At this point a frame _will_ be drawn  - a skip may happen on
               the next however */
            skip_level = 0;

            if (offset > CLOCK_RATE*110/1000)
            {
                /* Decide which skip level is needed in order to catch up */

                /* TODO: Calculate this rather than if...else - this is rather
                   exponential though */
                if (offset > CLOCK_RATE*367/1000)
                    skip_level = 5; /* Decoder skip: I/D */
                if (offset > CLOCK_RATE*233/1000)
                    skip_level = 4; /* Decoder skip: P */
                else if (offset > CLOCK_RATE*167/1000)
                    skip_level = 3; /* Render skip */
                else if (offset > CLOCK_RATE*133/1000)
                    skip_level = 2; /* Decoder skip: B */
                else
                    skip_level = 1; /* Decoder skip: B */
            }

        picture_wait:
            /* Wait until audio catches up */
            if (video_thumb_print)
                video_str.status = STREAM_STOPPED;
            else
                while (eta_video > eta_audio)
                {
                    rb->priority_yield();
                  
                    /* Make sure not to get stuck waiting here forever */
                    if (str_have_msg(&video_str))
                    {
                        str_look_msg(&video_str, &ev);
                      
                        /* If not to play, process up top */
                        if (ev.id != STREAM_PLAY)
                            goto rendering_finished;
                      
                        /* Told to play but already playing */
                        str_get_msg(&video_str, &ev);
                        str_reply_msg(&video_str, 1);
                    }

                    eta_audio = get_stream_time();
                }
       
        picture_draw:
            /* Record last frame time */
            last_render = *rb->current_tick;

            if (video_thumb_print)
                vo_draw_frame_thumb(info->display_fbuf->buf);
            else
                vo_draw_frame(info->display_fbuf->buf);

            num_drawn++;

        picture_skip:
            if (!settings.showfps)
                break;

            /* Calculate and display fps */
            if (TIME_AFTER(*rb->current_tick, last_showfps + HZ))
            {
                uint32_t clock_ticks = get_playback_time() - count_start;
                int fps = 0;

                if (clock_ticks != 0)
                    fps = num_drawn*CLOCK_RATE*10ll / clock_ticks;

                rb->snprintf(str, sizeof(str), "%d.%d %d %d    ",
                             fps / 10, fps % 10, num_skipped,
                             info->display_picture->temporal_reference);
                rb->lcd_putsxy(0, 0, str);
                rb->lcd_update_rect(0, 0, LCD_WIDTH, 8);

                last_showfps = *rb->current_tick;
            }
            break;
            }

        default:
            break;
        }
    rendering_finished:

        rb->yield();
    }

 video_thread_quit:
    /* if video ends before time sync'd, 
       besure the audio thread is closed */
    if (video_sync_start == 0)
    {
        audio_str.status = STREAM_STOPPED;
        audio_sync_start = 1;
    }

  #if NUM_CORES > 1
      flush_icache();
  #endif
  
    mpeg2_close (mpeg2dec);
  
    /* Commit suicide */
    video_str.status = STREAM_TERMINATED;
}

void initialize_stream( Stream *str, uint8_t *buffer_start, size_t disk_buf_len, int id )
{
    str->curr_packet_end = str->curr_packet = NULL;
    str->prev_packet_length = 0;
    str->prev_packet = str->curr_packet_end = buffer_start;
    str->buffer_remaining = disk_buf_len;
    str->id = id;
}
    
void display_thumb(int in_file)
{
    size_t disk_buf_len;

    video_thumb_print = 1;
    audio_sync_start = 1;
    video_sync_start = 1;
  
    disk_buf_len = rb->read (in_file, disk_buf_start, disk_buf_size - MPEG_GUARDBUF_SIZE);
    disk_buf_tail = disk_buf_start + disk_buf_len;
    file_remaining = 0;
    initialize_stream(&video_str,disk_buf_start,disk_buf_len,0xe0);
    
    video_str.status = STREAM_PLAYING;

    if ((video_str.thread = rb->create_thread(video_thread,
       (uint8_t*)video_stack,VIDEO_STACKSIZE, 0,"mpgvideo" 
       IF_PRIO(,PRIORITY_PLAYBACK) IF_COP(, COP))) == NULL)
    {
        rb->splash(HZ, "Cannot create video thread!");
    }
    else
    {
        rb->thread_wait(video_str.thread);
    }

    if ( video_str.curr_packet_end == video_str.curr_packet)
      rb->splash(0, "frame not available");
}

int find_start_pts( int in_file )
{
    uint8_t *p;
    size_t read_length = 60*1024;
    size_t disk_buf_len;
  
    start_pts_time = 0;

    /* temporary read buffer size cannot exceed buffer size */
    if ( read_length > disk_buf_size )
        read_length = disk_buf_size;
  
    /* read tail of file */
    rb->lseek( in_file, 0, SEEK_SET );
    disk_buf_len = rb->read( in_file, disk_buf_start, read_length );
    disk_buf_tail = disk_buf_start + disk_buf_len;
  
    /* sync reader to this segment of the stream */
    p=disk_buf_start;
    if (sync_data_stream(&p))
    {
        DEBUGF("Could not sync stream\n");
        return PLUGIN_ERROR;
    }
  
    /* find first PTS in audio stream. if the PTS can not be determined, 
       set start_pts_time to 0 */
    audio_sync_start = 0;
    audio_sync_time = 0;
    video_sync_start = 0;
    {
        Stream tmp;
        initialize_stream(&tmp,p,disk_buf_len-(disk_buf_start-p),0xc0);
        int count=0;
        do
        {
            count++;
            get_next_data(&tmp, 2);
        }
        while (tmp.tagged != 1 && count < 30);
        if (tmp.tagged == 1)
            start_pts_time = (int)((tmp.curr_pts/45000)/30);
    } 
    return 0;
}

int find_end_pts( int in_file )
{
    uint8_t *p;
    size_t read_length = 60*1024;
    size_t disk_buf_len;
  
    end_pts_time = 0;

    /* temporary read buffer size cannot exceed buffer size */
    if ( read_length > disk_buf_size )
        read_length = disk_buf_size;
  
    /* read tail of file */
    rb->lseek( in_file, -1*read_length, SEEK_END );
    disk_buf_len = rb->read( in_file, disk_buf_start, read_length );
    disk_buf_tail = disk_buf_start + disk_buf_len;
  
    /* sync reader to this segment of the stream */
    p=disk_buf_start;
    if (sync_data_stream(&p))
    {
        DEBUGF("Could not sync stream\n");
        return PLUGIN_ERROR;
    }
  
    /* find last PTS in audio stream; will movie always have audio? if
       the play time can not be determined, set end_pts_time to 0 */
    audio_sync_start = 0;
    audio_sync_time = 0;
    video_sync_start = 0;
    {
        Stream tmp;
        initialize_stream(&tmp,p,disk_buf_len-(disk_buf_start-p),0xc0);
        
        do
        {  
            get_next_data(&tmp, 2);
            if (tmp.tagged == 1)
                /* 10 sec less to insure the video frame exist */
                end_pts_time = (int)((tmp.curr_pts/45000-10)/30);
        }
        while (tmp.curr_packet_end != NULL);
    } 
    return 0;
}

ssize_t seek_PTS( int in_file, int start_time, int accept_button )
{
    static ssize_t last_seek_pos = 0;
    static int last_start_time = 0;
    ssize_t seek_pos;
    size_t disk_buf_len;
    uint8_t *p;
    size_t read_length = 60*1024;

    /* temporary read buffer size cannot exceed buffer size */
    if ( read_length > disk_buf_size )
        read_length = disk_buf_size;

    if ( start_time == last_start_time )
    {
        seek_pos = last_seek_pos;
        rb->lseek(in_file,seek_pos,SEEK_SET);
    }
    else if ( start_time != 0 )
    {
        seek_pos = rb->filesize(in_file)*start_time/
          (end_pts_time-start_pts_time);
        int seek_pos_sec_inc = rb->filesize(in_file)/
          (end_pts_time-start_pts_time)/30;
        
        if (seek_pos<0)
            seek_pos=0;
        if ((size_t)seek_pos > rb->filesize(in_file) - read_length)
            seek_pos = rb->filesize(in_file) - read_length;
        rb->lseek( in_file, seek_pos, SEEK_SET );
        disk_buf_len = rb->read( in_file, disk_buf_start, read_length );
        disk_buf_tail = disk_buf_start + disk_buf_len;
      
        /* sync reader to this segment of the stream */
        p=disk_buf_start;
        if (sync_data_stream(&p))
        {
            DEBUGF("Could not sync stream\n");
            return PLUGIN_ERROR;
        }
      
        /* find PTS >= start_time */
        audio_sync_start = 0;
        audio_sync_time = 0;
        video_sync_start = 0;
        {
            Stream tmp;
            initialize_stream(&tmp,p,disk_buf_len-(disk_buf_start-p),0xc0);
            int cont_seek_loop = 1;
            int coarse_seek = 1;
            do
            {
                if ( accept_button )
                {
                    rb->yield();
                    if (rb->button_queue_count())
                      return -101;
                }

                while ( get_next_data(&tmp, 1) == 1 )
                {
                    if ( tmp.curr_packet_end == disk_buf_start )
                        seek_pos += disk_buf_tail - disk_buf_start;
                    else
                        seek_pos += tmp.curr_packet_end - disk_buf_start;
                    if ((size_t)seek_pos > rb->filesize(in_file) - read_length)
                        seek_pos = rb->filesize(in_file) - read_length;
                    rb->lseek( in_file, seek_pos, SEEK_SET );
                    disk_buf_len = rb->read ( in_file, disk_buf_start, read_length );
                    disk_buf_tail = disk_buf_start + disk_buf_len;
                    
                    /* sync reader to this segment of the stream */
                    p=disk_buf_start;
                    initialize_stream(&tmp,p,disk_buf_len,0xc0);
                }
                  
                /* are we after start_time in the stream? */
                if ( coarse_seek && (int)(tmp.curr_pts/45000) >= 
                     (start_time+start_pts_time)*30 )
                {
                    int time_to_backup = (int)(tmp.curr_pts/45000) - 
                      (start_time+start_pts_time)*30;
                    if (time_to_backup == 0)
                        time_to_backup++;
                    seek_pos -= seek_pos_sec_inc * time_to_backup;
                    seek_pos_sec_inc -= seek_pos_sec_inc/20; /* for stability */
                    if (seek_pos<0)
                        seek_pos=0;
                    if ((size_t)seek_pos > rb->filesize(in_file) - read_length)
                        seek_pos = rb->filesize(in_file) - read_length;
                    rb->lseek( in_file, seek_pos, SEEK_SET );
                    disk_buf_len = rb->read( in_file, disk_buf_start, read_length );
                    disk_buf_tail = disk_buf_start + disk_buf_len;
                    
                    /* sync reader to this segment of the stream */
                    p=disk_buf_start;
                    if (sync_data_stream(&p))
                    {
                        DEBUGF("Could not sync stream\n");
                        return PLUGIN_ERROR;
                    }         
                    initialize_stream(&tmp,p,disk_buf_len-(disk_buf_start-p),0xc0);
                    continue;
                }
          
                /* are we well before start_time in the stream? */
                if ( coarse_seek && (start_time+start_pts_time)*30 - 
                     (int)(tmp.curr_pts/45000) > 2 )
                {
                    int time_to_advance = (start_time+start_pts_time)*30 - 
                      (int)(tmp.curr_pts/45000) - 2;
                    if (time_to_advance <= 0)
                        time_to_advance = 1;
                    seek_pos += seek_pos_sec_inc * time_to_advance;
                    if (seek_pos<0)
                        seek_pos=0;
                    if ((size_t)seek_pos > rb->filesize(in_file) - read_length)
                        seek_pos = rb->filesize(in_file) - read_length;
                    rb->lseek( in_file, seek_pos, SEEK_SET );
                    disk_buf_len = rb->read ( in_file, disk_buf_start, read_length );
                    disk_buf_tail = disk_buf_start + disk_buf_len;
              
                    /* sync reader to this segment of the stream */
                    p=disk_buf_start;
                    if (sync_data_stream(&p))
                    {
                        DEBUGF("Could not sync stream\n");
                        return PLUGIN_ERROR;
                    }         
                    initialize_stream(&tmp,p,disk_buf_len-(disk_buf_start-p),0xc0);
                    continue;
                }
          
                coarse_seek = 0;
          
                /* are we at start_time in the stream? */
                if ( (int)(tmp.curr_pts/45000) >= (start_time+start_pts_time)*
                     30 )
                    cont_seek_loop = 0;
                
            } 
            while ( cont_seek_loop );
        
        
            DEBUGF("start diff: %u %u\n",(unsigned int)(tmp.curr_pts/45000),
                   (start_time+start_pts_time)*30);
            seek_pos+=tmp.curr_packet_end-disk_buf_start;
            
            last_seek_pos = seek_pos;
            last_start_time = start_time;
            
            rb->lseek(in_file,seek_pos,SEEK_SET);
        }
    }
    else
    {
        seek_pos = 0;
        rb->lseek(in_file,0,SEEK_SET);
        last_seek_pos = seek_pos;
        last_start_time = start_time;
    }
    return seek_pos;
}
  
enum plugin_status plugin_start(struct plugin_api* api, void* parameter)
{
    int status = PLUGIN_ERROR; /* assume failure */
    int result;
    int start_time = -1;
    void* audiobuf;
    ssize_t audiosize;
    int in_file;
    size_t disk_buf_len;
    ssize_t seek_pos;
    size_t audio_stack_size = 0; /* Keep gcc happy and init */
    int i;
#ifndef HAVE_LCD_COLOR
    long graysize;
    int grayscales;
#endif

    audio_sync_start = 0;
    audio_sync_time = 0;
    video_sync_start = 0;

    if (parameter == NULL)
    {
        api->splash(HZ*2, "No File");
        return PLUGIN_ERROR;        
    }
    api->talk_disable(true);

    /* Initialize IRAM - stops audio and voice as well */
    PLUGIN_IRAM_INIT(api)

    rb = api;
    rb->splash(0, "Loading...");

    /* sets audiosize and returns buffer pointer */
    audiobuf = rb->plugin_get_audio_buffer(&audiosize);

#if INPUT_SRC_CAPS != 0
    /* Select playback */
    rb->audio_set_input_source(AUDIO_SRC_PLAYBACK, SRCF_PLAYBACK);
    rb->audio_set_output_source(AUDIO_SRC_PLAYBACK);
#endif

    rb->pcm_set_frequency(SAMPR_44);

#ifndef HAVE_LCD_COLOR
    /* initialize the grayscale buffer: 32 bitplanes for 33 shades of gray. */
    grayscales = gray_init(rb, audiobuf, audiosize, false, LCD_WIDTH, LCD_HEIGHT,
                           32, 2<<8, &graysize) + 1;
    audiobuf += graysize;
    audiosize -= graysize;
    if (grayscales < 33 || audiosize <= 0)
    {
        rb->talk_disable(false);
        rb->splash(HZ, "gray buf error");
        return PLUGIN_ERROR;
    }
#endif

    /* Initialise our malloc buffer */
    audiosize = mpeg_alloc_init(audiobuf,audiosize, LIBMPEG2BUFFER_SIZE);
    if (audiosize == 0)
    {
        rb->talk_disable(false);
        return PLUGIN_ERROR;
    }

    /* Set disk pointers to NULL */
    disk_buf_end = disk_buf_start = NULL;

    /* Grab most of the buffer for the compressed video - leave some for
       PCM audio data and some for libmpeg2 malloc use. */
    disk_buf_size = audiosize - (PCMBUFFER_SIZE+PCMBUFFER_GUARD_SIZE+
                               MPABUF_SIZE);

    DEBUGF("audiosize=%ld, disk_buf_size=%ld\n",audiosize,disk_buf_size);
    disk_buf_start = mpeg_malloc(disk_buf_size,-1);

    if (disk_buf_start == NULL)
    {
        rb->talk_disable(false);
        return PLUGIN_ERROR;
    }

    if (!init_mpabuf())
    {
        rb->talk_disable(false);
        return PLUGIN_ERROR;
    }
    if (!init_pcmbuf())
    {
        rb->talk_disable(false);
        return PLUGIN_ERROR;
    }

    /* The remaining buffer is for use by libmpeg2 */

    /* Open the video file */
    in_file = rb->open((char*)parameter,O_RDONLY);

    if (in_file < 0){
        DEBUGF("Could not open %s\n",(char*)parameter);
        rb->talk_disable(false);
        return PLUGIN_ERROR;
    }
    filename = (char*)parameter;

#ifdef HAVE_LCD_COLOR
    rb->lcd_set_backdrop(NULL);
    rb->lcd_set_foreground(LCD_WHITE);
    rb->lcd_set_background(LCD_BLACK);
#endif

    init_settings((char*)parameter);

    /* Initialise libmad */
    rb->memset(mad_frame_overlap, 0, sizeof(mad_frame_overlap));
    init_mad(mad_frame_overlap);

    disk_buf_end = disk_buf_start + disk_buf_size-MPEG_GUARDBUF_SIZE;

    /* initalize start_pts_time and end_pts_time with the length (in half 
       minutes) of the movie. zero if the time could not be determined */
    find_start_pts( in_file );
    find_end_pts( in_file );

 
    /* start menu */
    rb->lcd_clear_display();
    rb->lcd_update();
    result = mpeg_start_menu(end_pts_time-start_pts_time, in_file);
    
    switch (result)
    {
        case MPEG_START_QUIT:
            rb->talk_disable(false);
            return 0;
        default:
            start_time = settings.resume_time;
            break;
    }

    /* basic time checks */
    if ( start_time < 0 )
        start_time = 0;
    else if ( start_time > (end_pts_time-start_pts_time) )
        start_time = (end_pts_time-start_pts_time);

    /* Turn off backlight timeout */
    backlight_force_on(rb); /* backlight control in lib/helper.c */

#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    rb->cpu_boost(true);
#endif

    /* From this point on we've altered settings, colors, cpu_boost, etc. and
       cannot just return PLUGIN_ERROR - instead drop though to cleanup code
     */

#ifdef SIMULATOR
    /* The simulator thread implementation doesn't have stack buffers, and
       these parameters are ignored. */
    (void)i;  /* Keep gcc happy */
    audio_stack = NULL;
    audio_stack_size = 0;
#else
    /* Borrow the codec thread's stack (in IRAM on most targets) */
    audio_stack = NULL;
    for (i = 0; i < MAXTHREADS; i++)
    {
        if (rb->strcmp(rb->threads[i].name,"codec")==0)
        {
            /* Wait to ensure the codec thread has blocked */
            while (rb->threads[i].state!=STATE_BLOCKED)
                rb->yield();

            /* Now we can steal the stack */
            audio_stack = rb->threads[i].stack;
            audio_stack_size = rb->threads[i].stack_size;

            /* Backup the codec thread's stack */
            rb->memcpy(codec_stack_copy,audio_stack,audio_stack_size);    

            break;
        }
    }

    if (audio_stack == NULL)
    {
        /* This shouldn't happen, but deal with it anyway by using
           the copy instead */
        audio_stack = codec_stack_copy;
        audio_stack_size = AUDIO_STACKSIZE;
    }
#endif
    
    rb->splash(0, "Loading...");
    
    /* seek start time */
    seek_pos = seek_PTS( in_file, start_time, 0 );

    video_thumb_print = 0;
    audio_sync_start = 0;
    audio_sync_time = 0;
    video_sync_start = 0;

    /* Read some stream data */
    disk_buf_len = rb->read (in_file, disk_buf_start, disk_buf_size - MPEG_GUARDBUF_SIZE);

    disk_buf_tail = disk_buf_start + disk_buf_len;
    file_remaining = rb->filesize(in_file);
    file_remaining -= disk_buf_len + seek_pos;

    initialize_stream( &video_str, disk_buf_start, disk_buf_len, 0xe0 );
    initialize_stream( &audio_str, disk_buf_start, disk_buf_len, 0xc0 );

    rb->mutex_init(&audio_str.msg_lock);
    rb->mutex_init(&video_str.msg_lock);

    audio_str.status = STREAM_BUFFERING;
    video_str.status = STREAM_PLAYING;

#ifndef HAVE_LCD_COLOR
    gray_show(true);
#endif

    init_stream_lock();

#if NUM_CORES > 1
    flush_icache();
#endif

    /* We put the video thread on the second processor for multi-core targets. */
    if ((video_str.thread = rb->create_thread(video_thread,
        (uint8_t*)video_stack, VIDEO_STACKSIZE, 0,
        "mpgvideo" IF_PRIO(,PRIORITY_PLAYBACK) IF_COP(, COP))) == NULL)
    {
        rb->splash(HZ, "Cannot create video thread!");
    }
    else if ((audio_str.thread = rb->create_thread(audio_thread,
        (uint8_t*)audio_stack,audio_stack_size, 0,"mpgaudio"
        IF_PRIO(,PRIORITY_PLAYBACK) IF_COP(, CPU))) == NULL)
    {
        rb->splash(HZ, "Cannot create audio thread!");
    }
    else
    {
        rb->lcd_setfont(FONT_SYSFIXED);

        /* Wait until both threads have finished their work */
        while ((audio_str.status >= 0) || (video_str.status >= 0))
        {
            size_t audio_remaining = audio_str.buffer_remaining;
            size_t video_remaining = video_str.buffer_remaining;

            if (MIN(audio_remaining,video_remaining) < MPEG_LOW_WATERMARK) 
            {

                size_t bytes_to_read = disk_buf_size - MPEG_GUARDBUF_SIZE -
                                       MAX(audio_remaining,video_remaining);

                bytes_to_read = MIN(bytes_to_read,(size_t)(disk_buf_end-disk_buf_tail));

                while (( bytes_to_read > 0) && (file_remaining > 0) &&
                       ((audio_str.status != STREAM_DONE) || (video_str.status != STREAM_DONE)))
                {

                    size_t n;
                    if ( video_sync_start != 0 )
                        n = rb->read(in_file, disk_buf_tail, MIN(32*1024,bytes_to_read));
                    else 
                    {
                        n = rb->read(in_file, disk_buf_tail,bytes_to_read);
                        if (n==0)
                            rb->splash(30,"buffer fill error");
                    }                   

                    bytes_to_read -= n;
                    file_remaining -= n;

                    lock_stream();
                    audio_str.buffer_remaining += n;
                    video_str.buffer_remaining += n;
                    unlock_stream();

                    disk_buf_tail += n;

                    rb->yield();
                }

                if (disk_buf_tail == disk_buf_end)
                    disk_buf_tail = disk_buf_start;
            }

            rb->sleep(HZ/10);
        }

        rb->lcd_setfont(FONT_UI);
        status = PLUGIN_OK;
    }

    /* Stop the threads and wait for them to terminate */
    if (video_str.thread != NULL)
    {
        str_send_msg(&video_str, STREAM_QUIT, 0);
        rb->thread_wait(video_str.thread);
    }

    if (audio_str.thread != NULL)
    {
        str_send_msg(&audio_str, STREAM_QUIT, 0);
        rb->thread_wait(audio_str.thread);
    }

#if NUM_CORES > 1
    invalidate_icache();
#endif

    vo_cleanup();

#ifndef HAVE_LCD_COLOR
    gray_release();
#endif

    rb->lcd_clear_display();
    rb->lcd_update();

    mpeg2_close (mpeg2dec);

    rb->close (in_file);

#ifndef SIMULATOR
    /* Restore the codec thread's stack */
    rb->memcpy(audio_stack, codec_stack_copy, audio_stack_size);    
#endif

#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    rb->cpu_boost(false);
#endif

    save_settings();  /* Save settings (if they have changed) */

    rb->pcm_set_frequency(HW_SAMPR_DEFAULT);

    /* Turn on backlight timeout (revert to settings) */
    backlight_use_settings(rb); /* backlight control in lib/helper.c */
    rb->talk_disable(false);
    return status;
}
