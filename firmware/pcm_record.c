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
#if defined(HAVE_UDA1380)
#include "uda1380.h"
#elif defined(HAVE_TLV320)
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
#include "pcm_record.h"


/***************************************************************************/

static volatile bool is_recording;              /* We are recording */
static volatile bool is_stopping;               /* Are we going to stop */
static volatile bool is_paused;                 /* We have paused   */

static volatile int num_rec_bytes;
static volatile int int_count;                  /* Number of DMA completed interrupts */
static volatile int error_count;                /* Number of DMA errors */

static unsigned long record_start_time;         /* Value of current_tick when recording was started */
static unsigned long pause_start_time;          /* Value of current_tick when pause was started */

static int rec_gain, rec_volume;
static bool show_waveform;
static int init_done = 0;
static int wav_file;
static char recording_filename[MAX_PATH];

/***************************************************************************/

/*
   Some estimates:
    44100 HZ * 4 = 176400 bytes/s
    Refresh LCD 10 HZ = 176400 / 10 = 17640 bytes ~=~ 1024*16 bytes

    If NUM_BUFFERS is 80 we can hold ~8 sec of data in memory
    ALL_BUFFER_SIZE will be 1024*16 * 80 = 1310720 bytes
*/

#define NUM_BUFFERS       80
#define EACH_BUFFER_SIZE  (1024*16)     /* Multiple of 4. Use small value to get responsive waveform */
#define ALL_BUFFERS_SIZE  (NUM_BUFFERS * EACH_BUFFER_SIZE)

#define WRITE_THRESHOLD   40            /* Minimum number of buffers before write to file */

static unsigned char *rec_buffers[NUM_BUFFERS];

/* 
 Overrun occures when DMA needs to write a new buffer and write_index == read_index 
 Solution to this is to optimize pcmrec_callback, use cpu_boost somewhere or increase 
 the total buffer size (or WRITE_THRESHOLD)
*/

static int write_index;       /* Which buffer the DMA is currently recording */
static int read_index;        /* The oldest buffer that the pcmrec_callback has not read */

/***************************************************************************/

static struct event_queue  pcmrec_queue;
static long                pcmrec_stack[(DEFAULT_STACK_SIZE + 0x1000)/sizeof(long)];
static const char          pcmrec_thread_name[] = "pcmrec";

static void pcmrec_thread(void);

/* Event IDs */
#define PCMREC_OPEN         1     /* Enable recording */
#define PCMREC_CLOSE        2     /* Disable recording */
#define PCMREC_START        3     /* Start a new recording */
#define PCMREC_STOP         4     /* Stop the current recording */
#define PCMREC_PAUSE        10
#define PCMREC_RESUME       11
#define PCMREC_NEW_FILE     12
#define PCMREC_SET_GAIN     13
#define PCMREC_GOT_DATA     20    /* DMA1 notifies when data has arrived */


/*******************************************************************/
/* Functions that are not executing in the pcmrec_thread first     */
/*******************************************************************/

void pcm_init_recording(void)
{
    int_count = 0;
    error_count = 0;

    show_waveform = 0;
    is_recording = 0;
    is_stopping = 0;
    num_rec_bytes = 0;
    wav_file = -1;  
    read_index = 0;
    write_index = 0;

    queue_init(&pcmrec_queue);
    create_thread(pcmrec_thread, pcmrec_stack, sizeof(pcmrec_stack), pcmrec_thread_name);
}

void pcm_open_recording(void)
{
    init_done = 0;

    logf("pcm_open_rec");

    queue_post(&pcmrec_queue, PCMREC_OPEN, 0);

    while (init_done)
    {
        sleep(HZ >> 8);
    }

    logf("pcm_open_rec done");
}

void pcm_close_recording(void)
{
    /* todo: synchronize completion with pcmrec thread */
    queue_post(&pcmrec_queue, PCMREC_CLOSE, 0);
}



unsigned long pcm_status(void)
{
    unsigned long ret = 0;

    if (is_recording)
        ret |= AUDIO_STATUS_RECORD;

    return ret;
}



