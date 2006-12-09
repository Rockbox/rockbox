/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Driver for WM8975 audio codec
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
#include "wm8975.h"
#include "i2s.h"

/* convert tenth of dB volume (-730..60) to master volume register value */
int tenthdb2master(int db)
{
    /* +6 to -73dB 1dB steps (plus mute == 80levels) 7bits */
    /* 1111111 == +6dB  (0x7f) */
    /* 1111001 == 0dB   (0x79) */
    /* 0110000 == -73dB (0x30 */
    /* 0101111 == mute  (0x2f) */

    if (db < VOLUME_MIN) {
        return 0x0;
    } else {
        return((db/10)+73+0x30);
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


/* Silently enable / disable audio output */
void audiohw_enable_output(bool enable)
{
    if (enable) 
    {
        /* reset the I2S controller into known state */
        i2s_reset();

        /*
         * 1. Switch on power supplies.
         *    By default the WM8750L is in Standby Mode, the DAC is
         *    digitally muted and the Audio Interface, Line outputs
         *    and Headphone outputs are all OFF (DACMU = 1 Power
         *    Management registers 1 and 2 are all zeros).
         */
        wmcodec_write(RESET, 0x1ff);    /*Reset*/
        wmcodec_write(RESET, 0x0);
    
         /* 2. Enable Vmid and VREF. */
        wmcodec_write(PWRMGMT1, 0xc0);    /*Pwr Mgmt(1)*/
    
         /* 3. Enable DACs as required. */
        wmcodec_write(PWRMGMT2, 0x180);    /*Pwr Mgmt(2)*/
    
         /* 4. Enable line and / or headphone output buffers as required. */
        wmcodec_write(PWRMGMT2, 0x1f8);    /*Pwr Mgmt(2)*/
    
        /* BCLKINV=0(Dont invert BCLK) MS=1(Enable Master) LRSWAP=0 LRP=0 */
        /* IWL=00(16 bit) FORMAT=10(I2S format) */
        wmcodec_write(AINTFCE, 0x42);
    
        /* The iPod can handle multiple frequencies, but fix at 44.1KHz for now */
        audiohw_set_sample_rate(WM8975_44100HZ);
    
        /* set the volume to -6dB */
        wmcodec_write(LOUT1VOL, IPOD_PCM_LEVEL);
        wmcodec_write(ROUT1VOL,0x100 | IPOD_PCM_LEVEL);
        wmcodec_write(LOUT1VOL, IPOD_PCM_LEVEL);
        wmcodec_write(ROUT1VOL,0x100 | IPOD_PCM_LEVEL);

        wmcodec_write(LOUTMIX1, 0x150);    /* Left out Mix(def) */
        wmcodec_write(LOUTMIX2, 0x50);
    
        wmcodec_write(ROUTMIX1, 0x50);    /* Right out Mix(def) */
        wmcodec_write(ROUTMIX2, 0x150);
    
        wmcodec_write(MOUTMIX1, 0x0);     /* Mono out Mix */
        wmcodec_write(MOUTMIX2, 0x0);

        audiohw_mute(0);
    } else {
        audiohw_mute(1);
    }
}

int audiohw_set_master_vol(int vol_l, int vol_r)
{
    /* +6 to -73dB 1dB steps (plus mute == 80levels) 7bits */
    /* 1111111 == +6dB */
    /* 1111001 == 0dB */
    /* 0110000 == -73dB */
    /* 0101111 == mute (0x2f) */

    /* OUT1 */
    wmcodec_write(LOUT1VOL, vol_l);
    wmcodec_write(ROUT1VOL, 0x100 | vol_r);

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
  int regvalues[]={11, 10, 10,  9,  8,  8, 0xf , 6, 6, 5, 4, 4, 3, 2, 1, 0};

  if ((value >= -6) && (value <= 9)) {
      /* We use linear bass control with 130Hz cutoff */
      wmcodec_write(BASSCTRL, regvalues[value+6]);
  }
}

void audiohw_set_treble(int value)
{
  int regvalues[]={11, 10, 10,  9,  8,  8, 0xf , 6, 6, 5, 4, 4, 3, 2, 1, 0};

  if ((value >= -6) && (value <= 9)) {
      /* We use a 8Khz cutoff */
      wmcodec_write(TREBCTRL, regvalues[value+6]);
  }
}

int audiohw_mute(int mute)
{
    if (mute)
    {
        /* Set DACMU = 1 to soft-mute the audio DACs. */
        wmcodec_write(DACCTRL, 0x8);
    } else {
        /* Set DACMU = 0 to soft-un-mute the audio DACs. */
        wmcodec_write(DACCTRL, 0x0);
    }

    return 0;
}

/* Nice shutdown of WM8975 codec */
void audiohw_close(void)
{
    /* 1. Set DACMU = 1 to soft-mute the audio DACs. */
    wmcodec_write(DACCTRL, 0x8);

    /* 2. Disable all output buffers. */
    wmcodec_write(PWRMGMT2, 0x0);     /*Pwr Mgmt(2)*/

    /* 3. Switch off the power supplies. */
    wmcodec_write(PWRMGMT1, 0x0);     /*Pwr Mgmt(1)*/
}

/* Change the order of the noise shaper, 5th order is recommended above 32kHz */
void audiohw_set_nsorder(int order)
{
    (void)order;
}

/* Note: Disable output before calling this function */
void audiohw_set_sample_rate(int sampling_control) {

  wmcodec_write(0x08, sampling_control);

}

void audiohw_enable_recording(bool source_mic) {

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
