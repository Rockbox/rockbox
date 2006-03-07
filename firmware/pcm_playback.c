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
#include <stdbool.h>
#include "config.h"
#include "debug.h"
#include "panic.h"
#include <kernel.h>
#include "cpu.h"
#include "i2c.h"
#if defined(HAVE_UDA1380)
#include "uda1380.h"
#elif defined(HAVE_WM8975)
#include "wm8975.h"
#elif defined(HAVE_WM8758)
#include "wm8758.h"
#elif defined(HAVE_TLV320)
#include "tlv320.h"
#elif defined(HAVE_WM8731)
#include "wm8731l.h"
#endif
#include "system.h"
#include "logf.h"

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include "pcm_playback.h"
#include "lcd.h"
#include "button.h"
#include "file.h"
#include "buffer.h"
#include "sprintf.h"
#include "button.h"
#include <string.h>

#ifdef HAVE_UDA1380

#ifdef HAVE_SPDIF_OUT
#define EBU_DEFPARM        ((7 << 12) | (3 << 8) | (1 << 5) | (5 << 2))
#endif
#define IIS_DEFPARM(freq)  ((freq << 12) | 0x300 | 4 << 2)
#define IIS_RESET          0x800

static bool pcm_playing;
static bool pcm_paused;
static int pcm_freq = 0x6; /* 44.1 is default */

size_t next_size IBSS_ATTR;
unsigned char *next_start IBSS_ATTR;

/* Set up the DMA transfer that kicks in when the audio FIFO gets empty */
static void dma_start(const void *addr, size_t size)
{
    pcm_playing = true;

    addr = (void *)((unsigned long)addr & ~3); /* Align data */
    size &= ~3; /* Size must be multiple of 4 */

    /* Reset the audio FIFO */
#ifdef HAVE_SPDIF_OUT
    EBU1CONFIG = IIS_RESET | EBU_DEFPARM;
#endif

    /* Set up DMA transfer  */
    SAR0 = ((unsigned long)addr); /* Source address */
    DAR0 = (unsigned long)&PDOR3; /* Destination address */
    BCR0 = size;                  /* Bytes to transfer */

    /* Enable the FIFO and force one write to it */
    IIS2CONFIG = IIS_DEFPARM(pcm_freq);
    /* Also send the audio to S/PDIF */
#ifdef HAVE_SPDIF_OUT
    EBU1CONFIG = EBU_DEFPARM;
#endif
    DCR0 = DMA_INT | DMA_EEXT | DMA_CS | DMA_SINC | DMA_START;
}

/* Stops the DMA transfer and interrupt */
static void dma_stop(void)
{
    pcm_playing = false;

    DCR0 = 0;
    DSR0 = 1;
    /* Reset the FIFO */
    IIS2CONFIG = IIS_RESET | IIS_DEFPARM(pcm_freq);
#ifdef HAVE_SPDIF_OUT
    EBU1CONFIG = IIS_RESET | EBU_DEFPARM;
#endif

    pcm_paused = false;
}

/* sets frequency of input to DAC */
void pcm_set_frequency(unsigned int frequency)
{
    switch(frequency)
    {
    case 11025:
        pcm_freq = 0x4;
        uda1380_set_nsorder(3);
        break;
    case 22050:
        pcm_freq = 0x6;
        uda1380_set_nsorder(3);
        break;
    case 44100:
    default:
        pcm_freq = 0xC;
        uda1380_set_nsorder(5);
        break;
    }
}

/* the registered callback function to ask for more mp3 data */
static void (*callback_for_more)(unsigned char**, size_t*) IDATA_ATTR = NULL;

void pcm_play_data(void (*get_more)(unsigned char** start, size_t* size),
        unsigned char* start, size_t size)
{
    callback_for_more = get_more;

    if (!(start && size))
    {
        if (get_more)
            get_more(&start, &size);
        else
            return;
    }
    if (start && size)
        dma_start(start, size);
}

