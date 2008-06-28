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
 
 #include "i2c-dm320.h"
 
 #define I2C_ADDRESS 0x51
 
 unsigned char* rtc_send_command(short unk1, short unk2)
 {
    unsigned char ret[12];
    i2c_write(I2C_ADDRESS, (unk2 & 0xFF) | (unk << 8), 1);
    i2c_read(I2C_ADDRESS, ret, 12);
    return ret;
 }
 
 unsigned char* rtc_read(void)
 {
    unsigned char ret[12];
    i2c_read(I2C_ADDRESS, ret, 12);
    return ret;
 }