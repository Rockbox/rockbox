/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2013 Andrew Ryabinin
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
#include "kernel.h"
#include "audiohw.h"
#include "df1704.h"
#include "pca9555.h"
#include "i2c-rk27xx.h"

void df1704_set_ml_dir(const int dir)
{
    pca9555_write_config(dir<<8, (1<<8));
}
void pcm1792_set_ml_dir (const int) __attribute__((alias("df1704_set_ml_dir")));

void df1704_set_ml(const int val)
{
    pca9555_write_output(val<<8, 1<<8);
}
void pcm1792_set_ml (const int) __attribute__((alias("df1704_set_ml")));

void df1704_set_mc(const int val)
{
    pca9555_write_output(val<<1, 1<<1);
}
void pcm1792_set_mc (const int) __attribute__((alias("df1704_set_mc")));

void df1704_set_md(const int val)
{
    pca9555_write_output(val<<0, 1<<0);
}
void pcm1792_set_md (const int) __attribute__((alias("df1704_set_md")));

static void pop_ctrl(const int val)
{
    pca9555_write_output(val<<5, 1<<5);
}

static void amp_enable(const int val)
{
    pca9555_write_output(val<<3, 1<<3);
}

static void dac_enable(const int val)
{
    pca9555_write_output(val<<4, 1<<4);
}

void audiohw_postinit(void)
{
    pop_ctrl(0);
    sleep(HZ/4);
    dac_enable(1);
    amp_enable(1);
    sleep(HZ/100);
    audiohw_init();
    sleep(HZ/4);
    pop_ctrl(1);
}

void audiohw_close(void)
{
    audiohw_mute();
    pop_ctrl(0);
    sleep(HZ/5);
    amp_enable(0);
    dac_enable(0);
    sleep(HZ/5);
    pop_ctrl(1);
}
