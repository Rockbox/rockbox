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

#include "wmcodec.h"
#include "wm8731l.h"

#define IPOD_PCM_LEVEL 0x65       /* -6dB */

/* convert tenth of dB volume (-730..60) to master volume register value */
int tenthdb2master(int db)
{
    /* +6 to -73dB 1dB steps (plus mute == 80levels) 7bits */
    /* 1111111 == +6dB  (0x7f) */
    /* 1111001 == 0dB   (0x79) */
    /* 0110000 == -73dB (0x30 */
    /* 0101111 == mute  (0x2f) */

    if (db < VOLUME_MIN) {
        return 0x2f;
    } else {
        return((db/10)+0x30+73);
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

/** From ipodLinux **/
static void codec_set_active(int active)
{
    /* set active to 0x0 or 0x1 */
    if (active) {
        wmcodec_write(ACTIVECTRL, 0x01);
    } else {
        wmcodec_write(ACTIVECTRL, 0x00);
    }
}


/* Silently enable / disable audio output */
void audiohw_enable_output(bool enable)
{
    if (enable)
    {
        /* reset the I2S controller into known state */
        i2s_reset();

        wmcodec_write(RESET, 0x0);        /*Reset*/

        codec_set_active(0x0);

#ifdef HAVE_WM8721
        /* DACSEL=1 */
        wmcodec_write(0x4, 0x10);
#elif defined HAVE_WM8731
        /* DACSEL=1, BYPASS=1 */
        wmcodec_write(0x4, 0x18);
#endif
    
        /* set power register to POWEROFF=0 on OUTPD=0, DACPD=0 */
        wmcodec_write(PWRMGMT, 0x67);

        /* BCLKINV=0(Dont invert BCLK) MS=1(Enable Master) LRSWAP=0 LRP=0 */
        /* IWL=00(16 bit) FORMAT=10(I2S format) */
        wmcodec_write(AINTFCE, 0x42);

        audiohw_set_sample_rate(WM8731L_44100HZ);

        /* set the volume to -6dB */
        wmcodec_write(LOUTVOL, IPOD_PCM_LEVEL);
        wmcodec_write(ROUTVOL, 0x100 | IPOD_PCM_LEVEL);

        /* ACTIVE=1 */
        codec_set_active(1);

        /* 5. Set DACMU = 0 to soft-un-mute the audio DACs. */
        wmcodec_write(DACCTRL, 0x0);
        
#if defined(IRIVER_H10) || defined(IRIVER_H10_5GB)
        /* We need to enable bit 4 of GPIOL for output for sound on H10 */
        GPIOL_OUTPUT_VAL |= 0x10;
#endif
        audiohw_mute(0);
    } else {
#if defined(IRIVER_H10) || defined(IRIVER_H10_5GB)
        /* We need to disable bit 4 of GPIOL to disable sound on H10 */
        GPIOL_OUTPUT_VAL &= ~0x10;
#endif
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

    wmcodec_write(LOUTVOL, vol_l);
    wmcodec_write(ROUTVOL, vol_r);
 
    return 0;
}

int audiohw_set_mixer_vol(int channel1, int channel2)
{
    (void)channel1;
    (void)channel2;

    return 0;
}

void audiohw_set_bass(int value)
{
    (void)value;
}

void audiohw_set_treble(int value)
{
    (void)value;
}

/* Nice shutdown of WM8731 codec */
void audiohw_close(void)
{
   /* set DACMU=1 DEEMPH=0 */
    wmcodec_write(DACCTRL, 0x8);

    /* ACTIVE=0 */
    codec_set_active(0x0);

    /* line in mute left & right*/
    wmcodec_write(LINVOL, 0x100 | 0x80);

    /* set DACSEL=0, MUTEMIC=1 */
    wmcodec_write(0x4, 0x2);

    /* set POWEROFF=0 OUTPD=0 DACPD=1 */
    wmcodec_write(PWRMGMT, 0x6f);

    /* set POWEROFF=1 OUTPD=1 DACPD=1 */
    wmcodec_write(PWRMGMT, 0xff);
}

/* Change the order of the noise shaper, 5th order is recommended above 32kHz */
void audiohw_set_nsorder(int order)
{
    (void)order;
}

void audiohw_set_sample_rate(int sampling_control)
{
    codec_set_active(0x0);
    wmcodec_write(SAMPCTRL, sampling_control);
    codec_set_active(0x1);
}

void audiohw_enable_recording(bool source_mic)
{
    (void)source_mic;
}

void audiohw_disable_recording(void)
{

}

void audiohw_set_recvol(int left, int right, int type)
{
    (void)left;
    (void)right;
    (void)type;
}

void audiohw_set_monitor(int enable)
{
    (void)enable;
}
