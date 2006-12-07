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
#include "system.h"
#include "kernel.h"
#include "logf.h"
#include "panic.h"
#include "thread.h"
#include <string.h>
#include "ata.h"
#include "usb.h"
#if defined(HAVE_UDA1380)
#include "uda1380.h"
#include "general.h"
#elif defined(HAVE_TLV320)
#include "tlv320.h"
#endif
#include "buffer.h"
#include "audio.h"
#include "sound.h"
#include "id3.h"
#ifdef HAVE_SPDIF_IN
#include "spdif.h"
#endif

/***************************************************************************/

/**
 * APIs implemented in the target tree portion:
 * Public -
 *      pcm_init_recording
 *      pcm_close_recording
 *      pcm_rec_mux
 * Semi-private -
 *      pcm_rec_dma_start
 *      pcm_rec_dma_stop
 */

/** These items may be implemented target specifically or need to
    be shared semi-privately **/

/* the registered callback function for when more data is available */
volatile pcm_more_callback_type2 pcm_callback_more_ready = NULL;
/* DMA transfer in is currently active */
volatile bool                    pcm_recording           = false;

/** General recording state **/
static bool is_recording;              /* We are recording */
static bool is_paused;                 /* We have paused   */
static bool is_stopping;                 /* We are currently stopping      */
static bool is_error;                  /* An error has occured */

/** Stats on encoded data for current file **/
static size_t        num_rec_bytes;      /* Num bytes recorded             */
static unsigned long num_rec_samples;    /* Number of PCM samples recorded */

/** Stats on encoded data for all files from start to stop **/
#if 0
static unsigned long long accum_rec_bytes; /* total size written to chunks */
static unsigned long long accum_pcm_samples; /* total pcm count processed  */
#endif

/* Keeps data about current file and is sent as event data for codec */
static struct enc_file_event_data rec_fdata IDATA_ATTR =
{
    .chunk           =  NULL,
    .new_enc_size    =  0,
    .new_num_pcm     =  0,
    .rec_file        = -1,
    .num_pcm_samples =  0
};

/** These apply to current settings **/
static int           rec_source;         /* current rec_source setting     */
static int           rec_frequency;      /* current frequency setting      */
static unsigned long sample_rate;        /* Sample rate in HZ              */
static int           num_channels;       /* Current number of channels     */
static struct encoder_config enc_config; /* Current encoder configuration  */
  
/****************************************************************************
  use 2 circular buffers:
  pcm_buffer=DMA output buffer:    chunks (8192 Bytes) of raw pcm audio data
  enc_buffer=encoded audio buffer: storage for encoder output data

  Flow:
  1. when entering recording_screen DMA feeds the ringbuffer pcm_buffer
  2. if enough pcm data are available the encoder codec does encoding of pcm
      chunks (4-8192 Bytes) into ringbuffer enc_buffer in codec_thread
  3. pcmrec_callback detects enc_buffer 'near full' and writes data to disk

  Functions calls (basic encoder steps):
  1.main:    audio_load_encoder();   start the encoder
  2.encoder: enc_get_inputs();       get encoder recording settings
  3.encoder: enc_set_parameters();   set the encoder parameters
  4.encoder: enc_get_pcm_data();     get n bytes of unprocessed pcm data
  5.encoder: enc_pcm_buf_near_empty(); if 1: reduce cpu_boost
  6.encoder: enc_alloc_chunk();      get a ptr to next enc chunk
  7.encoder: <process enc chunk>     compress and store data to enc chunk
  8.encoder: enc_free_chunk();       inform main about chunk process finished
  9.encoder: repeat 4. to 8.
  A.pcmrec:  enc_events_callback();   called for certain events
****************************************************************************/

/** buffer parameters where incoming PCM data is placed **/
#define PCM_NUM_CHUNKS            256 /* Power of 2 */
#define PCM_CHUNK_SIZE           8192 /* Power of 2 */
#define PCM_CHUNK_MASK          (PCM_NUM_CHUNKS*PCM_CHUNK_SIZE - 1)

#define GET_PCM_CHUNK(offset)   ((long *)(pcm_buffer + (offset)))
#define GET_ENC_CHUNK(index)    ENC_CHUNK_HDR(enc_buffer + enc_chunk_size*(index))

#define INC_ENC_INDEX(index) \
            { if (++index >= enc_num_chunks) index = 0; }
#define DEC_ENC_INDEX(index) \
            { if (--index < 0) index = enc_num_chunks - 1; }

static size_t         rec_buffer_size; /* size of available buffer         */
static unsigned char *pcm_buffer;      /* circular recording buffer        */
static unsigned char *enc_buffer;      /* circular encoding buffer         */
static volatile int   dma_wr_pos;      /* current DMA write pos            */
static int            pcm_rd_pos;      /* current PCM read pos             */
static volatile bool  dma_lock;        /* lock DMA write position          */
static unsigned long  pre_record_ticks;/* pre-record time in ticks         */
static int            enc_wr_index;    /* encoder chunk write index        */
static int            enc_rd_index;    /* encoder chunk read index         */
static int            enc_num_chunks;  /* number of chunks in ringbuffer     */
static size_t         enc_chunk_size;  /* maximum encoder chunk size       */
static size_t         enc_data_size;   /* maximum data size for encoder    */
static unsigned long  enc_sample_rate; /* sample rate used by encoder      */
static bool           wav_queue_empty; /* all wav chunks processed?          */
 
/** file flushing **/
static int            write_threshold; /* max chunk limit for data flush   */
static int            panic_threshold; /* boost thread prio when here      */
static int            spinup_time = -1;/* last ata_spinup_time             */

/** encoder events **/
static void (*enc_events_callback)(enum enc_events event, void *data);

/** Path queue for files to write **/
#define FNQ_MIN_NUM_PATHS 16           /* minimum number of paths to hold  */
static unsigned char *fn_queue;        /* pointer to first filename        */
static ssize_t        fnq_size;        /* capacity of queue in bytes       */
static int            fnq_rd_pos;      /* current read position            */
static int            fnq_wr_pos;      /* current write position           */

/***************************************************************************/

static struct event_queue  pcmrec_queue;
static long                pcmrec_stack[3*DEFAULT_STACK_SIZE/sizeof(long)];
static const char          pcmrec_thread_name[] = "pcmrec";

static void pcmrec_thread(void);

