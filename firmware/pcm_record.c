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

#include "config.h"
#include "debug.h"
#include "panic.h"
#include "thread.h"

#include <kernel.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <string.h>

#include "cpu.h"
#include "i2c.h"
#include "power.h"
#ifdef HAVE_UDA1380
#include "uda1380.h"
#endif
#ifdef HAVE_TLV320
#include "tlv320.h"
#endif
#include "system.h"
#include "usb.h"

#include "buffer.h"
#include "audio.h"
#include "button.h"
#include "file.h"
#include "sprintf.h"
#include "logf.h"
#include "button.h"
#include "lcd.h"
#include "lcd-remote.h"
#include "pcm_playback.h"
#include "sound.h"
#include "id3.h"
#include "pcm_record.h"

extern int boost_counter; /* used for boost check */

/***************************************************************************/

static bool is_recording;              /* We are recording */
static bool is_paused;                 /* We have paused   */
static bool is_error;                  /* An error has occured */

static unsigned long num_rec_bytes;    /* Num bytes recorded */
static unsigned long num_file_bytes;   /* Num bytes written to current file */
static int error_count;                /* Number of DMA errors */
static unsigned long num_pcm_samples;  /* Num pcm samples written to current file */

static long record_start_time;         /* current_tick when recording was started */
static long pause_start_time;          /* current_tick when pause was started */
static unsigned int sample_rate;       /* Sample rate at time of recording start */
static int rec_source;                 /* Current recording source */

static int wav_file;
static char recording_filename[MAX_PATH];

static volatile bool init_done, close_done, record_done;
static volatile bool stop_done, pause_done, resume_done, new_file_done;

static int peak_left, peak_right;

#ifdef IAUDIO_X5
#define SET_IIS_PLAY(x) IIS1CONFIG = (x);
#define SET_IIS_REC(x)  IIS1CONFIG = (x);
#else
#define SET_IIS_PLAY(x) IIS2CONFIG = (x);
#define SET_IIS_REC(x)  IIS1CONFIG = (x);
#endif
  
/****************************************************************************
  use 2 circular buffers of same size:
  rec_buffer=DMA output buffer:    chunks (8192 Bytes) of raw pcm audio data
  enc_buffer=encoded audio buffer: storage for encoder output data

  Flow:
  1. when entering recording_screen DMA feeds the ringbuffer rec_buffer
  2. if enough pcm data are available the encoder codec does encoding of pcm
      chunks (4-8192 Bytes) into ringbuffer enc_buffer in codec_thread
  3. pcmrec_callback detects enc_buffer 'near full' and writes data to disk

  Functions calls:
  1.main: codec_load_encoder();      start the encoder
  2.encoder: enc_get_inputs();       get encoder buffsize, mono/stereo, quality
  3.encoder: enc_set_parameters();   set the encoder parameters (max.chunksize)
  4.encoder: enc_get_wav_data();     get n bytes of unprocessed pcm data
  5.encoder: enc_wavbuf_near_empty();if true: reduce cpu_boost
  6.encoder: enc_alloc_chunk();      get a ptr to next enc chunk
  7.encoder: <process enc chunk>     compress and store data to enc chunk
  8.encoder: enc_free_chunk();       inform main about chunk process finished
  9.encoder: repeat 4. to 8.
  A.main: enc_set_header_callback(); create the current format header (file)
****************************************************************************/
#define NUM_CHUNKS                   256 /* Power of 2 */
#define CHUNK_SIZE                  8192 /* Power of 2 */
#define MAX_FEED_SIZE              20000 /* max pcm size passed to encoder */
#define CHUNK_MASK                 (NUM_CHUNKS * CHUNK_SIZE - 1)
#define WRITE_THRESHOLD            (44100 * 5 / enc_samp_per_chunk) /* 5sec */
#define GET_CHUNK(x)               (long*)(&rec_buffer[x])
#define GET_ENC_CHUNK(x)           (long*)(&enc_buffer[enc_chunk_size*(x)])