void pcm_new_file(const char *filename)
{
    /* todo */
    filename = filename;

}

unsigned long pcm_recorded_time(void)
{
    if (is_recording)
    {
        if(is_paused)
            return pause_start_time - record_start_time;
        else
            return current_tick - record_start_time;
    }

    return 0;
}

unsigned long pcm_num_recorded_bytes(void)
{

    if (is_recording)
    {
        return num_rec_bytes;
    }
    else
        return 0;
}

void pcm_pause_recording(void)
{
    /* todo */
}

void pcm_resume_recording(void)
{
    /* todo */
}


/**
 * Sets the audio source 
 * 
 * Side effect: This functions starts feeding the CPU with audio data over the I2S bus
 *
 * @param source 0=line-in, 1=mic 
 */
void pcm_set_recording_options(int source, bool enable_waveform)
{
#if defined(HAVE_UDA1380)
    uda1380_enable_recording(source);
#elif defined(HAVE_TLV320)
    tlv320_enable_recording(source);
#endif
    show_waveform = enable_waveform;
}


/**
 *
 * @param gain   line-in and microphone gain (0-15)
 * @param volume ADC volume (0-255)
 */
void pcm_set_recording_gain(int gain, int volume)
{
    rec_gain = gain;
    rec_volume = volume;

    queue_post(&pcmrec_queue, PCMREC_SET_GAIN, 0);
}

/**
 * Start recording
 * 
 * Use pcm_set_recording_options before calling record
 */
void pcm_record(const char *filename)
{
    strncpy(recording_filename, filename, MAX_PATH - 1);
    recording_filename[MAX_PATH - 1] = 0;

    queue_post(&pcmrec_queue, PCMREC_START, 0);
}

/**
 * 
 */
void pcm_stop_recording(void)
{
    if (is_recording)
    is_stopping = 1;

    queue_post(&pcmrec_queue, PCMREC_STOP, 0);

    logf("pcm_stop_recording");

    while (is_stopping)
    {
        sleep(HZ >> 4);
    }

    logf("pcm_stop_recording done");
}


/***************************************************************************/
/* Functions that executes in the context of pcmrec_thread                 */
/***************************************************************************/


/**
 * Process the buffers using read_index and write_index.
 *
 * DMA1 handler posts to pcmrec_queue so that pcmrec_thread calls this 
 * function. Also pcmrec_stop will call this function when the recording 
 * is stopping, and that call will have flush = true.
 *
 */

void pcmrec_callback(bool flush) __attribute__ ((section (".icode")));
void pcmrec_callback(bool flush)
{
    int num_ready;

    num_ready = write_index - read_index;
    if (num_ready < 0)
        num_ready += NUM_BUFFERS;

    /* we can consume up to num_ready buffers */

#ifdef HAVE_REMOTE_LCD
    /* Draw waveform on remote LCD */
    if (show_waveform && num_ready>0)
    {
        short *buf; 
        long x,y,offset;
        int show_index;

        /* Just display the last buffer (most recent one) */  
        show_index = read_index + num_ready - 1;
        buf = (short*)rec_buffers[show_index]; 

        lcd_remote_clear_display();    

        offset = 0;
        for (x=0; x<LCD_REMOTE_WIDTH-1; x++)
        {
            y = buf[offset] * (LCD_REMOTE_HEIGHT / 2)  *5;       /* The 5 is just 'zooming' */
            y = y >> 15;                /* Divide with SHRT_MAX */
            y += LCD_REMOTE_HEIGHT/2;

            if (y < 2) y=2;
            if (y >= LCD_REMOTE_HEIGHT-2) y = LCD_REMOTE_HEIGHT-2;

            lcd_remote_drawpixel(x,y);

            offset += (EACH_BUFFER_SIZE/2) / LCD_REMOTE_WIDTH;  
        }

        lcd_remote_update();
    }

#endif

    /* Note: This might be a good place to call the 'codec' later */

    /* Check that we have the minimum amount of data to save or */
    /* that if it's closing time which mean we have to save..   */
    if (wav_file != -1)
    {
        if (num_ready >= WRITE_THRESHOLD || flush)
        {
            unsigned short *ptr = (unsigned short*)rec_buffers[read_index];
            int i;

            for (i=0; i<EACH_BUFFER_SIZE * num_ready / 2; i++)
            {
                *ptr = htole16(*ptr);
                ptr++;
            }

            write(wav_file, rec_buffers[read_index], EACH_BUFFER_SIZE * num_ready);

            read_index+=num_ready;
            if (read_index >= NUM_BUFFERS)
                read_index -= NUM_BUFFERS;    
        }

    } else
    {
        /* In this case we must consume the buffers otherwise we will */
        /* get 'dma1 overrun' pretty fast */

        read_index+=num_ready;
        if (read_index >= NUM_BUFFERS)
            read_index -= NUM_BUFFERS;
    }
}


