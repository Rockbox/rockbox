/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2008 by Michael Sevakis
 *
 * Driver for WM8978 audio codec 
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
#include "audiohw.h"
#include "wmcodec.h"
#include "audio.h"
//#define LOGF_ENABLE
#include "logf.h"

/* TODO: Define/refine an API for special hardware steps outside the
 * main codec driver such as special GPIO handling. */
/* NOTE: Much of the volume code is very interdependent and calibrated for
 * the Gigabeat S. If you change anything for another device that uses this
 * file it may break things. */
extern void audiohw_enable_headphone_jack(bool enable);

const struct sound_settings_info audiohw_settings[] =
{
    [SOUND_VOLUME]        = {"dB", 0,  1, -90,   6, -25},
    [SOUND_BASS]          = {"dB", 0,  1, -12,  12,   0},
    [SOUND_TREBLE]        = {"dB", 0,  1, -12,  12,   0},
    [SOUND_BALANCE]       = {"%",  0,  1,-100, 100,   0},
    [SOUND_CHANNELS]      = {"",   0,  1,   0,   5,   0},
    [SOUND_STEREO_WIDTH]  = {"%",  0,  5,   0, 250, 100},
#ifdef HAVE_RECORDING
    /* Digital: -119.0dB to +8.0dB in 0.5dB increments
     * Analog:  Relegated to volume control
     * Circumstances unfortunately do not allow a great deal of positive
     * gain. */
    [SOUND_LEFT_GAIN]     = {"dB", 1,  1,-238,  16,   0},
    [SOUND_RIGHT_GAIN]    = {"dB", 1,  1,-238,  16,   0},
#if 0
    [SOUND_MIC_GAIN]      = {"dB", 1,  1,-238,  16,   0},
#endif
#endif
#if 0
    [SOUND_BASS_CUTOFF]   = {"",   0,  1,   1,   4,   1},
    [SOUND_TREBLE_CUTOFF] = {"",   0,  1,   1,   4,   1},
#endif
};

static uint16_t wmc_regs[WMC_NUM_REGISTERS] =
{
    /* Initialized with post-reset default values - the 2-wire interface
     * cannot be read. Or-in additional bits desired for some registers. */
    [0 ... WMC_NUM_REGISTERS-1]   = 0x8000, /* To ID invalids in gaps */
    [WMC_SOFTWARE_RESET]          = 0x000,
    [WMC_POWER_MANAGEMENT1]       = 0x000,
    [WMC_POWER_MANAGEMENT2]       = 0x000,
    [WMC_POWER_MANAGEMENT3]       = 0x000,
    [WMC_AUDIO_INTERFACE]         = 0x050,
    [WMC_COMPANDING_CTRL]         = 0x000,
    [WMC_CLOCK_GEN_CTRL]          = 0x140,
    [WMC_ADDITIONAL_CTRL]         = 0x000,
    [WMC_GPIO]                    = 0x000,
    [WMC_JACK_DETECT_CONTROL1]    = 0x000,
    [WMC_DAC_CONTROL]             = 0x000,
    [WMC_LEFT_DAC_DIGITAL_VOL]    = 0x0ff | WMC_VU,
    [WMC_RIGHT_DAC_DIGITAL_VOL]   = 0x0ff | WMC_VU,
    [WMC_JACK_DETECT_CONTROL2]    = 0x000,
    [WMC_ADC_CONTROL]             = 0x100,
    [WMC_LEFT_ADC_DIGITAL_VOL]    = 0x0ff | WMC_VU,
    [WMC_RIGHT_ADC_DIGITAL_VOL]   = 0x0ff | WMC_VU,
    [WMC_EQ1_LOW_SHELF]           = 0x12c,
    [WMC_EQ2_PEAK1]               = 0x02c,
    [WMC_EQ3_PEAK2]               = 0x02c,
    [WMC_EQ4_PEAK3]               = 0x02c,
    [WMC_EQ5_HIGH_SHELF]          = 0x02c,
    [WMC_DAC_LIMITER1]            = 0x032,
    [WMC_DAC_LIMITER2]            = 0x000,
    [WMC_NOTCH_FILTER1]           = 0x000,
    [WMC_NOTCH_FILTER2]           = 0x000,
    [WMC_NOTCH_FILTER3]           = 0x000,
    [WMC_NOTCH_FILTER4]           = 0x000,
    [WMC_ALC_CONTROL1]            = 0x038,
    [WMC_ALC_CONTROL2]            = 0x00b,
    [WMC_ALC_CONTROL3]            = 0x032,
    [WMC_NOISE_GATE]              = 0x000,
    [WMC_PLL_N]                   = 0x008,
    [WMC_PLL_K1]                  = 0x00c,
    [WMC_PLL_K2]                  = 0x093,
    [WMC_PLL_K3]                  = 0x0e9,
    [WMC_3D_CONTROL]              = 0x000,
    [WMC_BEEP_CONTROL]            = 0x000,
    [WMC_INPUT_CTRL]              = 0x033,
    [WMC_LEFT_INP_PGA_GAIN_CTRL]  = 0x010 | WMC_VU | WMC_ZC,
    [WMC_RIGHT_INP_PGA_GAIN_CTRL] = 0x010 | WMC_VU | WMC_ZC,
    [WMC_LEFT_ADC_BOOST_CTRL]     = 0x100,
    [WMC_RIGHT_ADC_BOOST_CTRL]    = 0x100,
    [WMC_OUTPUT_CTRL]             = 0x002,
    [WMC_LEFT_MIXER_CTRL]         = 0x001,
    [WMC_RIGHT_MIXER_CTRL]        = 0x001,
    [WMC_LOUT1_HP_VOLUME_CTRL]    = 0x039 | WMC_VU | WMC_ZC,
    [WMC_ROUT1_HP_VOLUME_CTRL]    = 0x039 | WMC_VU | WMC_ZC,
    [WMC_LOUT2_SPK_VOLUME_CTRL]   = 0x039 | WMC_VU | WMC_ZC,
    [WMC_ROUT2_SPK_VOLUME_CTRL]   = 0x039 | WMC_VU | WMC_ZC,
    [WMC_OUT3_MIXER_CTRL]         = 0x001,
    [WMC_OUT4_MONO_MIXER_CTRL]    = 0x001,
};

