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

#include "i2c-pp5020.h"
#include "wm8975.h"
#include "pcf50605.h"

void wmcodec_reset(void);

#define IPOD_PCM_LEVEL 0x65       /* -6dB */

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

void wm8975_write(int reg, int data)
{
    ipod_i2c_send(0x1a, (reg<<1) | ((data&0x100)>>8),data&0xff);
}

/*
 * Initialise the WM8975 for playback via headphone and line out.
 * Note, I'm using the WM8750 datasheet as its apparently close.
 */
int wmcodec_init(void) {
    /* reset I2C */
    i2c_init();

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

    return 0;
}

/* Silently enable / disable audio output */
void wmcodec_enable_output(bool enable)
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
        wm8975_write(RESET, 0x1ff);    /*Reset*/
        wm8975_write(RESET, 0x0);
    
         /* 2. Enable Vmid and VREF. */
        wm8975_write(PWRMGMT1, 0xc0);    /*Pwr Mgmt(1)*/
    
         /* 3. Enable DACs as required. */
        wm8975_write(PWRMGMT2, 0x180);    /*Pwr Mgmt(2)*/
    
         /* 4. Enable line and / or headphone output buffers as required. */
        wm8975_write(PWRMGMT2, 0x1f8);    /*Pwr Mgmt(2)*/
    
        /* BCLKINV=0(Dont invert BCLK) MS=1(Enable Master) LRSWAP=0 LRP=0 */
        /* IWL=00(16 bit) FORMAT=10(I2S format) */
        wm8975_write(AINTFCE, 0x42);
    
        /* The iPod can handle multiple frequencies, but fix at 44.1KHz for now */
        wmcodec_set_sample_rate(WM8975_44100HZ);
    
        /* set the volume to -6dB */
        wm8975_write(LOUT1VOL, IPOD_PCM_LEVEL);
        wm8975_write(ROUT1VOL,0x100 | IPOD_PCM_LEVEL);
        wm8975_write(LOUT1VOL, IPOD_PCM_LEVEL);
        wm8975_write(ROUT1VOL,0x100 | IPOD_PCM_LEVEL);

        wm8975_write(LOUTMIX1, 0x150);    /* Left out Mix(def) */
        wm8975_write(LOUTMIX2, 0x50);
    
        wm8975_write(ROUTMIX1, 0x50);    /* Right out Mix(def) */
        wm8975_write(ROUTMIX2, 0x150);
    
        wm8975_write(MOUTMIX1, 0x0);     /* Mono out Mix */
        wm8975_write(MOUTMIX2, 0x0);

        wmcodec_mute(0);
    } else {
        wmcodec_mute(1);
    }
}

int wmcodec_set_master_vol(int vol_l, int vol_r)
{
    /* +6 to -73dB 1dB steps (plus mute == 80levels) 7bits */
    /* 1111111 == +6dB */
    /* 1111001 == 0dB */
    /* 0110000 == -73dB */
    /* 0101111 == mute (0x2f) */

    /* OUT1 */
    wm8975_write(LOUT1VOL, vol_l);
    wm8975_write(ROUT1VOL, 0x100 | vol_r);

    return 0;
}

int wmcodec_set_lineout_vol(int vol_l, int vol_r)
{
    /* OUT2 */
    wm8975_write(LOUT2VOL, vol_l);
    wm8975_write(ROUT2VOL, 0x100 | vol_r);

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
  int regvalues[]={11, 10, 10,  9,  8,  8, 0xf , 6, 6, 5, 4, 4, 3, 2, 1, 0};

  if ((value >= -6) && (value <= 9)) {
      /* We use linear bass control with 130Hz cutoff */
      wm8975_write(BASSCTRL, regvalues[value+6]);
  }
}

void wmcodec_set_treble(int value)
{
  int regvalues[]={11, 10, 10,  9,  8,  8, 0xf , 6, 6, 5, 4, 4, 3, 2, 1, 0};

  if ((value >= -6) && (value <= 9)) {
      /* We use a 8Khz cutoff */
      wm8975_write(TREBCTRL, regvalues[value+6]);
  }
}

int wmcodec_mute(int mute)
{
    if (mute)
    {
        /* Set DACMU = 1 to soft-mute the audio DACs. */
        wm8975_write(DACCTRL, 0x8);
    } else {
        /* Set DACMU = 0 to soft-un-mute the audio DACs. */
        wm8975_write(DACCTRL, 0x0);
    }

    return 0;
}

/* Nice shutdown of WM8975 codec */
void wmcodec_close(void)
{
    /* 1. Set DACMU = 1 to soft-mute the audio DACs. */
    wm8975_write(DACCTRL, 0x8);

    /* 2. Disable all output buffers. */
    wm8975_write(PWRMGMT2, 0x0);     /*Pwr Mgmt(2)*/

    /* 3. Switch off the power supplies. */
    wm8975_write(PWRMGMT1, 0x0);     /*Pwr Mgmt(1)*/
}

/* Change the order of the noise shaper, 5th order is recommended above 32kHz */
void wmcodec_set_nsorder(int order)
{
    (void)order;
}

/* Note: Disable output before calling this function */
void wmcodec_set_sample_rate(int sampling_control) {

  wm8975_write(0x08, sampling_control);

}

void wmcodec_enable_recording(bool source_mic) {

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
