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

const struct sound_settings_info audiohw_settings[] = {
    [SOUND_VOLUME]        = {"dB", 0,  1, -74,   6, -25},
#ifdef USE_ADAPTIVE_BASS
    [SOUND_BASS]          = {"",   0,  1,   0,  15,   0},
#else
    [SOUND_BASS]          = {"dB", 1, 15, -60,  90,   0},
#endif
    [SOUND_TREBLE]        = {"dB", 1, 15, -60,  90,   0},
    [SOUND_BALANCE]       = {"%",  0,  1,-100, 100,   0},
    [SOUND_CHANNELS]      = {"",   0,  1,   0,   5,   0},
    [SOUND_STEREO_WIDTH]  = {"%",  0,  5,   0, 250, 100},
#ifdef AUDIOHW_HAVE_DEPTH_3D
    [SOUND_DEPTH_3D]      = {"",   0,  1,   0,  15,   0},
#endif
#ifdef HAVE_RECORDING
    [SOUND_LEFT_GAIN]     = {"dB", 1,  1,-172, 300,   0},
    [SOUND_RIGHT_GAIN]    = {"dB", 1,  1,-172, 300,   0},
    [SOUND_MIC_GAIN]      = {"dB", 1,  1,-172, 300,   0},
#endif
};

/* Flags used in combination with settings */

/* use zero crossing to reduce clicks during volume changes */
#define LOUT1_BITS      (LOUT1_LO1ZC)
/* latch left volume first then update left+right together */
#define ROUT1_BITS      (ROUT1_RO1ZC | ROUT1_RO1VU)
#define LOUT2_BITS      (LOUT2_LO2ZC)
#define ROUT2_BITS      (ROUT2_RO2ZC | ROUT2_RO2VU)
/* We use linear bass control with 200 Hz cutoff */
#ifdef USE_ADAPTIVE_BASE
#define BASSCTRL_BITS   (BASSCTRL_BC | BASSCTRL_BB)
#else
#define BASSCTRL_BITS   (BASSCTRL_BC)
#endif
/* We use linear treble control with 4 kHz cutoff */
#define TREBCTRL_BITS   (TREBCTRL_TC)

static int prescaler = 0;

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

#if 0
static int alc_tenthdb2hw(int value)
{
   /* -28.5dB - -6dB step 1.5dB - translate -285 - -60 step 15
      to 0 - 15 step 1
    */

    value = 15 - (value + 60)/15;
}


static int alc_hldus2hw(unsigned int value)
{
    /* 0000 -        0us
     * 0001 -     2670us
     * 0010 -     5330us
     *
     * 1111 - 43691000us
     */
    return 0;
}

static int alc_dcyms2hw(int value)
{
    /* 0000 - 24ms
     * 0001 - 48ms
     * 0010 - 96ms
     *
     * 1010 or higher 24580ms
     */
    return 0;  
}

static int alc_atkms2hw(int value)
{
    /* 0000 -  6ms
     * 0001 - 12ms
     * 0010 - 24ms
     *
     * 1010 or higher 6140ms
     */
    return 0;
}
#endif

#ifdef USE_ADAPTIVE_BASS
static int adaptivebass2hw(int value)
{
    /* 0 to 15 step 1 - step -1  0 = off is a 15 in the register */
    value = 15 - value;

    return value;
}
#endif

#if defined(HAVE_WM8750)
#if 0
static int ngath_tenthdb2hw(int value)
{
    /* -76.5dB - -30dB in 1.5db steps   -765 - -300 in 15 steps */
    value = 31 - (value + 300)/15;
    return value;
}
#endif
static int recvol2hw(int value)
{
/* convert tenth of dB of input volume (-172...300) to input register value */
    /* +30dB to -17.25 0.75dB step 6 bits */
    /* 111111 == +30dB  (0x3f)            */
    /* 010111 ==   0dB  (0x17)            */
    /* 000000 == -17.25dB                 */

    return ((4*(value))/30 + 0x17);
}
#endif
static void audiohw_mute(bool mute)
{
    /* Mute:   Set DACMU = 1 to soft-mute the audio DACs. */
    /* Unmute: Set DACMU = 0 to soft-un-mute the audio DACs. */
    wmcodec_write(DACCTRL, mute ? DACCTRL_DACMU : 0);
}