size_t pcm_get_bytes_waiting(void)
{
    return (BCR0 & 0xffffff);
}

void pcm_mute(bool mute)
{
    uda1380_mute(mute);
    if (mute)
        sleep(HZ/16);
}

void pcm_play_stop(void)
{
    if (pcm_playing) {
        dma_stop();
    }
}

void pcm_play_pause(bool play)
{
    if (!pcm_playing)
        return ;

    if(pcm_paused && play)
    {
        if (BCR0 & 0xffffff)
        {
            logf("unpause");
            /* Enable the FIFO and force one write to it */
            IIS2CONFIG = IIS_DEFPARM(pcm_freq);
#ifdef HAVE_SPDIF_OUT
            EBU1CONFIG = EBU_DEFPARM;
#endif
            DCR0 |= DMA_EEXT | DMA_START;
        }
        else
        {
            logf("unpause, no data waiting");
            void (*get_more)(unsigned char**, size_t*) = callback_for_more;
            if (get_more)
                get_more(&next_start, &next_size);
            if (next_start && next_size)
                dma_start(next_start, next_size);
            else
            {
                dma_stop();
                logf("unpause attempted, no data");
            }
        }
    }
    else if(!pcm_paused && !play)
    {
        logf("pause");

        /* Disable DMA peripheral request. */
        DCR0 &= ~DMA_EEXT;
        IIS2CONFIG = IIS_RESET | IIS_DEFPARM(pcm_freq);
#ifdef HAVE_SPDIF_OUT
        EBU1CONFIG = IIS_RESET | EBU_DEFPARM;
#endif
    }
    pcm_paused = !play;
}

bool pcm_is_paused(void)
{
    return pcm_paused;
}

bool pcm_is_playing(void)
{
    return pcm_playing;
}

/* DMA0 Interrupt is called when the DMA has finished transfering a chunk */
void DMA0(void) __attribute__ ((interrupt_handler, section(".icode")));
void DMA0(void)
{
    int res = DSR0;

    DSR0 = 1;    /* Clear interrupt */
    DCR0 &= ~DMA_EEXT;

    /* Stop on error */
    if(res & 0x70)
    {
       dma_stop();
       logf("DMA Error:0x%04x", res);
    }
    else
    {
        {
            void (*get_more)(unsigned char**, size_t*) = callback_for_more;
            if (get_more)
                get_more(&next_start, &next_size);
            else
            {
                next_size = 0;
                next_start = NULL;
            }
        }
        if(next_size)
        {
            SAR0 = (unsigned long)next_start;  /* Source address */
            BCR0 = next_size;                  /* Bytes to transfer */
            DCR0 |= DMA_EEXT;

        }
        else
        {
            /* Finished playing */
            dma_stop();
            logf("DMA No Data:0x%04x", res);
        }
    }

    IPR |= (1<<14); /* Clear pending interrupt request */
}

void pcm_init(void)
{
    pcm_playing = false;
    pcm_paused = false;

    MPARK = 0x81;    /* PARK[1,0]=10 + BCR24BIT */
    DIVR0 = 54;      /* DMA0 is mapped into vector 54 in system.c */
    DMAROUTE = (DMAROUTE & 0xffffff00) | DMA0_REQ_AUDIO_1;
    DMACONFIG = 1;   /* DMA0Req = PDOR3 */

    /* Reset the audio FIFO */
    IIS2CONFIG = IIS_RESET;

    /* Enable interrupt at level 7, priority 0 */
    ICR6 = 0x1c;
    IMR &= ~(1<<14);      /* bit 14 is DMA0 */

    pcm_set_frequency(44100);

    /* Prevent pops (resets DAC to zero point) */
    IIS2CONFIG = IIS_DEFPARM(pcm_freq) | IIS_RESET;
    
#if defined(HAVE_UDA1380)
    /* Initialize default register values. */
    uda1380_init();
    
    /* Sleep a while so the power can stabilize (especially a long
       delay is needed for the line out connector). */
    sleep(HZ);

    /* Power on FSDAC and HP amp. */
    uda1380_enable_output(true);

    /* Unmute the master channel (DAC should be at zero point now). */
    uda1380_mute(false);
    
#elif defined(HAVE_TLV320)
    tlv320_init();
    tlv320_enable_output(true);
    sleep(HZ/4);
    tlv320_mute(false);
#endif
    
    /* Call dma_stop to initialize everything. */
    dma_stop();
}

