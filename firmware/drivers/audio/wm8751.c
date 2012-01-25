/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Driver for WM8751 audio codec
 *
 * Based on code from the ipodlinux project - http://ipodlinux.org/
 * Adapted for Rockbox in December 2005
 *
 * Original file: linux/arch/armnommu/mach-ipod/audio.c
 *
 * Copyright (c) 2003-2005 Bernard Leach (leachbj@bouncycastle.org)
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
#include "kernel.h"
#include "wmcodec.h"
#include "audio.h"
#include "audiohw.h"
#include "system.h"
#include "sound.h"

const struct sound_settings_info audiohw_settings[] = {
    [SOUND_VOLUME]        = {"dB", 0,  1, -74,   6, -25},
    [SOUND_BASS]          = {"dB", 1, 15, -60,  90,   0},
    [SOUND_TREBLE]        = {"dB", 1, 15, -60,  90,   0},
    [SOUND_BALANCE]       = {"%",  0,  1,-100, 100,   0},
    [SOUND_CHANNELS]      = {"",   0,  1,   0,   5,   0},
    [SOUND_STEREO_WIDTH]  = {"%",  0,  5,   0, 250, 100},
#ifdef HAVE_RECORDING
    /* PGA -17.25dB to 30.0dB in 0.75dB increments 64 steps
     * digital gain 0dB to 30.0dB in 0.5dB increments
     * we use 0.75dB fake steps through whole range
     *
     * This combined gives -17.25 to 60.0dB 
     */
    [SOUND_LEFT_GAIN]     = {"dB", 2,  75, -1725, 6000, 0},
    [SOUND_RIGHT_GAIN]    = {"dB", 2,  75, -1725, 6000, 0},
    [SOUND_MIC_GAIN]      = {"dB", 2,  75, -1725, 6000, 3000},
#endif
#ifdef AUDIOHW_HAVE_BASS_CUTOFF
    [SOUND_BASS_CUTOFF]   = {"Hz", 0, 70, 130, 200, 200},
#endif
#ifdef AUDIOHW_HAVE_TREBLE_CUTOFF
    [SOUND_TREBLE_CUTOFF] = {"kHz", 0,  4,   4,   8,   4},
#endif
#ifdef AUDIOHW_HAVE_DEPTH_3D
    [SOUND_DEPTH_3D]      = {"%",   0,  1,  0,  15,   0},
#endif
};

static uint16_t wmcodec_regs[WM_NUM_REGS] =
{
    [0 ... WM_NUM_REGS-1] = 0x200, /* set invalid data in gaps */
#ifdef HAVE_WM8750
    [LINVOL]              = 0x097,
    [RINVOL]              = 0x097,
#endif
    [LOUT1]               = 0x079,
    [ROUT1]               = 0x079,
    [DACCTRL]             = 0x008,
    [AINTFCE]             = 0x00a,
    [CLOCKING]            = 0x000,
    [LEFTGAIN]            = 0x0ff,
    [RIGHTGAIN]           = 0x0ff,
    [BASSCTRL]            = 0x00f,
    [TREBCTRL]            = 0x00f,
/*  [RESET] */
#ifdef HAVE_WM8750
    [ENHANCE_3D]          = 0x000,
    [ALC1]                = 0x07b,
    [ALC2]                = 0x000,
    [ALC3]                = 0x032,
    [NGAT]                = 0x000,
    [LADCVOL]             = 0x0c3,
    [RADCVOL]             = 0x0c3,
#endif
    [ADDITIONAL1]         = 0x0c0,
    [ADDITIONAL2]         = 0x000,
    [PWRMGMT1]            = 0x000,
    [PWRMGMT2]            = 0x000,
    [ADDITIONAL3]         = 0x000,
#ifdef HAVE_WM8750
    [ADCIM]               = 0x000,
    [ADCL]                = 0x000,
    [ADCR]                = 0x000,
#endif
    [LEFTMIX1]            = 0x050,
    [LEFTMIX2]            = 0x050,
    [RIGHTMIX1]           = 0x050,
    [RIGHTMIX2]           = 0x050,
    [MONOMIX1]            = 0x050,
    [MONOMIX2]            = 0x050,
    [LOUT2]               = 0x079,
    [ROUT2]               = 0x079,
    [MONOOUT]             = 0x079
};

