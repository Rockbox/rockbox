/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Driver for WM8731L audio codec
 *
 * Based on code from the ipodlinux project - http://ipodlinux.org/
 * Adapted for Rockbox in January 2006
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

#include "i2c-pp5002.h"
#include "wm8731l.h"
#include "pcf50605.h"

void wm8731l_reset(void);

#define IPOD_PCM_LEVEL 0x65       /* -6dB */

#define RESET     (0x0f<<1)
#define PWRMGMT1  (0x19<<1)
#define PWRMGMT2  (0x1a<<1)
#define AINTFCE   (0x07<<1)
#define LOUT1VOL  (0x02<<1)
#define ROUT1VOL  (0x03<<1)
#define LOUT2VOL  (0x28<<1)
#define ROUT2VOL  (0x29<<1)

int wm8731l_mute(int mute)
{
    if (mute)
    {
        /* Set DACMU = 1 to soft-mute the audio DACs. */
        ipod_i2c_send(0x1a, 0xa, 0x8);
    } else {
        /* Set DACMU = 0 to soft-un-mute the audio DACs. */
        ipod_i2c_send(0x1a, 0xa, 0x0);
    }

    return 0;
}
/** From ipodLinux **/
static void codec_set_active(int active)
{
    /* set active to 0x0 or 0x1 */
    if (active) {
        ipod_i2c_send(0x1a, 0x12, 0x01);
    } else {
        ipod_i2c_send(0x1a, 0x12, 0x00);
    }
}

/*
 * Reset the I2S BIT.FORMAT I2S, 16bit, FIFO.FORMAT 32bit
 */
static void i2s_reset(void)
{
    /* I2S device reset */
    outl(inl(0xcf005030) | 0x80, 0xcf005030);
    outl(inl(0xcf005030) & ~0x80, 0xcf005030);

    /* I2S controller enable */
    outl(inl(0xc0002500) | 0x1, 0xc0002500);

    /* BIT.FORMAT [11:10] = I2S (default) */
    /* BIT.SIZE [9:8] = 24bit */
    /* FIFO.FORMAT = 24 bit LSB */

    /* reset DAC and ADC fifo */
    outl(inl(0xc000251c) | 0x30000, 0xc000251c);
}

/*
 * Initialise the WM8975 for playback via headphone and line out.
 * Note, I'm using the WM8750 datasheet as its apparently close.
 */
int wm8731l_init(void) {
    /* reset I2C */
    i2c_init();

    /* device reset */
    outl(inl(0xcf005030) | 0x80, 0xcf005030);
    outl(inl(0xcf005030) & ~0x80, 0xcf005030);

    /* device enable */
    outl(inl(0xcf005000) | 0x80, 0xcf005000);

    /* GPIO D06 enable for output */
    outl(inl(0xcf00000c) | 0x40, 0xcf00000c);
    outl(inl(0xcf00001c) & ~0x40, 0xcf00001c);
    /* bits 11,10 == 01 */
    outl(inl(0xcf004040) | 0x400, 0xcf004040);
    outl(inl(0xcf004040) & ~0x800, 0xcf004040);

    outl(inl(0xcf004048) & ~0x1, 0xcf004048);

    outl(inl(0xcf000004) & ~0xf, 0xcf000004);
    outl(inl(0xcf004044) & ~0xf, 0xcf004044);

    /* C03 = 0 */
    outl(inl(0xcf000008) | 0x8, 0xcf000008);
    outl(inl(0xcf000018) | 0x8, 0xcf000018);
    outl(inl(0xcf000028) & ~0x8, 0xcf000028);
    
    return 0;
}

/* Silently enable / disable audio output */
void wm8731l_enable_output(bool enable)
{
    if (enable) 
    {
        /* reset the I2S controller into known state */
        i2s_reset();

        ipod_i2c_send(0x1a, 0x1e, 0x0);        /*Reset*/

        codec_set_active(0x0);

        /* DACSEL=1 */
        /* BYPASS=1 */
        ipod_i2c_send(0x1a, 0x8, 0x18);
    
        /* set power register to POWEROFF=0 on OUTPD=0, DACPD=0 */
        ipod_i2c_send(0x1a, 0xc, 0x67);

        /* BCLKINV=0(Dont invert BCLK) MS=1(Enable Master) LRSWAP=0 LRP=0 */
        /* IWL=00(16 bit) FORMAT=10(I2S format) */
        ipod_i2c_send(0x1a, 0xe, 0x42);

        wm8731l_set_sample_rate(WM8731L_44100HZ);

        /* set the volume to -6dB */
        ipod_i2c_send(0x1a, 0x4, IPOD_PCM_LEVEL);
        ipod_i2c_send(0x1a, 0x6 | 0x1, IPOD_PCM_LEVEL);

        /* ACTIVE=1 */
        codec_set_active(1);

        /* 5. Set DACMU = 0 to soft-un-mute the audio DACs. */
        ipod_i2c_send(0x1a, 0xa, 0x0);

        wm8731l_mute(0);
    } else {
        wm8731l_mute(1);
    }
}

int wm8731l_set_master_vol(int vol_l, int vol_r)
{
    /* +6 to -73dB 1dB steps (plus mute == 80levels) 7bits */
    /* 1111111 == +6dB */
    /* 1111001 == 0dB */
    /* 0110000 == -73dB */
    /* 0101111 == mute (0x2f) */
    if (vol_l == vol_r) {
        ipod_i2c_send(0x1a, 0x4 | 0x1, vol_l);
    } else {
        ipod_i2c_send(0x1a, 0x4, vol_l);
        ipod_i2c_send(0x1a, 0x6, vol_r);
    }
 
    return 0;
}

int wm8975_set_mixer_vol(int channel1, int channel2)
{
    (void)channel1;
    (void)channel2;

    return 0;
}

void wm8731l_set_bass(int value)
{
    (void)value;
}

void wm8731l_set_treble(int value)
{
    (void)value;
}

/* Nice shutdown of WM8975 codec */
void wm8731l_close(void)
{
   /* set DACMU=1 DEEMPH=0 */
    ipod_i2c_send(0x1a, 0xa, 0x8);

    /* ACTIVE=0 */
    codec_set_active(0x0);

    /* line in mute left & right*/
    ipod_i2c_send(0x1a, 0x0 | 0x1, 0x80);

    /* set DACSEL=0, MUTEMIC=1 */
    ipod_i2c_send(0x1a, 0x8, 0x2);

    /* set POWEROFF=0 OUTPD=0 DACPD=1 */
    ipod_i2c_send(0x1a, 0xc, 0x6f);

    /* set POWEROFF=1 OUTPD=1 DACPD=1 */
    ipod_i2c_send(0x1a, 0xc, 0xff);
}

/* Change the order of the noise shaper, 5th order is recommended above 32kHz */
void wm8731l_set_nsorder(int order)
{
    (void)order;
}

/*  */
void wm8731l_set_sample_rate(int sampling_control)
{
    codec_set_active(0x0);
    ipod_i2c_send(0x1a, 0x10, sampling_control);
    codec_set_active(0x1);
}

void wm8731l_enable_recording(bool source_mic)
{
    (void)source_mic;
}

void wm8731l_disable_recording(void)
{

}

void wm8731l_set_recvol(int left, int right, int type)
{
    (void)left;
    (void)right;
    (void)type;
}

void wm8731l_set_monitor(int enable)
{
    (void)enable;
}