static int            audio_enc_id;    /* current encoder id                 */
static unsigned char *rec_buffer;      /* Circular recording buffer          */
static unsigned char *enc_buffer;      /* Circular encoding buffer           */
static unsigned char *enc_head_buffer; /* encoder header buffer              */
static int            enc_head_size;   /* used size in header buffer         */
static int            write_pos;       /* Current chunk pos for DMA writing  */
static int            read_pos;        /* Current chunk pos for encoding     */
static long           pre_record_ticks;/* pre-record time expressed in ticks */
static int            enc_wr_index;    /* Current encoding chunk write index */
static int            enc_rd_index;    /* Current encoding chunk read index  */
static int            enc_chunk_size;  /* maximum encoder chunk size         */
static int            enc_num_chunks;  /* number of chunks in ringbuffer     */
static int            enc_buffer_size; /* encode buffer size                 */
static int            enc_channels;    /* 1=mono 2=stereo                    */
static int            enc_quality;     /* mp3: 64,96,128,160,192,320 kBit    */
static int            enc_samp_per_chunk;/* pcm samples per encoder chunk    */
static bool           wav_queue_empty; /* all wav chunks processed?          */
static unsigned long  avrg_bit_rate;   /* average bit rates from chunks      */
static unsigned long  curr_bit_rate;   /* cumulated bit rates from chunks    */
static unsigned long  curr_chunk_cnt;  /* number of processed chunks         */
 
void (*enc_set_header_callback)(void *head_buffer, int head_size,
                                int num_pcm_samples, bool is_file_header);

/***************************************************************************/

static struct event_queue  pcmrec_queue;
static long                pcmrec_stack[2*DEFAULT_STACK_SIZE/sizeof(long)];
static const char          pcmrec_thread_name[] = "pcmrec";

static void pcmrec_thread(void);
static void pcmrec_dma_start(void);
static void pcmrec_dma_stop(void);
static void close_wave(void);

/* Event IDs */
#define PCMREC_INIT         1     /* Enable recording */
#define PCMREC_CLOSE        2   

#define PCMREC_START        3     /* Start a new recording */
#define PCMREC_STOP         4     /* Stop the current recording */
#define PCMREC_PAUSE        10
#define PCMREC_RESUME       11
#define PCMREC_NEW_FILE     12
#define PCMREC_SET_GAIN     13

/*******************************************************************/
/* Functions that are not executing in the pcmrec_thread first     */
/*******************************************************************/

/* Creates pcmrec_thread */
void pcm_rec_init(void)
{
    queue_init(&pcmrec_queue, true);
    create_thread(pcmrec_thread, pcmrec_stack, sizeof(pcmrec_stack),
                  pcmrec_thread_name IF_PRIO(, PRIORITY_RECORDING));
}


int audio_get_encoder_id(void)
{
    return audio_enc_id;
}

/* Initializes recording:
 * - Set up the UDA1380/TLV320 for recording 
 * - Prepare for DMA transfers
 */
 
void audio_init_recording(unsigned int buffer_offset)
{
    (void)buffer_offset;

    init_done = false;
    queue_post(&pcmrec_queue, PCMREC_INIT, 0);
    
    while(!init_done)
        sleep_thread(1);
}

void audio_close_recording(void)
{
    close_done = false;
    queue_post(&pcmrec_queue, PCMREC_CLOSE, 0);
    
    while(!close_done)
        sleep_thread(1);

    audio_remove_encoder();
}

unsigned long pcm_rec_status(void)
{
    unsigned long ret = 0;

    if (is_recording)
        ret |= AUDIO_STATUS_RECORD;
    if (is_paused)
        ret |= AUDIO_STATUS_PAUSE;
    if (is_error)
        ret |= AUDIO_STATUS_ERROR;
    if (!is_recording && pre_record_ticks && init_done && !close_done)
        ret |= AUDIO_STATUS_PRERECORD;

    return ret;
}

int pcm_rec_current_bitrate(void)
{
    return avrg_bit_rate;
}

unsigned long audio_recorded_time(void)
{
    if (is_recording)
    {
        if (is_paused)
            return pause_start_time - record_start_time;
        else
            return current_tick - record_start_time;
    }

    return 0;
}

unsigned long audio_num_recorded_bytes(void)
{
    if (is_recording)
        return num_rec_bytes;

    return 0;
}

#ifdef HAVE_SPDIF_IN
/* Only the last six of these are standard rates, but all sample rates are
 * possible, so we support some other common ones as well.
 */
static unsigned long spdif_sample_rates[] = {
    8000, 11025, 12000, 16000, 22050, 24000,
    32000, 44100, 48000, 64000, 88200, 96000
};

/* Return SPDIF sample rate. Since we base our reading on the actual SPDIF
 * sample rate (which might be a bit inaccurate), we round off to the closest
 * sample rate that is supported by SPDIF.
 */