/* Event values which are also single-bit flags */
#define PCMREC_INIT         0x00000001  /* enable recording                */
#define PCMREC_CLOSE        0x00000002

#define PCMREC_START        0x00000004  /* start recording (when stopped)  */
#define PCMREC_STOP         0x00000008  /* stop the current recording      */
#define PCMREC_PAUSE        0x00000010  /* pause the current recording     */
#define PCMREC_RESUME       0x00000020  /* resume the current recording    */
#define PCMREC_NEW_FILE     0x00000040  /* start new file (when recording) */
#define PCMREC_SET_GAIN     0x00000080
#define PCMREC_FLUSH_NUM    0x00000100  /* flush a number of files out     */
#define PCMREC_FINISH_STOP  0x00000200  /* finish the stopping recording   */

/* mask for signaling events */
static volatile long pcm_thread_event_mask;

static void pcm_thread_sync_post(long event, void *data)
{
    pcm_thread_event_mask &= ~event;
    queue_post(&pcmrec_queue, event, data);
    while(!(event & pcm_thread_event_mask))
        yield();
} /* pcm_thread_sync_post */

static inline void pcm_thread_signal_event(long event)
{
    pcm_thread_event_mask |= event;
} /* pcm_thread_signal_event */

static inline void pcm_thread_unsignal_event(long event)
{
    pcm_thread_event_mask &= ~event;
} /* pcm_thread_unsignal_event */

static inline bool pcm_thread_event_state(long signaled, long unsignaled)
{
    return ((signaled | unsignaled) & pcm_thread_event_mask) == signaled;
} /* pcm_thread_event_state */
 
static void pcm_thread_wait_for_stop(void)
{
    if (is_stopping)
    {
        logf("waiting for stop to finish");
        while (is_stopping)
            yield();
    }
} /* pcm_thread_wait_for_stop */

/*******************************************************************/
/* Functions that are not executing in the pcmrec_thread first     */
/*******************************************************************/
    
/* Callback for when more data is ready */
static int pcm_rec_have_more(int status)
{
    if (status < 0)
    {
        /* some error condition */
        if (status == DMA_REC_ERROR_DMA)
        {
            /* Flush recorded data to disk and stop recording */
            queue_post(&pcmrec_queue, PCMREC_STOP, NULL);
            return -1;
        }
        /* else try again next transmission */
    }
    else if (!dma_lock)
    {
        /* advance write position */
        dma_wr_pos = (dma_wr_pos + PCM_CHUNK_SIZE) & PCM_CHUNK_MASK;
    }

    pcm_record_more(GET_PCM_CHUNK(dma_wr_pos), PCM_CHUNK_SIZE);
    return 0;
} /* pcm_rec_have_more */

static void reset_hardware(void)
{
    /* reset pcm to defaults (playback only) */
    pcm_set_frequency(HW_SAMPR_DEFAULT);
    audio_set_output_source(AUDIO_SRC_PLAYBACK);
    pcm_apply_settings(true);
}

/** pcm_rec_* group **/
void pcm_rec_error_clear(void)
{
    is_error = false;
} /* pcm_rec_error_clear */

unsigned long pcm_rec_status(void)
{
    unsigned long ret = 0;

    if (is_recording)
        ret |= AUDIO_STATUS_RECORD;

    if (is_paused)
        ret |= AUDIO_STATUS_PAUSE;

    if (is_error)
        ret |= AUDIO_STATUS_ERROR;

    if (!is_recording && pre_record_ticks &&
        pcm_thread_event_state(PCMREC_INIT, PCMREC_CLOSE))
        ret |= AUDIO_STATUS_PRERECORD;

    return ret;
} /* pcm_rec_status */

#if 0
int pcm_rec_current_bitrate(void)
{
    if (accum_pcm_samples == 0)
        return 0;

    return (int)(8*accum_rec_bytes*enc_sample_rate / (1000*accum_pcm_samples));
} /* pcm_rec_current_bitrate */
#endif

#if 0
int pcm_rec_encoder_afmt(void)
{
    return enc_config.afmt;
} /* pcm_rec_encoder_afmt */
#endif

#if 0
int pcm_rec_rec_format(void)
{
    return afmt_rec_format[enc_config.afmt];
} /* pcm_rec_rec_format */
#endif

#ifdef HAVE_SPDIF_IN
unsigned long pcm_rec_sample_rate(void)
{
    /* Which is better ?? */
#if 0
    return enc_sample_rate;
#endif
    return sample_rate;
} /* audio_get_sample_rate */
#endif

/**
 * Creates pcmrec_thread
 */
void pcm_rec_init(void)
{
    queue_init(&pcmrec_queue, true);
    create_thread(pcmrec_thread, pcmrec_stack, sizeof(pcmrec_stack),
                  pcmrec_thread_name, PRIORITY_RECORDING);
} /* pcm_rec_init */

/** audio_* group **/

void audio_init_recording(unsigned int buffer_offset)
{
    (void)buffer_offset;
    pcm_thread_wait_for_stop();
    pcm_thread_sync_post(PCMREC_INIT, NULL);
} /* audio_init_recording */

void audio_close_recording(void)
{
    pcm_thread_wait_for_stop();
    pcm_thread_sync_post(PCMREC_CLOSE, NULL);
    reset_hardware();
    audio_remove_encoder();
} /* audio_close_recording */

unsigned long audio_recorded_time(void)
{
    if (!is_recording || enc_sample_rate == 0)
        return 0;

    /* return actual recorded time a la encoded data even if encoder rate
       doesn't match the pcm rate */
    return (long)(HZ*(unsigned long long)num_rec_samples / enc_sample_rate);
} /* audio_recorded_time */

unsigned long audio_num_recorded_bytes(void)
{
    if (!is_recording)
        return 0;

    return num_rec_bytes;
} /* audio_num_recorded_bytes */
    
#ifdef HAVE_SPDIF_IN
/**
 * Return SPDIF sample rate index in audio_master_sampr_list. Since we base
 * our reading on the actual SPDIF sample rate (which might be a bit
 * inaccurate), we round off to the closest sample rate that is supported by
 * SPDIF.
 */
