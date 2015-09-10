/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (c) 2009 Mark Arigo
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include "config.h"
#include "system.h"
#include "string.h"

/*#define LOGF_ENABLE*/
#include "logf.h"

#include "pcm_sampr.h"
#include "audio.h"
#include "akcodec.h"
#include "audiohw.h"
#include "sound.h"

static unsigned char akc_regs[AKC_NUM_REGS];

static void akc_write(int reg, unsigned val)
{
    if ((unsigned)reg >= AKC_NUM_REGS)
        return;

    akc_regs[reg] = (unsigned char)val;
    akcodec_write(reg, val);
}

static void akc_set(int reg, unsigned bits)
{
    akc_write(reg, akc_regs[reg] | bits);
}

static void akc_clear(int reg, unsigned bits)
{
    akc_write(reg, akc_regs[reg] & ~bits);
}

static void akc_write_masked(int reg, unsigned bits, unsigned mask)
{
    akc_write(reg, (akc_regs[reg] & ~mask) | (bits & mask));
}

#if 0
static void codec_set_active(int active)
{
    (void)active;
}
#endif

/* convert tenth of dB volume (-1270..0) to master volume register value */
static int vol_tenthdb2hw(int db)
{
    if (db <= -1280)
        return 0xff; /* mute */
    else if (db >= 0)
        return 0x00;
    else
        return ((-db)/5);
}

/*static void audiohw_mute(bool mute)
{
    if (mute)
    {
        akc_set(AK4537_DAC, SMUTE);
        udelay(200000);
    }
    else
    {
        udelay(200000);
        akc_clear(AK4537_DAC, SMUTE);
    }
}*/

void audiohw_preinit(void)
{
    int i;
    for (i = 0; i < AKC_NUM_REGS; i++)
        akc_regs[i] = akcodec_read(i);

    /* POWER UP SEQUENCE (from the datasheet) */
    /* Note: the delay length is what the OF uses, although the datasheet
       suggests they can be shorter */

    /* power up VCOM */
    akc_set(AK4537_PM1, PMVCM);
    udelay(100000);

    /* setup AK4537_SIGSEL1 */
    akc_set(AK4537_SIGSEL1, ALCS | MOUT2);        
    udelay(100000);

    /* setup AK4537_SIGSEL2 */
    akc_write_masked(AK4537_SIGSEL2, DAHS, (DAHS | HPL | HPR));
    udelay(100000);

    /* setup AK4537_MODE1 */
    akc_write_masked(AK4537_MODE1, DIF_I2S | BICK_32FS | MCKI_PLL_12000KHZ,
                     (DIF_MASK | BICK_MASK | MCKI_MASK));
    udelay(100000);

    /* CLOCK SETUP - X'tal used in PLL mode (master mode) */

    /* release the pull-down of the XTI pin and power-up the X'tal osc */
    akc_write_masked(AK4537_PM2, PMXTL, (MCLKPD | PMXTL));
    udelay(100000);

    /* power-up the PLL */
    akc_set(AK4537_PM2, PMPLL);
    udelay(100000);

    /* enable MCKO output and setup MCKO output freq */
    akc_set(AK4537_MODE1, MCKO_EN);
    udelay(100000);

    /* ENABLE HEADPHONE AMP OUTPUT */

    /* setup the sampling freq if PLL mode is used */
    akc_write_masked(AK4537_MODE2, AKC_PLL_44100HZ, FS_MASK);

    /* setup the low freq boost level */
    akc_write_masked(AK4537_DAC, BST_OFF, BST_MASK);

    /* setup the digital volume */
    akc_write(AK4537_ATTL, 0x10);
    akc_write(AK4537_ATTR, 0x10);

    /* power up the DAC */
    akc_set(AK4537_PM2, PMDAC);
    udelay(100000);

    /* power up the headphone amp */
    akc_clear(AK4537_SIGSEL2, HPL | HPR);
    udelay(100000);

    /* power up the common voltage of headphone amp */
    akc_set(AK4537_PM2, PMHPL | PMHPR);
    udelay(100000);
}