unsigned long audio_get_spdif_sample_rate(void)
{
    int i = 0;
    unsigned long measured_rate;
    const int upper_bound = sizeof(spdif_sample_rates)/sizeof(long) - 1;
    
    /* The following formula is specified in MCF5249 user's manual section
     * 17.6.1. The 3*(1 << 13) part will need changing if the setup of the
     * PHASECONFIG register is ever changed. The 128 divide is because of the
     * fact that the SPDIF clock is the sample rate times 128.
     */
    measured_rate = (unsigned long)((unsigned long long)FREQMEAS*CPU_FREQ/
                                               ((1 << 15)*3*(1 << 13))/128);
    /* Find which SPDIF sample rate we're closest to. */
    while (spdif_sample_rates[i] < measured_rate && i < upper_bound) ++i;
    if (i > 0 && i < upper_bound)
    {
        long diff1 = measured_rate - spdif_sample_rates[i - 1];
        long diff2 = spdif_sample_rates[i] - measured_rate;

        if (diff2 > diff1) --i;
    }
    return i;
}
#endif

#if 0
/* not needed atm */
#ifdef HAVE_SPDIF_POWER
static bool spdif_power_setting;

void audio_set_spdif_power_setting(bool on)
{
    spdif_power_setting = on;
}
#endif
#endif

/**
 * Sets recording parameters
 * 
 * This functions starts feeding the CPU with audio data over the I2S bus
 */
void audio_set_recording_options(int frequency, int quality,
                                int source, int channel_mode,
                                bool editable, int prerecord_time)
{
    /* TODO: */
    (void)editable;

    /* NOTE: Coldfire UDA based recording does not yet support anything other
     * than 44.1kHz sampling rate, so we limit it to that case here now. SPDIF
     * based recording will overwrite this value with the proper sample rate in
     * audio_record(), and will not be affected by this.
     */
    frequency        = 44100;
    enc_quality      = quality;
    rec_source       = source;
    enc_channels     = channel_mode == CHN_MODE_MONO ? 1 : 2;
    pre_record_ticks = prerecord_time * HZ;

    switch (source)
    {
        case AUDIO_SRC_MIC:
        case AUDIO_SRC_LINEIN:
#ifdef HAVE_FMRADIO_IN
        case AUDIO_SRC_FMRADIO:
#endif
            /* Generate int. when 6 samples in FIFO, PDIR2 src = IIS1recv */
            DATAINCONTROL = 0xc020;
        break;

#ifdef HAVE_SPDIF_IN
        case AUDIO_SRC_SPDIF:
            /* Int. when 6 samples in FIFO. PDIR2 source = ebu1RcvData */
            DATAINCONTROL = 0xc038;
        break;
#endif /* HAVE_SPDIF_IN */
    }

    sample_rate = frequency;

    /* Monitoring: route the signals through the coldfire audio interface. */

    SET_IIS_PLAY(0x800); /* Reset before reprogram */
    
#ifdef HAVE_SPDIF_IN
    if (source == AUDIO_SRC_SPDIF)
    {
        /* SCLK2 = Audioclk/4 (can't use EBUin clock), TXSRC = EBU1rcv, 64 bclk/wclk */
        IIS2CONFIG = (6 << 12) | (7 << 8) | (4 << 2);
        /* S/PDIF feed-through already configured */
    }
    else
    {
        /* SCLK2 follow IIS1 (UDA clock), TXSRC = IIS1rcv, 64 bclk/wclk */
        IIS2CONFIG = (8 << 12) | (4 << 8) | (4 << 2);
        
        EBU1CONFIG = 0x800; /* Reset before reprogram */
        /* SCLK2, TXSRC = IIS1recv, validity, normal operation */
        EBU1CONFIG = (7 << 12) | (4 << 8) | (1 << 5) | (5 << 2);
    }
#else
    /* SCLK2 follow IIS1 (UDA clock), TXSRC = IIS1rcv, 64 bclk/wclk */
    SET_IIS_PLAY( (8 << 12) | (4 << 8) | (4 << 2) );
#endif

    audio_load_encoder(rec_quality_info_afmt[quality]);
}


/**
 * Note that microphone is mono, only left value is used 
 * See {uda1380,tlv320}_set_recvol() for exact ranges.
 *
 * @param type   0=line-in (radio), 1=mic
 * 
 */