int audio_get_spdif_sample_rate(void)
{
    unsigned long measured_rate = spdif_measure_frequency();
    /* Find which SPDIF sample rate we're closest to. */
    return round_value_to_list32(measured_rate, audio_master_sampr_list,
                                 SAMPR_NUM_FREQ, false);
} /* audio_get_spdif_sample_rate */
#endif /* HAVE_SPDIF_IN */

/**
 * Sets recording parameters
 */
void audio_set_recording_options(struct audio_recording_options *options)
{
    pcm_thread_wait_for_stop();

    /* stop DMA transfer */
    dma_lock = true;
    pcm_stop_recording();

    rec_frequency      = options->rec_frequency;
    rec_source         = options->rec_source;
    num_channels       = options->rec_channels == 1 ? 1 : 2;
    pre_record_ticks   = options->rec_prerecord_time * HZ;
    enc_config         = options->enc_config;
    enc_config.afmt    = rec_format_afmt[enc_config.rec_format];

#ifdef HAVE_SPDIF_IN
    if (rec_source == AUDIO_SRC_SPDIF)
    {
        /* must measure SPDIF sample rate before configuring codecs */
        unsigned long sr = spdif_measure_frequency();
        /* round to master list for SPDIF rate */
        int index = round_value_to_list32(sr, audio_master_sampr_list,
                                          SAMPR_NUM_FREQ, false);
        sample_rate = audio_master_sampr_list[index];
        /* round to HW playback rates for monitoring */
        index = round_value_to_list32(sr, hw_freq_sampr,
                                      HW_NUM_FREQ, false);
        pcm_set_frequency(hw_freq_sampr[index]);
        /* encoders with a limited number of rates do their own rounding */
    }
    else
#endif
    {
        /* set sample rate from frequency selection */
        sample_rate = rec_freq_sampr[rec_frequency];
        pcm_set_frequency(sample_rate);
    }

    /* set monitoring */
    audio_set_output_source(rec_source);

    /* apply pcm settings to hardware */
    pcm_apply_settings(true);

    if (audio_load_encoder(enc_config.afmt))
    {
        /* start DMA transfer */
        dma_lock = pre_record_ticks == 0;
        pcm_record_data(pcm_rec_have_more, GET_PCM_CHUNK(dma_wr_pos),
                        PCM_CHUNK_SIZE);
    }
    else
    {
        logf("set rec opt: enc load failed");
        is_error = true;
    }
} /* audio_set_recording_options */

/**
 * Start recording
 * 
 * Use audio_set_recording_options first to select recording options
 */
void audio_record(const char *filename)
{
    logf("audio_record: %s", filename);

    pcm_thread_wait_for_stop();
    pcm_thread_sync_post(PCMREC_START, (void *)filename);

    logf("audio_record_done");
} /* audio_record */

void audio_new_file(const char *filename)
{
    logf("audio_new_file: %s", filename);

    pcm_thread_wait_for_stop();
    pcm_thread_sync_post(PCMREC_NEW_FILE, (void *)filename);

    logf("audio_new_file done");
} /* audio_new_file */

void audio_stop_recording(void)
{
    logf("audio_stop_recording");

    pcm_thread_wait_for_stop();
    pcm_thread_sync_post(PCMREC_STOP, NULL);

    logf("audio_stop_recording done");
} /* audio_stop_recording */

void audio_pause_recording(void)
{
    logf("audio_pause_recording");
    
    pcm_thread_wait_for_stop();
    pcm_thread_sync_post(PCMREC_PAUSE, NULL);

    logf("audio_pause_recording done");
} /* audio_pause_recording */
    
void audio_resume_recording(void)
{
    logf("audio_resume_recording");

    pcm_thread_wait_for_stop();
    pcm_thread_sync_post(PCMREC_RESUME, NULL);

    logf("audio_resume_recording done");
} /* audio_resume_recording */

/***************************************************************************/
/*                                                                         */
/*         Functions that execute in the context of pcmrec_thread          */
/*                                                                         */
/***************************************************************************/

/** Filename Queue **/

/* returns true if the queue is empty */
static inline bool pcmrec_fnq_is_empty(void)
{
    return  fnq_rd_pos == fnq_wr_pos;
} /* pcmrec_fnq_is_empty */

/* empties the filename queue */
static inline void pcmrec_fnq_set_empty(void)
{
    fnq_rd_pos = fnq_wr_pos;
} /* pcmrec_fnq_set_empty */
        
/* returns true if the queue is full */
static bool pcmrec_fnq_is_full(void)
{
    ssize_t size = fnq_wr_pos - fnq_rd_pos;
    if (size < 0)
        size += fnq_size;
    
    return size >= fnq_size - MAX_PATH;
} /* pcmrec_fnq_is_full */
    
/* queue another filename - will overwrite oldest one if full */
static bool pcmrec_fnq_add_filename(const char *filename)
{
    strncpy(fn_queue + fnq_wr_pos, filename, MAX_PATH);
    
    if ((fnq_wr_pos += MAX_PATH) >= fnq_size)
        fnq_wr_pos = 0;
    
    if (fnq_rd_pos != fnq_wr_pos)
        return true;

    /* queue full */
    if ((fnq_rd_pos += MAX_PATH) >= fnq_size)
        fnq_rd_pos = 0;

    return true;
} /* pcmrec_fnq_add_filename */

/* replace the last filename added */
static bool pcmrec_fnq_replace_tail(const char *filename)
{
    int pos;

    if (pcmrec_fnq_is_empty())
        return false;

    pos = fnq_wr_pos - MAX_PATH;
    if (pos < 0)
        pos = fnq_size - MAX_PATH;

    strncpy(fn_queue + pos, filename, MAX_PATH);

    return true;
} /* pcmrec_fnq_replace_tail */

/* pulls the next filename from the queue */
static bool pcmrec_fnq_get_filename(char *filename)
{
    if (pcmrec_fnq_is_empty())
        return false;

    if (filename)
        strncpy(filename, fn_queue + fnq_rd_pos, MAX_PATH);
    
    if ((fnq_rd_pos += MAX_PATH) >= fnq_size)
        fnq_rd_pos = 0;

    return true;
} /* pcmrec_fnq_get_filename */

/* close the file number pointed to by fd_p */
static void pcmrec_close_file(int *fd_p)
{
    if (*fd_p < 0)
        return; /* preserve error */

    close(*fd_p);
    *fd_p = -1;
} /* pcmrec_close_file */

/** Data Flushing **/

