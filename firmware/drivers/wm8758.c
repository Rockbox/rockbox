/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Driver for WM8758 audio codec
 *
 * Based on code from the ipodlinux project - http://ipodlinux.org/
 * Adapted for Rockbox in December 2005
 *
 * Original file: linux/arch/armnommu/mach-ipod/audio.c
 *
 * Copyright (c) 2003-2005 Bernard Leach (leachbj@bouncycastle.org)
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include "lcd.h"
#include "cpu.h"
#include "kernel.h"
#include "thread.h"
#include "power.h"
#include "debug.h"
#include "system.h"
#include "sprintf.h"
#include "button.h"
#include "string.h"
#include "file.h"
#include "buffer.h"
#include "audio.h"

#include "i2c-pp5020.h"
#include "wm8758.h"
#include "pcf50605.h"

void wmcodec_reset(void);

#define IPOD_PCM_LEVEL 0x65       /* -6dB */

//#define BASSCTRL   0x
//#define TREBCTRL   0x0b

/*
 * Reset the I2S BIT.FORMAT I2S, 16bit, FIFO.FORMAT 32bit
 */
static void i2s_reset(void)
{
    /* PP502x */

    /* I2S soft reset */
    outl(inl(0x70002800) | 0x80000000, 0x70002800);
    outl(inl(0x70002800) & ~0x80000000, 0x70002800);

    /* BIT.FORMAT [11:10] = I2S (default) */
    outl(inl(0x70002800) & ~0xc00, 0x70002800);
    /* BIT.SIZE [9:8] = 16bit (default) */
    outl(inl(0x70002800) & ~0x300, 0x70002800);

    /* FIFO.FORMAT [6:4] = 32 bit LSB */
    /* since BIT.SIZ < FIFO.FORMAT low 16 bits will be 0 */
    outl(inl(0x70002800) | 0x30, 0x70002800);

    /* RX_ATN_LVL=1 == when 12 slots full */
    /* TX_ATN_LVL=1 == when 12 slots empty */
    outl(inl(0x7000280c) | 0x33, 0x7000280c);

    /* Rx.CLR = 1, TX.CLR = 1 */
    outl(inl(0x7000280c) | 0x1100, 0x7000280c);
}

void wm8758_write(int reg, int data)
{
    ipod_i2c_send(0x1a, (reg<<1) | ((data&0x100)>>8),data&0xff);
}

/*
 * Initialise the WM8758 for playback via headphone and line out.
 * Note, I'm using the WM8750 datasheet as its apparently close.
 */
int wmcodec_init(void) {
    int old_irq_level = set_irq_level(HIGHEST_IRQ_LEVEL);

    /* normal outputs for CDI and I2S pin groups */
    outl(inl(0x70000020) & ~0x300, 0x70000020);

    /*mini2?*/
    outl(inl(0x70000010) & ~0x3000000, 0x70000010);
    /*mini2?*/

    /* device reset */
    outl(inl(0x60006004) | 0x800, 0x60006004);
    outl(inl(0x60006004) & ~0x800, 0x60006004);

    /* device enable */
    outl(inl(0x6000600C) | 0x807, 0x6000600C);

    /* enable external dev clock clocks */
    outl(inl(0x6000600c) | 0x2, 0x6000600c);

    /* external dev clock to 24MHz */
    outl(inl(0x70000018) & ~0xc, 0x70000018);

    set_irq_level(old_irq_level);
    return 0;
}

/* Silently enable / disable audio output */
void wmcodec_enable_output(bool enable)
{
    if (enable) 
    {
        int old_irq_level = set_irq_level(HIGHEST_IRQ_LEVEL);

        /* reset the I2S controller into known state */
        i2s_reset();

        /* TODO: Review the power-up sequence to prevent pops */

        wm8758_write(RESET, 0x1ff);    /*Reset*/
    
	wm8758_write(PWRMGMT1, 0x2b);
        wm8758_write(PWRMGMT2, 0x180);
	wm8758_write(PWRMGMT3, 0x6f);

	wm8758_write(AINTFCE, 0x10);
	wm8758_write(CLKCTRL, 0x49);

        wm8758_write(OUTCTRL, 1);

        /* The iPod can handle multiple frequencies, but fix at 44.1KHz
           for now */
        wmcodec_set_sample_rate(WM8758_44100HZ);
    
        wm8758_write(LOUTMIX,0x1); /* Enable mixer */
        wm8758_write(ROUTMIX,0x1); /* Enable mixer */
        wmcodec_mute(0);
        set_irq_level(old_irq_level);
    } else {
        wmcodec_mute(1);
    }
}