/* global prescaler vars */
static int prescalertone = 0;
static int prescaler3d = 0;

static void wmcodec_set_reg(unsigned int reg, unsigned int val)
{
    if (reg >= WM_NUM_REGS || (wmcodec_regs[reg] & 0x200))
        /* invalid register */
        return;

    wmcodec_regs[reg] = val & 0x1ff;
    wmcodec_write(reg, wmcodec_regs[reg]);
}

static void wmcodec_set_bits(unsigned int reg, unsigned int bits)
{
    wmcodec_set_reg(reg, wmcodec_regs[reg] | bits);
}

static void wmcodec_clear_bits(unsigned int reg, unsigned int bits)
{
    wmcodec_set_reg(reg, wmcodec_regs[reg] & ~bits);
}

static void wmcodec_set_masked(unsigned int reg, unsigned int val, 
                          unsigned int mask )
{
    wmcodec_set_reg(reg, (wmcodec_regs[reg] & ~mask) | val);
}

/* convert tenth of dB volume (-730..60) to master volume register value */
int tenthdb2master(int db)
{
    /* +6 to -73dB 1dB steps (plus mute == 80levels) 7bits */
    /* 1111111 ==  +6dB  (0x7f)                            */
    /* 1111001 ==   0dB  (0x79)                            */
    /* 0110000 == -73dB  (0x30)                            */
    /* 0101111..0000000 == mute  (<= 0x2f)                 */
    if (db < VOLUME_MIN)
        return 0x0;
    else
        return (db / 10) + 73 + 0x30;
}

static int tone_tenthdb2hw(int value)
{
    /* -6.0db..+0db..+9.0db step 1.5db - translate -60..+0..+90 step 15
        to 10..6..0 step -1.
    */
    value = 10 - (value + 60) / 15;

    if (value == 6)
        value = 0xf; /* 0db -> off */

    return value;
}

#ifdef AUDIOHW_HAVE_BASS_CUTOFF
void audiohw_set_bass_cutoff(int val)
{
     if ( val == 130 )
         wmcodec_clear_bits(BASSCTRL, BASSCTRL_BC);
     else
         wmcodec_set_bits(BASSCTRL, BASSCTRL_BC);

}
#endif

#ifdef AUDIOHW_HAVE_TREBLE_CUTOFF
void audiohw_set_treble_cutoff(int val)
{
    if ( val == 8 )
        wmcodec_clear_bits(TREBCTRL, TREBCTRL_TC);
    else
        wmcodec_set_bits(TREBCTRL, TREBCTRL_TC); 
}
#endif


int sound_val2phys(int setting, int value)
{
    int result;

    switch (setting)
    {
#ifdef AUDIOHW_HAVE_DEPTH_3D
    case SOUND_DEPTH_3D:
        result = (100 * value + 8) / 15;
        break;
#endif
    default:
        result = value;
    }

    return result;
}

static void audiohw_mute(bool mute)
{
    /* Mute:   Set DACMU = 1 to soft-mute the audio DACs. */
    /* Unmute: Set DACMU = 0 to soft-un-mute the audio DACs. */
    if (mute)
        wmcodec_set_bits(DACCTRL, DACCTRL_DACMU);
    else
        wmcodec_clear_bits(DACCTRL, DACCTRL_DACMU);
}