/* Reset and power up the WM8751 */
void audiohw_preinit(void)
{
#ifdef MROBE_100
    /* controls headphone ouput */
    GPIOL_ENABLE     |= 0x10;
    GPIOL_OUTPUT_EN  |= 0x10;
    GPIOL_OUTPUT_VAL |= 0x10; /* disable */
#endif

#ifdef MPIO_HD200
    /* control headphone output
     * disabled on startup
     */
    and_l(~(1<<25),&GPIO1_OUT);
    or_l((1<<25), &GPIO1_ENABLE);
    or_l((1<<25), &GPIO1_FUNCTION);
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
    wmcodec_write(PWRMGMT1, PWRMGMT1_VREF | PWRMGMT1_VMIDSEL_5K);

#ifdef CODEC_SLAVE
    wmcodec_write(AINTFCE,AINTFCE_WL_16|AINTFCE_FORMAT_I2S);
#else
    /* BCLKINV=0(Dont invert BCLK) MS=1(Enable Master) LRSWAP=0 LRP=0 */
    /* IWL=00(16 bit) FORMAT=10(I2S format) */
    wmcodec_write(AINTFCE, AINTFCE_MS | AINTFCE_WL_16 |
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

     /* 3. Enable DACs as required. */
    wmcodec_write(PWRMGMT2, PWRMGMT2_DACL | PWRMGMT2_DACR);

     /* 4. Enable line and / or headphone output buffers as required. */
#if defined(MROBE_100) || defined(MPIO_HD200)
    /* fix for high pitch noise after power-up on HD200
     * it is *NOT* required step according to the
     * Datasheet for WM8750L but real life is different :-)
     */
    wmcodec_write(LOUT1, LOUT1_BITS);
    wmcodec_write(ROUT1, ROUT1_BITS);

    /* power-up output stage */
    wmcodec_write(PWRMGMT2, PWRMGMT2_DACL | PWRMGMT2_DACR |
                  PWRMGMT2_LOUT1 | PWRMGMT2_ROUT1);
#else
    wmcodec_write(PWRMGMT2, PWRMGMT2_DACL | PWRMGMT2_DACR |
                  PWRMGMT2_LOUT1 | PWRMGMT2_ROUT1 | PWRMGMT2_LOUT2 |
                  PWRMGMT2_ROUT2);
#endif

    /* Full -0dB on the DACS */
    wmcodec_write(LEFTGAIN, 0xff);
    wmcodec_write(RIGHTGAIN, RIGHTGAIN_RDVU | 0xff);

    wmcodec_write(ADDITIONAL1, ADDITIONAL1_TSDEN | ADDITIONAL1_TOEN |
                    ADDITIONAL1_DMONOMIX_LLRR | ADDITIONAL1_VSEL_DEFAULT);

    wmcodec_write(LEFTMIX1, LEFTMIX1_LD2LO | LEFTMIX1_LI2LO_DEFAULT);
    wmcodec_write(RIGHTMIX2, RIGHTMIX2_RD2RO | RIGHTMIX2_RI2RO_DEFAULT);

#ifdef TOSHIBA_GIGABEAT_F
#ifdef HAVE_HARDWARE_BEEP
    /* Single-ended mono input */
    wmcodec_write(MONOMIX1, 0);

    /* Route mono input to both outputs at 0dB */
    wmcodec_write(LEFTMIX2, LEFTMIX2_MI2LO | LEFTMIX2_MI2LOVOL(2));
    wmcodec_write(RIGHTMIX1, RIGHTMIX1_MI2RO | RIGHTMIX1_MI2ROVOL(2));
#endif
#endif

    /* lower power consumption */
    wmcodec_write(PWRMGMT1, PWRMGMT1_VREF | PWRMGMT1_VMIDSEL_50K);

    audiohw_mute(false);

#ifdef MROBE_100
    /* enable headphone output */
    GPIOL_OUTPUT_VAL &= ~0x10;
    GPIOL_OUTPUT_EN  |=  0x10;
#endif

#ifdef MPIO_HD200
   /* enable headphone output */
   or_l((1<<25),&GPIO1_OUT);
#endif
}

void audiohw_set_master_vol(int vol_l, int vol_r)
{
    /* +6 to -73dB 1dB steps (plus mute == 80levels) 7bits */
    /* 1111111 ==  +6dB                                    */
    /* 1111001 ==   0dB                                    */
    /* 0110000 == -73dB                                    */
    /* 0101111 == mute (0x2f)                              */

    wmcodec_write(LOUT1, LOUT1_BITS | LOUT1_LOUT1VOL(vol_l));
    wmcodec_write(ROUT1, ROUT1_BITS | ROUT1_ROUT1VOL(vol_r));
}

#ifndef MROBE_100
void audiohw_set_lineout_vol(int vol_l, int vol_r)
{
    wmcodec_write(LOUT2, LOUT2_BITS | LOUT2_LOUT2VOL(vol_l));
    wmcodec_write(ROUT2, ROUT2_BITS | ROUT2_ROUT2VOL(vol_r));
}
#endif

void audiohw_set_bass(int value)
{
    wmcodec_write(BASSCTRL, BASSCTRL_BITS |

#ifdef USE_ADAPTIVE_BASS
        BASSCTRL_BASS(adaptivebass2hw(value)));
#else
        BASSCTRL_BASS(tone_tenthdb2hw(value)));
#endif
}