/**
 * called after callback to update sizes if codec changed the amount of data
 * a chunk represents
 */
static inline void pcmrec_update_sizes_inl(size_t prev_enc_size,
                                           unsigned long prev_num_pcm)
{
    if (rec_fdata.new_enc_size != prev_enc_size)
    {
        ssize_t size_diff = rec_fdata.new_enc_size - prev_enc_size;
        num_rec_bytes   += size_diff;
#if 0
        accum_rec_bytes += size_diff;
#endif
    }

    if (rec_fdata.new_num_pcm != prev_num_pcm)
    {
        unsigned long pcm_diff = rec_fdata.new_num_pcm - prev_num_pcm;
        num_rec_samples   += pcm_diff;
#if 0
        accum_pcm_samples += pcm_diff;
#endif
    }
} /* pcmrec_update_sizes_inl */

/* don't need to inline every instance */
static void pcmrec_update_sizes(size_t prev_enc_size,
                                unsigned long prev_num_pcm)
{
    pcmrec_update_sizes_inl(prev_enc_size, prev_num_pcm);
} /* pcmrec_update_sizes */

static void pcmrec_start_file(void)
{
    size_t        enc_size = rec_fdata.new_enc_size;
    unsigned long num_pcm  = rec_fdata.new_num_pcm;
    int curr_rec_file      = rec_fdata.rec_file;
    char filename[MAX_PATH];

    /* must always pull the filename that matches with this queue */
    if (!pcmrec_fnq_get_filename(filename))
    {
        logf("start file: fnq empty");
        *filename = '\0';
        is_error = true;
    }
    else if (is_error)
    {
        logf("start file: is_error already");
    }
    else if (curr_rec_file >= 0)
    {
        /* Any previous file should have been closed */
        logf("start file: file already open");
        is_error = true;
    }
    
    if (is_error)
        rec_fdata.chunk->flags |= CHUNKF_ERROR;

    /* encoder can set error flag here and should increase
       enc_new_size and pcm_new_size to reflect additional
       data written if any */
    rec_fdata.filename = filename;
    enc_events_callback(ENC_START_FILE, &rec_fdata);

    if (!is_error && (rec_fdata.chunk->flags & CHUNKF_ERROR))
    {
        logf("start file: enc error");
        is_error = true;
    }

    if (is_error)
    {
        pcmrec_close_file(&curr_rec_file);
        /* Write no more to this file */
        rec_fdata.chunk->flags |= CHUNKF_END_FILE;
    }
    else
    {
        pcmrec_update_sizes(enc_size, num_pcm);
    }
    
    rec_fdata.chunk->flags &= ~CHUNKF_START_FILE;
} /* pcmrec_start_file */

static inline void pcmrec_write_chunk(void)
{
    size_t        enc_size = rec_fdata.new_enc_size;
    unsigned long num_pcm  = rec_fdata.new_num_pcm;

    if (is_error)
        rec_fdata.chunk->flags |= CHUNKF_ERROR;

    enc_events_callback(ENC_WRITE_CHUNK, &rec_fdata);

    if ((long)rec_fdata.chunk->flags >= 0)
    {
        pcmrec_update_sizes_inl(enc_size, num_pcm);
    }
    else if (!is_error)
    {
        logf("wr chk enc error %d %d",
             rec_fdata.chunk->enc_size, rec_fdata.chunk->num_pcm);
        is_error = true;
    }
} /* pcmrec_write_chunk */

static void pcmrec_end_file(void)
{
    /* all data in output buffer for current file will have been
       written and encoder can now do any nescessary steps to
       finalize the written file */
    size_t        enc_size = rec_fdata.new_enc_size;
    unsigned long num_pcm  = rec_fdata.new_num_pcm;

    enc_events_callback(ENC_END_FILE, &rec_fdata);

    if (!is_error)
    {
        if (rec_fdata.chunk->flags & CHUNKF_ERROR)
        {
            logf("end file: enc error");
            is_error = true;
        }
        else
        {
            pcmrec_update_sizes(enc_size, num_pcm);
        }
    }

    /* Force file close if error */
    if (is_error)
        pcmrec_close_file(&rec_fdata.rec_file);

    rec_fdata.chunk->flags &= ~CHUNKF_END_FILE;
} /* pcmrec_end_file */

/**
 * Process the chunks
 *
 * This function is called when queue_get_w_tmo times out.
 *
 * Set flush_num to the number of files to flush to disk.
 * flush_num = -1 to flush all available chunks to disk.
 * flush_num =  0 normal write thresholding
 * flush_num =  1 or greater - all available chunks of current file plus
 *              flush_num file starts if first chunk has been processed.
 *
 */
static void pcmrec_flush(unsigned flush_num)
{
    static unsigned long last_flush_tick = 0;
    unsigned long start_tick;
    int num_ready, num;
    int prio;
    int i;

    num_ready = enc_wr_index - enc_rd_index;
    if (num_ready < 0)
        num_ready += enc_num_chunks;

    num = num_ready;

    if (flush_num == 0)
    {
        if (!is_recording)
            return;

        if (ata_spinup_time != spinup_time)
        {
            /* spinup time has changed, calculate new write threshold */
            logf("new t spinup : %d", ata_spinup_time);
            unsigned long st = spinup_time = ata_spinup_time;

            /* write at 5s + st remaining in enc_buffer */
            if (st < 2*HZ)
                st = 2*HZ; /* my drive is usually < 250 ticks :) */
            else if (st > 10*HZ)
                st = 10*HZ;

            write_threshold = enc_num_chunks -
                (int)(((5ull*HZ + st)*4ull*sample_rate + (enc_chunk_size-1)) /
                       (enc_chunk_size*HZ));

            if (write_threshold < 0)
                write_threshold = 0;
            else if (write_threshold > panic_threshold)
                write_threshold = panic_threshold;

            logf("new wr thresh: %d", write_threshold);
        }

        if (num_ready < write_threshold)
            return;

        /* if we're getting called too much and this isn't forced,
           boost stat */
        if (current_tick - last_flush_tick < HZ/2)
            num = panic_threshold;
    }

    start_tick = current_tick;
    prio = -1;

    logf("writing: %d (%d)", num_ready, flush_num);
        
    cpu_boost(true);

    for (i=0; i<num_ready; i++)
    {
        if (prio == -1 && (num >= panic_threshold ||
                           current_tick - start_tick > 10*HZ))
        {
            /* losing ground - boost priority until finished */
            logf("pcmrec: boost priority");
            prio = thread_set_priority(NULL, thread_get_priority(NULL)-1);
        }

        rec_fdata.chunk        = GET_ENC_CHUNK(enc_rd_index);
        rec_fdata.new_enc_size = rec_fdata.chunk->enc_size;
        rec_fdata.new_num_pcm  = rec_fdata.chunk->num_pcm;

        if (rec_fdata.chunk->flags & CHUNKF_START_FILE)
        {
            pcmrec_start_file();
            if (--flush_num == 0)
                i = num_ready; /* stop on next loop - must write this
                                  chunk if it has data */
        }

        pcmrec_write_chunk();

        if (rec_fdata.chunk->flags & CHUNKF_END_FILE)
            pcmrec_end_file();

        INC_ENC_INDEX(enc_rd_index);

        if (is_error)
            break;

        if (prio == -1)
        {
            num = enc_wr_index - enc_rd_index;
            if (num < 0)
                num += enc_num_chunks;
        }

        /* no yielding, the file apis called in the codecs do that */
    } /* end for */

        /* sync file */
    if (rec_fdata.rec_file >= 0)
        fsync(rec_fdata.rec_file);

        cpu_boost(false);

    if (prio != -1)
    {
        /* return to original priority */
        logf("pcmrec: unboost priority");
        thread_set_priority(NULL, prio);
    }

    last_flush_tick = current_tick; /* save tick when we left */
    logf("done");
} /* pcmrec_flush */

