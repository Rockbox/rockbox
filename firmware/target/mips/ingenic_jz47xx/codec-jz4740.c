/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2008 by Maurus Cuelenaere
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
#include "jz4740.h"
#include "system.h"
#include "audiohw.h"

/* TODO */
const struct sound_settings_info audiohw_settings[] = {
    [SOUND_VOLUME]        = {"dB", 0,  1, -73,   6, -20},
    /* HAVE_SW_TONE_CONTROLS */
    [SOUND_BASS]          = {"dB", 0,  1, -24,  24,   0},
    [SOUND_TREBLE]        = {"dB", 0,  1, -24,  24,   0},
    [SOUND_BALANCE]       = {"%",  0,  1,-100, 100,   0},
    [SOUND_CHANNELS]      = {"",   0,  1,   0,   5,   0},
    [SOUND_STEREO_WIDTH]  = {"%",  0,  5,   0, 250, 100},
#ifdef HAVE_RECORDING
    [SOUND_LEFT_GAIN]     = {"dB", 1,  1,   0,  31,  23},
    [SOUND_RIGHT_GAIN]    = {"dB", 1,  1,   0,  31,  23},
    [SOUND_MIC_GAIN]      = {"dB", 1,  1,   0,   1,   1},
#endif
};

static unsigned short codec_volume;
static unsigned short codec_base_gain;
static unsigned short codec_mic_gain;
static bool HP_on_off_flag; 
static int HP_register_value; 

static void i2s_codec_set_samplerate(unsigned short rate);

static void i2s_codec_reset(void)
{
    REG_ICDC_CDCCR1 = (ICDC_CDCCR1_SW2ON | ICDC_CDCCR1_PDVR | ICDC_CDCCR1_PDVRA | ICDC_CDCCR1_VRCGL |
                       ICDC_CDCCR1_VRCGH | ICDC_CDCCR1_HPOV0 | ICDC_CDCCR1_PDHPM | ICDC_CDCCR1_PDHP |
                       ICDC_CDCCR1_SUSPD | ICDC_CDCCR1_RST);
    udelay(10);
    REG_ICDC_CDCCR1 = (ICDC_CDCCR1_SW2ON | ICDC_CDCCR1_PDVR | ICDC_CDCCR1_PDVRA | ICDC_CDCCR1_VRCGL |
                       ICDC_CDCCR1_VRCGH | ICDC_CDCCR1_HPOV0 | ICDC_CDCCR1_PDHPM | ICDC_CDCCR1_PDHP );
}

static void i2s_codec_init(void)
{
    __cpm_start_aic1();
    __cpm_start_aic2();
    
    __aic_enable();
    
    __i2s_internal_codec();
    __i2s_as_slave();
    __i2s_select_i2s();
    __aic_select_i2s(); 
    
    __aic_disable_byteswap();
    __aic_disable_unsignadj();
    __aic_disable_mono2stereo();
    
    i2s_codec_reset();
    
    //REG_ICDC_CDCCR2 = (ICDC_CDCCR2_AINVOL(ICDC_CDCCR2_AINVOL_DB(0)) | ICDC_CDCCR2_SMPR(ICDC_CDCCR2_SMPR_48)
    REG_ICDC_CDCCR2 = ( ICDC_CDCCR2_AINVOL(23) | ICDC_CDCCR2_SMPR(ICDC_CDCCR2_SMPR_44)
                      | ICDC_CDCCR2_HPVOL(ICDC_CDCCR2_HPVOL_6));
    
    REG_ICDC_CDCCR1 &= ~(ICDC_CDCCR1_SUSPD | ICDC_CDCCR1_RST);

    mdelay(15);
    REG_ICDC_CDCCR1 &= ~(ICDC_CDCCR1_PDVR | ICDC_CDCCR1_VRCGL | ICDC_CDCCR1_VRCGH);
    REG_ICDC_CDCCR1 |= (ICDC_CDCCR1_EDAC | ICDC_CDCCR1_HPCG);
    
    mdelay(600);
    REG_ICDC_CDCCR1 &= ~(ICDC_CDCCR1_PDVRA | ICDC_CDCCR1_HPCG | ICDC_CDCCR1_PDHPM | ICDC_CDCCR1_PDHP);
    
    mdelay(2);
    
    /* CDCCR1.ELININ=0, CDCCR1.EMIC=0, CDCCR1.EADC=0, CDCCR1.SW1ON=0, CDCCR1.EDAC=1, CDCCR1.SW2ON=1, CDCCR1.HPMUTE=0 */
    REG_ICDC_CDCCR1 = (REG_ICDC_CDCCR1 & ~(ICDC_CDCCR1_ELININ | ICDC_CDCCR1_EMIC | ICDC_CDCCR1_EADC |
                                           ICDC_CDCCR1_SW1ON | ICDC_CDCCR1_HPMUTE)) | (ICDC_CDCCR1_EDAC
                                                                                    | ICDC_CDCCR1_SW2ON);
    
    REG_ICDC_CDCCR2 |= 3;
    
    HP_on_off_flag = 1; /* HP is on */
}

static void i2s_codec_set_mic(unsigned short v) /* 0 <= v <= 100 */
{
    v &= 0xff;

    if(v > 100)
        v = 100; 
    codec_mic_gain = 31 * v/100;
    
    REG_ICDC_CDCCR2 = ((REG_ICDC_CDCCR2 & ~(0x1f << 16)) | (codec_mic_gain << 16));
}

static void i2s_codec_set_bass(unsigned short v) /* 0 <= v <= 100 */
{
    v &= 0xff;

    if(v > 100)
        v = 100;

    if(v < 25)
        codec_base_gain = 0;
    if(v >= 25 && v < 50)
        codec_base_gain = 1;
    if(v >= 50 && v < 75)
        codec_base_gain = 2;
    if(v >= 75 && v <= 100)
        codec_base_gain = 3;
    
    REG_ICDC_CDCCR2 = ((REG_ICDC_CDCCR2 & ~(0x3 << 4)) | (codec_base_gain << 4));
}

