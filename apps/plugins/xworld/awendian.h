/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2014 Franklin Wei, Benjamin Brown
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ***************************************************************************/

#ifndef __SYS_H__
#define __SYS_H__

#include "rbendian.h"
#include "stdint.h"


#ifdef ROCKBOX_LITTLE_ENDIAN
#define SYS_LITTLE_ENDIAN
#else
#define SYS_BIG_ENDIAN
#endif


#if defined SYS_LITTLE_ENDIAN
#define READ_BE_UINT16(p) ((((const uint8_t*)p)[0] << 8) | ((const uint8_t*)p)[1])
#define READ_BE_UINT32(p) ((((const uint8_t*)p)[0] << 24) | (((const uint8_t*)p)[1] << 16) | (((const uint8_t*)p)[2] << 8) | ((const uint8_t*)p)[3])

#elif defined SYS_BIG_ENDIAN

#define READ_BE_UINT16(p) (*(const uint16_t*)p)
#define READ_BE_UINT32(p) (*(const uint32_t*)p)

#else

#error No endianness defined

#endif

#endif