#elif defined(HAVE_WM8975) || defined(HAVE_WM8758) || defined(HAVE_WM8731)

/* We need to unify this code with the uda1380 code as much as possible, but
   we will keep it separate during early development.
*/

#if CONFIG_CPU == PP5020
#define FIFO_FREE_COUNT ((IISFIFO_CFG & 0x3f0000) >> 16)
#elif CONFIG_CPU == PP5002
#define FIFO_FREE_COUNT ((IISFIFO_CFG & 0x7800000) >> 23)
#endif

static bool pcm_playing;
static bool pcm_paused;
static int pcm_freq = 0x6; /* 44.1 is default */

/* NOTE: The order of these two variables is important if you use the iPod
   assembler optimised fiq handler, so don't change it. */
unsigned short* p IBSS_ATTR;
size_t p_size IBSS_ATTR;

static void dma_start(const void *addr, size_t size)
{
    p=(unsigned short*)addr;
    p_size=size;

    pcm_playing = true;

#if CONFIG_CPU == PP5020
    /* setup I2S interrupt for FIQ */
    outl(inl(0x6000402c) | I2S_MASK, 0x6000402c);
    outl(I2S_MASK, 0x60004024);
#else
    /* setup I2S interrupt for FIQ */
    outl(inl(0xcf00103c) | DMA_OUT_MASK, 0xcf00103c);
    outl(DMA_OUT_MASK, 0xcf001034);
#endif

    /* Clear the FIQ disable bit in cpsr_c */
    enable_fiq();

    /* Enable playback FIFO */
#if CONFIG_CPU == PP5020
    IISCONFIG |= 0x20000000;
#elif CONFIG_CPU == PP5002
    IISCONFIG |= 0x4;
#endif

    /* Fill the FIFO - we assume there are enough bytes in the pcm buffer to
       fill the 32-byte FIFO. */
    while (p_size > 0) {
        if (FIFO_FREE_COUNT < 2) {
            /* Enable interrupt */
#if CONFIG_CPU == PP5020
            IISCONFIG |= 0x2;
#elif CONFIG_CPU == PP5002
            IISFIFO_CFG |= (1<<9);
#endif
            return;
        }

        IISFIFO_WR = (*(p++))<<16;
        IISFIFO_WR = (*(p++))<<16;
        p_size-=4;
    }
}

/* Stops the DMA transfer and interrupt */
static void dma_stop(void)
{
    pcm_playing = false;

#if CONFIG_CPU == PP5020

    /* Disable playback FIFO */
    IISCONFIG &= ~0x20000000;

    /* Disable the interrupt */
    IISCONFIG &= ~0x2;

#elif CONFIG_CPU == PP5002

    /* Disable playback FIFO */
    IISCONFIG &= ~0x4;

    /* Disable the interrupt */
    IISFIFO_CFG &= ~(1<<9);
#endif

    disable_fiq();

    pcm_paused = false;
}

void pcm_set_frequency(unsigned int frequency)
{
    pcm_freq=frequency;
}

/* the registered callback function to ask for more PCM data */
static void (*callback_for_more)(unsigned char**, size_t*) IDATA_ATTR = NULL;

void pcm_play_data(void (*get_more)(unsigned char** start, size_t* size),
        unsigned char* start, size_t size)
{
    callback_for_more = get_more;

    if (!(start && size))
    {
        if (get_more)
            get_more(&start, &size);
        else
            return;
    }
    if (start && size)
        dma_start(start, size);
}

size_t pcm_get_bytes_waiting(void)
{
    return p_size;
}