static void i2s_codec_set_volume(unsigned short v) /* 0 <= v <= 100 */
{
    v &= 0xff;

    if(v > 100)
        v = 100;

    if(v < 25)
        codec_volume = 0;
    if(v >= 25 && v < 50)
        codec_volume = 1;
    if(v >= 50 && v < 75)
        codec_volume = 2;
    if(v >= 75 && v <= 100)
        codec_volume = 3;
    
    REG_ICDC_CDCCR2 = ((REG_ICDC_CDCCR2 & ~(0x3)) | codec_volume);
}

static unsigned short i2s_codec_get_bass(void)
{
    unsigned short val;
    int ret;
    
    if(codec_base_gain == 0)
        val = 0;
    if(codec_base_gain == 1)
        val = 25;
    if(codec_base_gain == 2)
        val = 50;
    if(codec_base_gain == 3)
        val = 75;
    
    ret = val << 8;
    val |= ret;
    
    return val;
}

static unsigned short i2s_codec_get_mic(void)
{
    unsigned short val;
    int ret;
    val = 100 * codec_mic_gain / 31;
    ret = val << 8;
    val |= ret;
    
    return val;
}

static unsigned short i2s_codec_get_volume(void)
{
    unsigned short val;
    int ret;

    if(codec_volume == 0)
        val = 0;
    if(codec_volume == 1)
        val = 25;
    if(codec_volume == 2)
        val = 50;
    if(codec_volume == 3)
        val = 75;
    
    ret = val << 8;
    val |= ret;
    
    return val;
}

static void i2s_codec_set_samplerate(unsigned short rate)
{
    unsigned short speed = 0;
    
    switch (rate)
    {
        case 8000:
            speed = 0 << 8;
            break;
        case 11025:
            speed = 1 << 8;
            break;
        case 12000:
            speed = 2 << 8;
            break;
        case 16000:
            speed = 3 << 8;
            break;
        case 22050:
            speed = 4 << 8;
            break;
        case 24000:
            speed = 5 << 8;
            break;
        case 32000:
            speed = 6 << 8;
            break;
        case 44100:
            speed = 7 << 8;
            break;
        case 48000:
            speed = 8 << 8;
            break;
        default:
            break;
    }
    REG_ICDC_CDCCR2 |= 0x00000f00;

    speed |= 0xfffff0ff;
    REG_ICDC_CDCCR2 &= speed;
}

static void HP_turn_on(void)
{ 
    //see 1.3.4.1
    
    REG_ICDC_CDCCR1 &= ~(ICDC_CDCCR1_SUSPD | ICDC_CDCCR1_RST); //set suspend 0

    mdelay(15);
    REG_ICDC_CDCCR1 &= ~(ICDC_CDCCR1_PDVR | ICDC_CDCCR1_VRCGL | ICDC_CDCCR1_VRCGH);
    REG_ICDC_CDCCR1 |= (ICDC_CDCCR1_EDAC | ICDC_CDCCR1_HPCG);
    
    mdelay(600);
    REG_ICDC_CDCCR1 &= ~(ICDC_CDCCR1_PDVRA | ICDC_CDCCR1_HPCG | ICDC_CDCCR1_PDHPM | ICDC_CDCCR1_PDHP);
    
    mdelay(2);
    HP_register_value = REG_ICDC_CDCCR1; 

    //see 1.3.4.2
    /*REG_ICDC_CDCCR1 &= 0xfffffffc;
    mdelay(7);
    REG_ICDC_CDCCR1 |= 0x00040400;
    mdelay(15);
    REG_ICDC_CDCCR1 &= 0xfffbfbff;
    udelay(500);
    REG_ICDC_CDCCR1 &= 0xffe5fcff;
    REG_ICDC_CDCCR1 |= 0x01000000;
    mdelay(400);
    REG_ICDC_CDCCR1 &= 0xfffeffff;
    mdelay(7);
    HP_register_value = REG_ICDC_CDCCR1;*/

    //see 1.3.4.3
    
}


static void HP_turn_off(void)
{
    //see 1.3.4.1
    mdelay(2);
    REG_ICDC_CDCCR1 = HP_register_value;
    REG_ICDC_CDCCR1 |= 0x001b0300;
    REG_ICDC_CDCCR1 &= 0xfeffffff;

    mdelay(15);
    REG_ICDC_CDCCR1 |= 0x00000002;//set suspend 1

    //see 1.3.4.2
    /*mdelay(4);
    REG_ICDC_CDCCR1 = HP_register_value;
    REG_ICDC_CDCCR1 |= 0x001b0300;
    REG_ICDC_CDCCR1 &= 0xfeffffff;
    mdelay(4);
    REG_ICDC_CDCCR1 |= 0x00000400;
    mdelay(15);
    REG_ICDC_CDCCR1 &= 0xfffffdff;
    mdelay(7);
    REG_ICDC_CDCCR1 |= 0x00000002;*/
    
    //see 1.3.4.3

}

void audiohw_mute(bool mute)
{
    if(mute)
        REG_ICDC_CDCCR1 |= ICDC_CDCCR1_HPMUTE;
    else
        REG_ICDC_CDCCR1 &= ~ICDC_CDCCR1_HPMUTE;
}

void audiohw_preinit(void)
{
}

void audiohw_postinit(void)
{
    audiohw_mute(false);
}

void audiohw_init(void)
{
    i2s_codec_init();
}

void audiohw_set_frequency(int freq)
{
    i2s_codec_set_samplerate(freq);
}
