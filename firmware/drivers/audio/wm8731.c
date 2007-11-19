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
#include "config.h"
#include "logf.h"
#include "system.h"
#include "string.h"
#include "audio.h"

#include "wmcodec.h"
#include "audiohw.h"
#include "i2s.h"

const struct sound_settings_info audiohw_settings[] = {
    [SOUND_VOLUME]        = {"dB", 0,  1, -74,   6, -25},
    /* HAVE_SW_TONE_CONTROLS */
    [SOUND_BASS]          = {"dB", 0,  1, -24,  24,   0},
    [SOUND_TREBLE]        = {"dB", 0,  1, -24,  24,   0},
    [SOUND_BALANCE]       = {"%",  0,  1,-100, 100,   0},
    [SOUND_CHANNELS]      = {"",   0,  1,   0,   5,   0},
    [SOUND_STEREO_WIDTH]  = {"%",  0,  5,   0, 250, 100},
#ifdef HAVE_RECORDING
    [SOUND_LEFT_GAIN]     = {"dB", 1,  1,   0,  31,  23},
    [SOUND_RIGHT_GAIN]    = {"dB", 1,  1,   0,  31,  23},
    [SOUND_MIC_GAIN]      = {"dB", 1,  1,   0,   1,   0},
#endif
};

/* Init values/shadows
 * Ignore bit 8 since that only specifies "both" for updating
 * gains */
static unsigned char wm8731_regs[7] =
{
    [LINVOL]     = LINVOL_LINMUTE,
    [RINVOL]     = RINVOL_RINMUTE,
    [LOUTVOL]    = ROUTVOL_RZCEN,
    [ROUTVOL]    = ROUTVOL_RZCEN,
    [AAPCTRL]    = AAPCTRL_MUTEMIC | AAPCTRL_DACSEL,
    [DAPCTRL]    = DAPCTRL_DACMU | DAPCTRL_DEEMP_44KHz | DAPCTRL_ADCHPD,
    [PDCTRL]     = PDCTRL_LINEINPD | PDCTRL_MICPD | PDCTRL_ADCPD |
                   PDCTRL_OUTPD | PDCTRL_OSCPD | PDCTRL_CLKOUTPD,
};

static void wm8731_write(int reg, unsigned val)
{
    wm8731_regs[reg] = (unsigned char)val;
    wmcodec_write(reg, val);
}

static void wm8731_write_and(int reg, unsigned bits)
{
    wm8731_write(reg, wm8731_regs[reg] & bits);
}

static void wm8731_write_or(int reg, unsigned bits)
{
    wm8731_write(reg, wm8731_regs[reg] | bits);
}

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

int sound_val2phys(int setting, int value)
{
    int result;

    switch(setting)
    {
    case SOUND_LEFT_GAIN:
    case SOUND_RIGHT_GAIN:
        result = (value - 23) * 15;
        break;
    case SOUND_MIC_GAIN:
        result = value * 200;
        break;
    default:
        result = value;
        break;
    }

    return result;
}

void audiohw_mute(bool mute)
{
    if (mute) {
        /* Set DACMU = 1 to soft-mute the audio DACs. */
        wm8731_write_or(DAPCTRL, DAPCTRL_DACMU);
    } else {
        /* Set DACMU = 0 to soft-un-mute the audio DACs. */
        wm8731_write_and(DAPCTRL, ~DAPCTRL_DACMU);
    }
}

static void codec_set_active(int active)
{
    /* set active to 0x0 or 0x1 */
    wmcodec_write(ACTIVECTRL, active ? ACTIVECTRL_ACTIVE : 0);
}

void audiohw_preinit(void)
{
    i2s_reset();

    /* POWER UP SEQUENCE */
    /* 1) Switch on power supplies. By default the WM8731 is in Standby Mode,
     *    the DAC is digitally muted and the Audio Interface and Outputs are
     *    all OFF. */
    wmcodec_write(RESET, RESET_RESET);

    /* 2) Set all required bits in the Power Down register (0Ch) to '0';
     *    EXCEPT the OUTPD bit, this should be set to '1' (Default). */
    wm8731_write(PDCTRL, wm8731_regs[PDCTRL]);

    /* 3) Set required values in all other registers except 12h (Active). */
    wmcodec_write(AINTFCE, AINTFCE_FORMAT_I2S | AINTFCE_IWL_16BIT |
                           AINTFCE_MS);
    wm8731_write(AAPCTRL, wm8731_regs[AAPCTRL]);
    wm8731_write(DAPCTRL, wm8731_regs[DAPCTRL]);
    wmcodec_write(SAMPCTRL, WM8731_USB24_44100HZ);

    /* 5) The last write of the sequence should be setting OUTPD to '0'
     *    (active) in register 0Ch, enabling the DAC signal path, free
     *     of any significant power-up noise. */
    wm8731_write_and(PDCTRL, ~PDCTRL_OUTPD);
}

void audiohw_postinit(void)
{
    sleep(HZ);

    /* 4) Set the 'Active' bit in register 12h. */
    codec_set_active(true);

    audiohw_mute(false);

#if defined(IRIVER_H10) || defined(IRIVER_H10_5GB)
    /* We need to enable bit 4 of GPIOL for output for sound on H10 */
    GPIOL_OUTPUT_VAL |= 0x10;
#endif
}

