/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2005 Linus Nielsen Feltzing
 * Copyright (C) 2006 Antonius Hellmann
 * Copyright (C) 2006-2013 Michael Sevakis
 *
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
#include "string-extra.h"
#include "pcm_record.h"
#include "codecs.h"
#include "logf.h"
#include "thread.h"
#include "storage.h"
#include "general.h"
#include "codec_thread.h"
#include "audio.h"
#include "sound.h"
#include "metadata.h"
#include "appevents.h"
#ifdef HAVE_SPDIF_IN
#include "spdif.h"
#endif
#include "audio_thread.h"
#include "core_alloc.h"
#include "talk.h"

/* Macros to enable logf for queues
   logging on SYS_TIMEOUT can be disabled */
#ifdef SIMULATOR
/* Define this for logf output of all queuing except SYS_TIMEOUT */
#define PCMREC_LOGQUEUES
/* Define this to logf SYS_TIMEOUT messages */
/*#define PCMREC_LOGQUEUES_SYS_TIMEOUT*/
#endif /* SIMULATOR */

#ifdef PCMREC_LOGQUEUES
#define LOGFQUEUE logf
#else
#define LOGFQUEUE(...)
#endif

#ifdef PCMREC_LOGQUEUES_SYS_TIMEOUT
#define LOGFQUEUE_SYS_TIMEOUT logf
#else
#define LOGFQUEUE_SYS_TIMEOUT(...)
#endif

/** Target-related configuration **/

/**
 * PCM_NUM_CHUNKS:    Number of PCM chunks
 * PCM_CHUNK_SAMP:    Number of samples in a PCM chunk
 * PCM_BOOST_SECONDS: PCM level at which to boost CPU
 * PANIC_SECONDS:     Flood watermark time until full
 * FLUSH_SECONDS:     Flush watermark time until full
 * STREAM_BUF_SIZE:   Size of stream write buffer
 * PRIO_SECONDS:      Max flush time before prio boost
 *
 * Total PCM buffer size should be mem aligned
 *
 * Fractions should be left without parentheses so the multiplier is
 * multiplied by the numerator first.
 */
#if MEMORYSIZE <= 2
#define PCM_NUM_CHUNKS         56
#define PCM_CHUNK_SAMP       1024
#define PCM_BOOST_SECONDS       1/2
#define PANIC_SECONDS           1/2
#define FLUSH_SECONDS           1
#define FLUSH_MON_INTERVAL      1/6
#define STREAM_BUF_SIZE     32768
#elif MEMORYSIZE <= 16
#define PANIC_SECONDS           5
#define FLUSH_SECONDS           7
#else /* MEMORYSIZE > 16 */
#define PANIC_SECONDS           8
#define FLUSH_SECONDS          10
#endif /* MEMORYSIZE */

/* Default values if not overridden above */
#ifndef PCM_NUM_CHUNKS
#define PCM_NUM_CHUNKS        256
#endif
#ifndef PCM_CHUNK_SAMP
#define PCM_CHUNK_SAMP       2048
#endif
#ifndef PCM_BOOST_SECONDS
#define PCM_BOOST_SECONDS       1
#endif
#ifndef FLUSH_MON_INTERVAL
#define FLUSH_MON_INTERVAL      1/4
#endif
#ifndef STREAM_BUF_SIZE
#define STREAM_BUF_SIZE     65536
#endif
#ifndef PRIO_SECONDS
#define PRIO_SECONDS           10
#endif

/* FAT limit for filesize. Recording will accept no further data from the
 * codec if this limit is reached in order to preserve its own data
 * integrity. A split should have made by the higher-ups long before this
 * point.
 *
 * Leave a generous 64k margin for metadata being added to file. */
#define MAX_NUM_REC_BYTES   ((size_t)0x7fff0000u)

/***************************************************************************/
extern struct codec_api ci;            /* in codec_thread.c */
extern struct event_queue audio_queue; /* in audio_thread.c */
extern unsigned int audio_thread_id;   /* in audio_thread.c */

/** General recording state **/

/* Recording action being performed */
static enum record_status
{
    RECORD_STOPPED      = 0,
    RECORD_PRERECORDING = AUDIO_STATUS_PRERECORD,
    RECORD_RECORDING    = AUDIO_STATUS_RECORD,
    RECORD_PAUSED       = (AUDIO_STATUS_RECORD | AUDIO_STATUS_PAUSE),
} record_status = RECORD_STOPPED;

/* State of engine operations */
static enum record_state
{
    REC_STATE_IDLE,    /* Stopped or prerecording  */
    REC_STATE_MONITOR, /* Monitoring buffer status */
    REC_STATE_FLUSH,   /* Flushing buffer          */
} record_state = REC_STATE_IDLE;

static uint32_t      errors;             /* An error has occured (bitmask) */
static uint32_t      warnings;           /* Non-fatal warnings (bitmask)   */

static uint32_t      rec_errors;         /* Mirror of errors but private to
                                          * avoid race with controlling
                                          * thread. Engine uses this
                                          * internally. */

/** Stats on encoded data for current file **/
static int           rec_fd = -1;        /* Currently open file descriptor */
static size_t        num_rec_bytes;      /* Number of bytes recorded       */
static uint64_t      num_rec_samples;    /* Number of PCM samples recorded */
static uint64_t      encbuf_rec_count;   /* Count of slots written to buffer
                                            for current file               */

/** These apply to current settings **/
static int           rec_source;         /* Current rec_source setting     */
static unsigned long sample_rate;        /* Samplerate setting in HZ       */
static int           num_channels;       /* Current number of channels     */
static struct encoder_config enc_config; /* Current encoder configuration  */
static unsigned int  pre_record_seconds; /* Pre-record time in seconds     */

/****************************************************************************
  Use 2 circular buffers:
  pcm_buffer=DMA output buffer:    chunks (8192 Bytes) of raw pcm audio data
  enc_buffer=encoded audio buffer: storage for encoder output data

  Flow:
  1. When entering recording_screen DMA feeds the ringbuffer pcm_buffer
  2. If enough pcm data are available the encoder codec does encoding of pcm
      chunks (4-8192 Bytes) into ringbuffer enc_buffer in codec_thread
  3. pcmrec_callback detects enc_buffer 'near full' and writes data to disk

  Functions calls (basic encoder steps):
   1.audio:   codec_load();               load the encoder
   2.encoder: enc_init_parameters();      set the encoder parameters (at load)
   3.audio:   enc_callback();             configure encoder recording settings
   4.audio:   codec_go();                 start encoding the new stream
   5.encoder: enc_encbuf_get_buffer();    obtain an output buffer of size n
   6.encoder: enc_pcmbuf_read();          read n bytes of unprocessed pcm data
   7.encoder: enc_encbuf_finish_buffer(); add the obtained buffer to output
   8.encoder: enc_pcmbuf_advance();       advance pcm by n samples
   9.encoder: while more PCM available, repeat 5. to 9.
  10.audio:   codec_finish_stream();      finish the output for current stream

  Function calls (basic stream flushing steps through enc_callback()):
   1.audio:   flush_stream_start();       stream flush destination is opening
   2.audio:   flush_stream_data();        flush encoded audio to stream
   3.audio:   while encoded data available, repeat 2.
   4.audio:   flush_stream_end();         stream flush destination is closing

****************************************************************************/