void audio_set_recording_gain(int left, int right, int type)
{
    //logf("rcmrec: t=%d l=%d r=%d", type, left, right);
#if defined(HAVE_UDA1380)
    uda1380_set_recvol(left, right, type);
#elif defined (HAVE_TLV320)
    tlv320_set_recvol(left, right, type);
#endif            
}


/**
 * Start recording
 * 
 * Use audio_set_recording_options first to select recording options
 */
void audio_record(const char *filename)
{
    if (is_recording)
    {
        logf("record while recording");
        return;
    }
    
    strncpy(recording_filename, filename, MAX_PATH - 1);
    recording_filename[MAX_PATH - 1] = 0;

#ifdef HAVE_SPDIF_IN
    if (rec_source == AUDIO_SRC_SPDIF)
        sample_rate = audio_get_spdif_sample_rate();
#endif
    
    record_done = false;
    queue_post(&pcmrec_queue, PCMREC_START, 0);
    
    while(!record_done)
        sleep_thread(1);
}


void audio_new_file(const char *filename)
{
    logf("pcm_new_file");
        
    new_file_done = false;
    
    strncpy(recording_filename, filename, MAX_PATH - 1);
    recording_filename[MAX_PATH - 1] = 0;
    
    queue_post(&pcmrec_queue, PCMREC_NEW_FILE, 0);
    
    while(!new_file_done)
        sleep_thread(1);
    
    logf("pcm_new_file done");
}

/**
 * 
 */
void audio_stop_recording(void)
{
    if (!is_recording)
        return;

    logf("pcm_stop");
    
    is_paused = true;  /* fix pcm write ptr at current position */
    stop_done = false;
    queue_post(&pcmrec_queue, PCMREC_STOP, 0);

    while(!stop_done)
        sleep_thread(1);

    logf("pcm_stop done");
}

void audio_pause_recording(void)
{
    if (!is_recording)
    {
        logf("pause when not recording");
        return;
    }
    if (is_paused)
    {
        logf("pause when paused");
        return;
    }
    
    pause_done = false;
    queue_post(&pcmrec_queue, PCMREC_PAUSE, 0);

    while(!pause_done)
        sleep_thread(1);
}

void audio_resume_recording(void)
{
    if (!is_paused)
    {
        logf("resume when not paused");
        return;
    }
    
    resume_done = false;
    queue_post(&pcmrec_queue, PCMREC_RESUME, 0);

    while(!resume_done)
        sleep_thread(1);
}

/* return peaks as int, so convert from short first
   note that peak values are always positive */
void pcm_rec_get_peaks(int *left, int *right)
{
    if (left)
        *left = peak_left;
    if (right)
        *right = peak_right;
    peak_left = 0;
    peak_right = 0;
}

/***************************************************************************/
/* Functions that executes in the context of pcmrec_thread                 */
/***************************************************************************/

/**
 * Process the chunks
 *
 * This function is called when queue_get_w_tmo times out.
 *
 * Other functions can also call this function with flush = true when 
 * they want to save everything in the buffers to disk.
 *
 */
static void pcmrec_callback(bool flush)
{
    int  i, num_ready, size_yield;
    long *enc_chunk, chunk_size;

    if (!is_recording && !flush)
        return;

    num_ready = enc_wr_index - enc_rd_index;
    if (num_ready < 0)
        num_ready += enc_num_chunks;

    /* calculate an estimate of recorded bytes */
    num_rec_bytes = num_file_bytes + num_ready * /* enc_chunk_size */
            ((avrg_bit_rate * 1000 / 8 * enc_samp_per_chunk + 22050) / 44100);

    /* near full state reached: less than 5sec remaining space */
    if (enc_num_chunks - num_ready < WRITE_THRESHOLD || flush)
    {
        logf("writing: %d (%d)", num_ready, flush);
        
        cpu_boost(true);

        size_yield = 0;
        for (i=0; i<num_ready; i++)
        {
            enc_chunk  = GET_ENC_CHUNK(enc_rd_index);
            chunk_size = *enc_chunk++;

            /* safety net: if size entry got corrupted => limit */
            if (chunk_size > (long)(enc_chunk_size - sizeof(long)))
                chunk_size = enc_chunk_size - sizeof(long);

            if (enc_set_header_callback != NULL)
                enc_set_header_callback(enc_chunk, enc_chunk_size,
                                              num_pcm_samples, false);

            if (write(wav_file, enc_chunk, chunk_size) != chunk_size)
            {
                close_wave();
                if(must_boost)
                    cpu_boost(false);
                logf("pcmrec: write err");
                is_error = true;
                break;
            }

            num_file_bytes  += chunk_size;
            num_pcm_samples += enc_samp_per_chunk;
            size_yield      += chunk_size;

            if (size_yield >= 32768)
            {   /* yield when 32kB written */
                size_yield = 0;
                yield();
            }

            enc_rd_index = (enc_rd_index + 1) % enc_num_chunks;
        }

        cpu_boost(false);

        /* sync file */
        fsync(wav_file);

        logf("done");
    }
}