void audiohw_postinit(void)
{
    /* nothing */
}

void audiohw_close(void)
{
    /* POWER DOWN SEQUENCE (from the datasheet) */

    /* mute */
    akc_write(AK4537_ATTL, 0xff);
    akc_write(AK4537_ATTR, 0xff);
    akc_set(AK4537_DAC, SMUTE);
    udelay(100000);

    /* power down the common voltage of headphone amp */
    akc_clear(AK4537_PM2, PMHPL | PMHPR);

    /* power down the DAC */
    akc_clear(AK4537_PM2, PMDAC);

    /* Let the common voltage fall down before powering down headphone amp,
       or a pop noise will occur. The fall time depends on the capacitor value
       connected with the MUTET pin and is 100k*C up to 250k*C.
       For Samsung YH devices (4.7uF) a minimum time of 470ms is needed. */
    udelay(800000);

    /* power down the headphone amp */
    akc_set(AK4537_SIGSEL2, HPL | HPR);

    /* disable MCKO */
    akc_clear(AK4537_MODE1, MCKO_EN);

    /* power down X'tal and PLL, pull down the XTI pin */
    akc_write_masked(AK4537_PM2, MCLKPD, (MCLKPD | PMXTL | PMPLL));

    /* power down VCOM */
    akc_clear(AK4537_PM1, PMVCM);

    akcodec_close(); /* target-specific */
}

void audiohw_set_volume(int vol_l, int vol_r)
{
    vol_l = vol_tenthdb2hw(vol_l);
    vol_r = vol_tenthdb2hw(vol_r);
    akc_write(AK4537_ATTL, vol_l & 0xff);
    akc_write(AK4537_ATTR, vol_r & 0xff);
}

void audiohw_set_frequency(int fsel)
{
    static const unsigned char srctrl_table[HW_NUM_FREQ] =
    {
        HW_HAVE_8_([HW_FREQ_8]   = AKC_PLL_8000HZ, )
        HW_HAVE_11_([HW_FREQ_11] = AKC_PLL_11025HZ,)
        HW_HAVE_16_([HW_FREQ_16] = AKC_PLL_16000HZ,)
        HW_HAVE_22_([HW_FREQ_22] = AKC_PLL_22050HZ,)
        HW_HAVE_24_([HW_FREQ_24] = AKC_PLL_24000HZ,)
        HW_HAVE_32_([HW_FREQ_32] = AKC_PLL_32000HZ,)
        HW_HAVE_44_([HW_FREQ_44] = AKC_PLL_44100HZ,)
        HW_HAVE_48_([HW_FREQ_48] = AKC_PLL_48000HZ,)
    };

    if ((unsigned)fsel >= HW_NUM_FREQ)
        fsel = HW_FREQ_DEFAULT;

    akc_write_masked(AK4537_MODE2, srctrl_table[fsel], FS_MASK);
}

#if defined(HAVE_RECORDING)

void akc_disable_mic(void)
{
        /* disable mic power supply */
#if defined(SAMSUNG_YH920) || defined(SAMSUNG_YH925)
        akc_clear(AK4537_MIC, MPWRE);
#else
        akc_clear(AK4537_MIC, MPWRI);
#endif
        /* power down mic preamp */
        akc_clear(AK4537_PM1, PMMICL);
        akc_clear(AK4537_PM3, PMMICR);
}

void akc_enable_mic(void)
{
        /* enable mic power supply */
#if defined(SAMSUNG_YH920) || defined(SAMSUNG_YH925)
        /* additionally select external mic */
        akc_set(AK4537_MIC, MPWRE | MSEL);
#else
        akc_set(AK4537_MIC, MPWRI);
#endif
        /* power up mic preamp */
        akc_set(AK4537_PM1, PMMICL);
}