/** Buffer parameters where incoming PCM data is placed **/
#define PCM_DEPTH_BYTES (sizeof (int16_t))
#define PCM_SAMP_SIZE   (2*PCM_DEPTH_BYTES)
#define PCM_CHUNK_SIZE  (PCM_CHUNK_SAMP*PCM_SAMP_SIZE)
#define PCM_BUF_SIZE    (PCM_NUM_CHUNKS*PCM_CHUNK_SIZE)

/* Convert byte sizes into buffer slot counts */
#define CHUNK_SIZE_COUNT(size) \
    (((size) + ENC_HDR_SIZE - 1) / ENC_HDR_SIZE)
#define CHUNK_FILE_COUNT(size) \
    ({ typeof (size) __size = (size); \
       CHUNK_SIZE_COUNT(MIN(__size, MAX_PATH) + ENC_HDR_SIZE); })
#define CHUNK_FILE_COUNT_PATH(path) \
    CHUNK_FILE_COUNT(strlen(path) + 1)
#define CHUNK_DATA_COUNT(size) \
    CHUNK_SIZE_COUNT((size) + sizeof (struct enc_chunk_data))

/* Min margin to write stream split headers without overwrap risk */
#define ENCBUF_MIN_SPLIT_MARGIN \
    (2*(1 + CHUNK_FILE_COUNT(MAX_PATH)) - 1)

static void          *rec_buffer;       /* Root rec buffer pointer         */
static size_t         rec_buffer_size;  /* Root rec buffer size            */

static void          *pcm_buffer;       /* Circular buffer for PCM samples */
static volatile bool  pcm_pause;        /* Freeze DMA write position       */
static volatile size_t pcm_widx;        /* Current DMA write position      */
static volatile size_t pcm_ridx;        /* Current PCM read position       */

static union enc_chunk_hdr *enc_buffer; /* Circular encoding buffer        */
static size_t         enc_widx;         /* Encoder chunk write index       */
static size_t         enc_ridx;         /* Encoder chunk read index        */
static size_t         enc_buflen;       /* Length of buffer in slots       */

static unsigned char *stream_buffer;    /* Stream-to-disk write buffer     */
static ssize_t        stream_buf_used;  /* Stream write buffer occupancy   */

static struct enc_chunk_file *fname_buf;/* Buffer with next file to create */

static unsigned long  enc_sample_rate;  /* Samplerate used by encoder      */
static bool           pcm_buffer_empty; /* All PCM chunks processed?       */

static typeof (memcpy) *pcm_copyfn;     /* PCM memcpy or copy_buffer_mono  */
static enc_callback_t enc_cb;           /* Encoder's recording callback    */

/** File flushing **/
static unsigned long  encbuf_datarate;  /* Rate of data per second         */
#if (CONFIG_STORAGE & STORAGE_ATA)
static int            spinup_time;      /* Last spinup time                */
#endif
static size_t         high_watermark;   /* Max limit for data flush        */

#ifdef HAVE_PRIORITY_SCHEDULING
static size_t         flood_watermark;  /* Max limit for thread prio boost */
static bool           prio_boosted;
#endif

/** Stream marking **/
enum mark_stream_action
{
    MARK_STREAM_END       = 0x1, /* Mark end current stream */
    MARK_STREAM_START     = 0x2, /* Mark start of new stream */
    MARK_STREAM_SPLIT     = 0x3, /* Insert split; orr of above values */
    MARK_STREAM_PRE       = 0x4, /* Do prerecord data tally */
    MARK_STREAM_START_PRE = MARK_STREAM_PRE | MARK_STREAM_START,
};


/***************************************************************************/

/* Buffer pointer (p) to PCM sample memory address */
static inline void * pcmbuf_ptr(size_t p)
{
    return pcm_buffer + p;
}

/* Buffer pointer (p) plus value (v), wrapped if necessary */
static size_t pcmbuf_add(size_t p, size_t v)
{
    size_t res = p + v;

    if (res >= PCM_BUF_SIZE)
        res -= PCM_BUF_SIZE;

    return res;
}

/* Size of data in PCM buffer */
size_t pcmbuf_used(void)
{
    size_t p1 = pcm_ridx;
    size_t p2 = pcm_widx;

    if (p1 > p2)
        p2 += PCM_BUF_SIZE;

    return p2 - p1;
}

/* Buffer pointer (p) to memory address of header */
static inline union enc_chunk_hdr * encbuf_ptr(size_t p)
{
    return enc_buffer + p;
}

/* Buffer pointer (p) plus value (v), wrapped if necessary */
static size_t encbuf_add(size_t p, size_t v)
{
    size_t res = p + v;

    if (res >= enc_buflen)
        res -= enc_buflen;

    return res;
}

/* Number of free buffer slots */
static size_t encbuf_free(void)
{
    size_t p1 = enc_ridx;
    size_t p2 = enc_widx;

    if (p2 >= p1)
        p1 += enc_buflen;

    return p1 - p2;
}

/* Number of used buffer slots */
static size_t encbuf_used(void)
{
    size_t p1 = enc_ridx;
    size_t p2 = enc_widx;

    if (p1 > p2)
        p2 += enc_buflen;

    return p2 - p1;
}

/* Is the encoder buffer empty? */
static bool encbuf_empty(void)
{
    return enc_ridx == enc_widx;
}

/* Buffer pointer (p) plus size (v), written to enc_widx, new widx
 * zero-initialized */
static void encbuf_widx_advance(size_t widx, size_t v)
{
    widx = encbuf_add(widx, v);
    encbuf_ptr(widx)->zero = 0;
    enc_widx = widx;
}

/* Buffer pointer (p) plus size of chunk at (p), wrapped to (0) if
 * necessary.
 *
 * pout points to variable to receive increment result
 *
 * Returns NULL if it was a wrap marker */
static void * encbuf_read_ptr_incr(size_t p, size_t *pout)
{
    union enc_chunk_hdr *hdr = encbuf_ptr(p);
    size_t v;

    switch (hdr->type)
    {
    case CHUNK_T_DATA:
        v = CHUNK_DATA_COUNT(hdr->size);
        break;
    case CHUNK_T_STREAM_START:
        v = hdr->size;
        break;
    case CHUNK_T_STREAM_END:
    default:
        v = 1;
        break;
    case CHUNK_T_WRAP:
        /* Wrap markers are not returned but caller may have to know that
           the index was changed since it impacts available space */
        *pout = 0;
        return NULL;
    }

    *pout = encbuf_add(p, v);
    return hdr;
}

/* Buffer pointer (p) of contiguous free space (v), wrapped to (0) if
 * necessary.
 *
 * pout points to variable to receive possible-adjusted p
 *
 * Returns header at (p) or wrapped header at (0) if wrap was
 * required in order to provide contiguous space. Header is zero-
 * initialized.
 *
 * Marks the wrap point if a wrap is required to make the allocation. */
static void * encbuf_get_write_ptr(size_t p, size_t v, size_t *pout)
{
    union enc_chunk_hdr *hdr = encbuf_ptr(p);

    if (p + v > enc_buflen)
    {
        hdr->type = CHUNK_T_WRAP; /* All other fields ignored */
        p = 0;
        hdr = encbuf_ptr(0);
    }

    *pout = p;
    hdr->zero = 0;
    return hdr;
}

/* Post a flush request to audio thread, if none is currently queued */
static void encbuf_request_flush(void)
{
    if (!queue_peek_ex(&audio_queue, NULL, 0,
                       &(const long [2]){ Q_AUDIO_RECORD_FLUSH,
                                          Q_AUDIO_RECORD_FLUSH }))
        queue_post(&audio_queue, Q_AUDIO_RECORD_FLUSH, 0);
}

