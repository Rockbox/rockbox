/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2016 by Vortex
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
#include "i2c-rk27xx.h"

#define ES9018_I2C_ADDR    0x90

static unsigned char buf;

void es9018_write_reg(uint8_t reg, uint8_t val)
{
    buf = val;
    i2c_write(ES9018_I2C_ADDR, reg, sizeof(buf), (void*)&buf);
}

uint8_t es9018_read_reg(uint8_t reg)
{
    i2c_read(ES9018_I2C_ADDR, reg, sizeof(buf), (void*)&buf);
    return buf;
}

static void pop_ctrl(const int val)
{
    if (val)
        GPIO_PADR |= (1<<7);
    else
        GPIO_PADR &= ~(1<<7);
}

void audiohw_postinit(void)
{
    pop_ctrl(0);
    sleep(HZ/4);
    audiohw_init();
    sleep(HZ/2);
    pop_ctrl(1);
    sleep(HZ/4);
    audiohw_unmute();
}

void audiohw_close(void)
{
    audiohw_mute();
    pop_ctrl(0);
    sleep(HZ/4);
}