/* Reset and power up the WM8751 */
void audiohw_preinit(void)
{
#if defined(MROBE_100)
    /* controls headphone ouput */
    GPIOL_ENABLE     |= 0x10;
    GPIOL_OUTPUT_EN  |= 0x10;
    GPIOL_OUTPUT_VAL |= 0x10; /* disable */
#elif defined(MPIO_HD200)
    /* control headphone output
     * disabled on startup
     */
    and_l(~(1<<25), &GPIO1_OUT);
    or_l((1<<25), &GPIO1_ENABLE);
    or_l((1<<25), &GPIO1_FUNCTION);
#elif defined(MPIO_HD300)
    and_l(~(1<<5), &GPIO1_OUT);
    or_l((1<<5), &GPIO1_ENABLE);
    or_l((1<<5), &GPIO1_FUNCTION);
#endif

    /*
     * 1. Switch on power supplies.
     *    By default the WM8751 is in Standby Mode, the DAC is
     *    digitally muted and the Audio Interface, Line outputs
     *    and Headphone outputs are all OFF (DACMU = 1 Power
     *    Management registers 1 and 2 are all zeros).
     */

    wmcodec_write(RESET, RESET_RESET);    /*Reset*/

     /* 2. Enable Vmid and VREF. */
    wmcodec_set_bits(PWRMGMT1, PWRMGMT1_VREF | PWRMGMT1_VMIDSEL_5K);

#ifdef CODEC_SLAVE
    wmcodec_set_bits(AINTFCE,AINTFCE_WL_16 | AINTFCE_FORMAT_I2S);
#else
    /* BCLKINV=0(Dont invert BCLK) MS=1(Enable Master) LRSWAP=0 LRP=0 */
    /* IWL=00(16 bit) FORMAT=10(I2S format) */
    wmcodec_set_bits(AINTFCE, AINTFCE_MS | AINTFCE_WL_16 |
                  AINTFCE_FORMAT_I2S);
#endif
    /* Set default samplerate */

    audiohw_set_frequency(HW_FREQ_DEFAULT);
}

/* Enable DACs and audio output after a short delay */
void audiohw_postinit(void)
{
    /* From app notes: allow Vref to stabilize to reduce clicks */
    sleep(HZ);

#ifdef AUDIOHW_HAVE_DEPTH_3D
    wmcodec_set_bits(ENHANCE_3D, ENHANCE_3D_MODE3D_PLAYBACK);
#endif

     /* 3. Enable DACs as required. */
    wmcodec_set_bits(PWRMGMT2, PWRMGMT2_DACL | PWRMGMT2_DACR);

     /* 4. Enable line and / or headphone output buffers as required. */
#if defined(GIGABEAT_F)
    /* headphones + line-out */
    wmcodec_set_bits(PWRMGMT2, PWRMGMT2_LOUT1 | PWRMGMT2_ROUT1 | 
                     PWRMGMT2_LOUT2 | PWRMGMT2_ROUT2);
#else
    /* headphones */
    wmcodec_set_bits(PWRMGMT2, PWRMGMT2_LOUT1 | PWRMGMT2_ROUT1);
#endif

    /* Full -0dB on the DACS */
    wmcodec_set_bits(LEFTGAIN, 0xff);
    wmcodec_set_bits(RIGHTGAIN, RIGHTGAIN_RDVU | 0xff);

    /* Enable Thermal shutdown, Timeout when zero-crossing in use,
     * set analog bias for 3.3V, monomix to DACR
     */
    wmcodec_set_reg(ADDITIONAL1, ADDITIONAL1_TSDEN | ADDITIONAL1_TOEN |
                    ADDITIONAL1_DMONOMIX_LLRR | ADDITIONAL1_VSEL_DEFAULT);


    /* Enable zero-crossing in out stage */
    wmcodec_set_bits(LOUT1, LOUT1_LO1ZC);
    wmcodec_set_bits(ROUT1, ROUT1_RO1ZC);
    wmcodec_set_bits(LOUT2, LOUT2_LO2ZC);
    wmcodec_set_bits(ROUT2, ROUT2_RO2ZC);

    /* route signal from DAC to mixers */
    wmcodec_set_bits(LEFTMIX1, LEFTMIX1_LD2LO);
    wmcodec_set_bits(RIGHTMIX2, RIGHTMIX2_RD2RO);

#ifdef TOSHIBA_GIGABEAT_F
#ifdef HAVE_HARDWARE_BEEP
    /* Single-ended mono input */
    wmcodec_set_reg(MONOMIX1, 0x00);

    /* Route mono input to both outputs at 0dB */
    wmcodec_set_reg(LEFTMIX2, LEFTMIX2_MI2LO | LEFTMIX2_MI2LOVOL(2));
    wmcodec_set_reg(RIGHTMIX1, RIGHTMIX1_MI2RO | RIGHTMIX1_MI2ROVOL(2));
#endif
#endif

    /* lower power consumption */
    wmcodec_set_masked(PWRMGMT1, PWRMGMT1_VMIDSEL_50K,
                       PWRMGMT1_VMIDSEL_MASK);

    audiohw_mute(false);

#if defined(MROBE_100)
    /* enable headphone output */
    GPIOL_OUTPUT_VAL &= ~0x10;
    GPIOL_OUTPUT_EN  |=  0x10;
#elif defined(MPIO_HD200)
   /* enable headphone output */
   or_l((1<<25), &GPIO1_OUT);
#elif defined(MPIO_HD300)
   or_l((1<<5), &GPIO1_OUT);
#endif
}