/* Set the error bits in (e): no lock */
static inline void set_error_bits(uint32_t e)
{
    errors |= e;
    rec_errors |= e;
}

/* Clear the error bits in (e): no lock */
static inline void clear_error_bits(uint32_t e)
{
    errors &= ~e;
}

/* Set the error bits in (e) */
static void raise_error_status(uint32_t e)
{
    pcm_rec_lock();
    set_error_bits(e);
    pcm_rec_unlock();
}

/* Clear the error bits in (e) */
static void clear_error_status(uint32_t e)
{
    pcm_rec_lock();
    clear_error_bits(e);
    pcm_rec_unlock();
}

/* Set the warning bits in (w): no lock */
static inline void set_warning_bits(uint32_t w)
{
    warnings |= w;
}

/* Clear the warning bits in (w): no lock */
static inline void clear_warning_bits(uint32_t w)
{
    warnings &= ~w;
}

/* Set the warning bits in (w) */
static void raise_warning_status(uint32_t w)
{
    pcm_rec_lock();
    set_warning_bits(w);
    pcm_rec_unlock();
}

/* Clear the warning bits in (w) */
static void clear_warning_status(uint32_t w)
{
    pcm_rec_lock();
    clear_warning_bits(w);
    pcm_rec_unlock();
}

/* Callback for when more data is ready - called by DMA ISR */
static void pcm_rec_have_more(void **start, size_t *size)
{
    size_t next_idx = pcm_widx;

    if (!pcm_pause)
    {
        /* One empty chunk must remain after widx is advanced */
        if (pcmbuf_used() <= PCM_BUF_SIZE - 2*PCM_CHUNK_SIZE)
            next_idx = pcmbuf_add(next_idx, PCM_CHUNK_SIZE);
        else
            set_warning_bits(PCMREC_W_PCM_BUFFER_OVF);
    }

    *start = pcmbuf_ptr(next_idx);
    *size = PCM_CHUNK_SIZE;

    pcm_widx = next_idx;
}

static enum pcm_dma_status pcm_rec_status_callback(enum pcm_dma_status status)
{
    if (status < PCM_DMAST_OK)
    {
        /* Some error condition */
        if (status == PCM_DMAST_ERR_DMA)
        {
            set_error_bits(PCMREC_E_DMA);
            return status;
        }
        else
        {
            /* Try again next transmission - frame is invalid */
            set_warning_bits(PCMREC_W_DMA);
        }
    }

    return PCM_DMAST_OK;
}

/* Start DMA transfer */
static void pcm_start_recording(void)
{
    pcm_record_data(pcm_rec_have_more, pcm_rec_status_callback,
                    pcmbuf_ptr(pcm_widx), PCM_CHUNK_SIZE);
}

/* Initialize the various recording buffers */
static void init_rec_buffers(void)
{
    /* Layout of recording buffer: |PCMBUF|STREAMBUF|FILENAME|ENCBUF| */
    void *buf = rec_buffer;
    size_t size = rec_buffer_size;

    /* PCMBUF */
    pcm_buffer = CACHEALIGN_UP(buf); /* Line align */
    size -= pcm_buffer + PCM_BUF_SIZE - buf;
    buf = pcm_buffer + PCM_BUF_SIZE;

    /* STREAMBUF */
    stream_buffer = buf;             /* Also line-aligned */
    buf += STREAM_BUF_SIZE;
    size -= STREAM_BUF_SIZE;

    /* FILENAME */
    fname_buf = buf;
    buf += CHUNK_FILE_COUNT(MAX_PATH)*ENC_HDR_SIZE;
    size -= CHUNK_FILE_COUNT(MAX_PATH)*ENC_HDR_SIZE;
    fname_buf->hdr.zero = 0;

    /* ENCBUF */
    enc_buffer = buf;
    enc_buflen = size;
    ALIGN_BUFFER(enc_buffer, enc_buflen, ENC_HDR_SIZE);
    enc_buflen = CHUNK_SIZE_COUNT(enc_buflen);
}

/* Reset the circular buffers */
static void reset_fifos(bool hard)
{
    /* PCM FIFO */
    pcm_pause = true;

    if (hard)
        pcm_widx = 0; /* Don't just empty but reset it */

    pcm_ridx = pcm_widx;

    /* Encoder FIFO */
    encbuf_widx_advance(0, 0);
    enc_ridx = 0;

    /* No overflow-related warnings now */
    clear_warning_status(PCMREC_W_PCM_BUFFER_OVF | PCMREC_W_ENC_BUFFER_OVF);
}

/* Initialize file statistics */
static void reset_rec_stats(void)
{
    num_rec_bytes = 0;
    num_rec_samples = 0;
    encbuf_rec_count = 0;
    clear_warning_status(PCMREC_W_FILE_SIZE);
}

/* Boost or unboost recording threads' priorities */
static void do_prio_boost(bool boost)
{
#ifdef HAVE_PRIORITY_SCHEDULING
    prio_boosted = boost;

    int prio = PRIORITY_RECORDING;

    if (boost)
        prio -= 4;

    codec_thread_set_priority(prio);
    thread_set_priority(audio_thread_id, prio);
#endif
    (void)boost;
}

/* Reset all relevant state */
static void init_state(void)
{
    reset_fifos(true);
    reset_rec_stats();
    do_prio_boost(false);
    cancel_cpu_boost();
    record_state = REC_STATE_IDLE;
    record_status = RECORD_STOPPED;
}

/* Set hardware samplerate and save it */
static void update_samplerate_config(unsigned long sampr)
{
    /* PCM samplerate is either the same as the setting or the nearest
       one hardware supports if using S/PDIF */
    unsigned long pcm_sampr = sampr;

#ifdef HAVE_SPDIF_IN
    if (rec_source == AUDIO_SRC_SPDIF)
    {
        int index = round_value_to_list32(sampr, hw_freq_sampr,
                                          HW_NUM_FREQ, false);
        pcm_sampr = hw_freq_sampr[index];
    }
#endif /* HAVE_SPDIF_IN */

    pcm_set_frequency(pcm_sampr | SAMPR_TYPE_REC);
    sample_rate = sampr;
}

/* Calculate the average data rate */
static unsigned long get_encbuf_datarate(void)
{
    /* If not yet calculable, start with uncompressed PCM byterate */
    if (num_rec_samples && sample_rate && encbuf_rec_count)
    {
        return (encbuf_rec_count*sample_rate + num_rec_samples - 1)
                    / num_rec_samples;
    }
    else
    {
        return CHUNK_SIZE_COUNT(sample_rate*num_channels*PCM_DEPTH_BYTES);
    }
}

/* Returns true if the watermarks should be updated due to data rate
   change */
static bool monitor_encbuf_datarate(void)
{
    unsigned long rate = get_encbuf_datarate();
    long diff = rate - encbuf_datarate;
    /* Off by more than 1/2 FLUSH_MON_INTERVAL? */
    return 2*(unsigned long)abs(diff) > encbuf_datarate*FLUSH_MON_INTERVAL;
}

/* Get adjusted spinup time */
static int get_spinup_time(void)
{
    int spin = storage_spinup_time();

#if (CONFIG_STORAGE & STORAGE_ATA)
    /* Write at FLUSH_SECONDS + st remaining in enc_buffer - range fs+2s to
       fs+10s total - default to 3.5s spinup. */
    if (spin == 0)
        spin = 35*HZ/10;  /* default - cozy                */
    else if (spin < 2*HZ)
        spin = 2*HZ;      /* ludicrous - ramdisk?          */
    else if (spin > 10*HZ)
        spin = 10*HZ;     /* do you have a functioning HD? */
#endif /* (CONFIG_STORAGE & STORAGE_ATA) */

    return spin;
}

