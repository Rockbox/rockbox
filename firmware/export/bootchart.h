/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2010 by Torne Wuff
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
#ifndef BOOTCHART_H
#define BOOTCHART_H
#include <config.h>
#include <stdbool.h>
#include "../include/_ansi.h"
#include "logf.h"
#include "kernel.h"

#ifdef DO_BOOTCHART

/* we call _logf directly to avoid needing LOGF_ENABLE per-file */
#define CHART2(x,y) _logf("BC:%s%s,%d,%ld", (x), (y), __LINE__, current_tick)
#define CHART(x) CHART2(x,"")

#else /* !DO_BOOTCHART */

#define CHART2(x,y)
#define CHART(x)

#endif /* DO_BOOTCHART */

#endif /* BOOTCHART_H */