void audiohw_set_master_vol(int vol_l, int vol_r)
{
    /* +6 to -73dB 1dB steps (plus mute == 80levels) 7bits */
    /* 1111111 ==  +6dB                                    */
    /* 1111001 ==   0dB                                    */
    /* 0110000 == -73dB                                    */
    /* 0101111 == mute (0x2f)                              */

    wmcodec_set_masked(LOUT1, LOUT1_LOUT1VOL(vol_l),
                       LOUT1_LOUT1VOL_MASK);
    wmcodec_set_masked(ROUT1, ROUT1_RO1VU | ROUT1_ROUT1VOL(vol_r),
                       ROUT1_ROUT1VOL_MASK);
}

#ifdef TOSHIBA_GIGABEAT_F
void audiohw_set_lineout_vol(int vol_l, int vol_r)
{
    wmcodec_set_masked(LOUT2, LOUT2_LOUT2VOL(vol_l),
                       LOUT2_LOUT2VOL_MASK);
    wmcodec_set_masked(ROUT2, ROUT2_RO2VU | ROUT2_ROUT2VOL(vol_r),
                       ROUT2_ROUT2VOL_MASK);
}
#endif

void audiohw_set_bass(int value)
{
    wmcodec_set_masked(BASSCTRL,
                       BASSCTRL_BASS(tone_tenthdb2hw(value)),
                       BASSCTRL_BASS_MASK);
}

void audiohw_set_treble(int value)
{
    wmcodec_set_masked(TREBCTRL, TREBCTRL_TREB(tone_tenthdb2hw(value)),
                       TREBCTRL_TREB_MASK);
}

static void sync_prescaler(void)
{
    int prescaler;
    prescaler = prescalertone + prescaler3d;

    /* attenuate in 0.5dB steps (0dB - -127dB) */
    wmcodec_set_reg(LEFTGAIN, 0xff - (prescaler & LEFTGAIN_LDACVOL));
    wmcodec_set_reg(RIGHTGAIN, RIGHTGAIN_RDVU |
                    (0xff - (prescaler & RIGHTGAIN_RDACVOL)));
}

void audiohw_set_prescaler(int value)
{
    prescalertone = 3 * value / 15; /* value in tdB */
    sync_prescaler();
}

/* Nice shutdown of WM8751 codec */
void audiohw_close(void)
{
    /* 1. Set DACMU = 1 to soft-mute the audio DACs. */
    audiohw_mute(true);

#if defined(MPIO_HD200)
    /* disable headphone out */
    and_l(~(1<<25), &GPIO1_OUT);
#elif defined(MPIO_HD300)
    and_l(~(1<<5), &GPIO1_OUT);
#endif

    /* 2. Disable all output buffers. */
    wmcodec_set_reg(PWRMGMT2, 0x0);

    /* 3. Switch off the power supplies. */
    wmcodec_set_reg(PWRMGMT1, 0x0);
}

/* According to datasheet of WM8750
 * clocking setup is needed in both slave and master mode
 */
void audiohw_set_frequency(int fsel)
{
    (void)fsel;
    static const unsigned char srctrl_table[HW_NUM_FREQ] =
    {
        HW_HAVE_11_([HW_FREQ_11] = CODEC_SRCTRL_11025HZ,)
        HW_HAVE_22_([HW_FREQ_22] = CODEC_SRCTRL_22050HZ,)
        HW_HAVE_44_([HW_FREQ_44] = CODEC_SRCTRL_44100HZ,)
        HW_HAVE_88_([HW_FREQ_88] = CODEC_SRCTRL_88200HZ,)
    };

    if ((unsigned)fsel >= HW_NUM_FREQ)
        fsel = HW_FREQ_DEFAULT;

    wmcodec_set_reg(CLOCKING, srctrl_table[fsel]);
}