/* Returns true if the watermarks should be updated due to spinup time
   change */
static inline bool monitor_spinup_time(void)
{
#if (CONFIG_STORAGE & STORAGE_ATA)
    return get_spinup_time() != spinup_time;
#else
    return false;
#endif
}

/* Update buffer watermarks with spinup time compensation */
static void refresh_watermarks(void)
{
    int spin = get_spinup_time();
#if (CONFIG_STORAGE & STORAGE_ATA)
    logf("ata spinup: %d", spin);
    spinup_time = spin;
#endif

    unsigned long rate = get_encbuf_datarate();
    logf("byterate: %lu", rate * ENC_HDR_SIZE);
    encbuf_datarate = rate;

    /* Try to start writing with FLUSH_SECONDS remaining after disk spinup */
    high_watermark = (uint64_t)rate*(FLUSH_SECONDS*HZ + spin) / HZ;

    if (high_watermark > enc_buflen)
        high_watermark = enc_buflen;

    high_watermark = enc_buflen - high_watermark;

    logf("high wm: %lu", (unsigned long)high_watermark);

#ifdef HAVE_PRIORITY_SCHEDULING
    /* Boost thread priority if enough ground is lost since flushing started
       or is taking an unreasonably long time */
    flood_watermark = rate*PANIC_SECONDS;

    if (flood_watermark > enc_buflen)
        flood_watermark = enc_buflen;

    flood_watermark = enc_buflen - flood_watermark;

    logf("flood wm: %lu", (unsigned long)flood_watermark);
#endif /* HAVE_PRIORITY_SCHEDULING */
}

/* Tell encoder the stream parameters and get information back */
static bool configure_encoder_stream(void)
{
    struct enc_inputs inputs;
    inputs.sample_rate = sample_rate;
    inputs.num_channels = num_channels;
    inputs.config = &enc_config;

    /* encoder can change these - init with defaults */
    inputs.enc_sample_rate = sample_rate;

    if (enc_cb(ENC_CB_INPUTS, &inputs) < 0)
    {
        raise_error_status(PCMREC_E_ENC_SETUP);
        return false;
    }

    enc_sample_rate = inputs.enc_sample_rate;

    if (enc_sample_rate != sample_rate)
    {
        /* Codec doesn't want to/can't use the setting and has chosen a
           different sample rate */
        raise_warning_status(PCMREC_W_SAMPR_MISMATCH);
        logf("enc sampr:%lu", enc_sample_rate);
    }
    else
    {
        clear_warning_status(PCMREC_W_SAMPR_MISMATCH);
    }

    refresh_watermarks();
    return true;
}

#ifdef HAVE_SPDIF_IN
/* Return the S/PDIF sample rate closest to a value in the master list */
static unsigned long get_spdif_samplerate(void)
{
    unsigned long sr = spdif_measure_frequency();
    int index = round_value_to_list32(sr, audio_master_sampr_list,
                                      SAMPR_NUM_FREQ, false);
    return audio_master_sampr_list[index];
}

/* Check the S/PDIF rate and compare to current setting. Apply the new
 * rate if it changed. */
static void check_spdif_samplerate(void)
{
    unsigned long sampr = get_spdif_samplerate();

    if (sampr == sample_rate)
        return;

    codec_stop();
    pcm_stop_recording();
    reset_fifos(true);
    reset_rec_stats();
    update_samplerate_config(sampr);
    pcm_apply_settings();

    if (!configure_encoder_stream() || rec_errors)
        return;

    pcm_start_recording();

    if (record_status == RECORD_PRERECORDING)
    {
        codec_go();
        pcm_pause = false;
    }
}
#endif /* HAVE_SPDIF_IN */

/* Discard the stream buffer contents */
static inline void stream_discard_buf(void)
{
    stream_buf_used = 0;
}

/* Flush stream buffer to disk */
static bool stream_flush_buf(void)
{
    if (stream_buf_used == 0)
        return true;

    ssize_t rc = write(rec_fd, stream_buffer, stream_buf_used);

    if (LIKELY(rc == stream_buf_used))
    {
        stream_discard_buf();
        return true;
    }

    if (rc > 0)
    {
        /* Some was written; keep in sync */
        stream_buf_used -= rc;
        memmove(stream_buffer, stream_buffer + rc, stream_buf_used);
    }

    return false;
}

/* Close the output file */
static void close_rec_file(void)
{
    if (rec_fd < 0)
        return;

    bool ok = stream_flush_buf();

    if (close(rec_fd) != 0 || !ok)
        raise_error_status(PCMREC_E_IO);

    rec_fd = -1;
}

/* Creates or opens the current path */
static bool open_rec_file(bool create)
{
    if (rec_fd >= 0)
    {
        /* Any previous file should have been closed */
        logf("open file: file already open");
        close_rec_file();
    }

    stream_discard_buf();
    int oflags = create ? O_CREAT|O_TRUNC : 0;
    rec_fd = open(fname_buf->path, O_RDWR|oflags, 0666);

    if (rec_fd < 0)
    {
        raise_error_status(PCMREC_E_IO);
        return false;
    }

    return true;
}

/* Copy with mono conversion - output 1/2 size of input */
static void * ICODE_ATTR
copy_buffer_mono_lr(void *dst, const void *src, size_t src_size)
{
    int16_t *d = (int16_t*) dst;
    int16_t const *s = (int16_t const*) src;
    ssize_t copy_size = src_size;
 
     /* mono = (L + R) / 2 */
    while(copy_size > 0) {
        *d++ = ((int32_t)s[0] + (int32_t)s[1] + 1) >> 1;
        s += 2;
        copy_size -= PCM_SAMP_SIZE;
    }

    return dst;
}

static void * ICODE_ATTR
copy_buffer_mono_l(void *dst, const void *src, size_t src_size)
{
    int16_t *d = (int16_t*) dst;
    int16_t const *s = (int16_t const*) src;
    ssize_t copy_size = src_size;

    /* mono = L */
    while(copy_size > 0) {
        *d++ = *s;
        s += 2;
        copy_size -= PCM_SAMP_SIZE;
    }

    return dst;
}

static void * ICODE_ATTR
copy_buffer_mono_r(void *dst, const void *src, size_t src_size)
{
    return copy_buffer_mono_l(dst, src + 2, src_size);
}


/** pcm_rec_* group **/

/* Clear all errors and warnings */
void pcm_rec_error_clear(void)
{
    clear_error_status(PCMREC_E_ALL);
    clear_warning_status(PCMREC_W_ALL);
}

/* Check mode, errors and warnings */
unsigned int pcm_rec_status(void)
{
    unsigned int ret = record_status;

    if (errors)
        ret |= AUDIO_STATUS_ERROR;

    if (warnings)
        ret |= AUDIO_STATUS_WARNING;

    return ret;
}

/* Return warnings that have occured since recording started */
uint32_t pcm_rec_get_warnings(void)
{
    return warnings;
}

#ifdef HAVE_SPDIF_IN
/* Return the currently-configured sample rate */
unsigned long pcm_rec_sample_rate(void)
{
    return sample_rate;
}
#endif


/** audio_* group **/

/* Initializes recording - call before calling any other recording function */
void audio_init_recording(void)
{
    LOGFQUEUE("audio >| pcmrec Q_AUDIO_INIT_RECORDING");
    audio_queue_send(Q_AUDIO_INIT_RECORDING, 1);
}

