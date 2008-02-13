/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Driver for WM8721 audio codec
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
#include "config.h"
#include "logf.h"
#include "system.h"
#include "string.h"
#include "audio.h"

#include "wmcodec.h"
#include "audiohw.h"
#include "i2s.h"

#define IPOD_PCM_LEVEL 0x65       /* -6dB */

/* use zero crossing to reduce clicks during volume changes */
#define VOLUME_ZC_WAIT (1<<7)

const struct sound_settings_info audiohw_settings[] = {
    [SOUND_VOLUME]        = {"dB", 0,  1, -74,   6, -25},
    /* HAVE_SW_TONE_CONTROLS */
    [SOUND_BASS]          = {"dB", 0,  1, -24,  24,   0},
    [SOUND_TREBLE]        = {"dB", 0,  1, -24,  24,   0},
    [SOUND_BALANCE]       = {"%",  0,  1,-100, 100,   0},
    [SOUND_CHANNELS]      = {"",   0,  1,   0,   5,   0},
    [SOUND_STEREO_WIDTH]  = {"%",  0,  5,   0, 250, 100},
};

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

void audiohw_mute(bool mute)
{
    if (mute)
    {
        /* Set DACMU = 1 to soft-mute the audio DACs. */
        wmcodec_write(DAPCTRL, 0x8);
    } else {
        /* Set DACMU = 0 to soft-un-mute the audio DACs. */
        wmcodec_write(DAPCTRL, 0x0);
    }
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

        /* DACSEL=1 */
        wmcodec_write(0x4, 0x10);
    
        /* set power register to POWEROFF=0 on OUTPD=0, DACPD=0 */
        wmcodec_write(PDCTRL, 0x67);

        /* BCLKINV=0(Dont invert BCLK) MS=1(Enable Master) LRSWAP=0 LRP=0 */
        /* IWL=00(16 bit) FORMAT=10(I2S format) */
        wmcodec_write(AINTFCE, 0x42);

        audiohw_set_sample_rate(WM8721_USB24_44100HZ);

        /* set the volume to -6dB */
        wmcodec_write(LOUTVOL, IPOD_PCM_LEVEL);
        wmcodec_write(ROUTVOL, 0x100 | IPOD_PCM_LEVEL);

        /* ACTIVE=1 */
        codec_set_active(1);

        /* 5. Set DACMU = 0 to soft-un-mute the audio DACs. */
        wmcodec_write(DAPCTRL, 0x0);
        
        audiohw_mute(0);
    } else {
        audiohw_mute(1);
    }
}

void audiohw_set_master_vol(int vol_l, int vol_r)
{
    /* +6 to -73dB 1dB steps (plus mute == 80levels) 7bits */
    /* 1111111 == +6dB */
    /* 1111001 == 0dB */
    /* 0110000 == -73dB */
    /* 0101111 == mute (0x2f) */
    wmcodec_write(LOUTVOL, VOLUME_ZC_WAIT | vol_l);
    wmcodec_write(ROUTVOL, VOLUME_ZC_WAIT | vol_r);
}

/* Nice shutdown of WM8721 codec */
void audiohw_close(void)
{
   /* set DACMU=1 DEEMPH=0 */
    wmcodec_write(DAPCTRL, 0x8);

    /* ACTIVE=0 */
    codec_set_active(0x0);

    /* set DACSEL=0, MUTEMIC=1 */
    wmcodec_write(0x4, 0x2);

    /* set POWEROFF=0 OUTPD=0 DACPD=1 */
    wmcodec_write(PDCTRL, 0x6f);

    /* set POWEROFF=1 OUTPD=1 DACPD=1 */
    wmcodec_write(PDCTRL, 0xff);
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
