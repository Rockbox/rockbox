/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Driver for TSC2100 audio codec
 *
 * Copyright (c) 2008 Jonathan Gordon
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include "cpu.h"
#include "debug.h"
#include "system.h"
#include "audio.h"

#include "audiohw.h"
#include "tsc2100.h"

const struct sound_settings_info audiohw_settings[] = {
    [SOUND_VOLUME]        = {"dB",   0,   1, -63,   0, -25},
#if 0
    /* HAVE_SW_TONE_CONTROLS */
    [SOUND_BASS]          = {"dB",   0,   1, -24,  24,   0},
    [SOUND_TREBLE]        = {"dB",   0,   1, -24,  24,   0},
    [SOUND_BALANCE]       = {"%",    0,   1,-100, 100,   0},
    [SOUND_CHANNELS]      = {"",     0,   1,   0,   5,   0},
    [SOUND_STEREO_WIDTH]  = {"%",    0,   5,   0, 250, 100},
    [SOUND_MIC_GAIN]      = {"dB",   1,   1,   0,  39,  23},
    [SOUND_LEFT_GAIN]     = {"dB",   1,   1,   0,  31,  23},
    [SOUND_RIGHT_GAIN]    = {"dB",   1,   1,   0,  31,  23},
#endif
};
static bool is_muted = false;
/* convert tenth of dB volume to master volume register value */
int tenthdb2master(int db)
{
    /* 0 to -63.0dB in 1dB steps, tsc2100 can goto -63.5 in 0.5dB steps */
    if (db < VOLUME_MIN) {
        return 0x0;
    } else if (db >= VOLUME_MAX) {
        return 0x1f;
    } else {
        return((db-VOLUME_MIN)/10); /* VOLUME_MIN is negative */
    }
}

int sound_val2phys(int setting, int value)
{
    int result;

    switch(setting)
    {
#if 0
    case SOUND_LEFT_GAIN:
    case SOUND_RIGHT_GAIN:
    case SOUND_MIC_GAIN:
        result = (value - 23) * 15;
        break;
#endif
    default:
        result = value;
        break;
    }

    return result;
}

void audiohw_init(void)
{
    short val = tsc2100_readreg(TSAC4_PAGE, TSAC4_ADDRESS);
    /* disable DAC PGA soft-stepping */
    val |= TSAC4_DASTDP;
    
    tsc2100_writereg(TSAC4_PAGE, TSAC4_ADDRESS, val);
}

void audiohw_postinit(void)
{
}

/* Silently enable / disable audio output */
void audiohw_enable_output(bool enable)
{
    if (enable) {
        audiohw_mute(0);
    } else {
        audiohw_mute(1);
    }
}

void audiohw_set_master_vol(int vol_l, int vol_r)
{
   short vol = (vol_l<<14)|(vol_r);
   if (is_muted)
       vol |= (1<<15)|(1<<7);
   tsc2100_writereg(TSDACGAIN_PAGE, TSDACGAIN_ADDRESS, vol);
}

void audiohw_set_lineout_vol(int vol_l, int vol_r)
{
    audiohw_set_master_vol(vol_l, vol_r);
}

void audiohw_mute(bool mute)
{
    short vol = tsc2100_readreg(TSDACGAIN_PAGE, TSDACGAIN_ADDRESS);
    /* left  mute bit == 1<<15
       right mute bit == 1<<7
     */
    if (mute) 
    {
        vol |= (1<<15)|(1<<7);
    } else 
    {
        vol &= ~((1<<15)|(1<<7));
    }
    is_muted = mute;
    tsc2100_writereg(TSDACGAIN_PAGE, TSDACGAIN_ADDRESS, vol);
}

void audiohw_close(void)
{
    /* mute headphones */
    audiohw_mute(true);

}

void audiohw_set_sample_rate(int sampling_control)
{
    (void)sampling_control;
}