#ifdef HAVE_WM8750
#ifdef AUDIOHW_HAVE_DEPTH_3D
/* Set the depth of the 3D effect */
void audiohw_set_depth_3d(int val)
{
    if (val > 0)
    {
        wmcodec_set_bits(ENHANCE_3D, ENHANCE_3D_3DEN);
        wmcodec_set_masked(ENHANCE_3D, ENHANCE_3D_DEPTH(val),
                           ENHANCE_3D_DEPTH_MASK);
    }
    else
    {
        wmcodec_clear_bits(ENHANCE_3D, ENHANCE_3D_3DEN);
    }

    /* -4 dB @ full setting
     * this gives approximately constant volume on setting change
     * and prevents clipping (at least on my HD300)
     */
    prescaler3d = 8*val / 15;
    sync_prescaler();
}
#endif

#ifdef HAVE_RECORDING
#if 0
static void audiohw_set_ngat(int ngath, int type, bool enable)
{
    /* This function controls Noise gate function
     * of the codec. This can only run in conjunction
     * with ALC
     */

    if(enable)
        wmcodec_set_reg(NGAT, NGAT_NGG(type) |
                        NGAT_NGTH(ngath) | NGAT_NGAT);
    else
        wmcodec_clear_bits(NGAT, NGAT_NGAT);
}


static void audiohw_set_alc(unsigned char level,  /* signal level at ADC */
                            bool zc,              /* zero cross detection */
                            unsigned char hold,   /* hold time */
                            unsigned char decay,  /* decay time */
                            unsigned char attack, /* attack time */
                            bool enable)          /* hw function on/off */
{
    if(enable)
    {
        wmcodec_set_reg(ALC1, ALC1_ALCSEL_STEREO | ALC1_MAXGAIN(0x07) |
                        ALC1_ALCL(level));
        wmcodec_set_reg(ALC2, zc?ALC2_ALCZC:0 | ALC2_HLD(hold));
        wmcodec_set_reg(ALC3, ALC3_DCY(decay) | ALC3_ATK(attack));
    }
    else
    {
        wmcodec_set_masked(ALC1, ALC1_ALCSEL_DISABLED, ALC1_ALCSEL_MASK);
    }
}
#endif