struct
{
    int vol_l;
    int vol_r;
    bool ahw_mute;
} wmc_vol =
{
    0, 0, false
};

static void wmc_write(unsigned int reg, unsigned int val)
{
    if (reg >= WMC_NUM_REGISTERS || (wmc_regs[reg] & 0x8000))
    {
        logf("wm8978 invalid register: %d", reg);
        return;
    }

    wmc_regs[reg] = val & ~0x8000;
    wmcodec_write(reg, val);
}

void wmc_set(unsigned int reg, unsigned int bits)
{
    wmc_write(reg, wmc_regs[reg] | bits);
}

void wmc_clear(unsigned int reg, unsigned int bits)
{
    wmc_write(reg, wmc_regs[reg] & ~bits);
}

static void wmc_write_masked(unsigned int reg, unsigned int bits,
                             unsigned int mask)
{
    wmc_write(reg, (wmc_regs[reg] & ~mask) | (bits & mask));
}

/* convert tenth of dB volume (-890..60) to master volume register value
 * (000000...111111) */
int tenthdb2master(int db)
{
    /* -90dB to +6dB 1dB steps (96 levels) 7bits */
    /* 1100000 ==  +6dB  (0x60,96)               */
    /* 1101010 ==   0dB  (0x5a,90)               */
    /* 1000001 == -57dB  (0x21,33,DAC)           */
    /* 0000001 == -89dB  (0x01,01)               */
    /* 0000000 == -90dB  (0x00,00,Mute)          */
    if (db <= VOLUME_MIN)
    {
        return 0x0;
    }
    else
    {
        return (db - VOLUME_MIN) / 10;
    }
}

int sound_val2phys(int setting, int value)
{
    int result;

    switch (setting)
    {
#ifdef HAVE_RECORDING
    case SOUND_LEFT_GAIN:
    case SOUND_RIGHT_GAIN:
    case SOUND_MIC_GAIN:
        result = value * 5;
        break;
#endif

    default:
        result = value;
    }

    return result;
}