void audiohw_set_treble(int value)
{
    wmcodec_write(TREBCTRL, TREBCTRL_BITS |
        TREBCTRL_TREB(tone_tenthdb2hw(value)));
}

void audiohw_set_prescaler(int value)
{
    prescaler = 3 * value / 15;
    wmcodec_write(LEFTGAIN, 0xff - (prescaler & LEFTGAIN_LDACVOL));
    wmcodec_write(RIGHTGAIN, RIGHTGAIN_RDVU |
                  (0xff - (prescaler & RIGHTGAIN_RDACVOL)));
}

/* Nice shutdown of WM8751 codec */
void audiohw_close(void)
{
    /* 1. Set DACMU = 1 to soft-mute the audio DACs. */
    audiohw_mute(true);

#ifdef MPIO_HD200
    /* disable headphone out */
    and_l(~(1<<25), &GPIO1_OUT);
#endif

    /* 2. Disable all output buffers. */
    wmcodec_write(PWRMGMT2, 0x0);

    /* 3. Switch off the power supplies. */
    wmcodec_write(PWRMGMT1, 0x0);
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

    wmcodec_write(CLOCKING, srctrl_table[fsel]);
}

#if defined(HAVE_WM8750)
#ifdef AUDIOHW_HAVE_DEPTH_3D
/* Set the depth of the 3D effect */
void audiohw_set_depth_3d(int val)
{
    if (val)
        wmcodec_write(ENHANCE_3D,
                      ENHANCE_3D_MODE3D_PLAYBACK | ENHANCE_3D_DEPTH(val) |
                      ENHANCE_3D_3DEN);
    else
        wmcodec_write(ENHANCE_3D,ENHANCE_3D_MODE3D_PLAYBACK);
}
#endif

#ifdef HAVE_RECORDING
#if 0
void audiohw_set_ngath(int ngath, int type, bool enable)
{
    /* This function controls Noise gate function
     * of the codec. This can only run in conjunction
     * with ALC
     */

    if(enable)
        wmcodec_write(NGAT, NGAT_NGG(type)|NGAT_NGTH(ngath)|NGAT_NGAT);
    else
        wmcodec_write(NGAT, NGAT_NGG(type)|NGAT_NGTH(ngath_tenthdb2hw(ngath)));
}