/**
 * Marks a new stream in the buffer and gives the encoder a chance for special
 * handling of transition from one to the next. The encoder may change the
 * chunk that ends the old stream by requesting more chunks and similiarly for
 * the new but must always advance the position though the interface. It can
 * later reject any data it cares to when writing the file but should mark the
 * chunk so it can recognize this. ENC_WRITE_CHUNK event must be able to accept
 * a NULL data pointer without error as well.
 */
static void pcmrec_new_stream(const char *filename, /* next file name */
                              unsigned long flags,  /* CHUNKF_* flags */
                              int pre_index) /* index for prerecorded data */
{
    logf("pcmrec_new_stream");

    struct enc_buffer_event_data data;
    bool (*fnq_add_fn)(const char *) = NULL;
    struct enc_chunk_hdr *start = NULL;

    int get_chunk_index(struct enc_chunk_hdr *chunk)
    {
        return ((char *)chunk - (char *)enc_buffer) / enc_chunk_size;
    }

    struct enc_chunk_hdr * get_prev_chunk(int index)
    {
        DEC_ENC_INDEX(index);
        return GET_ENC_CHUNK(index);
    }

    data.pre_chunk = NULL;
    data.chunk = GET_ENC_CHUNK(enc_wr_index);

    /* end chunk */
    if (flags & CHUNKF_END_FILE)
    {
        data.chunk->flags &= CHUNKF_START_FILE | CHUNKF_END_FILE;

        if (data.chunk->flags & CHUNKF_START_FILE)
        {
            /* cannot start and end on same unprocessed chunk */
            logf("file end on start");
            flags &= ~CHUNKF_END_FILE;
        }
        else if (enc_rd_index == enc_wr_index)
        {
            /* all data flushed but file not ended - chunk will be left
               empty */
            logf("end on dead end");
            data.chunk->flags    = 0;
            data.chunk->enc_size = 0;
            data.chunk->num_pcm  = 0;
            data.chunk->enc_data = NULL;
            INC_ENC_INDEX(enc_wr_index);
            data.chunk = GET_ENC_CHUNK(enc_wr_index);
        }
        else
        {
            struct enc_chunk_hdr *last = get_prev_chunk(enc_wr_index);

            if (last->flags & CHUNKF_END_FILE)
            {
                /* end already processed and marked - can't end twice */
                logf("file end again");
                flags &= ~CHUNKF_END_FILE;
            }
        }
    }

    /* start chunk */
    if (flags & CHUNKF_START_FILE)
    {
        bool pre = flags & CHUNKF_PRERECORD;

        if (pre)
        {
            logf("stream prerecord start");
            start = data.pre_chunk = GET_ENC_CHUNK(pre_index);
            start->flags &= CHUNKF_START_FILE | CHUNKF_PRERECORD;
        } 
        else
        {
            logf("stream normal start");
            start = data.chunk;
            start->flags &= CHUNKF_START_FILE;
        }

        /* if encoder hasn't yet processed the last start - abort the start
           of the previous file queued or else it will be empty and invalid */
        if (start->flags & CHUNKF_START_FILE)
        {
            logf("replacing fnq tail: %s", filename);
            fnq_add_fn = pcmrec_fnq_replace_tail;
        }
        else
        {
                logf("adding filename: %s", filename);
                fnq_add_fn = pcmrec_fnq_add_filename;
        }
    }

    data.flags = flags;
    enc_events_callback(ENC_REC_NEW_STREAM, &data);

    if (flags & CHUNKF_END_FILE)
    {
        int i = get_chunk_index(data.chunk);
        get_prev_chunk(i)->flags |= CHUNKF_END_FILE;
    }

    if (start)
    {
        if (!(flags & CHUNKF_PRERECORD))
        {
            /* get stats on data added to start - sort of a prerecord operation */
            int i = get_chunk_index(data.chunk);
            struct enc_chunk_hdr *chunk = data.chunk;

            logf("start data: %d %d", i, enc_wr_index);

            num_rec_bytes   = 0;
            num_rec_samples = 0;

            while (i != enc_wr_index)
            {
                num_rec_bytes   += chunk->enc_size;
                num_rec_samples += chunk->num_pcm;
                INC_ENC_INDEX(i);
                chunk = GET_ENC_CHUNK(i);
            }

            start->flags &= ~CHUNKF_START_FILE;
            start = data.chunk;
        }

        start->flags |= CHUNKF_START_FILE;

        /* flush all pending files out if full and adding */
        if (fnq_add_fn == pcmrec_fnq_add_filename && pcmrec_fnq_is_full())
        {
            logf("fnq full");
            pcmrec_flush(-1);
        }
   
        fnq_add_fn(filename);
    }
} /* pcmrec_new_stream */

/** event handlers for pcmrec thread */