void audiohw_preinit(void)
{
    /* 1. Turn on external power supplies. Wait for supply voltage to settle. */

    /* Step 1 should be completed already. Reset and return all registers to
     * defaults */
    wmcodec_write(WMC_SOFTWARE_RESET, 0xff);
    sleep(HZ/10);

    /* 2. Mute all analogue outputs */
    wmc_set(WMC_LOUT1_HP_VOLUME_CTRL, WMC_MUTE);
    wmc_set(WMC_ROUT1_HP_VOLUME_CTRL, WMC_MUTE);
    wmc_set(WMC_LOUT2_SPK_VOLUME_CTRL, WMC_MUTE);
    wmc_set(WMC_ROUT2_SPK_VOLUME_CTRL, WMC_MUTE);
    wmc_set(WMC_OUT3_MIXER_CTRL, WMC_MUTE);
    wmc_set(WMC_OUT4_MONO_MIXER_CTRL, WMC_MUTE);

    /* 3. Set L/RMIXEN = 1 and DACENL/R = 1 in register R3. */
    wmc_write(WMC_POWER_MANAGEMENT3,
              WMC_RMIXEN | WMC_LMIXEN | WMC_DACENR | WMC_DACENL);

    /* 4. Set BUFIOEN = 1 and VMIDSEL[1:0] to required value in register
     *    R1. Wait for VMID supply to settle */
    wmc_write(WMC_POWER_MANAGEMENT1, WMC_BUFIOEN | WMC_VMIDSEL_300K);
    sleep(HZ/10);

    /* 5. Set BIASEN = 1 in register R1. */
    wmc_set(WMC_POWER_MANAGEMENT1, WMC_BIASEN);
}

void audiohw_postinit(void)
{
    sleep(HZ);

    /* 6. Set L/ROUTEN = 1 in register R2. */
    wmc_write(WMC_POWER_MANAGEMENT2, WMC_LOUT1EN | WMC_ROUT1EN);

    /* 7. Enable other mixers as required */

    /* 8. Enable other outputs as required */

    /* 9. Set remaining registers */
    wmc_write(WMC_AUDIO_INTERFACE, WMC_WL_16 | WMC_FMT_I2S);
    wmc_write(WMC_DAC_CONTROL, WMC_DACOSR_128 | WMC_AMUTE);

    /* No ADC, no HP filter, no popping */
    wmc_clear(WMC_ADC_CONTROL, WMC_HPFEN);

    wmc_clear(WMC_LEFT_ADC_BOOST_CTRL, WMC_PGABOOSTL);
    wmc_clear(WMC_RIGHT_ADC_BOOST_CTRL, WMC_PGABOOSTR);

    /* Specific to HW clocking */
    wmc_write_masked(WMC_CLOCK_GEN_CTRL, WMC_BCLKDIV_4 | WMC_MS,
                     WMC_BCLKDIV | WMC_MS | WMC_CLKSEL);
    audiohw_set_frequency(HW_FREQ_DEFAULT);

    audiohw_enable_headphone_jack(true);
}

static void get_headphone_levels(int val, int *dac_p, int *hp_p,
                                 int *mix_p, int *boost_p)
{
    int dac, hp, mix, boost;

    if (val >= 33)
    {
        dac = 255;
        hp = val - 33;
        mix = 7;
        boost = 5;
    }
    else if (val >= 21)
    {
        dac = 189 + val / 3 * 6;
        hp = val % 3;
        mix = 7;
        boost = (val - 18) / 3;
    }
    else
    {
        dac = 189 + val / 3 * 6;
        hp = val % 3;
        mix = val / 3;
        boost = 1;
    }

    *dac_p = dac;
    *hp_p = hp;
    *mix_p = mix;
    *boost_p = boost;
}