/* Closes recording - call audio_stop_recording first or risk data loss */
void audio_close_recording(void)
{
    LOGFQUEUE("audio >| pcmrec Q_AUDIO_CLOSE_RECORDING");
    audio_queue_send(Q_AUDIO_CLOSE_RECORDING, 0);
}

/* Sets recording parameters */
void audio_set_recording_options(struct audio_recording_options *options)
{
    LOGFQUEUE("audio >| pcmrec Q_AUDIO_RECORDING_OPTIONS");
    audio_queue_send(Q_AUDIO_RECORDING_OPTIONS, (intptr_t)options);
}

/* Start recording if not recording or else split */
void audio_record(const char *filename)
{
    LOGFQUEUE("audio >| pcmrec Q_AUDIO_RECORD: %s", filename);
    audio_queue_send(Q_AUDIO_RECORD, (intptr_t)filename);
}

/* audio_record alias for API compatibility with HW codec */
void audio_new_file(const char *filename)
    __attribute__((alias("audio_record")));

/* Stop current recording if recording */
void audio_stop_recording(void)
{
    LOGFQUEUE("audio > pcmrec Q_AUDIO_RECORD_STOP");
    audio_queue_post(Q_AUDIO_RECORD_STOP, 0);
}

/* Pause current recording */
void audio_pause_recording(void)
{
    LOGFQUEUE("audio > pcmrec Q_AUDIO_RECORD_PAUSE");
    audio_queue_post(Q_AUDIO_RECORD_PAUSE, 0);
}

/* Resume current recording if paused */
void audio_resume_recording(void)
{
    LOGFQUEUE("audio > pcmrec Q_AUDIO_RECORD_RESUME");
    audio_queue_post(Q_AUDIO_RECORD_RESUME, 0);
}

/* Set the input source gain. For mono sources, only left gain is used */
void audio_set_recording_gain(int left, int right, int type)
{
#if 0
    logf("pcmrec: t=%d l=%d r=%d", type, left, right);
#endif
    audiohw_set_recvol(left, right, type);
}


/** Information about current state **/

/* Return sample clock in HZ */
static unsigned long get_samples_time(void)
{
    if (enc_sample_rate == 0)
        return 0;

    return (unsigned long)(HZ*num_rec_samples / enc_sample_rate);
}

/* Return current prerecorded time in ticks (playback equivalent time) */
unsigned long audio_prerecorded_time(void)
{
    if (record_status != RECORD_PRERECORDING)
        return 0;

    unsigned long t = get_samples_time();
    return MIN(t, pre_record_seconds*HZ);
}

/* Return current recorded time in ticks (playback equivalent time) */
unsigned long audio_recorded_time(void)
{
    if (record_state == REC_STATE_IDLE)
        return 0;

    return get_samples_time();
}

/* Return number of bytes encoded to output */
unsigned long audio_num_recorded_bytes(void)
{
    if (record_state == REC_STATE_IDLE)
        return 0;

    return num_rec_bytes;
}


/** Data Flushing **/

/* Stream start chunk with path was encountered */
static void flush_stream_start(struct enc_chunk_file *file)
{
    /* Save filename; don't open file here which avoids creating files
       with no audio content. Splitting while paused can create those
       in large numbers. */
    fname_buf->hdr = file->hdr;
    /* Correct size if this was wrap-padded */
    fname_buf->hdr.size = CHUNK_FILE_COUNT(
        strlcpy(fname_buf->path, file->path, MAX_PATH) + 1);
}

/* Data chunk was encountered */
static bool flush_stream_data(struct enc_chunk_data *data)
{
    if (fname_buf->hdr.zero)
    {
        /* First data chunk; create the file */
        if (open_rec_file(true))
        {
            /* Inherit some flags from initial data chunk */
            fname_buf->hdr.err  = data->hdr.err;
            fname_buf->hdr.pre  = data->hdr.pre;
            fname_buf->hdr.aux0 = data->hdr.aux0;

            if (enc_cb(ENC_CB_STREAM, fname_buf) < 0)
                raise_error_status(PCMREC_E_ENCODER_STREAM);
        }

        fname_buf->hdr.zero = 0;

        if (rec_errors)
            return false;
    }

    if (rec_fd < 0)
        return true; /* Just keep discarding */

    if (enc_cb(ENC_CB_STREAM, data) < 0)
    {
        raise_error_status(PCMREC_E_ENCODER_STREAM);
        return false;
    }

    return true;
}

/* Stream end chunk was encountered */
static bool flush_stream_end(union enc_chunk_hdr *hdr)
{
    if (rec_fd < 0)
        return true;

    if (enc_cb(ENC_CB_STREAM, hdr) < 0)
    {
        raise_error_status(PCMREC_E_ENCODER_STREAM);
        return false;
    }

    close_rec_file();
    return true;
}

/* Discard remainder of stream in encoder buffer */
static void discard_stream(void)
{
    /* Discard everything up until the next non-data chunk */
    while (!encbuf_empty())
    {
        size_t ridx;
        union enc_chunk_hdr *hdr = encbuf_read_ptr_incr(enc_ridx, &ridx);

        if (hdr && hdr->type != CHUNK_T_DATA)
        {
            if (hdr->type != CHUNK_T_STREAM_START)
                enc_ridx = ridx;
            break;
        }

        enc_ridx = ridx;
    }

    /* Try to finish header by closing and reopening the file. A seek or
       other operation will likely fail because buffers will need to be
       flushed (here and in file code). That will likely fail but a close
       will just close the fd and discard everything. We reopen with what
       actually made it to disk. Modifying existing file contents will
       more than likely succeed even on a full disk. The result might not
       be entirely correct as far as the headers' sizes and counts unless
       the codec can correct that but the sample format information
       should be. */
    if (rec_fd >= 0 && open_rec_file(false))
    {
        /* Synthesize a special end chunk here */
        union enc_chunk_hdr end;
        end.zero = 0;
        end.err  = 1; /* Codec should try to correct anything that's off */
        end.type = CHUNK_T_STREAM_END;
        if (!flush_stream_end(&end))
            close_rec_file();
    }
}

/* Flush a chunk to disk
 *
 * Transitions state from REC_STATE_MONITOR to REC_STATE_FLUSH when buffer
 * is filling. 'margin' is fullness threshold that transitions to flush state.
 *
 * Call with REC_STATE_IDLE to indicate a forced flush which flushes buffer
 * to less than 'margin'.
 */
static enum record_state flush_chunk(enum record_state state, size_t margin)
{
#ifdef HAVE_PRIORITY_SCHEDULING
    static unsigned long prio_tick; /* Timeout for auto boost */
#endif

    size_t used = encbuf_used();