/* PCMREC_INIT */
static void pcmrec_init(void)
{
    unsigned char *buffer;

    rec_fdata.rec_file = -1;

    /* pcm FIFO */
    dma_lock          = true;
    pcm_rd_pos        = 0;
    dma_wr_pos        = 0;

    /* encoder FIFO */
    enc_wr_index      = 0;
    enc_rd_index      = 0;

    /* filename queue */
    fnq_rd_pos        = 0;
    fnq_wr_pos        = 0;

    /* stats */
    num_rec_bytes     = 0;
    num_rec_samples   = 0;
#if 0
    accum_rec_bytes   = 0;
    accum_pcm_samples = 0;
#endif

    pcm_thread_unsignal_event(PCMREC_CLOSE);
    is_recording      = false;
    is_paused         = false;
    is_stopping       = false;
    is_error          = false;

    buffer = audio_get_recording_buffer(&rec_buffer_size);
    /* Line align pcm_buffer 2^4=16 bytes */
    pcm_buffer = (unsigned char *)ALIGN_UP_P2((unsigned long)buffer, 4);
    enc_buffer = pcm_buffer + ALIGN_UP_P2(PCM_NUM_CHUNKS*PCM_CHUNK_SIZE +
                                          PCM_MAX_FEED_SIZE, 2);
    /* Adjust available buffer for possible align advancement */
    rec_buffer_size -= pcm_buffer - buffer;

    pcm_init_recording();
    pcm_thread_signal_event(PCMREC_INIT);
} /* pcmrec_init */

/* PCMREC_CLOSE */
static void pcmrec_close(void)
{
    dma_lock = true;
    pcm_close_recording();
    pcm_thread_unsignal_event(PCMREC_INIT);
    pcm_thread_signal_event(PCMREC_CLOSE);
} /* pcmrec_close */

/* PCMREC_START */
static void pcmrec_start(const char *filename)
{
    unsigned long pre_sample_ticks;
    int           rd_start;

    logf("pcmrec_start: %s", filename);

    if (is_recording) 
    {
        logf("already recording");
        goto already_recording;
    }

    /* reset stats */
    num_rec_bytes     = 0;
    num_rec_samples   = 0;
#if 0
    accum_rec_bytes   = 0;
    accum_pcm_samples = 0;
#endif
    spinup_time       = -1;

    rd_start  = enc_wr_index;
    pre_sample_ticks = 0;

    if (pre_record_ticks)
    {
        int i;

        /* calculate number of available chunks */
        unsigned long avail_pre_chunks = (enc_wr_index - enc_rd_index +
                        enc_num_chunks) % enc_num_chunks;
        /* overflow at 974 seconds of prerecording at 44.1kHz */
        unsigned long pre_record_sample_ticks = enc_sample_rate*pre_record_ticks;

        /* Get exact measure of recorded data as number of samples aren't
           nescessarily going to be the max for each chunk */
        for (i = rd_start; avail_pre_chunks-- > 0;)
        {
            struct enc_chunk_hdr *chunk;
            unsigned long chunk_sample_ticks;

            DEC_ENC_INDEX(i);

            chunk = GET_ENC_CHUNK(i);

            /* must have data to be counted */
            if (chunk->enc_data == NULL)
                continue;

            chunk_sample_ticks = chunk->num_pcm*HZ;

            rd_start           = i;
            pre_sample_ticks  += chunk_sample_ticks;
            num_rec_bytes     += chunk->enc_size;
            num_rec_samples   += chunk->num_pcm;

            /* stop here if enough already */
            if (pre_sample_ticks >= pre_record_sample_ticks)
                break;
        }

#if 0
        accum_rec_bytes   = num_rec_bytes;
        accum_pcm_samples = num_rec_samples;
#endif
    }

    enc_rd_index = rd_start;

    /* filename queue should be empty */
    if (!pcmrec_fnq_is_empty())
    {
        logf("fnq: not empty!");
        pcmrec_fnq_set_empty();
    }
    
    dma_lock     = false;
    is_paused    = false;
    is_recording = true;

    pcmrec_new_stream(filename,
                      CHUNKF_START_FILE |
                      (pre_sample_ticks > 0 ? CHUNKF_PRERECORD : 0),
                      enc_rd_index);

already_recording:
    pcm_thread_signal_event(PCMREC_START);
    logf("pcmrec_start done");
} /* pcmrec_start */

/* PCMREC_STOP */
static void pcmrec_stop(void)
{
    logf("pcmrec_stop");
   
    if (!is_recording)
    {
        logf("not recording");
        goto not_recording_or_stopping;
    }
    else if (is_stopping)
    {
        logf("already stopping");
        goto not_recording_or_stopping;
    }

    is_stopping = true;
    dma_lock    = true;    /* lock dma write position */
    queue_post(&pcmrec_queue, PCMREC_FINISH_STOP, NULL);

not_recording_or_stopping:
    pcm_thread_signal_event(PCMREC_STOP);
    logf("pcmrec_stop done");
} /* pcmrec_stop */

/* PCMREC_FINISH_STOP */
static void pcmrec_finish_stop(void)
{
    logf("pcmrec_finish_stop");
    
    if (!is_stopping)
    {
        logf("not stopping");
        goto not_stopping;
    }
    
    /* flush all available data first to avoid overflow while waiting
       for encoding to finish */
    pcmrec_flush(-1);
    
    /* wait for encoder to finish remaining data */
    while (!is_error && !wav_queue_empty)
        yield();
    
    /* end stream at last data */
    pcmrec_new_stream(NULL, CHUNKF_END_FILE, 0);
    
    /* flush anything else encoder added */
    pcmrec_flush(-1);

    /* remove any pending file start not yet processed - should be at
       most one at enc_wr_index */
    pcmrec_fnq_get_filename(NULL);
    /* encoder should abort any chunk it was in midst of processing */
    GET_ENC_CHUNK(enc_wr_index)->flags = CHUNKF_ABORT;
    
    /* filename queue should be empty */
    if (!pcmrec_fnq_is_empty())
    {
        logf("fnq: not empty!");
        pcmrec_fnq_set_empty();
    }   

    /* be absolutely sure the file is closed */
    if (is_error)
        pcmrec_close_file(&rec_fdata.rec_file);
    rec_fdata.rec_file = -1;

    is_recording = false;
    is_paused    = false;
    is_stopping  = false;
    dma_lock     = pre_record_ticks == 0;

not_stopping:
    logf("pcmrec_finish_stop done");
} /* pcmrec_finish_stop */