void audiohw_set_headphone_vol(int vol_l, int vol_r)
{
    int prev_l = wmc_vol.vol_l;
    int prev_r = wmc_vol.vol_r;
    int dac_l, dac_r, hp_l, hp_r;
    int mix_l, mix_r, boost_l, boost_r;

    wmc_vol.vol_l = vol_l;
    wmc_vol.vol_r = vol_r;

    /* Mixers are synced to provide full volume range on both the analogue
     * and digital pathways */
    get_headphone_levels(vol_l, &dac_l, &hp_l, &mix_l, &boost_l);
    get_headphone_levels(vol_r, &dac_r, &hp_r, &mix_r, &boost_r);

    wmc_write_masked(WMC_LEFT_MIXER_CTRL, WMC_BYPLMIXVOLw(mix_l),
                     WMC_BYPLMIXVOL);
    wmc_write_masked(WMC_LEFT_ADC_BOOST_CTRL,
                     WMC_L2_2BOOSTVOLw(boost_l), WMC_L2_2BOOSTVOL);
    wmc_write_masked(WMC_LEFT_DAC_DIGITAL_VOL, dac_l, WMC_DVOL);
    wmc_write_masked(WMC_LOUT1_HP_VOLUME_CTRL, hp_l, WMC_AVOL);

    wmc_write_masked(WMC_RIGHT_MIXER_CTRL, WMC_BYPRMIXVOLw(mix_r),
                     WMC_BYPRMIXVOL);
    wmc_write_masked(WMC_RIGHT_ADC_BOOST_CTRL,
                     WMC_R2_2BOOSTVOLw(boost_r), WMC_R2_2BOOSTVOL);
    wmc_write_masked(WMC_RIGHT_DAC_DIGITAL_VOL, dac_r, WMC_DVOL);
    wmc_write_masked(WMC_ROUT1_HP_VOLUME_CTRL, hp_r, WMC_AVOL);

    if (vol_l > 0)
    {
        /* Not muted and going up from mute level? */
        if (prev_l <= 0 && !wmc_vol.ahw_mute)
            wmc_clear(WMC_LOUT1_HP_VOLUME_CTRL, WMC_MUTE);
    }
    else
    {
        /* Going to mute level? */
        if (prev_l > 0)
            wmc_set(WMC_LOUT1_HP_VOLUME_CTRL, WMC_MUTE);
    }

    if (vol_r > 0)
    {
        /* Not muted and going up from mute level? */
        if (prev_r <= 0 && !wmc_vol.ahw_mute)
            wmc_clear(WMC_ROUT1_HP_VOLUME_CTRL, WMC_MUTE);
    }
    else
    {
        /* Going to mute level? */
        if (prev_r > 0)
            wmc_set(WMC_ROUT1_HP_VOLUME_CTRL, WMC_MUTE);
    }
}

void audiohw_close(void)
{
    /* 1. Mute all analogue outputs */
    audiohw_mute(true);
    audiohw_enable_headphone_jack(false);

    /* 2. Disable power management register 1. R1 = 00 */
    wmc_write(WMC_POWER_MANAGEMENT1, 0x000);

    /* 3. Disable power management register 2. R2 = 00 */
    wmc_write(WMC_POWER_MANAGEMENT2, 0x000);

    /* 4. Disable power management register 3. R3 = 00 */
    wmc_write(WMC_POWER_MANAGEMENT3, 0x000);

    /* 5. Remove external power supplies. */
}

void audiohw_mute(bool mute)
{
    wmc_vol.ahw_mute = mute;

    /* No DAC mute here, please - take care of each enabled output. */
    if (mute)
    {
        wmc_set(WMC_LOUT1_HP_VOLUME_CTRL, WMC_MUTE);
        wmc_set(WMC_ROUT1_HP_VOLUME_CTRL, WMC_MUTE);
    }
    else
    {
        /* Unmute outputs not at mute level */
        if (wmc_vol.vol_l > 0)
            wmc_clear(WMC_LOUT1_HP_VOLUME_CTRL, WMC_MUTE);

        if (wmc_vol.vol_r > 0)
            wmc_clear(WMC_ROUT1_HP_VOLUME_CTRL, WMC_MUTE);
    }
}