    switch (state)
    {
    case REC_STATE_MONITOR:
        if (monitor_encbuf_datarate() || monitor_spinup_time())
            refresh_watermarks();

        if (used < margin)
            return REC_STATE_MONITOR;

        state = REC_STATE_FLUSH;
        trigger_cpu_boost();

#ifdef HAVE_PRIORITY_SCHEDULING
        prio_tick = current_tick + PRIO_SECONDS*HZ;
#if (CONFIG_STORAGE & STORAGE_ATA)
        prio_tick += spinup_time;
#endif
#endif /* HAVE_PRIORITY_SCHEDULING */

        /* Fall-through */
    case REC_STATE_IDLE: /* As a hint for "forced" */
        if (used < margin)
            break;

        /* Fall-through */
    case REC_STATE_FLUSH:
#ifdef HAVE_PRIORITY_SCHEDULING
        if (!prio_boosted && state != REC_STATE_IDLE &&
            (used >= flood_watermark || TIME_AFTER(current_tick, prio_tick)))
            do_prio_boost(true);
#endif /* HAVE_PRIORITY_SCHEDULING */

        while (used)
        {
            union enc_chunk_hdr *hdr = encbuf_ptr(enc_ridx);
            size_t count = 0;

            switch (hdr->type)
            {
            case CHUNK_T_DATA:
                if (flush_stream_data(ENC_DATA_HDR(hdr)))
                    count = CHUNK_DATA_COUNT(hdr->size);
                break;

            case CHUNK_T_STREAM_START:
                /* Doesn't do stream writes */
                flush_stream_start(ENC_FILE_HDR(hdr));
                count = hdr->size;
                break;

            case CHUNK_T_STREAM_END:
                if (flush_stream_end(hdr))
                    count = 1;
                break;

            case CHUNK_T_WRAP:
                enc_ridx = 0;
                used = encbuf_used();
                continue;
            }

            if (count)
                enc_ridx = encbuf_add(enc_ridx, count);
            else
                discard_stream();

            break;
        }

        if (!encbuf_empty())
            return state;

        break;
    }

    if (encbuf_empty())
    {
        do_prio_boost(false);
        cancel_cpu_boost();
    }

    return REC_STATE_MONITOR;
}

/* Monitor buffer and finish stream, freeing-up space at the same time */
static void finish_stream(bool stopping)
{
    size_t threshold = stopping ? 1 : enc_buflen - ENCBUF_MIN_SPLIT_MARGIN;
    enum record_state state = REC_STATE_MONITOR;
    size_t need = 1;

    while (1)
    {
        switch (state)
        {
        case REC_STATE_IDLE:
            state = flush_chunk(state, threshold);
            continue;

        default:
            if (!need)
                break;

            if (!stopping || pcm_buffer_empty)
            {
                need = codec_finish_stream();

                if (need)
                {
                    need = 2*CHUNK_DATA_COUNT(need) - 1;

                    if (need >= enc_buflen)
                    {
                        need = 0;
                        codec_stop();
                        threshold = 1;
                    }
                    else if (threshold > enc_buflen - need)
                    {
                        threshold = enc_buflen - need;
                    }
                }
            }

            if (!need || encbuf_used() >= threshold)
                state = REC_STATE_IDLE; /* Start flush */
            else
                sleep(HZ/10); /* Don't flood with pings */

            continue;
        }

        break;
    }
}

/* Start a new stream, transistion to a new one or end the current one */
static void mark_stream(const char *path, enum mark_stream_action action)
{
    if (action & MARK_STREAM_END)
    {
        size_t widx;
        union enc_chunk_hdr *hdr = encbuf_get_write_ptr(enc_widx, 1, &widx);
        hdr->type = CHUNK_T_STREAM_END;
        encbuf_widx_advance(widx, 1);
    }

    if (action & MARK_STREAM_START)
    {
        size_t count = CHUNK_FILE_COUNT_PATH(path);
        struct enc_chunk_file *file;
        size_t widx;

        if (action & MARK_STREAM_PRE)
        {
            /* Prerecord: START marker goes first or before existing data */
            if (enc_ridx < count)
            {
                /* Adjust to occupy end of buffer and pad accordingly */
                count += enc_ridx;
                enc_ridx += enc_buflen;
            }

            enc_ridx -= count;

            /* Won't adjust p since enc_ridx is already set as non-wrapping */
            file = encbuf_get_write_ptr(enc_ridx, count, &widx);
        }
        else
        {
            /* The usual: START marker goes first or after existing data */
            file = encbuf_get_write_ptr(enc_widx, count, &widx);
            encbuf_widx_advance(widx, count);
        }

        file->hdr.type = CHUNK_T_STREAM_START;
        file->hdr.size = count;
        strlcpy(file->path, path, MAX_PATH);
    }
}

/* Tally-up and keep the required amount of prerecord data.
 * Updates record stats accordingly. */
static void tally_prerecord_data(void)
{
    unsigned long count = 0;
    size_t bytes = 0;
    unsigned long samples = 0;

    /* Find out how much is there */
    for (size_t idx = enc_ridx; idx != enc_widx;)
    {
        struct enc_chunk_data *data = encbuf_read_ptr_incr(idx, &idx);

        if (!data)
            continue;

        count += CHUNK_DATA_COUNT(data->hdr.size);
        bytes += data->hdr.size;
        samples += data->pcm_count;
    }

    /* Have too much? Discard oldest data. */
    unsigned long pre_samples = enc_sample_rate*pre_record_seconds;

    while (samples > pre_samples)
    {
        struct enc_chunk_data *data =
            encbuf_read_ptr_incr(enc_ridx, &enc_ridx);

        if (!data)
            continue;

        count -= CHUNK_DATA_COUNT(data->hdr.size);
        bytes -= data->hdr.size;
        samples -= data->pcm_count;
    }

    encbuf_rec_count = count;
    num_rec_bytes = bytes;
    num_rec_samples = samples;
}


/** Event handlers for recording thread **/

static int pcmrec_handle;
/* Q_AUDIO_INIT_RECORDING */
static void on_init_recording(void)
{
    send_event(RECORDING_EVENT_START, NULL);
    /* FIXME: This buffer should play nicer and be shrinkable/movable */
    talk_buffer_set_policy(TALK_BUFFER_LOOSE);
    pcmrec_handle = core_alloc_maximum("pcmrec", &rec_buffer_size, &buflib_ops_locked);
    if (pcmrec_handle <= 0)
    /* someone is abusing core_alloc_maximum(). Fix this evil guy instead of
     * trying to handle OOM without hope */
        panicf("%s(): OOM\n", __func__);
    rec_buffer = core_get_data(pcmrec_handle);
    init_rec_buffers();
    init_state();
    pcm_init_recording();
}

/* Q_AUDIO_CLOSE_RECORDING */
static void on_close_recording(void)
{
    /* Simply shut down the recording system. Whatever wasn't saved is
       lost. */
    codec_unload();
    pcm_close_recording();
    close_rec_file();
    init_state();

    rec_errors = 0;
    pcm_rec_error_clear();

    /* Reset PCM to defaults */
    pcm_set_frequency(HW_SAMPR_RESET | SAMPR_TYPE_REC);
    audio_set_output_source(AUDIO_SRC_PLAYBACK);
    pcm_apply_settings();

    if (pcmrec_handle > 0)
        pcmrec_handle = core_free(pcmrec_handle);
    talk_buffer_set_policy(TALK_BUFFER_DEFAULT);

    send_event(RECORDING_EVENT_STOP, NULL);
}

