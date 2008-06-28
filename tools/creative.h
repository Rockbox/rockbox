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

#ifndef CREATIVE_H_
#define CREATIVE_H_

enum
{
    ZENVISIONM = 0,
    ZENVISIONM60 = 1,
    ZENVISION = 2,
    ZENV = 3,
    ZEN = 4
};

struct device_info
{
    const char* cinf; /*Must be Unicode encoded*/
    const unsigned int cinf_size;
    const char* null;
};

int zvm_encode(const char *iname, const char *oname, int device);

#endif /*CREATIVE_H_*/