void audiohw_set_frequency(int fsel)
{
    /* For 16.9344MHz MCLK, codec as master. */
    static const struct
    {
        uint32_t plln  : 8;
        uint32_t pllk1 : 6;
        uint32_t pllk2 : 9;
        uint32_t pllk3 : 9;
        unsigned char mclkdiv;
        unsigned char filter;
    } srctrl_table[HW_NUM_FREQ] =
    {
        [HW_FREQ_8] = /* PLL = 65.536MHz */
        {
            .plln    = WMC_PLLNw(7) | WMC_PLL_PRESCALE,
            .pllk1   = WMC_PLLK_23_18w(12414886ul >> 18),
            .pllk2   = WMC_PLLK_17_9w(12414886ul >> 9),
            .pllk3   = WMC_PLLK_8_0w(12414886ul >> 0),
            .mclkdiv = WMC_MCLKDIV_8,   /*  2.0480 MHz */
            .filter  = WMC_SR_8KHZ,
        },
        [HW_FREQ_11] = /* PLL = off */
        {
            .mclkdiv = WMC_MCLKDIV_6,   /*  2.8224 MHz */
            .filter  = WMC_SR_12KHZ,
        },
        [HW_FREQ_12] = /* PLL = 73.728 MHz */
        {
            .plln    = WMC_PLLNw(8) | WMC_PLL_PRESCALE,
            .pllk1   = WMC_PLLK_23_18w(11869595ul >> 18),
            .pllk2   = WMC_PLLK_17_9w(11869595ul >> 9),
            .pllk3   = WMC_PLLK_8_0w(11869595ul >> 0),
            .mclkdiv = WMC_MCLKDIV_6,   /*  3.0720 MHz */
            .filter  = WMC_SR_12KHZ,
        },
        [HW_FREQ_16] = /* PLL = 65.536MHz */
        {
            .plln    = WMC_PLLNw(7) | WMC_PLL_PRESCALE,
            .pllk1   = WMC_PLLK_23_18w(12414886ul >> 18),
            .pllk2   = WMC_PLLK_17_9w(12414886ul >> 9),
            .pllk3   = WMC_PLLK_8_0w(12414886ul >> 0),
            .mclkdiv = WMC_MCLKDIV_4,   /*  4.0960 MHz */
            .filter  = WMC_SR_16KHZ,
        },
        [HW_FREQ_22] = /* PLL = off */
        {
            .mclkdiv = WMC_MCLKDIV_3,   /*  5.6448 MHz */
            .filter  = WMC_SR_24KHZ,
        },
        [HW_FREQ_24] = /* PLL = 73.728 MHz */
        {
            .plln    = WMC_PLLNw(8) | WMC_PLL_PRESCALE,
            .pllk1   = WMC_PLLK_23_18w(11869595ul >> 18),
            .pllk2   = WMC_PLLK_17_9w(11869595ul >> 9),
            .pllk3   = WMC_PLLK_8_0w(11869595ul >> 0),
            .mclkdiv = WMC_MCLKDIV_3,   /*  6.1440 MHz */
            .filter  = WMC_SR_24KHZ,
        },
        [HW_FREQ_32] = /* PLL = 65.536MHz */
        {
            .plln    = WMC_PLLNw(7) | WMC_PLL_PRESCALE,
            .pllk1   = WMC_PLLK_23_18w(12414886ul >> 18),
            .pllk2   = WMC_PLLK_17_9w(12414886ul >> 9),
            .pllk3   = WMC_PLLK_8_0w(12414886ul >> 0),
            .mclkdiv = WMC_MCLKDIV_2,   /*  8.1920 MHz */
            .filter  = WMC_SR_32KHZ,
        },
        [HW_FREQ_44] = /* PLL = off */
        {
            .mclkdiv = WMC_MCLKDIV_1_5, /* 11.2896 MHz */
            .filter  = WMC_SR_48KHZ,
        },
        [HW_FREQ_48] = /* PLL = 73.728 MHz */
        {
            .plln    = WMC_PLLNw(8) | WMC_PLL_PRESCALE,
            .pllk1   = WMC_PLLK_23_18w(11869595ul >> 18),
            .pllk2   = WMC_PLLK_17_9w(11869595ul >> 9),
            .pllk3   = WMC_PLLK_8_0w(11869595ul >> 0),
            .mclkdiv = WMC_MCLKDIV_1_5, /* 12.2880 MHz */
            .filter  = WMC_SR_48KHZ,
        },
    };

    unsigned int plln;
    unsigned int mclkdiv;

    if ((unsigned)fsel >= HW_NUM_FREQ)
        fsel = HW_FREQ_DEFAULT;

    /* Setup filters. */
    wmc_write(WMC_ADDITIONAL_CTRL, srctrl_table[fsel].filter);

    plln = srctrl_table[fsel].plln;
    mclkdiv = srctrl_table[fsel].mclkdiv;

    if (plln != 0)
    {
        /* Using PLL to generate SYSCLK */

        /* Program PLL. */
        wmc_write(WMC_PLL_N, plln);
        wmc_write(WMC_PLL_K1, srctrl_table[fsel].pllk1);
        wmc_write(WMC_PLL_K2, srctrl_table[fsel].pllk2);
        wmc_write(WMC_PLL_K3, srctrl_table[fsel].pllk3);

        /* Turn on PLL. */
        wmc_set(WMC_POWER_MANAGEMENT1, WMC_PLLEN);

        /* Switch to PLL and set divider. */
        wmc_write_masked(WMC_CLOCK_GEN_CTRL, mclkdiv | WMC_CLKSEL,
                         WMC_MCLKDIV | WMC_CLKSEL);
    }
    else
    {
        /* Switch away from PLL and set MCLKDIV. */
        wmc_write_masked(WMC_CLOCK_GEN_CTRL, mclkdiv,
                         WMC_MCLKDIV | WMC_CLKSEL);

        /* Turn off PLL. */
        wmc_clear(WMC_POWER_MANAGEMENT1, WMC_PLLEN);
    }
}

