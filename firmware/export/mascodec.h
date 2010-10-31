/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id: mas.h 24154 2010-01-03 10:27:43Z Buschel $
 *
 * Copyright (C) 2002 by Linus Nielsen Feltzing
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
#ifndef _MASCODEC_H_
#define _MASCODEC_H_

int mas_default_read(unsigned short *buf);
int mas_run(unsigned short address);
int mas_readmem(int bank, int addr, unsigned long* dest, int len);
int mas_writemem(int bank, int addr, const unsigned long* src, int len);
int mas_readreg(int reg);
int mas_writereg(int reg, unsigned int val);
void mas_reset(void);
int mas_direct_config_read(unsigned char reg);
int mas_direct_config_write(unsigned char reg, unsigned int val);
int mas_codec_writereg(int reg, unsigned int val);
int mas_codec_readreg(int reg);
unsigned long mas_readver(void);

#endif

#if CONFIG_TUNER & S1A0903X01
void mas_store_pllfreq(int freq);
int mas_get_pllfreq(void);
#endif

