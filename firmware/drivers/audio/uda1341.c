/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id: uda1380.c 21975 2009-07-19 22:45:32Z bertrik $
 *
 * Copyright (C) 2009 by Bob Cousins
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
#include <string.h>

#include "config.h"
#include "logf.h"
#include "system.h"
#include "audio.h"
#include "debug.h"

#include "audiohw.h"


const struct sound_settings_info audiohw_settings[] = {
    [SOUND_VOLUME]        = {"dB", 0,  1, -84,   0, -25},
    [SOUND_BASS]          = {"dB", 0,  2,   0,  24,   0},
    [SOUND_TREBLE]        = {"dB", 0,  2,   0,   6,   0},
    [SOUND_BALANCE]       = {"%",  0,  1,-100, 100,   0},   /* not used */
    [SOUND_CHANNELS]      = {"",   0,  1,   0,   5,   0},   /* not used */
    [SOUND_STEREO_WIDTH]  = {"%",  0,  5,   0, 250, 100},   /* not used */
#ifdef HAVE_RECORDING
    [SOUND_LEFT_GAIN]     = {"dB", 1,  1,-128,  96,   0},
    [SOUND_RIGHT_GAIN]    = {"dB", 1,  1,-128,  96,   0},
    [SOUND_MIC_GAIN]      = {"dB", 1,  1,-128, 108,  16},
#endif
};

/* convert tenth of dB volume (-600..0) to master volume register value */
int tenthdb2master(int db)
{
    if (db < -600) 
        return 63;
    else                            /* 1 dB steps */
        return -(db / 10) + 1;
}

static unsigned short uda_regs[NUM_REG_ID];

/****************************************************************************/

/* ------------------------------------------------- */
/* Local functions and variables */
/* ------------------------------------------------- */

/* Generic L3 functions */

#define L3PORT      GPBDAT
#define L3MODE      (1 << 2)
#define L3DATA      (1 << 3)
#define L3CLOCK     (1 << 4)

static void l3_init (void)
{
    L3PORT |= L3MODE | L3CLOCK;
    L3PORT &= ~L3DATA; 

    S3C2440_GPIO_CONFIG (GPBCON, 2, GPIO_OUTPUT);   /* L3 MODE */
    S3C2440_GPIO_CONFIG (GPBCON, 3, GPIO_OUTPUT);   /* L3 DATA */
    S3C2440_GPIO_CONFIG (GPBCON, 4, GPIO_OUTPUT);   /* L3 CLOCK */
    
    S3C2440_GPIO_PULLUP (GPBUP, 2, GPIO_PULLUP_DISABLE);
    S3C2440_GPIO_PULLUP (GPBUP, 3, GPIO_PULLUP_DISABLE);
    S3C2440_GPIO_PULLUP (GPBUP, 4, GPIO_PULLUP_DISABLE);
}

static void bit_delay (void)
{
    volatile int j;
    for (j=0; j<5; j++)
        ;
}

static void l3_write_byte (unsigned char data, bool address_mode)
{
    int bit;
    
    L3PORT |= L3CLOCK;
    if (address_mode)
        L3PORT &= ~L3MODE;
    else
        L3PORT |= L3MODE;
    bit_delay();
    
    for (bit=0; bit < 8; bit++)
    {
        if (data & 1)
        {
            L3PORT |= L3DATA;
        }
        else
        {
            L3PORT &= ~L3DATA;
        }
        L3PORT &= ~L3CLOCK;
        bit_delay();
        L3PORT |= L3CLOCK;
        bit_delay();
        
        data >>= 1;
    }    
    
    if (address_mode)
        L3PORT |= L3MODE;
    else
        L3PORT &= ~L3MODE;
    bit_delay();
}

static void l3_write_addr (unsigned char addr)
{
    /* write address byte */
    l3_write_byte (addr, true);
}

static void l3_write_data (unsigned char data)
{
    /* write data byte */
    l3_write_byte (data, false);
}

/****************************************************************************/

/* UDA1341 access functions */

static int udacodec_write(unsigned char reg, unsigned short value)
{
    l3_write_addr (UDA1341_ADDR | reg);
    l3_write_data (value & 0xff);
    return 0;
}