#ifdef HAVE_RECORDING
void audiohw_set_recsrc(int source, bool recording)
{
    switch (source)
    {
    case AUDIO_SRC_PLAYBACK:
        /* Disable all audio paths but DAC */
        /* Disable ADCs */
        wmc_clear(WMC_ADC_CONTROL, WMC_HPFEN);
        wmc_clear(WMC_POWER_MANAGEMENT2, WMC_ADCENL | WMC_ADCENR);
        /* Disable bypass */
        wmc_clear(WMC_LEFT_MIXER_CTRL, WMC_BYPL2LMIX);
        wmc_clear(WMC_RIGHT_MIXER_CTRL, WMC_BYPR2RMIX);
        /* Disable IP BOOSTMIX and PGA */
        wmc_clear(WMC_POWER_MANAGEMENT2, WMC_INPPGAENL | WMC_INPPGAENR |
                  WMC_BOOSTENL | WMC_BOOSTENR);
        wmc_clear(WMC_INPUT_CTRL, WMC_L2_2INPPGA | WMC_R2_2INPPGA);
        wmc_clear(WMC_LEFT_ADC_BOOST_CTRL, WMC_PGABOOSTL);
        wmc_clear(WMC_RIGHT_ADC_BOOST_CTRL, WMC_PGABOOSTR);
        break;

    case AUDIO_SRC_FMRADIO:
        if (recording)
        {
            /* Disable bypass */
            wmc_clear(WMC_LEFT_MIXER_CTRL, WMC_BYPL2LMIX);
            wmc_clear(WMC_RIGHT_MIXER_CTRL, WMC_BYPR2RMIX);
            /* Enable ADCs, IP BOOSTMIX and PGA, route L/R2 through PGA */
            wmc_set(WMC_POWER_MANAGEMENT2, WMC_ADCENL | WMC_ADCENR |
                    WMC_BOOSTENL | WMC_BOOSTENR | WMC_INPPGAENL |
                    WMC_INPPGAENR);
            wmc_set(WMC_ADC_CONTROL, WMC_ADCOSR | WMC_HPFEN);
            /* PGA at 0dB with +20dB boost */
            wmc_write_masked(WMC_LEFT_INP_PGA_GAIN_CTRL, 0x10, WMC_AVOL);
            wmc_write_masked(WMC_RIGHT_INP_PGA_GAIN_CTRL, 0x10, WMC_AVOL);
            wmc_set(WMC_LEFT_ADC_BOOST_CTRL, WMC_PGABOOSTL);
            wmc_set(WMC_RIGHT_ADC_BOOST_CTRL, WMC_PGABOOSTR);
            /* Connect L/R2 inputs to PGA */
            wmc_set(WMC_INPUT_CTRL, WMC_L2_2INPPGA | WMC_R2_2INPPGA);
        }
        else
        {
            /* Disable PGA and ADC, enable IP BOOSTMIX, route L/R2 directly to
             * IP BOOSTMIX */
            wmc_clear(WMC_ADC_CONTROL, WMC_HPFEN);
            wmc_write_masked(WMC_POWER_MANAGEMENT2, WMC_BOOSTENL | WMC_BOOSTENR,
                WMC_BOOSTENL | WMC_BOOSTENR | WMC_INPPGAENL |
                WMC_INPPGAENR | WMC_ADCENL | WMC_ADCENR);
            wmc_clear(WMC_INPUT_CTRL, WMC_L2_2INPPGA | WMC_R2_2INPPGA);
            wmc_clear(WMC_LEFT_ADC_BOOST_CTRL, WMC_PGABOOSTL);
            wmc_clear(WMC_RIGHT_ADC_BOOST_CTRL, WMC_PGABOOSTR);
            /* Enable bypass to L/R mixers */
            wmc_set(WMC_LEFT_MIXER_CTRL, WMC_BYPL2LMIX);
            wmc_set(WMC_RIGHT_MIXER_CTRL, WMC_BYPR2RMIX);
        }
        break;
    }
}

void audiohw_set_recvol(int left, int right, int type)
{
    switch (type)
    {
    case AUDIO_GAIN_LINEIN:
        wmc_write_masked(WMC_LEFT_ADC_DIGITAL_VOL, left + 239, WMC_DVOL);
        wmc_write_masked(WMC_RIGHT_ADC_DIGITAL_VOL, right + 239, WMC_DVOL);
        return;
    }
}
#endif /* HAVE_RECORDING */
