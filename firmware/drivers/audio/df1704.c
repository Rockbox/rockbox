/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 *
 * Copyright (c) 2011 Andrew Ryabinin
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

#include "system.h"
#include "i2c-rk27xx.h"
#include "df1704.h"
#include "pca9555.h"

#define ML_BIT		8
#define MC_BIT		1
#define MD_BIT		0

static int df1704_set_ML_dir(unsigned char val)
{
    int ret=0;
    unsigned char data[2] = {0, 0};

    ret = i2c_read(0x40, 7, 2, data);	//IN/OUT cfg
    if(val==0)	// 0: input
        data[0] |= 0x01;
    else			// 1: output
        data[0] &= 0xfe;
    ret = i2c_write(0x40, 7, 2, data);	//IN/OUT cfg

    return ret;
}

static int df1704_set_ctrl_pin(unsigned char pin, int val)
{
    int ret=0;
    unsigned char data[2] = {0, 0};

    ret = i2c_read(0x40, 2, 2, data);
    if(pin<8) {
        data[0] = (data[0] & (0xff^(1<<pin)))|(val<<pin);
    }
    else {
        pin -= 8;
        data[1] = (data[1] & (0xff^(1<<pin)))|(val<<pin);
    }
    ret = i2c_write(0x40, 2, 2, data);

    return ret;
}


#define SET_ML_IN	df1704_set_ML_dir(0)
#define SET_ML_OUT	df1704_set_ML_dir(1)
#define SET_ML(val)	do{SET_ML_OUT;	df1704_set_ctrl_pin(ML_BIT, val);}while(0)
#define SET_MC(val)	df1704_set_ctrl_pin(MC_BIT, val)
#define SET_MD(val)	df1704_set_ctrl_pin(MD_BIT, val)
#define DF1704_DELAY	do{for(int i=0; i<100; i++);}while(0)


int df1704_write_mode_reg(unsigned int value)
{
    int ret=0;
    int i;
    unsigned int tmp = value;

    SET_ML(1);
    SET_MD(1);
    SET_MC(1);

    for(i=15; i>=0; i--){
        DF1704_DELAY;
        SET_MC(0);
        if(tmp&(1<<i))
            SET_MD(1);
        else
            SET_MD(0);
        DF1704_DELAY;
        SET_MC(1);
    }
    SET_ML(0);
    DF1704_DELAY;
    DF1704_DELAY;
    SET_ML(1);
    DF1704_DELAY;
    DF1704_DELAY;
    SET_ML_IN;
    return ret;
}

void Pop_Ctrl(unsigned char en)
{
    int ret=0;
    unsigned char data[2] = {0, 0};

    ret = i2c_read(0x40, 2, 2, data);
    if(en)
        data[0] = data[0]&(~0x20);
    else
        data[0] = data[0]|0x20;
    ret = i2c_write(0x40, 2, 2, data);
}

int df1704_init()
{
    int ret=0;
    df1704_write_mode_reg(0x01ff);	//MODE0 reg
    df1704_write_mode_reg(0x0461);	//MODE2 reg
    df1704_write_mode_reg(0x0627);	//MODE3 reg
    return ret;
}

void df1704_pwren(unsigned char en)
{
    int ret=0;
    unsigned char data[2] = {0, 0};

    ret = i2c_read(0x40, 2, 2, data);
    if(en)
        data[0] = data[0]|0x10;
    else
        data[0] = data[0]&(~0x10);
    ret = i2c_write(0x40, 2, 2, data);
}

void amp_pwren(unsigned char en)
{
    int ret=0;
    unsigned char data[2] = {0, 0};

    ret = i2c_read(0x40, 2, 2, data);
    if(en)
        data[0] = data[0]|0x08;
    else
        data[0] = data[0]&(~0x08);
    ret = i2c_write(0x40, 2, 2, data);
}

void df1704_mute(void)
{
    df1704_write_mode_reg(0x0460);	//MODE2 reg
}

void df1704_unmute(void)
{
    df1704_write_mode_reg(0x0461);	//MODE2 reg
}

void df1704_setVol(int  Volume)
{
    df1704_write_mode_reg(0x0100+Volume);	//MODE0 reg
}




#include "config.h"
#include "audio.h"
#include "audiohw.h"

const struct sound_settings_info audiohw_settings[] = {
    [SOUND_VOLUME]        = {"dB", 0,  1,   VOLUME_MIN/10,   VOLUME_MAX/10,   -25},
    /* HAVE_SW_TONE_CONTROLS */
    [SOUND_BASS]          = {"dB", 0,  1, -24,  24,   0},
    [SOUND_TREBLE]        = {"dB", 0,  1, -24,  24,   0},
    [SOUND_BALANCE]       = {"%",  0,  1,-100, 100,   0},
    [SOUND_CHANNELS]      = {"",   0,  1,   0,   5,   0},
    [SOUND_STEREO_WIDTH]  = {"%",  0,  5,   0, 250, 100},
};


void audiohw_preinit(void)
{
}

void audiohw_postinit(void)
{
    Pop_Ctrl(1);
    sleep(HZ/4);
    df1704_pwren(1);
    amp_pwren(1);
    sleep(HZ/100);
    df1704_init();
    df1704_setVol((28<<2)+0x7f);
    sleep(HZ/4);
    Pop_Ctrl(0);
}

void audiohw_close(void)
{
}

void audiohw_set_frequency(int fsel)
{
    (void)fsel;
}

void audiohw_set_volume(int v)
{
    df1704_setVol(v);
}
