/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2010 by Thomas Martitz
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
#ifndef __LOAD_CODE_H__
#define __LOAD_CODE_H__

#include "config.h"

/* this struct needs to be the first part of other headers
 * (lc_open() casts the other header to this one to load to the correct
 * address)
 */
struct lc_header {
    unsigned long magic;
    unsigned short target_id;
    unsigned short api_version;
    unsigned char *load_addr;
    unsigned char *end_addr;
};

#if CONFIG_BINFMT == BINFMT_ROCK
# include "lc-rock.h"
#elif CONFIG_BINFMT == BINFMT_DLOPEN
# include "lc-dlopen.h"
#else
# error "Unsupported CONFIG_BINFMT!"
#endif

#endif /* __LOAD_CODE_H__ */