void audiohw_set_alc(int level, unsigned int hold, int decay, int attack, bool enable)
{
    /* level in thenth of dB -28.5dB - -6dB in 1.5dB steps
     * hold time in us 0us - 43691000us
     * decay time in ms 24ms - 24580ms
     * attack time in ms 6ms - 6140ms
     */

    if(enable)
    {
        wmcodec_write(ALC1, ALC1_ALCSEL_STEREO|ALC1_MAXGAIN(0x07)|
                      ALC1_ALCL(alc_tenthdb2hw(level)));
        wmcodec_write(ALC2, ALC2_ALCZ|ALC2_HLD(alc_hldus2hw(hold)));
        wmcodec_write(ALC3, ALC3_DCY(alc_dcyms2hw(decay))|
                      ALC3_ATK(alc_atkms2hw(attack)));
    }
    else
    {
        wmcodec_write(ALC1, ALC1_ALCSEL_DISABLED|ALC1_MAXGAIN(0x07)|ALC1_ALCL(alc_tenthdb2hw(level)));
    }
}

void audiohw_set_alc(int level, unsigned int hold, int decay, int attack, bool enable)
{
    /* level in thenth of dB -28.5dB - -6dB in 1.5dB steps
     * hold time in 15 steps 0ms,2.67ms,5.33ms,...,43691ms
     * decay time in 10 steps 24ms,48ms,96ms,...,24580ms
     * attack time in 10 steps 6ms,12ms,24ms,...,6140ms
     */

    if(enable)
    {
        wmcodec_write(ALC1, ALC1_ALCSEL_STEREO|ALC1_MAXGAIN(0x07)|
                      ALC1_ALCL(alc_tenthdb2hw(level)));
        wmcodec_write(ALC2, ALC2_ALCZ|ALC2_HLD(hold));
        wmcodec_write(ALC3, ALC3_DCY(decay)|
                      ALC3_ATK(attack));
    }
    else
    {
        wmcodec_write(ALC1, ALC1_ALCSEL_DISABLED|ALC1_MAXGAIN(0x07)|
                      ALC1_ALCL(alc_tenthdb2hw(level)));
    }
}

