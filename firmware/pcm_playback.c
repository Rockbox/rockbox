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
#include "audio.h"
#if defined(HAVE_WM8975)
#include "wm8975.h"
#elif defined(HAVE_WM8758)
#include "wm8758.h"
#elif defined(HAVE_WM8731) || defined(HAVE_WM8721)
#include "wm8731l.h"
#elif CONFIG_CPU == PNX0101
#include "string.h"
#include "pnx0101.h"
#endif

/**
 * APIs implemented in the target-specific portion:
 * Public -
 *      pcm_init
 *      pcm_get_bytes_waiting
 *      pcm_calculate_peaks
 * Semi-private -
 *      pcm_play_dma_start
 *      pcm_play_dma_stop
 *      pcm_play_pause_pause
 *      pcm_play_pause_unpause
 */

/** These items may be implemented target specifically or need to
    be shared semi-privately **/

/* the registered callback function to ask for more mp3 data */
volatile pcm_more_callback_type pcm_callback_for_more = NULL;
volatile bool                   pcm_playing           = false;
volatile bool                   pcm_paused            = false;

void pcm_play_dma_start(const void *addr, size_t size);
void pcm_play_dma_stop(void);
void pcm_play_pause_pause(void);
void pcm_play_pause_unpause(void);

/** Functions that require targeted implementation **/

#ifndef CPU_COLDFIRE

#if (CONFIG_CPU == S3C2440) 

/* TODO: Implement for Gigabeat
   For now, just implement some dummy functions.
*/
void pcm_init(void)
{
}

void pcm_play_dma_start(const void *addr, size_t size)
{
    (void)addr;
    (void)size;
}

void pcm_play_dma_stop(void)
{
}

void pcm_play_pause_pause(void)
{
}

void pcm_play_pause_unpause(void)
{
}

void pcm_set_frequency(unsigned int frequency)
{
    (void)frequency;
}

size_t pcm_get_bytes_waiting(void)
{
    return 0;
}

#elif defined(HAVE_WM8975) || defined(HAVE_WM8758) \
   || defined(HAVE_WM8731) || defined(HAVE_WM8721) \
   || defined(HAVE_PP5024_CODEC)

/* We need to unify this code with the uda1380 code as much as possible, but
   we will keep it separate during early development.
*/

#if CONFIG_CPU == PP5020
#define FIFO_FREE_COUNT ((IISFIFO_CFG & 0x3f0000) >> 16)
#elif CONFIG_CPU == PP5002
#define FIFO_FREE_COUNT ((IISFIFO_CFG & 0x7800000) >> 23)
#elif CONFIG_CPU == PP5024
#define FIFO_FREE_COUNT 4 /* TODO: make this sensible */
#endif

static int pcm_freq = HW_SAMPR_DEFAULT; /* 44.1 is default */

/* NOTE: The order of these two variables is important if you use the iPod
   assembler optimised fiq handler, so don't change it. */
unsigned short* p IBSS_ATTR;
size_t p_size IBSS_ATTR;

void pcm_play_dma_start(const void *addr, size_t size)
{
    p=(unsigned short*)addr;
    p_size=size;

    pcm_playing = true;

#if CONFIG_CPU == PP5020
    /* setup I2S interrupt for FIQ */
    outl(inl(0x6000402c) | I2S_MASK, 0x6000402c);
    outl(I2S_MASK, 0x60004024);
#elif CONFIG_CPU == PP5024
#else
    /* setup I2S interrupt for FIQ */
    outl(inl(0xcf00102c) | DMA_OUT_MASK, 0xcf00102c);
    outl(DMA_OUT_MASK, 0xcf001024);
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
void pcm_play_dma_stop(void)
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
}