void audiohw_set_recsrc(int source, bool recording)
{
    /* INPUT1 - FM radio
     * INPUT2 - Line-in
     * INPUT3 - MIC
     *
     * if recording == false we use analog bypass from input
     * turn off ADC, PGA to save power
     * turn on output buffer(s)
     * 
     * if recording == true we route input signal to PGA
     * and monitoring picks up signal after PGA and ADC
     * turn on ADC, PGA, DAC, output buffer(s)
     */
    
    switch(source)
    {
    case AUDIO_SRC_PLAYBACK:
        audiohw_mute(true);

        /* mute PGA, disable all audio paths but DAC and output stage*/
        wmcodec_set_bits(LINVOL, LINVOL_LINMUTE); /* Mute */
        wmcodec_set_bits(RINVOL, RINVOL_RINMUTE); /* Mute */

        wmcodec_clear_bits(PWRMGMT2, PWRMGMT2_OUT3 | PWRMGMT2_MOUT |
                           PWRMGMT2_ROUT2 | PWRMGMT2_LOUT2);

        /* route DAC signal to output mixer, disable INPUT routing */
        wmcodec_clear_bits(LEFTMIX1, LEFTMIX1_LI2LO);
        wmcodec_set_bits(LEFTMIX1, LEFTMIX1_LD2LO);
        wmcodec_clear_bits(RIGHTMIX2, RIGHTMIX2_RI2RO);
        wmcodec_set_bits(RIGHTMIX2, RIGHTMIX2_RD2RO);

        /* unmute DAC */
        audiohw_mute(false);
        break;

    case AUDIO_SRC_FMRADIO:
        if(recording)
        {
            audiohw_mute(true);
            /* Turn on PGA and ADC */
            wmcodec_set_bits(PWRMGMT1, PWRMGMT1_AINL | PWRMGMT1_AINR | 
                             PWRMGMT1_ADCL | PWRMGMT1_ADCR);

            /* Setup input source for PGA as INPUT1 */
            wmcodec_set_masked(ADCL, ADCL_LINSEL_LINPUT1, ADCL_LINSEL_MASK);
            wmcodec_set_masked(ADCR, ADCR_RINSEL_RINPUT1, ADCR_RINSEL_MASK);

           /* turn off ALC and NGAT as OF do */
           /*
           audiohw_set_alc(0x00, false, 0x00, 0x00, 0x00, false);
           audiohw_set_ngat(0x00, 0x00, false);
           */

            /* setup output digital data
             * default is LADC -> LDATA, RADC -> RDATA
             * so we don't touch this
             */

            /* skip: power up DAC and output stage as its never powered down
            wmcodec_set_bits(PWRMGMT2, PWRMGMT2_DACL | PWRMGMT2_DACR |
                          PWRMGMT2_LOUT1 | PWRMGMT2_ROUT1);
            */

            /* route DAC signal to output mixer, disable INPUT routing */
            wmcodec_clear_bits(LEFTMIX1, LEFTMIX1_LI2LO);
            wmcodec_set_bits(LEFTMIX1, LEFTMIX1_LD2LO);
            wmcodec_clear_bits(RIGHTMIX2, RIGHTMIX2_RI2RO);
            wmcodec_set_bits(RIGHTMIX2, RIGHTMIX2_RD2RO);

            audiohw_mute(false);
        }
        else
        {
            audiohw_mute(true);

            /* turn off ADC, PGA */
            wmcodec_clear_bits(PWRMGMT1, PWRMGMT1_AINL | PWRMGMT1_AINR |
                               PWRMGMT1_ADCL | PWRMGMT1_ADCR);

            /* setup monitor mode by routing input signal to outmix 
             * at 0dB volume
             */
            wmcodec_set_masked(LEFTMIX1, LEFTMIX1_LI2LOVOL(0x20),
                               LEFTMIX1_LI2LOVOL_MASK);
            wmcodec_set_bits(LEFTMIX1, LEFTMIX1_LI2LO |
                             LEFTMIX1_LMIXSEL_LINPUT1 |
                             LEFTMIX1_LD2LO);
            wmcodec_set_masked(RIGHTMIX2, RIGHTMIX2_RI2ROVOL(0x20),
                               RIGHTMIX2_RI2ROVOL_MASK);
            wmcodec_set_bits(RIGHTMIX1, RIGHTMIX1_RMIXSEL_RINPUT1);
            wmcodec_set_bits(RIGHTMIX2, RIGHTMIX2_RI2RO |
                             RIGHTMIX2_RD2RO);

            audiohw_mute(false);
        }
        break;

#if (INPUT_SRC_CAPS & SRC_CAP_LINEIN)
    case AUDIO_SRC_LINEIN:
        audiohw_mute(true);

        /* Turn on PGA, ADC */
        wmcodec_set_bits(PWRMGMT1, PWRMGMT1_AINL | PWRMGMT1_AINR |
                         PWRMGMT1_ADCL | PWRMGMT1_ADCR);

        /* Setup input source for PGA as INPUT2 
         * MICBOOST disabled
         */
        wmcodec_set_masked(ADCL, ADCL_LINSEL_LINPUT2, ADCL_LINSEL_MASK);
        wmcodec_set_masked(ADCR, ADCR_RINSEL_RINPUT2, ADCR_RINSEL_MASK);

        /* setup ALC and NGAT as OF do */
        /* level, zc, hold, decay, attack, enable */
        /*  audiohw_set_alc(0x0b, true, 0x00, 0x03, 0x02, true); */
        /* ngath, type, enable */
        /* audiohw_set_ngat(0x08, 0x02, true); */

        /* setup output digital data
         * default is LADC -> LDATA, RADC -> RDATA
         * so we don't touch this
         */

        /* route DAC signal to output mixer, disable INPUT routing */
        wmcodec_clear_bits(LEFTMIX1, LEFTMIX1_LI2LO);
        wmcodec_set_bits(LEFTMIX1, LEFTMIX1_LD2LO);
        wmcodec_clear_bits(RIGHTMIX2, RIGHTMIX2_RI2RO);
        wmcodec_set_bits(RIGHTMIX2, RIGHTMIX2_RD2RO);

#if !defined(MPIO_HD300)
        /* HD300 uses the same socket for headphones and line-in */
        audiohw_mute(false);
#endif
        break;
#endif
#if (INPUT_SRC_CAPS & SRC_CAP_MIC)
    case AUDIO_SRC_MIC:
        audiohw_mute(true);

        /* Turn on PGA and ADC */
        wmcodec_set_bits(PWRMGMT1, PWRMGMT1_AINL | PWRMGMT1_AINR |
                         PWRMGMT1_ADCL | PWRMGMT1_ADCR);

        /* Setup input source for PGA as INPUT3 
         * MICBOOST disabled
         */
        wmcodec_set_masked(ADCL, ADCL_LINSEL_LINPUT3, ADCL_LINSEL_MASK);
        wmcodec_set_masked(ADCR, ADCR_RINSEL_RINPUT3, ADCR_RINSEL_MASK);

        /* setup ALC and NGAT as OF do */
        /* level, zc, hold, decay, attack, enable */
        /* audiohw_set_alc(0x0f, false, 0x00, 0x05, 0x02, true); */
        /* ngath, type, enable */
        /* audiohw_set_ngat(0x1f, 0x00, true); */

        /* setup output digital data
         * default is LADC -> LDATA, RADC -> RDATA
         * so we don't touch this
         */

        /* route DAC signal to output mixer, disable INPUT routing */
        wmcodec_clear_bits(LEFTMIX1, LEFTMIX1_LI2LO);
        wmcodec_set_bits(LEFTMIX1, LEFTMIX1_LD2LO);
        wmcodec_clear_bits(RIGHTMIX2, RIGHTMIX2_RI2RO);
        wmcodec_set_bits(RIGHTMIX2, RIGHTMIX2_RD2RO);

        audiohw_mute(false);
        break;
#endif
    } /* switch(source) */
}