/* Abort dma transfer */
static void pcmrec_dma_stop(void)
{
    DCR1 = 0;

    error_count++;

    DSR1 = 1;    /* Clear interrupt */
    IPR |= (1<<15); /* Clear pending interrupt request */

    logf("dma1 stopped");
}

static void pcmrec_dma_start(void)
{
    DAR1 = (unsigned long)GET_CHUNK(write_pos);  /* Destination address */
    SAR1 = (unsigned long)&PDIR2;                /* Source address */
    BCR1 = CHUNK_SIZE;                           /* Bytes to transfer */

    /* Start the DMA transfer.. */
#ifdef HAVE_SPDIF_IN
    INTERRUPTCLEAR = 0x03c00000;
#endif

    /* 16Byte transfers prevents from sporadic errors during cpu_boost() */
    DCR1 = DMA_INT | DMA_EEXT | DMA_CS | DMA_DINC | DMA_DSIZE(3) | DMA_START;

    logf("dma1 started");
}

/* DMA1 Interrupt is called when the DMA has finished transfering a chunk */
void DMA1(void) __attribute__ ((interrupt_handler, section(".icode")));
void DMA1(void)
{
    int res = DSR1;

    DSR1 = 1;    /* Clear interrupt */

    if (res & 0x70)
    {
        DCR1 = 0;   /* Stop DMA transfer */
        error_count++;

        logf("dma1 err: 0x%x", res);

        DAR1 = (unsigned long)GET_CHUNK(write_pos);  /* Destination address */
        BCR1 = CHUNK_SIZE;
        DCR1 = DMA_INT | DMA_EEXT | DMA_CS | DMA_DINC | DMA_START;

        /* Flush recorded data to disk and stop recording */
        queue_post(&pcmrec_queue, PCMREC_STOP, NULL);
    } 
#ifdef HAVE_SPDIF_IN
    else if ((rec_source == AUDIO_SRC_SPDIF) &&
        (INTERRUPTSTAT & 0x01c00000)) /* valnogood, symbolerr, parityerr */
    {
        INTERRUPTCLEAR = 0x03c00000;
        error_count++;

        logf("spdif err");

        DAR1 = (unsigned long)GET_CHUNK(write_pos);  /* Destination address */
        BCR1 = CHUNK_SIZE;
    }
#endif
    else
    {
        long peak_l, peak_r;
        long *ptr, j;

        ptr = GET_CHUNK(write_pos);

        if (!is_paused) /* advance write position */
            write_pos = (write_pos + CHUNK_SIZE) & CHUNK_MASK;

        DAR1 = (unsigned long)GET_CHUNK(write_pos);  /* Destination address */
        BCR1 = CHUNK_SIZE;

        peak_l = peak_r = 0;

        /* only peak every 4th sample */
        for (j=0; j<CHUNK_SIZE/4; j+=4)
        {
            long value = ptr[j];
#ifdef ROCKBOX_BIG_ENDIAN
            if (value > peak_l)       peak_l =  value;
            else if (-value > peak_l) peak_l = -value;

            value <<= 16;
            if (value > peak_r)       peak_r =  value;
            else if (-value > peak_r) peak_r = -value;
#else
            if (value > peak_r)       peak_r =  value;
            else if (-value > peak_r) peak_r = -value;

            value <<= 16;
            if (value > peak_l)       peak_l =  value;
            else if (-value > peak_l) peak_l = -value;
#endif
        }

        peak_left  = (int)(peak_l >> 16);
        peak_right = (int)(peak_r >> 16);
    }

    IPR |= (1<<15); /* Clear pending interrupt request */
}