void akc_setup_input(unsigned linesel, unsigned gcontrol)
{
        /* select line1 or line2 input */
        akc_write_masked(AK4537_PM3, linesel, INL | INR);
        /* route ALC output to ADC input */
        akc_set(AK4537_MIC, MICAD);
        /* set ALC (automatic level control) to manual mode */
        akc_clear(AK4537_ALC1, ALC1);
        /* set gain control to dependent or independent left & right */
        akc_write_masked(AK4537_MIC, gcontrol, IPGAC);
        /* power up left channel ADC and line in */
        akc_set(AK4537_PM1, PMADL | PMIPGL);
        /* power up right channel ADC and line in */
        akc_set(AK4537_PM3, PMADR | PMIPGR);
        /* ADC -> DAC, external data to DAC ignored */
        akc_set(AK4537_MODE2, LOOP);
}

void audiohw_set_recsrc(int source)
{
    switch(source)
    {
    case AUDIO_SRC_PLAYBACK:

        /* disable microphone */
        akc_disable_mic();

        /* power down ADC and line amp */
        akc_clear(AK4537_PM1, PMADL | PMIPGL);
        akc_clear(AK4537_PM3, PMADR | PMIPGR);

        /* break ADC -> DAC connection */
        akc_clear(AK4537_MODE2, LOOP);

        break;

#if (INPUT_SRC_CAPS & SRC_CAP_FMRADIO)
    case AUDIO_SRC_FMRADIO:

        /* disable microphone */
        akc_disable_mic();

        /* Select line2 input, set gain control to independent left & right gain */
        akc_setup_input(0xff, 0xff);

        /* set line in vol = 0 dB */
        akc_write(AK4537_IPGAL, 0x2f);
        akc_write(AK4537_IPGAR, 0x2f);

        break;
#endif /* INPUT_SRC_CAPS & SRC_CAP_FMRADIO */

#if (INPUT_SRC_CAPS & SRC_CAP_LINEIN)
    case AUDIO_SRC_LINEIN:

        /* disable microphone */
        akc_disable_mic();

        /* Select line1 input, set gain control to independent left & right gain */
        akc_setup_input(0x00, 0xff);

        break;
#endif /* INPUT_SRC_CAPS & SRC_CAP_LINEIN */

#if (INPUT_SRC_CAPS & SRC_CAP_MIC)
    case AUDIO_SRC_MIC:

        /* enable micropohone */
        akc_enable_mic();

        /* Select line1 input (mic connected), set gain control to 'dependent' */
        /* (left & right at the same time) */
        akc_setup_input(0x00, 0x00);

        break;
#endif /* INPUT_SRC_CAPS & SRC_CAP_MIC) */

    } /* switch(source) */
}

void audiohw_set_recvol(int left, int right, int type)
{
    switch (type)
    {
    case AUDIO_GAIN_MIC:
        /* the mic preamp has a fixed gain of +15 dB. There's an additional
         * activatable +20dB mic gain stage. The signal is then routed to
         * the Line1 input, where you find the line attenuator with a range
         * from -23.5 to +12dB, so we have a total gain range of -8.0 .. +47dB.
         * NOTE: the datasheet state's different attenuator levels for mic and
         * line input, but that's not precise. The +15dB difference result only
         * from the mic stage.
         * NOTE2: the mic is connected to the line1 input (via mic preamp),
         * so if a line signal is present, you will always record a mixup.
         */
        /* If gain is > 20 dB we use the additional gain stage */
        if (left > 20) {
            akc_set(AK4537_MIC, MGAIN);
            left -= 20;
        }
        else {
            akc_clear(AK4537_MIC, MGAIN);
        }
        /* the remains is done by the line input amp */
        left = (left+8)*2;
        akc_write(AK4537_IPGAL, left);
        break;
    case AUDIO_GAIN_LINEIN:
        /* convert dB to register value */
        left = (left+23)*2+1;
        right = (right+23)*2+1;
        akc_write(AK4537_IPGAL, left);
        akc_write(AK4537_IPGAR, right);
        break;
    default:
        return;
    }
}
#endif /* HAVE_RECORDING */