void pcm_play_pause_pause(void)
{
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

void pcm_play_pause_unpause(void)
{
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

void pcm_set_frequency(unsigned int frequency)
{
    (void)frequency;
    pcm_freq = HW_SAMPR_DEFAULT;
}

size_t pcm_get_bytes_waiting(void)
{
    return p_size;
}

/* ASM optimised FIQ handler. GCC fails to make use of the fact that FIQ mode
   has registers r8-r14 banked, and so does not need to be saved. This routine
   uses only these registers, and so will never touch the stack unless it
   actually needs to do so when calling pcm_callback_for_more. C version is
   still included below for reference.
 */
#if CONFIG_CPU == PP5020 || CONFIG_CPU == PP5002
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
#if CONFIG_CPU == PP5002
        "ldr r10, =0xcf001040 \n\t" /* Some magic from iPodLinux */
        "ldr r10, [r10]       \n\t"
        "ldr r10, [r12, #0x1c]\n\t"
        "bic r10, r10, #0x200 \n\t" /* clear interrupt */
        "str r10, [r12, #0x1c]\n\t"
#else
        "ldr r10, [r12]       \n\t"
        "bic r10, r10, #0x2   \n\t" /* clear interrupt */
        "str r10, [r12]       \n\t"
#endif
        "ldr r8, [r11, #4]    \n\t" /* r8 = p_size */
        "ldr r9, [r11]        \n\t" /* r9 = p */
    ".loop:                   \n\t"
        "cmp r8, #0           \n\t" /* is p_size 0? */
        "beq .more_data       \n\t" /* if so, ask pcmbuf for more data */
    ".fifo_loop:              \n\t"
#if CONFIG_CPU == PP5002
        "ldr r10, [r12, #0x1c]\n\t" /* read IISFIFO_CFG to check FIFO status */
        "and r10, r10, #0x7800000\n\t"
        "cmp r10, #0x800000    \n\t"
#else
        "ldr r10, [r12, #0x0c]\n\t" /* read IISFIFO_CFG to check FIFO status */
        "and r10, r10, #0x3f0000\n\t"
        "cmp r10, #0x10000    \n\t"
#endif
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
        "ldr r2, =pcm_callback_for_more\n\t"
        "ldr r2, [r2]         \n\t" /* get callback address */
        "cmp r2, #0           \n\t" /* check for null pointer */
        "movne lr, pc         \n\t" /* call pcm_callback_for_more */
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
#if CONFIG_CPU == PP5002
        "bic r10, r10, #0x4\n\t" /* disable playback FIFO */
        "str r10, [r12]       \n\t"
        "ldr r10, [r12, #0x1c]  \n\t"
        "bic r10, r10, #0x200   \n\t" /* clear interrupt */
        "str r10, [r12, #0x1c]  \n\t"
#else
        "bic r10, r10, #0x20000002\n\t" /* disable playback FIFO and IRQ */
        "str r10, [r12]       \n\t"
#endif
        "mrs r10, cpsr        \n\t"
        "orr r10, r10, #0x40  \n\t" /* disable FIQ */
        "msr cpsr_c, r10      \n\t"
    ".exit:                   \n\t"
        "str r8, [r11, #4]    \n\t"
        "str r9, [r11]        \n\t"
        "subs pc, lr, #4      \n\t" /* FIQ specific return sequence */
    ".fifo_full:              \n\t" /* enable IRQ and exit */
#if CONFIG_CPU == PP5002
        "ldr r10, [r12, #0x1c]\n\t"
        "orr r10, r10, #0x200 \n\t" /* set interrupt */
        "str r10, [r12, #0x1c]\n\t"
#else
        "ldr r10, [r12]       \n\t"
        "orr r10, r10, #0x2   \n\t" /* set interrupt */
        "str r10, [r12]       \n\t"
#endif
        "b .exit              \n\t"
    );
}
#else /* !(CONFIG_CPU == PP5020 || CONFIG_CPU == PP5002) */
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
        if (pcm_callback_for_more) {
            pcm_callback_for_more((unsigned char**)&p,&p_size);
        }
    } while (p_size);

    /* No more data, so disable the FIFO/FIQ */
    pcm_play_dma_stop();
}
#endif /* CONFIG_CPU == PP5020 || CONFIG_CPU == PP5002 */

#ifdef HAVE_PP5024_CODEC
void pcm_init(void)
{
}
#else
void pcm_init(void)
{
    pcm_playing = false;
    pcm_paused = false;
    pcm_callback_for_more = NULL;

    /* Initialize default register values. */
    wmcodec_init();
    
    /* Power on */
    wmcodec_enable_output(true);

    /* Unmute the master channel (DAC should be at zero point now). */
    wmcodec_mute(false);

    /* Call pcm_play_dma_stop to initialize everything. */
    pcm_play_dma_stop();
}
#endif /* HAVE_PP5024_CODEC */
#elif (CONFIG_CPU == PNX0101)

