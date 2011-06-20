/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
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
#ifndef BUFFER_H
#define BUFFER_H

#include "config.h"
/* defined in linker script */
#if (CONFIG_PLATFORM & PLATFORM_NATIVE)
#if defined(IPOD_VIDEO)
extern unsigned char *audiobufend_lds[];
unsigned char *audiobufend;
#else
extern unsigned char audiobufend[];
#endif
#else
extern unsigned char *audiobufend;
#endif

extern unsigned char *audiobuf;

void buffer_init(void) INIT_ATTR;
void *buffer_alloc(size_t size);

#ifdef BUFFER_ALLOC_DEBUG
void buffer_alloc_check(char *name);
#endif

#endif