/* PCMREC_PAUSE */
static void pcmrec_pause(void)
{
    logf("pcmrec_pause");

    if (!is_recording)
    {
        logf("not recording");
        goto not_recording_or_paused;
    }
    else if (is_paused)
    {
        logf("already paused");
        goto not_recording_or_paused;
    }
    
    dma_lock  = true;   /* fix DMA write pointer at current position */
    is_paused = true;  
    
not_recording_or_paused:
    pcm_thread_signal_event(PCMREC_PAUSE);
    logf("pcmrec_pause done");
} /* pcmrec_pause */

/* PCMREC_RESUME */
static void pcmrec_resume(void)
{
    logf("pcmrec_resume");
    
    if (!is_recording)
    {
        logf("not recording");
        goto not_recording_or_not_paused;
    }
    else if (!is_paused)
    {
        logf("not paused");
        goto not_recording_or_not_paused;
    }
    
    is_paused = false;
    is_recording = true;
    dma_lock     = false;
    
not_recording_or_not_paused:
    pcm_thread_signal_event(PCMREC_RESUME);
    logf("pcmrec_resume done");
} /* pcmrec_resume */

/* PCMREC_NEW_FILE */
static void pcmrec_new_file(const char *filename)
{
    logf("pcmrec_new_file: %s", filename);

    if (!is_recording)
    {
        logf("not recording");
        goto not_recording;
    }
    
    num_rec_bytes = 0;
    num_rec_samples = 0;

    pcmrec_new_stream(filename,
                      CHUNKF_START_FILE | CHUNKF_END_FILE,
                      0);

not_recording:
    pcm_thread_signal_event(PCMREC_NEW_FILE);
    logf("pcmrec_new_file done");
} /* pcmrec_new_file */

static void pcmrec_thread(void) __attribute__((noreturn));
static void pcmrec_thread(void)
{
    struct event ev;

    logf("thread pcmrec start");

    while(1)
    {
        if (is_recording && !is_stopping)
        {
            /* Poll periodically to flush data */
            queue_wait_w_tmo(&pcmrec_queue, &ev, HZ/5);

            if (ev.id == SYS_TIMEOUT)
            {
                pcmrec_flush(0); /* flush if getting full */
                continue;
            }
        }
        else
        {
            /* Not doing anything - sit and wait for commands */
            queue_wait(&pcmrec_queue, &ev);
        }

        switch (ev.id)
        {
            case PCMREC_INIT: 
                pcmrec_init();
                break;

            case PCMREC_CLOSE:
                pcmrec_close();
                break;

            case PCMREC_START:
                pcmrec_start((const char *)ev.data);
                break;

            case PCMREC_STOP:
                pcmrec_stop();
                break;

            case PCMREC_FINISH_STOP:
                pcmrec_finish_stop();
                break;

            case PCMREC_PAUSE:
                pcmrec_pause();
                break;

            case PCMREC_RESUME:
                pcmrec_resume();
                break;

            case PCMREC_NEW_FILE:
                pcmrec_new_file((const char *)ev.data);
                break;

            case PCMREC_FLUSH_NUM:
                pcmrec_flush((unsigned)ev.data);
                break;

            case SYS_USB_CONNECTED:
                if (is_recording)
                    break;
                pcmrec_close();
                reset_hardware();
                usb_acknowledge(SYS_USB_CONNECTED_ACK);
                usb_wait_for_disconnect(&pcmrec_queue);
                break;
        } /* end switch */
    } /* end while */
} /* pcmrec_thread */

/****************************************************************************/
/*                                                                          */
/*         following functions will be called by the encoder codec          */
/*                                                                          */
/****************************************************************************/

/* pass the encoder settings to the encoder */
void enc_get_inputs(struct enc_inputs *inputs)
{
    inputs->sample_rate  = sample_rate;
    inputs->num_channels = num_channels;
    inputs->config       = &enc_config;
} /* enc_get_inputs */
        
/* set the encoder dimensions (called by encoder codec at initialization and
   termination) */
void enc_set_parameters(struct enc_parameters *params)
{
    size_t bufsize, resbytes;
        
    logf("enc_set_parameters");
    
    if (!params)
    {
        logf("reset");
        /* Encoder is terminating */
        memset(&enc_config, 0, sizeof (enc_config));
        enc_sample_rate = 0;
        return;
    }

    enc_sample_rate = params->enc_sample_rate;
    logf("enc sampr:%d", enc_sample_rate);

    pcm_rd_pos = dma_wr_pos;

    enc_config.afmt     = params->afmt;
    /* addition of the header is always implied - chunk size 4-byte aligned */
    enc_chunk_size      =
                ALIGN_UP_P2(ENC_CHUNK_HDR_SIZE + params->chunk_size, 2);
    enc_data_size       = enc_chunk_size - ENC_CHUNK_HDR_SIZE;
    enc_events_callback = params->events_callback;

    logf("chunk size:%d", enc_chunk_size);

    /*** Configure the buffers ***/

    /* Layout of recording buffer:
     * [ax] = possible alignment x multiple
     * [sx] = possible size alignment of x multiple
     * |[a16]|[s4]:PCM Buffer+PCM Guard|[s4 each]:Encoder Chunks|->
     * |[[s4]:Reserved Bytes]|Filename Queue->|[space]|
     */
    resbytes = ALIGN_UP_P2(params->reserve_bytes, 2);
    logf("resbytes:%d", resbytes);

    bufsize   = rec_buffer_size - (enc_buffer - pcm_buffer) -
                resbytes - FNQ_MIN_NUM_PATHS*MAX_PATH;

    enc_num_chunks = bufsize / enc_chunk_size;
    logf("num chunks:%d", enc_num_chunks);
        
    /* get real amount used by encoder chunks */
    bufsize = enc_num_chunks*enc_chunk_size;
    logf("enc size:%d", bufsize);

    /* panic boost thread priority at 1 second remaining */
    panic_threshold = enc_num_chunks -
                      (4*sample_rate + (enc_chunk_size-1)) / enc_chunk_size;
    if (panic_threshold < 0)
        panic_threshold = 0;

    logf("panic thr:%d", panic_threshold);

    /** set OUT parameters **/
    params->enc_buffer     = enc_buffer;
    params->buf_chunk_size = enc_chunk_size;
    params->num_chunks     = enc_num_chunks;

    /* calculate reserve buffer start and return pointer to encoder */
    params->reserve_buffer = NULL;
    if (resbytes > 0)
    {
        params->reserve_buffer = enc_buffer + bufsize;
        bufsize               += resbytes;
    }

    /* place filename queue at end of buffer using up whatever remains */
    fnq_rd_pos = 0; /* reset */
    fnq_wr_pos = 0; /* reset */
    fn_queue   = enc_buffer + bufsize;
    fnq_size   = pcm_buffer + rec_buffer_size - fn_queue;
    fnq_size   = ALIGN_DOWN(fnq_size, MAX_PATH);
    logf("fnq files: %d", fnq_size / MAX_PATH);

#if 0
    logf("ab :%08X", (unsigned long)audiobuf);
    logf("pcm:%08X", (unsigned long)pcm_buffer);
    logf("enc:%08X", (unsigned long)enc_buffer);
    logf("res:%08X", (unsigned long)params->reserve_buffer);
    logf("fnq:%08X", (unsigned long)fn_queue);
    logf("end:%08X", (unsigned long)fn_queue + fnq_size);
    logf("abe:%08X", (unsigned long)audiobufend);
#endif

    /* init all chunk headers and reset indexes */
    enc_rd_index = 0;
    for (enc_wr_index = enc_num_chunks; enc_wr_index > 0; )
        GET_ENC_CHUNK(--enc_wr_index)->flags = 0;

    logf("enc_set_parameters done");
} /* enc_set_parameters */