void pcmrec_dma_start(void)
{
    DAR1 = (unsigned long)rec_buffers[write_index++];  /* Destination address */
    SAR1 = (unsigned long)&PDIR2;                      /* Source address */
    BCR1 = EACH_BUFFER_SIZE;                           /* Bytes to transfer */

    /* Start the DMA transfer.. */
    DCR1 = DMA_INT | DMA_EEXT | DMA_CS | DMA_DINC | DMA_START;

    logf("dma1 started");
}


/* DMA1 Interrupt is called when the DMA has finished transfering a chunk */
void DMA1(void) __attribute__ ((interrupt_handler, section(".icode")));
void DMA1(void)
{
    int res = DSR1;

    DSR1 = 1;    /* Clear interrupt */

    int_count++;

    if (res & 0x70)
    {
        DCR1 = 0;   /* Stop DMA transfer */
        error_count++;
        is_recording = 0;

        logf("dma1 err 0x%x", res);

    } else 
    {
        num_rec_bytes += EACH_BUFFER_SIZE;

        write_index++;
        if (write_index >= NUM_BUFFERS)
            write_index = 0;

        if (is_stopping || !is_recording)
        {
            DCR1 = 0;   /* Stop DMA transfer */
            is_recording = 0;

            logf("dma1 stopping");

        } else if (write_index == read_index)
        {
            DCR1 = 0;   /* Stop DMA transfer */
            is_recording = 0;

            logf("dma1 overrun");

        } else
        {
            DAR1 = (unsigned long)rec_buffers[write_index];  /* Destination address */
            BCR1 = EACH_BUFFER_SIZE;

            queue_post(&pcmrec_queue, PCMREC_GOT_DATA, NULL);

        }
    }

    IPR |= (1<<15); /* Clear pending interrupt request */
}

static int start_wave(void)
{
    unsigned char header[44] = 
    {
        'R','I','F','F',0,0,0,0,'W','A','V','E','f','m','t',' ',
        0x10,0,0,0,1,0,2,0,0x44,0xac,0,0,0x10,0xb1,2,0,
        4,0,0x10,0,'d','a','t','a',0,0,0,0
    };

    wav_file = open(recording_filename, O_RDWR|O_CREAT|O_TRUNC);
    if (wav_file < 0)
    {
        wav_file = -1;
        logf("create failed: %d", wav_file);
        return -1;
    }

    if (sizeof(header) != write(wav_file, header, sizeof(header)))
    {
        close(wav_file);
        wav_file = -1;
        logf("write failed");
        return -2;
    }

    return 0;
}

/* Update header and set correct length values */
static void close_wave(void)
{
    long l;

    l = htole32(num_rec_bytes + 36);
    lseek(wav_file, 4, SEEK_SET);
    write(wav_file, &l, 4);

    l = htole32(num_rec_bytes);
    lseek(wav_file, 40, SEEK_SET);
    write(wav_file, &l, 4);

    close(wav_file);
    wav_file = -1;
}

static void pcmrec_start(void)
{
    logf("pcmrec_start");

    if (is_recording) 
        return; 

    if (wav_file != -1)
        close(wav_file);

    logf("rec: %s", recording_filename);

    start_wave(); /* todo: send signal to pcm_record if we have failed */

    num_rec_bytes = 0;

    /* Store the current time */
    record_start_time = current_tick;

    write_index = 0;
    read_index = 0;

    is_stopping = 0;
    is_paused = 0;
    is_recording = 1;

    pcmrec_dma_start();

}