#define DMA_BUF_SAMPLES 0x100

short __attribute__((section(".dmabuf"))) dma_buf_left[DMA_BUF_SAMPLES];
short __attribute__((section(".dmabuf"))) dma_buf_right[DMA_BUF_SAMPLES];

static int pcm_freq = HW_SAMPR_DEFAULT; /* 44.1 is default */

unsigned short* p IBSS_ATTR;
size_t p_size IBSS_ATTR;

void pcm_play_dma_start(const void *addr, size_t size)
{
    p = (unsigned short*)addr;
    p_size = size;

    pcm_playing = true;
}

void pcm_play_dma_stop(void)
{
    pcm_playing = false;
}

void pcm_play_pause_pause(void)
{
}

void pcm_play_pause_unpause(void)
{
}

static inline void fill_dma_buf(int offset)
{
    short *l, *r, *lend;

    l = dma_buf_left + offset;
    lend = l + DMA_BUF_SAMPLES / 2;
    r = dma_buf_right + offset;

    if (pcm_playing && !pcm_paused)
    {
        do
        {
            int count;
            unsigned short *tmp_p;
            count = MIN(p_size / 4, (size_t)(lend - l));
            tmp_p = p;
            p_size -= count * 4;

            if ((int)l & 3)
            {
                *l++ = *tmp_p++;
                *r++ = *tmp_p++;
                count--;
            }
            while (count >= 4)
            {
                asm("ldmia %0!, {r0, r1, r2, r3}\n\t"
                    "and   r4, r0, %3\n\t"
                    "orr   r4, r4, r1, lsl #16\n\t"
                    "and   r5, r2, %3\n\t"
                    "orr   r5, r5, r3, lsl #16\n\t"
                    "stmia %1!, {r4, r5}\n\t"
                    "bic   r4, r1, %3\n\t"
                    "orr   r4, r4, r0, lsr #16\n\t"
                    "bic   r5, r3, %3\n\t"
                    "orr   r5, r5, r2, lsr #16\n\t"
                    "stmia %2!, {r4, r5}"
                    : "+r" (tmp_p), "+r" (l), "+r" (r)
                    : "r" (0xffff)
                    : "r0", "r1", "r2", "r3", "r4", "r5", "memory");
                count -= 4;
            }
            while (count > 0)
            {
                *l++ = *tmp_p++;
                *r++ = *tmp_p++;
                count--;
            }
            p = tmp_p;
            if (l >= lend)
                return;
            else if (pcm_callback_for_more)
                pcm_callback_for_more((unsigned char**)&p,
                                  &p_size);
        }
        while (p_size);
        pcm_playing = false;
    }

    if (l < lend)
    {
        memset(l, 0, sizeof(short) * (lend - l));
        memset(r, 0, sizeof(short) * (lend - l));
    }
}

static void audio_irq(void)
{
    unsigned long st = DMAINTSTAT & ~DMAINTEN;
    int i;
    for (i = 0; i < 2; i++)
        if (st & (1 << i))
        {
            fill_dma_buf((i == 1) ? 0 : DMA_BUF_SAMPLES / 2);
            DMAINTSTAT = 1 << i;
        }
}

unsigned long physical_address(void *p)
{
    unsigned long adr = (unsigned long)p;
    return (MMUBLOCK((adr >> 21) & 0xf) << 21) | (adr & ((1 << 21) - 1));
}