void pcm_mute(bool mute)
{
    wmcodec_mute(mute);
    if (mute)
        sleep(HZ/16);
}

void pcm_play_stop(void)
{
    if (pcm_playing) {
        dma_stop();
    }
}

void pcm_play_pause(bool play)
{
    size_t next_size;
    unsigned char *next_start;

    if (!pcm_playing)
        return ;

    if(pcm_paused && play)
    {
        if (pcm_get_bytes_waiting())
        {
            logf("unpause");
            /* Enable the FIFO and fill it */

            enable_fiq();

            /* Enable playback FIFO */
#if CONFIG_CPU == PP5020
            IISCONFIG |= 0x20000000;
#elif CONFIG_CPU == PP5002
            IISCONFIG |= 0x4;
#endif

            /* Fill the FIFO - we assume there are enough bytes in the 
               pcm buffer to fill the 32-byte FIFO. */
            while (p_size > 0) {
                if (FIFO_FREE_COUNT < 2) {
                    /* Enable interrupt */
#if CONFIG_CPU == PP5020
                    IISCONFIG |= 0x2;
#elif CONFIG_CPU == PP5002
                    IISFIFO_CFG |= (1<<9);
#endif
                    return;
                }

                IISFIFO_WR = (*(p++))<<16;
                IISFIFO_WR = (*(p++))<<16;
                p_size-=4;
            }
        }
        else
        {
            logf("unpause, no data waiting");
            void (*get_more)(unsigned char**, size_t*) = callback_for_more;
            if (get_more)
                get_more(&next_start, &next_size);
            if (next_start && next_size)
                dma_start(next_start, next_size);
            else
            {
                dma_stop();
                logf("unpause attempted, no data");
            }
        }
    }
    else if(!pcm_paused && !play)
    {
        logf("pause");

#if CONFIG_CPU == PP5020

        /* Disable the interrupt */
        IISCONFIG &= ~0x2;

        /* Disable playback FIFO */
        IISCONFIG &= ~0x20000000;

#elif CONFIG_CPU == PP5002

        /* Disable the interrupt */
        IISFIFO_CFG &= ~(1<<9);

        /* Disable playback FIFO */
        IISCONFIG &= ~0x4;
#endif

        disable_fiq();
    }
    pcm_paused = !play;
}

bool pcm_is_paused(void)
{
    return pcm_paused;
}

bool pcm_is_playing(void)
{
    return pcm_playing;
}

/* ASM optimised FIQ handler. GCC fails to make use of the fact that FIQ mode
   has registers r8-r14 banked, and so does not need to be saved. This routine
   uses only these registers, and so will never touch the stack unless it
   actually needs to do so when calling callback_for_more. C version is still
   included below for reference.
 */