static void pcmrec_stop(void)
{
    /* wait for recording to finish */

    /* todo: Abort current DMA transfer using DCR1.. */

    logf("pcmrec_stop");

    while (is_recording)
    {
        sleep(HZ >> 4);
    }

    logf("pcmrec_stop done");

    /* Write unfinished buffers to file */
    pcmrec_callback(true);

    close_wave();

    is_stopping = 0;   
}

static void pcmrec_open(void)
{
    unsigned long buffer_start;
    int i;

    show_waveform = 0;
    is_recording = 0;
    is_stopping = 0;
    num_rec_bytes = 0;
    wav_file = -1;  
    read_index = 0;
    write_index = 0;

    buffer_start = (unsigned long)(&audiobuf[(audiobufend - audiobuf) - (ALL_BUFFERS_SIZE + 16)]);
    buffer_start &= ~3;

    for (i=0; i<NUM_BUFFERS; i++)
    {
        rec_buffers[i] = (unsigned char*)(buffer_start + EACH_BUFFER_SIZE * i);
    }

    IIS1CONFIG = 0x800;             /* Stop any playback                              */
    AUDIOGLOB |= 0x180;             /* IIS1 fifo auto sync = on, PDIR2 auto sync = on */
    DATAINCONTROL = 0xc000;         /* Generate Interrupt when 6 samples in fifo      */
    DATAINCONTROL |= 0x20;          /* PDIR2 source = IIS1recv                        */

    DIVR1 = 55;                     /* DMA1 is mapped into vector 55 in system.c      */
    DMACONFIG = 1;                  /* DMA0Req = PDOR3, DMA1Req = PDIR2               */
    DMAROUTE = (DMAROUTE & 0xffff00ff) | DMA1_REQ_AUDIO_2;
    ICR7 = 0x1c;                    /* Enable interrupt at level 7, priority 0 */
    IMR &= ~(1<<15);                /* bit 15 is DMA1 */

    init_done = 1;
}

static void pcmrec_close(void)
{
#if defined(HAVE_UDA1380)
    uda1380_disable_recording();
#elif defined(HAVE_TLV320)
    tlv320_disable_recording();
#endif

    DMAROUTE = (DMAROUTE & 0xffff00ff);
    ICR7 = 0x00;     /* Disable interrupt */
    IMR |= (1<<15);  /* bit 15 is DMA1 */

}

static void pcmrec_thread(void)
{
    struct event ev;

    logf("thread pcmrec start");

    while (1)
    {
        queue_wait(&pcmrec_queue, &ev);

        switch (ev.id)
        {
            case PCMREC_OPEN:
                pcmrec_open();
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
                /* todo */
                break;

            case PCMREC_RESUME:
                /* todo */
                break;

            case PCMREC_NEW_FILE:
                /* todo */
                break;

            case PCMREC_SET_GAIN:
#if defined(HAVE_UDA1380)
                uda1380_set_recvol(rec_gain, rec_gain, rec_volume);
#elif defined(HAVE_TLV320)
                /* ToDo */
#endif
                break;

            case PCMREC_GOT_DATA:
                pcmrec_callback(false);
                break;

            case SYS_USB_CONNECTED:
                if (!is_recording && !is_stopping)
                {
                    usb_acknowledge(SYS_USB_CONNECTED_ACK);
                    usb_wait_for_disconnect(&pcmrec_queue);
                }
                break;
        }
    }

    logf("thread pcmrec done");
}

void pcmrec_set_mux(int source)
{
    if(source == 0)
        and_l(~0x00800000, &GPIO_OUT);  /* Line In */
    else
        or_l(0x00800000, &GPIO_OUT);    /* FM radio */
        
    or_l(0x00800000, &GPIO_ENABLE);
    or_l(0x00800000, &GPIO_FUNCTION);
}