static void udacodec_reset(void)
{
    /* uda reset */
    l3_init();
    
    udacodec_write (UDA_REG_STATUS, UDA_STATUS_0 | UDA_RESET | UDA_SYSCLK_256FS | 
                                    I2S_IFMT_IIS);
    udacodec_write (UDA_REG_STATUS, UDA_STATUS_0 | UDA_SYSCLK_256FS | I2S_IFMT_IIS);
    udacodec_write (UDA_REG_STATUS, UDA_STATUS_1 | UDA_POWER_DAC_ON);
    
    uda_regs[UDA_REG_ID_CTRL2] = UDA_PEAK_DETECT_POS_AFTER | 
            UDA_DE_EMPHASIS_NONE | UDA_MUTE_OFF | UDA_MODE_SWITCH_FLAT;
    
}

/****************************************************************************/

/* Audio API functions */

/* This table must match the table in pcm-xxxx.c if using Master mode */
/* [reserved, master clock rate] */
static const unsigned char uda_freq_parms[HW_NUM_FREQ][2] =
{
    [HW_FREQ_64] = { 0, UDA_SYSCLK_256FS },
    [HW_FREQ_44] = { 0, UDA_SYSCLK_384FS },
    [HW_FREQ_22] = { 0, UDA_SYSCLK_256FS },
    [HW_FREQ_11] = { 0, UDA_SYSCLK_256FS },
};

void audiohw_init(void)
{
    udacodec_reset();
     
    audiohw_set_bass (0);  
    audiohw_set_treble (0);  
    audiohw_set_master_vol (26, 26);    /* -25 dB */
}

void audiohw_postinit(void)
{
}

void audiohw_close(void)
{
    /* DAC, ADC off */
    udacodec_write (UDA_REG_STATUS, UDA_STATUS_1 | 0);
}

void audiohw_set_bass(int value)
{
    uda_regs [UDA_REG_ID_CTRL1] &= UDA_BASS_BOOST (UDA_BASS_BOOST_MASK);
    uda_regs [UDA_REG_ID_CTRL1] |= UDA_BASS_BOOST (value & UDA_BASS_BOOST_MASK);
     
    udacodec_write (UDA_REG_DATA0, UDA_DATA_CTRL1 | uda_regs [UDA_REG_ID_CTRL1] );
}

void audiohw_set_treble(int value)
{
    uda_regs [UDA_REG_ID_CTRL1] &= UDA_TREBLE (UDA_TREBLE_MASK);
    uda_regs [UDA_REG_ID_CTRL1] |= UDA_TREBLE (value & UDA_TREBLE_MASK);
     
    udacodec_write (UDA_REG_DATA0, UDA_DATA_CTRL1 | uda_regs [UDA_REG_ID_CTRL1] );
}

void audiohw_mute(bool mute)
{
    if (mute)
        uda_regs [UDA_REG_ID_CTRL2] |= UDA_MUTE_ON;
    else    
        uda_regs [UDA_REG_ID_CTRL2] &= ~UDA_MUTE_ON;
    
    udacodec_write (UDA_REG_DATA0, UDA_DATA_CTRL2 | uda_regs [UDA_REG_ID_CTRL2] );
}

void audiohw_set_prescaler(int val)
{
    (void)val;
}

/**
 * Sets left and right master volume  (1(max) to 62(muted))
 */
void audiohw_set_master_vol(int vol_l, int vol_r)
{
    uda_regs[UDA_REG_ID_CTRL0] = (vol_l + vol_r) / 2;
    udacodec_write (UDA_REG_DATA0, UDA_DATA_CTRL0 | uda_regs[UDA_REG_ID_CTRL0]);
}

void audiohw_set_frequency(int fsel)
{
    if ((unsigned)fsel >= HW_NUM_FREQ)
        fsel = HW_FREQ_DEFAULT;

    uda_regs[UDA_REG_ID_STATUS_0] = I2S_IFMT_IIS | uda_freq_parms[fsel][1];

    udacodec_write (UDA_REG_STATUS, UDA_STATUS_0 | uda_regs[UDA_REG_ID_STATUS_0]);
}