int wmcodec_set_master_vol(int vol_l, int vol_r)
{
    int old_irq_level = set_irq_level(HIGHEST_IRQ_LEVEL);
    /* OUT1 */
    wm8758_write(LOUT1VOL, vol_l);
    wm8758_write(ROUT1VOL, 0x100 | vol_r);

    /* OUT2 */
    wm8758_write(LOUT2VOL, vol_l);
    wm8758_write(ROUT2VOL, 0x100 | vol_r);
    
    set_irq_level(old_irq_level);

    return 0;
}

int wmcodec_set_mixer_vol(int channel1, int channel2)
{
    (void)channel1;
    (void)channel2;

    return 0;
}

/* We are using Linear bass control */
void wmcodec_set_bass(int value)
{
    (void)value;
#if 0
    /* Not yet implemented - this is the wm8975 code*/
    int regvalues[]={11, 10, 10,  9,  8,  8, 0xf , 6, 6, 5, 4, 4, 3, 2, 1, 0};

    if ((value >= -6) && (value <= 9)) {
        /* We use linear bass control with 130Hz cutoff */
        wm8758_write(BASSCTRL, regvalues[value+6]);
    }
#endif
}

void wmcodec_set_treble(int value)
{
    (void)value;
#if 0
    /* Not yet implemented - this is the wm8975 code*/
    int regvalues[]={11, 10, 10,  9,  8,  8, 0xf , 6, 6, 5, 4, 4, 3, 2, 1, 0};

    if ((value >= -6) && (value <= 9)) {
        /* We use a 8Khz cutoff */
        wm8758_write(TREBCTRL, regvalues[value+6]);
    }
#endif

}

int wmcodec_mute(int mute)
{
    int old_irq_level = set_irq_level(HIGHEST_IRQ_LEVEL);

    if (mute)
    {
        /* Set DACMU = 1 to soft-mute the audio DACs. */
        wm8758_write(DACCTRL, 0x40);
    } else {
        /* Set DACMU = 0 to soft-un-mute the audio DACs. */
        wm8758_write(DACCTRL, 0x0);
    }

    set_irq_level(old_irq_level);

    return 0;
}

/* Nice shutdown of WM8758 codec */
void wmcodec_close(void)
{
    int old_irq_level = set_irq_level(HIGHEST_IRQ_LEVEL);

    wmcodec_mute(1);

    wm8758_write(PWRMGMT3, 0x0);

    wm8758_write(PWRMGMT1, 0x0);

    wm8758_write(PWRMGMT2, 0x40);

    set_irq_level(old_irq_level);
}

/* Change the order of the noise shaper, 5th order is recommended above 32kHz */
void wmcodec_set_nsorder(int order)
{
    (void)order;
}

/* Note: Disable output before calling this function */
void wmcodec_set_sample_rate(int sampling_control)
{
    int old_irq_level = set_irq_level(HIGHEST_IRQ_LEVEL);

    /**** We force 44.1KHz for now. ****/
    (void)sampling_control;

    /* set clock div */
    wm8758_write(CLKCTRL, 1 | (0 << 2) | (2 << 5));

    /* setup PLL for MHZ=11.2896 */
    wm8758_write(PLLN, (1 << 4) | 0x7);
    wm8758_write(PLLK1, 0x21);
    wm8758_write(PLLK2, 0x161);
    wm8758_write(PLLK3, 0x26);

    /* set clock div */
    wm8758_write(CLKCTRL, 1 | (1 << 2) | (2 << 5) | (1 << 8));

    /* set srate */
    wm8758_write(SRATECTRL, (0 << 1));

    set_irq_level(old_irq_level);
}

void wmcodec_enable_recording(bool source_mic)
{
    (void)source_mic;
}

void wmcodec_disable_recording(void) {

}

void wmcodec_set_recvol(int left, int right, int type) {

    (void)left;
    (void)right;
    (void)type;
}

void wmcodec_set_monitor(int enable) {

    (void)enable;
}