/* Q_AUDIO_RECORDING_OPTIONS */
static void on_recording_options(struct audio_recording_options *options)
{
    if (!options)
    {
        logf("options: option NULL!");
        return;
    }

    if (record_state != REC_STATE_IDLE)
    {
        /* This would ruin things */
        logf("options: still recording!");
        return;
    }

    /* Stop everything else that might be running */
    pcm_stop_recording();

    int afmt = rec_format_afmt[options->enc_config.rec_format];
    bool enc_load = true;

    if (codec_loaded() != AFMT_UNKNOWN)
    {
        if (get_audio_base_codec_type(enc_config.afmt) !=
            get_audio_base_codec_type(afmt))
        {
            /* New format, new encoder; unload this one */
            codec_unload();
        }
        else
        {
            /* Keep current encoder */
            codec_stop();
            enc_load = false;
        }
    }

    init_state();

    /* Read recording options, remember the ones used elsewhere */
    unsigned frequency = options->rec_frequency;
    rec_source         = options->rec_source;
    num_channels       = options->rec_channels == 1 ? 1 : 2;
    unsigned mono_mode = options->rec_mono_mode;
    pre_record_seconds = options->rec_prerecord_time;
    enc_config         = options->enc_config;
    enc_config.afmt    = afmt;

    queue_reply(&audio_queue, 0);  /* Let caller go */

    /* Pick appropriate PCM copy routine */
    pcm_copyfn = memcpy;

    if (num_channels == 1)
    {
        static typeof (memcpy) * const copy_buffer_mono[] =
        {
            copy_buffer_mono_lr,
            copy_buffer_mono_l,
            copy_buffer_mono_r
        };

        if (mono_mode >= ARRAYLEN(copy_buffer_mono))
            mono_mode = 0;

        pcm_copyfn = copy_buffer_mono[mono_mode];
    }

    /* Get the hardware samplerate to be used */
    unsigned long sampr;

#ifdef HAVE_SPDIF_IN
    if (rec_source == AUDIO_SRC_SPDIF)
        sampr = get_spdif_samplerate(); /* Determined by source */
    else
#endif /* HAVE_SPDIF_IN */
        sampr = rec_freq_sampr[frequency];

    update_samplerate_config(sampr);

    /* Set monitoring */
    audio_set_output_source(rec_source);

    /* Apply hardware setting to start monitoring now */
    pcm_apply_settings();

    if (!enc_load || codec_load(-1, afmt | CODEC_TYPE_ENCODER))
    {
        enc_cb = codec_get_enc_callback();

        if (!enc_cb || !configure_encoder_stream())
        {
            codec_unload();
            return;
        }

        if (pre_record_seconds != 0)
        {
            record_status = RECORD_PRERECORDING;
            codec_go();
            pcm_pause = false;
        }

        pcm_start_recording();
    }
    else
    {
        logf("set rec opt: enc load failed");
        raise_error_status(PCMREC_E_LOAD_ENCODER);
    }
}

/* Q_AUDIO_RECORD - start recording (not gapless)
                    or split stream (gapless) */
static void on_record(const char *filename)
{
    if (rec_errors)
    {
        logf("on_record: errors not cleared");
        return;
    }

    if (!filename)
    {
        logf("on_record: No filename");
        return;
    }

    if (codec_loaded() == AFMT_UNKNOWN)
    {
        logf("on_record: Recording options not set");
        return;
    }

    logf("on_record: new file '%s'", filename);

    /* Copy path and let caller go */
    char path[MAX_PATH];
    strlcpy(path, filename, MAX_PATH);

    queue_reply(&audio_queue, 0);

    enum mark_stream_action mark_action;

    if (record_state == REC_STATE_IDLE)
    {
        mark_action = MARK_STREAM_START;

        if (pre_record_seconds)
        {
            codec_pause();
            tally_prerecord_data();
            mark_action = MARK_STREAM_START_PRE;
        }

        clear_warning_status(PCMREC_W_ALL &
                             ~(PCMREC_W_SAMPR_MISMATCH|PCMREC_W_DMA));
        record_state = REC_STATE_MONITOR;
        record_status = RECORD_RECORDING;
    }
    else
    {
        /* Already recording, just split the stream */
        logf("inserting split");
        mark_action = MARK_STREAM_SPLIT;
        finish_stream(false);
        reset_rec_stats();
    }

    if (rec_errors)
    {
        pcm_pause = true;
        codec_stop();
        reset_fifos(false);
        return;
    }

    mark_stream(path, mark_action);

    codec_go();
    pcm_pause = record_status != RECORD_RECORDING;
}

/* Q_AUDIO_RECORD_STOP */
static void on_record_stop(void)
{
    if (record_state == REC_STATE_IDLE)
        return;

    trigger_cpu_boost();

    /* Drain encoder and PCM buffers */
    pcm_pause = true;
    finish_stream(true);

    /* End stream at last data and flush end marker */
    mark_stream(NULL, MARK_STREAM_END);
    while (flush_chunk(REC_STATE_IDLE, 1) == REC_STATE_IDLE);

    reset_fifos(false);

    bool prerecord = pre_record_seconds != 0;

    if (rec_errors)
    {
        codec_stop();
        prerecord = false;
    }

    close_rec_file();
    rec_errors = 0;

    record_state = REC_STATE_IDLE;
    record_status = prerecord ? RECORD_PRERECORDING : RECORD_STOPPED;
    reset_rec_stats();

    if (prerecord)
    {
        codec_go();
        pcm_pause = false;
    }
}

/* Q_AUDIO_RECORD_PAUSE */
static void on_record_pause(void)
{
    if (record_status != RECORD_RECORDING)
        return;

    pcm_pause = true;
    record_status = RECORD_PAUSED;
}

/* Q_AUDIO_RECORD_RESUME */
static void on_record_resume(void)
{
    if (record_status != RECORD_PAUSED)
        return;

    record_status = RECORD_RECORDING;
    pcm_pause = !!rec_errors;
}

/* Called by audio thread when recording is initialized */
void audio_recording_handler(struct queue_event *ev)
{
#ifdef HAVE_PRIORITY_SCHEDULING
    /* Get current priorities since they get changed */
    int old_prio = thread_get_priority(audio_thread_id);
    int old_cod_prio = codec_thread_get_priority();
#endif

    LOGFQUEUE("record < Q_AUDIO_INIT_RECORDING");
    on_init_recording();

    while (1)
    {
        int watermark = high_watermark;

        switch (ev->id)
        {
        case Q_AUDIO_CLOSE_RECORDING:
            LOGFQUEUE("record < Q_AUDIO_CLOSE_RECORDING");
            goto recording_done;

        case Q_AUDIO_RECORDING_OPTIONS:
            LOGFQUEUE("record < Q_AUDIO_RECORDING_OPTIONS");
            on_recording_options((struct audio_recording_options *)ev->data);
            break;

        case Q_AUDIO_RECORD:
            LOGFQUEUE("record < Q_AUDIO_RECORD: %s", (const char *)ev->data);
            on_record((const char *)ev->data);
            break;

        case Q_AUDIO_RECORD_STOP:
            LOGFQUEUE("record < Q_AUDIO_RECORD_STOP");
            on_record_stop();
            break;

        case Q_AUDIO_RECORD_PAUSE:
            LOGFQUEUE("record < Q_AUDIO_RECORD_PAUSE");
            on_record_pause();
            break;

        case Q_AUDIO_RECORD_RESUME:
            LOGFQUEUE("record < Q_AUDIO_RECORD_RESUME");
            on_record_resume();
            break;

        case Q_AUDIO_RECORD_FLUSH:
            watermark = 1;
            break;

        case SYS_USB_CONNECTED:
            LOGFQUEUE("record < SYS_USB_CONNECTED");
            if (record_state != REC_STATE_IDLE)
            {
                LOGFQUEUE("  still recording");
                break;
            }

            goto recording_done;
        } /* switch */

        int timeout;

        switch (record_state)
        {
        case REC_STATE_FLUSH:
        case REC_STATE_MONITOR:
            do
                record_state = flush_chunk(record_state, watermark);
            while (record_state == REC_STATE_FLUSH &&
                   queue_empty(&audio_queue));

            timeout = record_state == REC_STATE_FLUSH ?
                            HZ*0 : HZ*FLUSH_MON_INTERVAL;
            break;
        case REC_STATE_IDLE:
#ifdef HAVE_SPDIF_IN
            if (rec_source == AUDIO_SRC_SPDIF)
            {
                check_spdif_samplerate();
                timeout = HZ/2;
                break;
            }
#endif /* HAVE_SPDIF_IN */
        default:
            timeout = TIMEOUT_BLOCK;
            break;
        }

        queue_wait_w_tmo(&audio_queue, ev, timeout);
    } /* while */

recording_done:
    on_close_recording();
#ifdef HAVE_PRIORITY_SCHEDULING
    /* Restore normal thread priorities */
    thread_set_priority(audio_thread_id, old_prio);
    codec_thread_set_priority(old_cod_prio);
#endif
}