void pcm_init(void)
{
    int i;

    pcm_playing = false;
    pcm_paused = false;
    pcm_callback_for_more = NULL;

    memset(dma_buf_left, 0, sizeof(dma_buf_left));
    memset(dma_buf_right, 0, sizeof(dma_buf_right));

    for (i = 0; i < 8; i++)
    {
        DMASRC(i) = 0;
        DMADEST(i) = 0;
        DMALEN(i) = 0x1ffff;
        DMAR0C(i) = 0;
        DMAR10(i) = 0;
        DMAR1C(i) = 0;
    }

    DMAINTSTAT = 0xc000ffff;
    DMAINTEN = 0xc000ffff;
    
    DMASRC(0) = physical_address(dma_buf_left);
    DMADEST(0) = 0x80200280;
    DMALEN(0) = 0xff;
    DMAR1C(0) = 0;
    DMAR0C(0) = 0x40408;

    DMASRC(1) = physical_address(dma_buf_right);
    DMADEST(1) = 0x80200284;
    DMALEN(1) = 0xff;
    DMAR1C(1) = 0;
    DMAR0C(1) = 0x40409;

    irq_set_int_handler(0x1b, audio_irq);
    irq_enable_int(0x1b);

    DMAINTSTAT = 1;
    DMAINTSTAT = 2;
    DMAINTEN &= ~3;
    DMAR10(0) |= 1;
    DMAR10(1) |= 1;
}

void pcm_set_frequency(unsigned int frequency)
{
    (void)frequency;
    pcm_freq = HW_SAMPR_DEFAULT;
}
size_t pcm_get_bytes_waiting(void)
{
    return p_size;
}
#endif /* CONFIG_CPU == */

/* dummy functions for those not actually supporting all this yet */
void pcm_apply_settings(bool reset)
{
    (void)reset;
}

void pcm_set_monitor(int monitor)
{
    (void)monitor;
}
/** **/

void pcm_mute(bool mute)
{
#if defined(HAVE_WM8975) || defined(HAVE_WM8758) \
   || defined(HAVE_WM8731) || defined(HAVE_WM8721)
    wmcodec_mute(mute);
#endif
    if (mute)
        sleep(HZ/16);
}

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
#if (CONFIG_CPU == S3C2440)
    (void)left;
    (void)right;
#else
    short *addr;
    short *end;
    {
#if defined(HAVE_WM8975) || defined(HAVE_WM8758) \
   || defined(HAVE_WM8731) || defined(HAVE_WM8721) \
   || (CONFIG_CPU == PNX0101) || defined(HAVE_PP5024_CODEC)
        size_t samples = p_size / 4;
        addr = p;
#endif

        if (samples > PEAK_SAMPLES)
            samples = PEAK_SAMPLES - (PEAK_STRIDE - 1);
        else
            samples -= MIN(PEAK_STRIDE - 1, samples);

        end = &addr[samples * 2];
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

            addr = &addr[PEAK_STRIDE * 2];
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
#endif
}

#endif /* CPU_COLDFIRE */

/****************************************************************************
 * Functions that do not require targeted implementation but only a targeted
 * interface
 */

/* Common code to pcm_play_data and pcm_play_pause
   Returns true if DMA playback was started, else false. */
bool pcm_play_data_start(pcm_more_callback_type get_more,
                         unsigned char *start, size_t size)
{
    if (!(start && size))
    {
        size = 0;
        if (get_more)
            get_more(&start, &size);
    }

    if (start && size)
    {
        pcm_play_dma_start(start, size);
        return true;
    }

    return false;
}

void pcm_play_data(pcm_more_callback_type get_more,
                   unsigned char *start, size_t size)
{
    pcm_callback_for_more = get_more;

    if (pcm_play_data_start(get_more, start, size) && pcm_paused)
    {
        pcm_paused = false;
        pcm_play_pause(false);
    }
}

void pcm_play_pause(bool play)
{
    bool needs_change = pcm_paused == play;

    /* This needs to be done ahead of the rest to prevent infinite
       recursion from pcm_play_data */
    pcm_paused = !play;

    if (pcm_playing && needs_change)
    {
        if (play)
        {
            if (pcm_get_bytes_waiting())
            {
                logf("unpause");
                pcm_play_pause_unpause();
            }
            else
            {
                logf("unpause, no data waiting");
                if (!pcm_play_data_start(pcm_callback_for_more, NULL, 0))
                {
                    pcm_play_dma_stop();
                    logf("unpause attempted, no data");
                }
            }
        }
        else
        {
            logf("pause");
            pcm_play_pause_pause();
        }
    } /* pcm_playing && needs_change */
}

void pcm_play_stop(void)
{
    if (pcm_playing)
        pcm_play_dma_stop();
}

bool pcm_is_playing(void)
{
    return pcm_playing;
}

bool pcm_is_paused(void)
{
    return pcm_paused;
}