#if CONFIG_CPU == PP5020
void fiq(void) ICODE_ATTR __attribute__((naked));
void fiq(void)
{
    /* r12 contains IISCONFIG address (set in crt0.S to minimise code in actual
     * FIQ handler. r11 contains address of p (also set in crt0.S). Most other
     * addresses we need are generated by using offsets with these two.
     * r12 + 0x40 is IISFIFO_WR, and r12 + 0x0c is IISFIFO_CFG.
     * r8 and r9 contains local copies of p_size and p respectively.
     * r10 is a working register.
     */
    asm volatile (
        "ldr r10, [r12]       \n\t"
        "bic r10, r10, #0x2   \n\t" /* clear interrupt */
        "str r10, [r12]       \n\t"
        "ldr r8, [r11, #4]    \n\t" /* r8 = p_size */
        "ldr r9, [r11]        \n\t" /* r9 = p */
    ".loop:                   \n\t"
        "cmp r8, #0           \n\t" /* is p_size 0? */
        "beq .more_data       \n\t" /* if so, ask pcmbuf for more data */
    ".fifo_loop:              \n\t"    
        "ldr r10, [r12, #0x0c]\n\t" /* read IISFIFO_CFG to check FIFO status */
        "and r10, r10, #0x3f0000\n\t"
        "cmp r10, #0x10000    \n\t" 
        "bls .fifo_full       \n\t" /* FIFO full, exit */
        "ldr r10, [r9], #4    \n\t" /* load two samples */
        "mov r10, r10, ror #16\n\t" /* put left sample at the top bits */
        "str r10, [r12, #0x40]\n\t" /* write top sample, lower sample ignored */
        "mov r10, r10, lsl #16\n\t" /* shift lower sample up */
        "str r10, [r12, #0x40]\n\t" /* then write it */
        "subs r8, r8, #4      \n\t" /* check if we have more samples */
        "bne .fifo_loop       \n\t" /* yes, continue */
    ".more_data:              \n\t"
        "stmdb sp!, { r0-r3, r12, lr}\n\t" /* stack scratch regs and lr */
        "mov r0, r11          \n\t" /* r0 = &p */
        "add r1, r11, #4      \n\t" /* r1 = &p_size */
        "str r9, [r0]         \n\t" /* save internal copies of variables back */
        "str r8, [r1]         \n\t"
        "ldr r2, =callback_for_more\n\t"
        "ldr r2, [r2]         \n\t" /* get callback address */
        "cmp r2, #0           \n\t" /* check for null pointer */
        "movne lr, pc         \n\t" /* call callback_for_more */
        "bxne r2              \n\t"
        "ldmia sp!, { r0-r3, r12, lr}\n\t"
        "ldr r8, [r11, #4]    \n\t" /* reload p_size and p */
        "ldr r9, [r11]        \n\t"
        "cmp r8, #0           \n\t" /* did we actually get more data? */
        "bne .loop            \n\t" /* yes, continue to try feeding FIFO */
    ".dma_stop:               \n\t" /* no more data, do dma_stop() and exit */
        "ldr r10, =pcm_playing\n\t"
        "strb r8, [r10]       \n\t" /* pcm_playing = false (r8=0, look above) */
        "ldr r10, [r12]       \n\t" 
        "bic r10, r10, #0x20000002\n\t" /* disable playback FIFO and IRQ */
        "str r10, [r12]       \n\t"
        "mrs r10, cpsr        \n\t"
        "orr r10, r10, #0x40  \n\t" /* disable FIQ */
        "msr cpsr_c, r10      \n\t"
        "ldr r10, =pcm_paused \n\t"
        "strb r8, [r10]       \n\t" /* pcm_paused = false */
    ".exit:                   \n\t"
        "str r8, [r11, #4]    \n\t"
        "str r9, [r11]        \n\t"
        "subs pc, lr, #4      \n\t" /* FIQ specific return sequence */
    ".fifo_full:              \n\t" /* enable IRQ and exit */
        "ldr r10, [r12]       \n\t"
        "orr r10, r10, #0x2   \n\t" /* set interrupt */
        "str r10, [r12]       \n\t"
        "b .exit              \n\t"
    );
}
#else
void fiq(void) ICODE_ATTR __attribute__ ((interrupt ("FIQ")));
void fiq(void)
{
    /* Clear interrupt */
#if CONFIG_CPU == PP5020
    IISCONFIG &= ~0x2;
#elif CONFIG_CPU == PP5002
    inl(0xcf001040);
    IISFIFO_CFG &= ~(1<<9);
#endif

    do {
        while (p_size) {
            if (FIFO_FREE_COUNT < 2) {
                /* Enable interrupt */
#if CONFIG_CPU == PP5020
                IISCONFIG |= 0x2;
#elif CONFIG_CPU == PP5002
                IISFIFO_CFG |= (1<<9);
#endif
                return;
            }

            IISFIFO_WR = (*(p++))<<16;
            IISFIFO_WR = (*(p++))<<16;
            p_size-=4;
        }

        /* p is empty, get some more data */
        if (callback_for_more) {
            callback_for_more((unsigned char**)&p,&p_size);
        }
    } while (p_size);

    /* No more data, so disable the FIFO/FIQ */
    dma_stop();
}
#endif