int audiohw_set_master_vol(int vol_l, int vol_r)
{
    /* +6 to -73dB 1dB steps (plus mute == 80levels) 7bits */
    /* 1111111 == +6dB */
    /* 1111001 == 0dB */
    /* 0110000 == -73dB */
    /* 0101111 == mute (0x2f) */
    wm8731_write(LOUTVOL, LOUTVOL_LZCEN | (vol_l & LOUTVOL_LHPVOL_MASK));
    wm8731_write(ROUTVOL, ROUTVOL_RZCEN | (vol_r & ROUTVOL_RHPVOL_MASK));
    return 0;
}

/* Nice shutdown of WM8731 codec */
void audiohw_close(void)
{
    /* POWER DOWN SEQUENCE */
    /* 1) Set the OUTPD bit to '1' (power down). */
    wm8731_write_or(PDCTRL, PDCTRL_OUTPD);
    /* 2) Remove the WM8731 supplies. */
}

void audiohw_set_nsorder(int order)
{
    static const unsigned char deemp[4] =
    {
        DAPCTRL_DEEMP_DISABLE,
        DAPCTRL_DEEMP_32KHz,
        DAPCTRL_DEEMP_44KHz,
        DAPCTRL_DEEMP_48KHz
    };

    if ((unsigned)order >= ARRAYLEN(deemp))
        order = 0;

    wm8731_write(DAPCTRL,
        (wm8731_regs[DAPCTRL] & ~DAPCTRL_DEEMP_MASK) | deemp[order]);
}

void audiohw_set_sample_rate(int sampling_control)
{
    codec_set_active(false);
    wmcodec_write(SAMPCTRL, sampling_control);
    codec_set_active(true);
}

void audiohw_enable_recording(bool source_mic)
{
    codec_set_active(false);
    
    wm8731_regs[PDCTRL] &= ~PDCTRL_ADCPD;
    /* NOTE: When switching to digital monitoring we will not want
     * the DAC disabled. */
    wm8731_regs[PDCTRL] |= PDCTRL_DACPD;
    wm8731_regs[AAPCTRL] &= ~AAPCTRL_DACSEL;
    
    if (source_mic) {
        wm8731_write_or(LINVOL, LINVOL_LINMUTE);
        wm8731_write_or(RINVOL, RINVOL_RINMUTE);
        wm8731_regs[PDCTRL] &= ~PDCTRL_MICPD;
        wm8731_regs[PDCTRL] |= PDCTRL_LINEINPD;
        wm8731_regs[AAPCTRL] |= AAPCTRL_INSEL | AAPCTRL_SIDETONE;
        wm8731_regs[AAPCTRL] &= ~(AAPCTRL_MUTEMIC | AAPCTRL_BYPASS);
    } else {
        wm8731_regs[PDCTRL] |= PDCTRL_MICPD;
        wm8731_regs[PDCTRL] &= ~PDCTRL_LINEINPD;
        wm8731_regs[AAPCTRL] |= AAPCTRL_MUTEMIC | AAPCTRL_BYPASS;
        wm8731_regs[AAPCTRL] &= ~(AAPCTRL_INSEL | AAPCTRL_SIDETONE);
    }

    wm8731_write(PDCTRL, wm8731_regs[PDCTRL]);
    wm8731_write(AAPCTRL, wm8731_regs[AAPCTRL]);

    if (!source_mic) {
        wm8731_write_and(LINVOL, ~LINVOL_LINMUTE);
        wm8731_write_and(RINVOL, ~RINVOL_RINMUTE);
    }
    
    codec_set_active(true);
}

void audiohw_disable_recording(void)
{
    codec_set_active(false);

    /* Mute inputs */
    wm8731_write_or(LINVOL, LINVOL_LINMUTE);
    wm8731_write_or(RINVOL, RINVOL_RINMUTE);
    wm8731_write_or(AAPCTRL, AAPCTRL_MUTEMIC);

    /* Turn off input analog audio paths */
    wm8731_regs[AAPCTRL] &= ~(AAPCTRL_BYPASS | AAPCTRL_SIDETONE);
    wm8731_write(AAPCTRL, wm8731_regs[AAPCTRL]);

    /* Set power config */
    wm8731_regs[PDCTRL] &= ~PDCTRL_DACPD;
    wm8731_regs[PDCTRL] |= PDCTRL_MICPD | PDCTRL_LINEINPD |
                           PDCTRL_ADCPD;
    wm8731_write(PDCTRL, wm8731_regs[PDCTRL]);

    /* Select DAC */
    wm8731_write_or(AAPCTRL, AAPCTRL_DACSEL);

    codec_set_active(true);
}

void audiohw_set_recvol(int left, int right, int type)
{
    switch (type)
    {
    case AUDIO_GAIN_MIC:
        if (left > 0) {
            wm8731_write_or(AAPCTRL, AAPCTRL_MIC_BOOST);
        }
        else {
            wm8731_write_and(AAPCTRL, ~AAPCTRL_MIC_BOOST);
        }
        break;
    case AUDIO_GAIN_LINEIN:
        wm8731_regs[LINVOL] &= ~LINVOL_MASK;
        wm8731_write(LINVOL, wm8731_regs[LINVOL] | (left & LINVOL_MASK));
        wm8731_regs[RINVOL] &= ~RINVOL_MASK;
        wm8731_write(RINVOL, wm8731_regs[RINVOL] | (right & RINVOL_MASK));
        break;
    default:
        return;
    }
}

void audiohw_set_monitor(bool enable)
{
    if(enable)
    {
        wm8731_write_and(PDCTRL, ~PDCTRL_LINEINPD);
        wm8731_write_or(AAPCTRL, AAPCTRL_BYPASS);
    }
    else {
        wm8731_write_and(AAPCTRL, ~AAPCTRL_BYPASS);
        wm8731_write_or(PDCTRL, PDCTRL_LINEINPD);
    }
}