/* Create WAVE file and write header */
/* Sets returns 0 if success, -1 on failure */
static int start_wave(void)
{
    wav_file = open(recording_filename, O_RDWR|O_CREAT|O_TRUNC);

    if (wav_file < 0)
    {
        wav_file = -1;
        logf("rec: create failed: %d", wav_file);
        is_error = true;
        return -1;
    }
   
    /* add main file header (enc_head_size=0 for encoders without) */
    if (enc_head_size != write(wav_file, enc_head_buffer, enc_head_size))
    {
        close(wav_file);
        wav_file = -1;
        logf("rec: write failed");
        is_error = true;
        return -1;
    }

    return 0;
}

/* Update header and set correct length values */
static void close_wave(void)
{
    unsigned char head[100]; /* assume maximum 100 bytes for file header */
    int           size_read;

    if (wav_file != -1)
    {
        /* update header before closing the file (wav+wv encoder will do) */
        if (enc_set_header_callback != NULL)
        {
            lseek(wav_file, 0, SEEK_SET);
            /* try to read the head size (but we'll accept less) */
            size_read = read(wav_file, head, sizeof(head));

            enc_set_header_callback(head, size_read, num_pcm_samples, true);
            lseek(wav_file, 0, SEEK_SET);
            write(wav_file, head, size_read);
        }
        close(wav_file);
        wav_file = -1;
    }
}

static void pcmrec_start(void)
{
    long max_pre_chunks, pre_ticks, max_pre_ticks;

    logf("pcmrec_start");

    if (is_recording) 
    {
        logf("already recording");
        record_done = true;
        return; 
    }

    if (wav_file != -1)
        close_wave();

    if (start_wave() != 0)
    {
        /* failed to create the file */
        record_done = true;
        return;
    }

    /* calculate maximum available chunks & resulting ticks */
    max_pre_chunks = (enc_wr_index - enc_rd_index +
                        enc_num_chunks) % enc_num_chunks;
    if (max_pre_chunks > enc_num_chunks - WRITE_THRESHOLD)
        max_pre_chunks = enc_num_chunks - WRITE_THRESHOLD;
    max_pre_ticks = max_pre_chunks * HZ * enc_samp_per_chunk / 44100;

    /* limit prerecord if not enough data available */
    pre_ticks = pre_record_ticks > max_pre_ticks ?
        max_pre_ticks : pre_record_ticks;
    max_pre_chunks = 44100 * pre_ticks / HZ / enc_samp_per_chunk;
    enc_rd_index = (enc_wr_index - max_pre_chunks +
                        enc_num_chunks) % enc_num_chunks;

    record_start_time = current_tick - pre_ticks;

    num_rec_bytes = enc_num_chunks * CHUNK_SIZE;
    num_file_bytes = 0;
    num_pcm_samples = 0;
    pause_start_time = 0;
    
    is_paused = false;
    is_recording = true;
    record_done = true;
}

static void pcmrec_stop(void)
{
    logf("pcmrec_stop");
   
    if (is_recording)
    {
        /* wait for encoding finish */
        is_paused = true;
        while(!wav_queue_empty)
            sleep_thread(1);

        is_recording = false;

        /* Flush buffers to file */
        pcmrec_callback(true);
        close_wave();
    }

    is_paused = false;
    stop_done = true;

    logf("pcmrec_stop done");
}

static void pcmrec_new_file(void)
{
    logf("pcmrec_new_file");
    
    if (!is_recording)
    {
        logf("not recording");
        new_file_done = true;
        return;    
    }
    
    /* Since pcmrec_callback() blocks until the data has been written,
       here is a good approximation when recording to the new file starts 
    */
    record_start_time = current_tick;
    
    if (is_paused)
        pause_start_time = record_start_time;
    
    /* Flush what we got in buffers to file */
    pcmrec_callback(true);
    
    close_wave();

    num_rec_bytes = 0;
    num_file_bytes = 0;
    num_pcm_samples = 0;
    
    /* start the new file */    
    if (start_wave() != 0)
    {
        logf("new_file failed");       
        pcmrec_stop();
    }   

    new_file_done = true;
    logf("pcmrec_new_file done");
}

static void pcmrec_pause(void)
{
    logf("pcmrec_pause");

    if (!is_recording)
    {
        logf("pause: not recording");
        pause_done = true;
        return;
    }
    
    pause_start_time = current_tick;
    is_paused = true;  
    pause_done = true;
    
    logf("pcmrec_pause done");
}


