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

#include "wmcodec.h"
#include "wm8758.h"
#include "i2s.h"

/* convert tenth of dB volume (-57..6) to master volume register value */
int tenthdb2master(int db)
{
    /* +6 to -57dB in 1dB steps == 64 levels = 6 bits */
    /* 0111111 == +6dB  (0x3f) = 63) */
    /* 0111001 == 0dB   (0x39) = 57) */
    /* 0000001 == -56dB (0x01) = */
    /* 0000000 == -57dB (0x00) */

    /* 1000000 == Mute (0x40) */

    if (db < VOLUME_MIN) {
        return 0x40;
    } else {
        return((db/10)+57);
    }
}

/* convert tenth of dB volume (-780..0) to mixer volume register value */
int tenthdb2mixer(int db)
{
    if (db < -660)                 /* 1.5 dB steps */
        return (2640 - db) / 15;
    else if (db < -600)            /* 0.75 dB steps */
        return (990 - db) * 2 / 15;
    else if (db < -460)            /* 0.5 dB steps */
        return (460 - db) / 5; 
    else                           /* 0.25 dB steps */
        return -db * 2 / 5;
}

void audiohw_reset(void);

#define IPOD_PCM_LEVEL 0x65       /* -6dB */

//#define BASSCTRL   0x
//#define TREBCTRL   0x0b

/* Silently enable / disable audio output */
void audiohw_enable_output(bool enable)
{
    if (enable) 
    {
        /* reset the I2S controller into known state */
        i2s_reset();

        /* TODO: Review the power-up sequence to prevent pops */

        wmcodec_write(RESET, 0x1ff);    /*Reset*/
    
        wmcodec_write(PWRMGMT1, 0x2b);
        wmcodec_write(PWRMGMT2, 0x180);
        wmcodec_write(PWRMGMT3, 0x6f);

        wmcodec_write(AINTFCE, 0x10);
        wmcodec_write(CLKCTRL, 0x49);

        wmcodec_write(OUTCTRL, 1);

        /* The iPod can handle multiple frequencies, but fix at 44.1KHz
           for now */
        audiohw_set_sample_rate(WM8758_44100HZ);
    
        wmcodec_write(LOUTMIX,0x1); /* Enable mixer */
        wmcodec_write(ROUTMIX,0x1); /* Enable mixer */
        audiohw_mute(0);
    } else {
        audiohw_mute(1);
    }
}

int audiohw_set_master_vol(int vol_l, int vol_r)
{
    /* OUT1 */
    wmcodec_write(LOUT1VOL, 0x080 | vol_l);
    wmcodec_write(ROUT1VOL, 0x180 | vol_r);

    return 0;
}

int audiohw_set_lineout_vol(int vol_l, int vol_r)
{
    /* OUT2 */
    wmcodec_write(LOUT2VOL, vol_l);
    wmcodec_write(ROUT2VOL, 0x100 | vol_r);
    
    return 0;
}

int audiohw_set_mixer_vol(int channel1, int channel2)
{
    (void)channel1;
    (void)channel2;

    return 0;
}

/* We are using Linear bass control */
void audiohw_set_bass(int value)
{
    (void)value;
#if 0
    /* Not yet implemented - this is the wm8975 code*/
    int regvalues[]={11, 10, 10,  9,  8,  8, 0xf , 6, 6, 5, 4, 4, 3, 2, 1, 0};

    if ((value >= -6) && (value <= 9)) {
        /* We use linear bass control with 130Hz cutoff */
        wmcodec_write(BASSCTRL, regvalues[value+6]);
    }
#endif
}

void audiohw_set_treble(int value)
{
    (void)value;
#if 0
    /* Not yet implemented - this is the wm8975 code*/
    int regvalues[]={11, 10, 10,  9,  8,  8, 0xf , 6, 6, 5, 4, 4, 3, 2, 1, 0};

    if ((value >= -6) && (value <= 9)) {
        /* We use a 8Khz cutoff */
        wmcodec_write(TREBCTRL, regvalues[value+6]);
    }
#endif

}

int audiohw_mute(int mute)
{
    if (mute)
    {
        /* Set DACMU = 1 to soft-mute the audio DACs. */
        wmcodec_write(DACCTRL, 0x40);
    } else {
        /* Set DACMU = 0 to soft-un-mute the audio DACs. */
        wmcodec_write(DACCTRL, 0x0);
    }

    return 0;
}

/* Nice shutdown of WM8758 codec */
void audiohw_close(void)
{
    audiohw_mute(1);

    wmcodec_write(PWRMGMT3, 0x0);

    wmcodec_write(PWRMGMT1, 0x0);

    wmcodec_write(PWRMGMT2, 0x40);
}

/* Change the order of the noise shaper, 5th order is recommended above 32kHz */
void audiohw_set_nsorder(int order)
{
    (void)order;
}

/* Note: Disable output before calling this function */
void audiohw_set_sample_rate(int sampling_control)
{
    /**** We force 44.1KHz for now. ****/
    (void)sampling_control;

    /* set clock div */
    wmcodec_write(CLKCTRL, 1 | (0 << 2) | (2 << 5));

    /* setup PLL for MHZ=11.2896 */
    wmcodec_write(PLLN, (1 << 4) | 0x7);
    wmcodec_write(PLLK1, 0x21);
    wmcodec_write(PLLK2, 0x161);
    wmcodec_write(PLLK3, 0x26);

    /* set clock div */
    wmcodec_write(CLKCTRL, 1 | (1 << 2) | (2 << 5) | (1 << 8));

    /* set srate */
    wmcodec_write(SRATECTRL, (0 << 1));
}

void audiohw_enable_recording(bool source_mic)
{
    (void)source_mic;
}

void audiohw_disable_recording(void) {

}

void audiohw_set_recvol(int left, int right, int type) {

    (void)left;
    (void)right;
    (void)type;
}

void audiohw_set_monitor(int enable) {

    (void)enable;
}

void audiohw_set_equalizer_band(int band, int freq, int bw, int gain)
{
    unsigned int eq = 0;

    /* Band 1..3 are peak filters */
    if (band >= 1 && band <= 3) {
        eq |= (bw << 8);
    }

    eq |= (freq << 5);
    eq |= 12 - gain;

    if (band == 0) {
        wmcodec_write(EQ1, eq | 0x100); /* Always apply EQ to the DAC path */
    } else if (band == 1) {
        wmcodec_write(EQ2, eq);
    } else if (band == 2) {
        wmcodec_write(EQ3, eq);
    } else if (band == 3) {
        wmcodec_write(EQ4, eq);
    } else if (band == 4) {
        wmcodec_write(EQ5, eq);
    }
}