void pcm_init(void)
{
    pcm_playing = false;
    pcm_paused = false;

    /* Initialize default register values. */
    wmcodec_init();
    
    /* The uda1380 needs a sleep(HZ) here - do we need one? */

    /* Power on */
    wmcodec_enable_output(true);

    /* Unmute the master channel (DAC should be at zero point now). */
    wmcodec_mute(false);

    /* Call dma_stop to initialize everything. */
    dma_stop();
}

#elif (CONFIG_CPU == PNX0101)

/* TODO: Implement for iFP7xx
   For now, just implement some dummy functions.
*/

void pcm_init(void)
{

}

void pcm_set_frequency(unsigned int frequency)
{
    (void)frequency;
}

void pcm_play_data(void (*get_more)(unsigned char** start, size_t* size),
        unsigned char* start, size_t size)
{
    (void)get_more;
    (void)start;
    (void)size;
}

void pcm_play_stop(void)
{
}

void pcm_mute(bool mute)
{
    (void)mute;
}

void pcm_play_pause(bool play)
{
    (void)play;
}

bool pcm_is_paused(void)
{
    return false;
}

bool pcm_is_playing(void)
{
    return false;
}

void pcm_calculate_peaks(int *left, int *right)
{
    (void)left;
    (void)right;
}

size_t pcm_get_bytes_waiting(void)
{
    return 0;
}

#endif

#if (CONFIG_CPU != PNX0101)
/*
 * This function goes directly into the DMA buffer to calculate the left and
 * right peak values. To avoid missing peaks it tries to look forward two full
 * peek periods (2/HZ sec, 100% overlap), although it's always possible that
 * the entire period will not be visible. To reduce CPU load it only looks at
 * every third sample, and this can be reduced even further if needed (even
 * every tenth sample would still be pretty accurate).
 */

/* Check for a peak every PEAK_STRIDE samples */
#define PEAK_STRIDE   3
/* Up to 1/50th of a second of audio for peak calculation */
/* This should use NATIVE_FREQUENCY, or eventually an adjustable freq. value */
#define PEAK_SAMPLES  (44100/50)

void pcm_calculate_peaks(int *left, int *right)
{
    short *addr;
    short *end;
    {
#ifdef HAVE_UDA1380
        size_t samples = (BCR0 & 0xffffff) / 4;
        addr = (short *) (SAR0 & ~3);
#elif defined(HAVE_WM8975) || defined(HAVE_WM8758) || defined(HAVE_WM8731)
        size_t samples = p_size / 4;
        addr = p;
#elif defined(HAVE_TLV320)
        size_t samples = 4;  /* TODO X5 */
        addr = NULL;
#endif

        if (samples > PEAK_SAMPLES)
            samples = PEAK_SAMPLES - (PEAK_STRIDE - 1);
        else
            samples -= MIN((PEAK_STRIDE - 1), samples);

        end = addr + samples * 4;
    }

    if (left && right) {
        int left_peak = 0, right_peak = 0;

        while (addr < end) {
            int value;
            if ((value = addr [0]) > left_peak)
                left_peak = value;
            else if (-value > left_peak)
                left_peak = -value;

            if ((value = addr [PEAK_STRIDE | 1]) > right_peak)
                right_peak = value;
            else if (-value > right_peak)
                right_peak = -value;

            addr += PEAK_STRIDE * 2;
        }

        *left = left_peak;
        *right = right_peak;
    }
    else if (left || right) {
        int peak_value = 0, value;

        if (right)
            addr += (PEAK_STRIDE | 1);

        while (addr < end) {
            if ((value = addr [0]) > peak_value)
                peak_value = value;
            else if (-value > peak_value)
                peak_value = -value;

            addr += PEAK_STRIDE * 2;
        }

        if (left)
            *left = peak_value;
        else
            *right = peak_value;
    }
}

#endif