static void pcmrec_resume(void)
{
    logf("pcmrec_resume");
    
    if (!is_paused)
    {
        logf("resume: not paused");
        resume_done = true;
        return;
    }
    
    is_paused = false;
    is_recording = true;
    
    /* Compensate for the time we have been paused */
    if (pause_start_time)
    {
        record_start_time += current_tick - pause_start_time;
        pause_start_time = 0;
    }
    
    resume_done = true;
    logf("pcmrec_resume done");
}

/**
 * audio_init_recording calls this function using PCMREC_INIT
 * 
 */
static void pcmrec_init(void)
{
    wav_file = -1;  
    read_pos = 0;
    write_pos = 0;
    enc_wr_index = 0;
    enc_rd_index = 0;

    avrg_bit_rate  = 0;
    curr_bit_rate  = 0;
    curr_chunk_cnt = 0;

    peak_left = 0;
    peak_right = 0;
    
    num_rec_bytes = 0;
    num_file_bytes = 0;
    num_pcm_samples = 0;
    record_start_time = 0;
    pause_start_time = 0;

    close_done = false;
    is_recording = false;
    is_paused = false;
    is_error = false;

    rec_buffer = (unsigned char*)(((long)audiobuf + 15) & ~15);
    enc_buffer = rec_buffer + NUM_CHUNKS * CHUNK_SIZE + MAX_FEED_SIZE;
    /* 8000Bytes at audiobufend */
    enc_buffer_size = audiobufend - enc_buffer - 8000;

    SET_IIS_PLAY(0x800); /* Stop any playback                              */
    AUDIOGLOB |= 0x180;  /* IIS1 fifo auto sync = on, PDIR2 auto sync = on */
    DATAINCONTROL = 0xc000; /* Generate Interrupt when 6 samples in fifo   */

    DIVR1 = 55;          /* DMA1 is mapped into vector 55 in system.c      */
    DMACONFIG = 1;       /* DMA0Req = PDOR3, DMA1Req = PDIR2               */
    DMAROUTE = (DMAROUTE & 0xffff00ff) | DMA1_REQ_AUDIO_2;
    ICR7 = 0x1c;         /* Enable interrupt at level 7, priority 0        */
    IMR &= ~(1<<15);     /* bit 15 is DMA1                                 */

#ifdef HAVE_SPDIF_IN
    PHASECONFIG = 0x34;  /* Gain = 3*2^13, source = EBUIN                  */
#endif
    pcmrec_dma_start();

    init_done = 1;
}

static void pcmrec_close(void)
{
    DMAROUTE = (DMAROUTE & 0xffff00ff);
    ICR7 = 0x00;     /* Disable interrupt */
    IMR |= (1<<15);  /* bit 15 is DMA1 */

    pcmrec_dma_stop();

    /* Reset PDIR2 data flow */
    DATAINCONTROL = 0x200;
    close_done = true;
    init_done = false;
}

static void pcmrec_thread(void)
{
    struct event ev;

    logf("thread pcmrec start");

    error_count = 0;

    while(1)
    {
        queue_wait_w_tmo(&pcmrec_queue, &ev, HZ / 4);

        switch (ev.id)
        {
            case PCMREC_INIT: 
                pcmrec_init();
                break;

            case PCMREC_CLOSE:
                pcmrec_close();
                break;

            case PCMREC_START:
                pcmrec_start();
                break;

            case PCMREC_STOP:
                pcmrec_stop();
                break;

            case PCMREC_PAUSE:
                pcmrec_pause();
                break;

            case PCMREC_RESUME:
                pcmrec_resume();
                break;

            case PCMREC_NEW_FILE:
                pcmrec_new_file();
                break;

            case SYS_TIMEOUT:
                pcmrec_callback(false);
                break;

            case SYS_USB_CONNECTED:
                if (!is_recording)
                {
                    pcmrec_close();
                    usb_acknowledge(SYS_USB_CONNECTED_ACK);
                    usb_wait_for_disconnect(&pcmrec_queue);
                }
                break;
        }
    }

    logf("thread pcmrec done");
}