/* return encoder chunk at current write position */
struct enc_chunk_hdr * enc_get_chunk(void)
{
    struct enc_chunk_hdr *chunk = GET_ENC_CHUNK(enc_wr_index);
    chunk->flags &= CHUNKF_START_FILE;

    if (!is_recording)
        chunk->flags |= CHUNKF_PRERECORD;

    return chunk;
} /* enc_get_chunk */

/* releases the current chunk into the available chunks */
void enc_finish_chunk(void)
{
    struct enc_chunk_hdr *chunk = GET_ENC_CHUNK(enc_wr_index);

    /* encoder may have set error flag or written too much data */
    if ((long)chunk->flags < 0 || chunk->enc_size > enc_data_size)
    {
        is_error = true;

#ifdef ROCKBOX_HAS_LOGF
        if (chunk->enc_size > enc_data_size)
        {
            /* illegal to scribble over next chunk */
            logf("finish chk ovf: %d>%d", chunk->enc_size, enc_data_size);
        }
        else
        {
            /* encoder set error flag */
            logf("finish chk enc error");
        }
#endif
    }

    /* advance enc_wr_index to the next encoder chunk */
    INC_ENC_INDEX(enc_wr_index);

    if (enc_rd_index != enc_wr_index)
    {
        num_rec_bytes      += chunk->enc_size;
#if 0
        accum_rec_bytes    += chunk->enc_size;
#endif
        num_rec_samples    += chunk->num_pcm;
#if 0
        accum_pcm_samples  += chunk->num_pcm;
#endif
    }
    else if (is_recording)        /* buffer full */
    {
        /* keep current position */
        logf("enc_buffer ovf");
        DEC_ENC_INDEX(enc_wr_index);
    }
    else
    {
        /* advance enc_rd_index for prerecording */
        INC_ENC_INDEX(enc_rd_index);
    }
} /* enc_finish_chunk */

/* checks near empty state on pcm input buffer */
int enc_pcm_buf_near_empty(void)
{
    /* less than 1sec raw data? => unboost encoder */
    int wp       = dma_wr_pos;
    size_t avail = (wp - pcm_rd_pos) & PCM_CHUNK_MASK;
    return avail < (sample_rate << 2) ? 1 : 0;
} /* enc_pcm_buf_near_empty */

/* passes a pointer to next chunk of unprocessed wav data */
/* TODO: this really should give the actual size returned */
unsigned char * enc_get_pcm_data(size_t size)
{
    int wp       = dma_wr_pos;
    size_t avail = (wp - pcm_rd_pos) & PCM_CHUNK_MASK;

    /* limit the requested pcm data size */
    if (size > PCM_MAX_FEED_SIZE)
        size = PCM_MAX_FEED_SIZE;

    if (avail >= size)
    {
        unsigned char *ptr = pcm_buffer + pcm_rd_pos;
        pcm_rd_pos = (pcm_rd_pos + size) & PCM_CHUNK_MASK;

        /* ptr must point to continous data at wraparound position */
        if ((size_t)pcm_rd_pos < size)
            memcpy(pcm_buffer + PCM_NUM_CHUNKS*PCM_CHUNK_SIZE,
                   pcm_buffer, pcm_rd_pos);

        wav_queue_empty = false;
        return ptr;
    }

    /* not enough data available - encoder should idle */
    wav_queue_empty = true;
    return NULL;
} /* enc_get_pcm_data */

/* puts some pcm data back in the queue */
size_t enc_unget_pcm_data(size_t size)
{
    /* can't let DMA advance write position when doing this */
    int level = set_irq_level(HIGHEST_IRQ_LEVEL);

    if (pcm_rd_pos != dma_wr_pos)
    {
        /* disallow backing up into current DMA write chunk  */
        size_t old_avail = (pcm_rd_pos - dma_wr_pos - PCM_CHUNK_SIZE)
                            & PCM_CHUNK_MASK;

        /* limit size to amount of old data remaining */
        if (size > old_avail)
            size = old_avail;

        pcm_rd_pos = (pcm_rd_pos - size) & PCM_CHUNK_MASK;
    }

    set_irq_level(level);

    return size;
} /* enc_unget_pcm_data */

/** Low level pcm recording apis **/

/****************************************************************************
 * Functions that do not require targeted implementation but only a targeted
 * interface
 */
void pcm_record_data(pcm_more_callback_type2 more_ready,
                     void *start, size_t size)
{
    if (!(start && size))
        return;

    pcm_callback_more_ready = more_ready;
    pcm_rec_dma_start(start, size);
} /* pcm_record_data */

void pcm_stop_recording(void)
{
    if (pcm_recording)
        pcm_rec_dma_stop();
} /* pcm_stop_recording */