/** Encoder callbacks **/

/* Read a block of unprocessed PCM data, with mono conversion if
 * num_channels == 1
 *
 * NOTE: Request must be less than the PCM buffer length in samples in order
 *       to progress.
 *       (ie. count <= PCM_NUM_CHUNKS*PCM_CHUNK_SAMP)
 */
static int enc_pcmbuf_read(void *buffer, int count)
{
    size_t avail = pcmbuf_used();
    size_t size = count*PCM_SAMP_SIZE;

    if (count > 0 && avail >= size)
    {
        size_t endidx = pcm_ridx + size;

        if (endidx > PCM_BUF_SIZE)
        {
            size_t wrap = endidx - PCM_BUF_SIZE;
            size_t offset = size -= wrap;

            if (num_channels == 1)
                offset /= 2; /* src offset -> dst offset */

            pcm_copyfn(buffer + offset, pcmbuf_ptr(0), wrap);
        }

        pcm_copyfn(buffer, pcmbuf_ptr(pcm_ridx), size);

        if (avail >= sample_rate*PCM_SAMP_SIZE*PCM_BOOST_SECONDS ||
            avail >= PCM_BUF_SIZE*1/2)
        {
            /* Filling up - boost threshold data available or more or 1/2 full
               or more - boost codec */
            trigger_cpu_boost();
        }

        pcm_buffer_empty = false;

        return count;
    }

    /* Not enough data available - encoder should idle */
    pcm_buffer_empty = true;

    cancel_cpu_boost();

    /* Sleep a little bit */
    sleep(0);

    return 0;
}

/* Advance PCM buffer by count samples */
static int enc_pcmbuf_advance(int count)
{
    if (count <= 0)
        return 0;

    size_t avail = pcmbuf_used();
    size_t size = count*PCM_SAMP_SIZE;

    if (avail < size)
    {
        size = avail;
        count = size / PCM_SAMP_SIZE;
    }

    pcm_ridx = pcmbuf_add(pcm_ridx, size);

    return count;
}

/* Return encoder chunk at current write position, wrapping to 0 if
 * requested size demands it.
 *
 * NOTE: No request should be more than 1/2 the buffer length, all elements
 *       included, or progress will not be guaranteed.
 *       (ie. CHUNK_DATA_COUNT(need) <= enc_buflen / 2)
 */
static struct enc_chunk_data * enc_encbuf_get_buffer(size_t need)
{
    /* Convert to buffer slot count, including the header */
    need = CHUNK_DATA_COUNT(need);

    enum record_state state = record_state;
    size_t avail = encbuf_free();

    /* Must have the split margin as well but it does not have to be
       continuous with the request */
    while (avail <= need + ENCBUF_MIN_SPLIT_MARGIN ||
            (enc_widx + need > enc_buflen &&
             enc_ridx <= need + ENCBUF_MIN_SPLIT_MARGIN))
    {
        if (UNLIKELY(state == REC_STATE_IDLE))
        {
            /* Prerecording - delete some old data */
            size_t ridx;
            struct enc_chunk_data *data =
                encbuf_read_ptr_incr(enc_ridx, &ridx);

            if (data)
            {
                encbuf_rec_count -= CHUNK_DATA_COUNT(data->hdr.size);
                num_rec_bytes -= data->hdr.size;
                num_rec_samples -= data->pcm_count;
            }

            enc_ridx = ridx;
            avail = encbuf_free();
            continue;
        }
        else if (avail == enc_buflen)
        {
            /* Empty but request larger than any possible space */
            raise_warning_status(PCMREC_W_ENC_BUFFER_OVF);
        }
        else if (state != REC_STATE_FLUSH && encbuf_used() < high_watermark)
        {
            /* Not yet even at high watermark but what's needed won't fit */
            encbuf_request_flush();
        }

        sleep(0);
        return NULL;
    }

    struct enc_chunk_data *data =
        encbuf_get_write_ptr(enc_widx, need, &enc_widx);

    if (state == REC_STATE_IDLE)
        data->hdr.pre = 1;

    return data;
}

/* Releases the current buffer into the available chunks */
static void enc_encbuf_finish_buffer(void)
{
    struct enc_chunk_data *data = ENC_DATA_HDR(encbuf_ptr(enc_widx));

    if (data->hdr.err)
    {
        /* Encoder set error flag */
        raise_error_status(PCMREC_E_ENCODER);
        return;
    }

    size_t data_size = data->hdr.size;

    if (data_size == 0)
        return; /* Claims nothing was written */

    size_t count = CHUNK_DATA_COUNT(data_size);
    size_t avail = encbuf_free();

    if (avail <= count || enc_widx + count > enc_buflen)
    {
        /* Claims it wrote too much? */
        raise_warning_status(PCMREC_W_ENC_BUFFER_OVF);
        return;
    }

    if (num_rec_bytes + data_size > MAX_NUM_REC_BYTES)
    {
        /* Would exceed filesize limit; should have split sooner.
           This chunk will be dropped. :'( */
        raise_warning_status(PCMREC_W_FILE_SIZE);
        return;
    }

    encbuf_widx_advance(enc_widx, count);

    encbuf_rec_count += count;
    num_rec_bytes += data_size;
    num_rec_samples += data->pcm_count;
}

/* Read from the output stream */
static ssize_t enc_stream_read(void *buf, size_t count)
{
    if (!stream_flush_buf())
        return -1;

    return read(rec_fd, buf, count);
}

/* Seek the output steam */
static off_t enc_stream_lseek(off_t offset, int whence)
{
    if (!stream_flush_buf())
        return -1;

    return lseek(rec_fd, offset, whence);
}

/* Write to the output stream */
static ssize_t enc_stream_write(const void *buf, size_t count)
{
    if (UNLIKELY(count >= STREAM_BUF_SIZE))
    {
        /* Too big to buffer */
        if (stream_flush_buf())
            return write(rec_fd, buf, count);
    }

    if (!count)
        return 0;

    if (stream_buf_used + count > STREAM_BUF_SIZE)
    {
        if (!stream_flush_buf() && stream_buf_used + count > STREAM_BUF_SIZE)
            count = STREAM_BUF_SIZE - stream_buf_used;
    }

    memcpy(stream_buffer + stream_buf_used, buf, count);
    stream_buf_used += count;

    return count;
}

/* One-time init at startup */
void INIT_ATTR recording_init(void)
{
    /* Init API */
    ci.enc_pcmbuf_read          = enc_pcmbuf_read;
    ci.enc_pcmbuf_advance       = enc_pcmbuf_advance;
    ci.enc_encbuf_get_buffer    = enc_encbuf_get_buffer;
    ci.enc_encbuf_finish_buffer = enc_encbuf_finish_buffer;
    ci.enc_stream_read          = enc_stream_read;
    ci.enc_stream_lseek         = enc_stream_lseek;
    ci.enc_stream_write         = enc_stream_write;
}