static int digital_gain2hw(int value)
{
    /* -9700 to 3000 => 0 ... 195 ... 255 */
    return (2 * value) / 100 + 195;
}

static int pga_gain2hw(int value)
{
    /* -1725 to 3000 => 0 ... 23 ... 63 */
    return ((4 * value) / 300) + 23;
}

void audiohw_set_recvol(int vol_l, int vol_r, int type)
{
    int d_vol_l = 0;
    int d_vol_r = 0;

    if (vol_l > 3000)
    {
        d_vol_l = vol_l - 3000;
        vol_l = 3000;
    }

    if (vol_r > 3000)
    {
        d_vol_r = vol_r - 3000;
        vol_r = 3000;
    }

    /* PGA left gain */
    wmcodec_set_reg(LINVOL, LINVOL_LIZC | LINVOL_LINVOL(pga_gain2hw(vol_l)));

    /* digital left gain set */
    wmcodec_set_reg(LADCVOL, digital_gain2hw(d_vol_l));

    if (type == AUDIO_GAIN_MIC)
    {
        /* PGA right gain = PGA left gain*/
        wmcodec_set_reg(RINVOL, RINVOL_RIVU | RINVOL_RIZC |
                                RINVOL_RINVOL(pga_gain2hw(vol_l)));

        /* digital right gain = digital left gain*/
        wmcodec_set_reg(RADCVOL, RADCVOL_RAVU | digital_gain2hw(d_vol_l));
    }
    else
    {
        /* PGA right gain */
        wmcodec_set_reg(RINVOL, RINVOL_RIVU | RINVOL_RIZC |
                                RINVOL_RINVOL(pga_gain2hw(vol_r)));

        /* digital right gain */
        wmcodec_set_reg(RADCVOL, RADCVOL_RAVU | digital_gain2hw(d_vol_r));
    }
}
#endif /* HAVE_RECORDING */
#endif /* HAVE_WM8750 */
