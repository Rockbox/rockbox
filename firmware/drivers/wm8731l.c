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

#define IPOD_PCM_LEVEL 0x65       /* -6dB */

#define LINVOL        0x00
#define RINVOL        0x01
#define LOUTVOL       0x02
#define ROUTVOL       0x03
#define DACCTRL       0x05
#define PWRMGMT       0x06
#define AINTFCE       0x07
#define SAMPCTRL      0x08
#define ACTIVECTRL    0x09
#define RESET         0x0f

void wm8731_write(int reg, int data)
{
    ipod_i2c_send(0x1a, (reg<<1) | ((data&0x100)>>8),data&0xff);
}

int wmcodec_mute(int mute)
{
    if (mute)
    {
        /* Set DACMU = 1 to soft-mute the audio DACs. */
        wm8731_write(DACCTRL, 0x8);
    } else {
        /* Set DACMU = 0 to soft-un-mute the audio DACs. */
        wm8731_write(DACCTRL, 0x0);
    }

    return 0;
}

/** From ipodLinux **/
static void codec_set_active(int active)
{
    /* set active to 0x0 or 0x1 */
    if (active) {
        wm8731_write(ACTIVECTRL, 0x01);
    } else {
        wm8731_write(ACTIVECTRL, 0x00);
    }
}

/*
 * Reset the I2S BIT.FORMAT I2S, 16bit, FIFO.FORMAT 32bit
 */
static void i2s_reset(void)
{
#if CONFIG_CPU == PP5020
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
#elif CONFIG_CPU == PP5002
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
#endif
}

/*
 * Initialise the WM8975 for playback via headphone and line out.
 * Note, I'm using the WM8750 datasheet as its apparently close.
 */
int wmcodec_init(void) {
    /* reset I2C */
    i2c_init();

#if CONFIG_CPU == PP5020
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
#else
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
#endif
    
    return 0;
}

/* Silently enable / disable audio output */
void wmcodec_enable_output(bool enable)
{
    if (enable)
    {
        /* reset the I2S controller into known state */
        i2s_reset();

        wm8731_write(RESET, 0x0);        /*Reset*/

        codec_set_active(0x0);

#ifdef HAVE_WM8721
        /* DACSEL=1 */
        wm8731_write(0x4, 0x10);
#elif defined HAVE_WM8731
        /* DACSEL=1, BYPASS=1 */
        wm8731_write(0x4, 0x18);
#endif
    
        /* set power register to POWEROFF=0 on OUTPD=0, DACPD=0 */
        wm8731_write(PWRMGMT, 0x67);

        /* BCLKINV=0(Dont invert BCLK) MS=1(Enable Master) LRSWAP=0 LRP=0 */
        /* IWL=00(16 bit) FORMAT=10(I2S format) */
        wm8731_write(AINTFCE, 0x42);

        wmcodec_set_sample_rate(WM8731L_44100HZ);

        /* set the volume to -6dB */
        wm8731_write(LOUTVOL, IPOD_PCM_LEVEL);
        wm8731_write(ROUTVOL, 0x100 | IPOD_PCM_LEVEL);

        /* ACTIVE=1 */
        codec_set_active(1);

        /* 5. Set DACMU = 0 to soft-un-mute the audio DACs. */
        wm8731_write(DACCTRL, 0x0);

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

    wm8731_write(LOUTVOL, vol_l);
    wm8731_write(ROUTVOL, vol_r);
 
    return 0;
}

int wmcodec_set_mixer_vol(int channel1, int channel2)
{
    (void)channel1;
    (void)channel2;

    return 0;
}

void wmcodec_set_bass(int value)
{
    (void)value;
}

void wmcodec_set_treble(int value)
{
    (void)value;
}

/* Nice shutdown of WM8731 codec */
void wmcodec_close(void)
{
   /* set DACMU=1 DEEMPH=0 */
    wm8731_write(DACCTRL, 0x8);

    /* ACTIVE=0 */
    codec_set_active(0x0);

    /* line in mute left & right*/
    wm8731_write(LINVOL, 0x100 | 0x80);

    /* set DACSEL=0, MUTEMIC=1 */
    wm8731_write(0x4, 0x2);

    /* set POWEROFF=0 OUTPD=0 DACPD=1 */
    wm8731_write(PWRMGMT, 0x6f);

    /* set POWEROFF=1 OUTPD=1 DACPD=1 */
    wm8731_write(PWRMGMT, 0xff);
}

/* Change the order of the noise shaper, 5th order is recommended above 32kHz */
void wmcodec_set_nsorder(int order)
{
    (void)order;
}

void wmcodec_set_sample_rate(int sampling_control)
{
    codec_set_active(0x0);
    wm8731_write(SAMPCTRL, sampling_control);
    codec_set_active(0x1);
}

void wmcodec_enable_recording(bool source_mic)
{
    (void)source_mic;
}

void wmcodec_disable_recording(void)
{

}

void wmcodec_set_recvol(int left, int right, int type)
{
    (void)left;
    (void)right;
    (void)type;
}

void wmcodec_set_monitor(int enable)
{
    (void)enable;
}