void audiohw_set_alc_level(int level)
{
    wmcodec_write(ALC1, ALC1_ALCSEL_STEREO|ALC1_MAXGAIN(0x07)|
                  ALC1_ALCL(alc_tenthdb2hw(level)));
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
        /* turn off DAC and ADC in order to setup Enchance 3D function
         * for playback. This does not turn on enchancement but
         * the switch between playback/record has to be done with
         * DAC and ADC off
         */
#ifdef AUDIOHW_HAVE_DEPTH_3D
        wmcodec_write(PWRMGMT1, PWRMGMT1_VREF | PWRMGMT1_VMIDSEL_50K);
        wmcodec_write(PWRMGMT2, 0x00);
        wmcodec_write(ENHANCE_3D, ENHANCE_3D_MODE3D_PLAYBACK);
#endif
        /* mute PGA, disable all audio paths but DAC and output stage*/
        wmcodec_write(LINVOL, LINVOL_LINMUTE | LINVOL_LINVOL(0x17)); /* 0dB */
        wmcodec_write(RINVOL, RINVOL_RINMUTE | RINVOL_RINVOL(0x17)); /* 0dB */

        wmcodec_write(LOUT1, LOUT1_BITS);
        wmcodec_write(ROUT1, ROUT1_BITS);

        wmcodec_write(PWRMGMT2, PWRMGMT2_DACL | PWRMGMT2_DACR |
                      PWRMGMT2_LOUT1 | PWRMGMT2_ROUT1);

        /* route DAC signal to output mixer */
        wmcodec_write(LEFTMIX1, LEFTMIX1_LD2LO);
        wmcodec_write(RIGHTMIX2, RIGHTMIX2_RD2RO);

        /* unmute DAC */
        audiohw_mute(false);
        break;

    case AUDIO_SRC_FMRADIO:
        if(recording)
        {
            /* turn off DAC and ADC in order to setup Enchance 3D function
             * for playback. This does not turn on enchancement but
             * the switch between playback/record has to be done with
             * DAC and ADC off
             */
#ifdef AUDIOHW_HAVE_DEPTH_3D
              wmcodec_write(PWRMGMT1, PWRMGMT1_VREF | PWRMGMT1_VMIDSEL_50K);
              wmcodec_write(PWRMGMT2, 0x00);
              wmcodec_write(ENHANCE_3D, ENHANCE_3D_MODE3D_RECORD);
#endif

            /* Turn on PGA and ADC */
            wmcodec_write(PWRMGMT1, PWRMGMT1_VREF | PWRMGMT1_VMIDSEL_50K |
                          PWRMGMT1_AINL | PWRMGMT1_AINR | 
                          PWRMGMT1_ADCL | PWRMGMT1_ADCR);

            /* Set input volume to PGA 0dB*/
            wmcodec_write(LINVOL, LINVOL_LIVU|LINVOL_LINVOL(0x17));
            wmcodec_write(RINVOL, RINVOL_RIVU|RINVOL_RINVOL(0x17));

            /* Setup input source for PGA as INPUT1 
             * MICBOOST disabled
             */
            wmcodec_write(ADCL, ADCL_LINSEL_LINPUT1 | ADCL_LMICBOOST_DISABLED);
            wmcodec_write(ADCR, ADCR_RINSEL_RINPUT1 | ADCR_RMICBOOST_DISABLED);

            /* setup output digital data
             * default is LADC -> LDATA, RADC -> RDATA
             * so we don't touch this
             */

            /* power up DAC and output stage */
            wmcodec_write(PWRMGMT2, PWRMGMT2_DACL | PWRMGMT2_DACR |
                          PWRMGMT2_LOUT1 | PWRMGMT2_ROUT1);

            /* route DAC signal to output mixer */
            wmcodec_write(LEFTMIX1, LEFTMIX1_LD2LO);
            wmcodec_write(RIGHTMIX2, RIGHTMIX2_RD2RO);
        }
        else
        {
            /* turn off ADC, PGA */
            wmcodec_write(PWRMGMT1, PWRMGMT1_VREF | PWRMGMT1_VMIDSEL_50K);

           /* turn on DAC and output stage */
            wmcodec_write(PWRMGMT2, PWRMGMT2_DACL | PWRMGMT2_DACR |
                          PWRMGMT2_LOUT1 | PWRMGMT2_ROUT1);

           /* setup monitor mode by routing input signal to outmix 
            * at 0dB volume
            */
            wmcodec_write(LEFTMIX1, LEFTMIX1_LI2LO | LEFTMIX1_LMIXSEL_LINPUT1 |
                          LEFTMIX1_LI2LOVOL(0x20) | LEFTMIX1_LD2LO);
            wmcodec_write(RIGHTMIX1, RIGHTMIX1_RMIXSEL_RINPUT1);
            wmcodec_write(RIGHTMIX2, RIGHTMIX2_RI2RO |
                          RIGHTMIX2_RI2ROVOL(0x20) | RIGHTMIX2_RD2RO);
        }
        break;

#if (INPUT_SRC_CAPS & SRC_CAP_LINEIN)
    case AUDIO_SRC_LINEIN:
#ifdef AUDIOHW_HAVE_DEPTH_3D
        wmcodec_write(PWRMGMT1, PWRMGMT1_VREF | PWRMGMT1_VMIDSEL_50K);
        wmcodec_write(PWRMGMT2, 0x00);
        wmcodec_write(ENHANCE_3D, ENHANCE_3D_MODE3D_RECORD);
#endif

        /* Set input volume to PGA */
        wmcodec_write(LINVOL, LINVOL_LIVU | LINVOL_LINVOL(23));
        wmcodec_write(RINVOL, RINVOL_RIVU | RINVOL_RINVOL(23));

        /* Turn on PGA, ADC */
        wmcodec_write(PWRMGMT1, PWRMGMT1_VREF | PWRMGMT1_VMIDSEL_50K |
                      PWRMGMT1_AINL | PWRMGMT1_AINR | 
                      PWRMGMT1_ADCL | PWRMGMT1_ADCR);

        /* turn on DAC and output stage */
        wmcodec_write(PWRMGMT2, PWRMGMT2_DACL | PWRMGMT2_DACR |
                      PWRMGMT2_LOUT1 | PWRMGMT2_ROUT1);

        /* Setup input source for PGA as INPUT2 
         * MICBOOST disabled
         */
        wmcodec_write(ADCL, ADCL_LINSEL_LINPUT2 | ADCL_LMICBOOST_DISABLED);
        wmcodec_write(ADCR, ADCR_RINSEL_RINPUT2 | ADCR_RMICBOOST_DISABLED);

        /* setup output digital data
         * default is LADC -> LDATA, RADC -> RDATA
         * so we don't touch this
         */
        /* route DAC signal to output mixer */
        wmcodec_write(LEFTMIX1, LEFTMIX1_LD2LO);
        wmcodec_write(RIGHTMIX2, RIGHTMIX2_RD2RO);
        break;
#endif
#if (INPUT_SRC_CAPS & SRC_CAP_MIC)
    case AUDIO_SRC_MIC:
#ifdef AUDIOHW_HAVE_DEPTH_3D
          wmcodec_write(PWRMGMT1, PWRMGMT1_VREF | PWRMGMT1_VMIDSEL_50K);
          wmcodec_write(PWRMGMT2, 0x00);
          wmcodec_write(ENHANCE_3D, ENHANCE_3D_MODE3D_RECORD);
#endif

        /* Set input volume to PGA */
        wmcodec_write(LINVOL, LINVOL_LIVU | LINVOL_LINVOL(23));
        wmcodec_write(RINVOL, RINVOL_RIVU | RINVOL_RINVOL(23));

        /* Turn on PGA and ADC, turn off DAC */
        wmcodec_write(PWRMGMT1, PWRMGMT1_VREF | PWRMGMT1_VMIDSEL_50K |
                      PWRMGMT1_AINL | PWRMGMT1_AINR | 
                      PWRMGMT1_ADCL | PWRMGMT1_ADCR);

        /* turn on DAC and output stage */
        wmcodec_write(PWRMGMT2, PWRMGMT2_DACL | PWRMGMT2_DACR |
                      PWRMGMT2_LOUT1 | PWRMGMT2_ROUT1);

        /* Setup input source for PGA as INPUT3 
         * MICBOOST disabled
         */
        wmcodec_write(ADCL, ADCL_LINSEL_LINPUT3 | ADCL_LMICBOOST_DISABLED);
        wmcodec_write(ADCR, ADCR_RINSEL_RINPUT3 | ADCR_RMICBOOST_DISABLED);

        /* setup output digital data
         * default is LADC -> LDATA, RADC -> RDATA
         * so we don't touch this
         */

        /* route DAC signal to output mixer */
        wmcodec_write(LEFTMIX1, LEFTMIX1_LD2LO);
        wmcodec_write(RIGHTMIX2, RIGHTMIX2_RD2RO);
        break;
#endif
    } /* switch(source) */
}

/* Setup PGA gain */
void audiohw_set_recvol(int vol_l, int vol_r, int type)
{
    (void)type;
    wmcodec_write(LINVOL, LINVOL_LIZC|LINVOL_LINVOL(recvol2hw(vol_l)));
    wmcodec_write(RINVOL, RINVOL_RIZC|RINVOL_RIVU|RINVOL_RINVOL(recvol2hw(vol_r)));
}
#endif /* HAVE_RECORDING */
#endif /* HAVE_WM8750 */