/* Select VINL & VINR source: 0=Line-in, 1=FM Radio */
void pcm_rec_mux(int source)
{
#ifdef IRIVER_H300_SERIES
    if(source == 0)
        and_l(~0x40000000, &GPIO_OUT);  /* Line In */
    else
        or_l(0x40000000, &GPIO_OUT);    /* FM radio */
        
    or_l(0x40000000, &GPIO_ENABLE);
    or_l(0x40000000, &GPIO_FUNCTION);
#elif defined(IRIVER_H100_SERIES)
    if(source == 0)
        and_l(~0x00800000, &GPIO_OUT);  /* Line In */
    else
        or_l(0x00800000, &GPIO_OUT);    /* FM radio */
        
    or_l(0x00800000, &GPIO_ENABLE);
    or_l(0x00800000, &GPIO_FUNCTION);
    
#elif defined(IAUDIO_X5)
    if(source == 0)
        or_l((1<<29), &GPIO_OUT);      /* Line In */
    else
        and_l(~(1<<29), &GPIO_OUT);    /* FM radio */
        
    or_l((1<<29), &GPIO_ENABLE);
    or_l((1<<29), &GPIO_FUNCTION);

    /* iAudio x5 */
#endif
}


/****************************************************************************/
/*                                                                          */
/*         following functions will be called by the encoder codec          */
/*                                                                          */
/****************************************************************************/

/* pass the encoder buffer pointer/size, mono/stereo, quality to the encoder */
void enc_get_inputs(int *buffer_size, int *channels, int *quality)
{
    *buffer_size = enc_buffer_size;
    *channels    = enc_channels;
    *quality     = enc_quality;
}

/* set the encoder dimensions (called by encoder codec at initialization) */
void enc_set_parameters(int chunk_size, int num_chunks, int samp_per_chunk,
                        char *head_ptr, int head_size, int enc_id)
{
    /* set read_pos just in front of current write_pos */
    read_pos = (write_pos - CHUNK_SIZE) & CHUNK_MASK;

    enc_rd_index       = 0;              /* reset */
    enc_wr_index       = 0;              /* reset */
    enc_chunk_size     = chunk_size;     /* max chunk size */
    enc_num_chunks     = num_chunks;     /* total number of chunks */
    enc_samp_per_chunk = samp_per_chunk; /* pcm samples / encoderchunk */
    enc_head_buffer    = head_ptr;       /* optional file header data (wav) */
    enc_head_size      = head_size;      /* optional file header data (wav) */
    audio_enc_id       = enc_id;         /* AFMT_* id */
}

/* allocate encoder chunk */
unsigned int *enc_alloc_chunk(void)
{
    return (unsigned int*)(enc_buffer + enc_wr_index * enc_chunk_size);
}

/* free previously allocated encoder chunk */
void enc_free_chunk(void)
{
    unsigned long *enc_chunk;

    enc_chunk = GET_ENC_CHUNK(enc_wr_index);
    curr_chunk_cnt++;
/*  curr_bit_rate += *enc_chunk * 44100 * 8 / (enc_samp_per_chunk * 1000); */
    curr_bit_rate += *enc_chunk * 441   * 8 / (enc_samp_per_chunk * 10  );
    avrg_bit_rate  = (curr_bit_rate + curr_chunk_cnt / 2) / curr_chunk_cnt;

    /* advance enc_wr_index to the next chunk */
    enc_wr_index = (enc_wr_index + 1) % enc_num_chunks;

    /* buffer full: advance enc_rd_index (for prerecording purpose) */
    if (enc_rd_index == enc_wr_index)
    {
        enc_rd_index = (enc_rd_index + 1) % enc_num_chunks;
    }
}

/* checks near empty state on wav input buffer */
int enc_wavbuf_near_empty(void)
{
    /* less than 1sec raw data? => unboost encoder */
    if (((write_pos - read_pos) & CHUNK_MASK) < 44100*4)
        return 1;
    else
        return 0;
}

/* passes a pointer to next chunk of unprocessed wav data */
char *enc_get_wav_data(int size)
{
    char *ptr;
    int  avail;

    /* limit the requested pcm data size */
    if(size > MAX_FEED_SIZE)
        size = MAX_FEED_SIZE;

    avail = (write_pos - read_pos) & CHUNK_MASK;

    if (avail >= size)
    {
        ptr = rec_buffer + read_pos;
        read_pos = (read_pos + size) & CHUNK_MASK;

        /* ptr must point to continous data at wraparound position */
        if (read_pos < size)
            memcpy(rec_buffer + NUM_CHUNKS * CHUNK_SIZE,
                rec_buffer, read_pos);

        wav_queue_empty = false;
        return ptr;
    }

    wav_queue_empty = true;
    return NULL;
}
