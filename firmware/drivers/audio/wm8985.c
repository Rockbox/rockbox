/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Stubs for WM8985 audio codec, (unwisely?) based on 8975 driver.
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include "logf.h"
#include "system.h"
#include "string.h"
#include "audio.h"

#include "wmcodec.h"
#include "audiohw.h"
#include "i2s.h"

/* TODO: fix these values, they're copied straight from WM8975 */
const struct sound_settings_info audiohw_settings[] = {
    [SOUND_VOLUME]        = {"dB", 0,  1, -74,   6, -25},
    [SOUND_BASS]          = {"dB", 0,  1,  -6,   9,   0},
    [SOUND_TREBLE]        = {"dB", 0,  1,  -6,   9,   0},
    [SOUND_BALANCE]       = {"%",  0,  1,-100, 100,   0},
    [SOUND_CHANNELS]      = {"",   0,  1,   0,   5,   0},
    [SOUND_STEREO_WIDTH]  = {"%",  0,  5,   0, 250, 100},
    [SOUND_LEFT_GAIN]     = {"dB", 1,  1,-128,  96,   0},
    [SOUND_RIGHT_GAIN]    = {"dB", 1,  1,-128,  96,   0},
    [SOUND_MIC_GAIN]      = {"dB", 1,  1,-128, 108,  16},
};

/* convert tenth of dB volume to master volume register value */
int tenthdb2master(int db)
{
    #warning function not implemented
  
    (void)db;
    return 0;
}

/* Silently enable / disable audio output */
void audiohw_enable_output(bool enable)
{
    #warning function not implemented
  
    (void)enable;
}

int audiohw_set_master_vol(int vol_l, int vol_r)
{
    #warning function not implemented

    (void)vol_l;
    (void)vol_r;
    return 0;
}

int audiohw_set_lineout_vol(int vol_l, int vol_r)
{
     #warning function not implemented

    (void)vol_l;
    (void)vol_r;
     return 0;
}

void audiohw_set_bass(int value)
{
    #warning function not implemented

    (void)value;
}

void audiohw_set_treble(int value)
{
    #warning function not implemented

    (void)value;
}

void audiohw_mute(bool mute)
{
    #warning function not implemented

    (void)mute;
}

void audiohw_close(void)
{
    #warning function not implemented
}

void audiohw_set_nsorder(int order)
{
    #warning function not implemented

    (void)order;
}

/* Note: Disable output before calling this function */
void audiohw_set_sample_rate(int sampling_control)
{
    #warning function not implemented

    (void)sampling_control;
}

void audiohw_enable_recording(bool source_mic)
{
    #warning function not implemented

    (void)source_mic;
}
 
void audiohw_disable_recording(void)
{
    #warning function not implemented
}

void audiohw_set_recvol(int left, int right, int type)
{
    #warning function not implemented

    (void)left;
    (void)right;
    (void)type;
}

void audiohw_set_monitor(bool enable)
{
    #warning function not implemented

    (void)enable;
}
