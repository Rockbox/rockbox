/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2009 Thomas Martitz
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
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "config.h"

#ifndef __STATUSBAR_SKINNED_H__
#define __STATUSBAR_SKINNED_H__


void sb_skin_data_load(enum screen_type screen, const char *buf, bool isfile);
void sb_skin_data_init(enum screen_type screen);

/* probably temporary, to shut the classic statusbar up */
bool sb_skin_active(void);
void sb_skin_init(void);

#ifdef HAVE_ALBUMART
bool sb_skin_uses_statusbar(int *width, int *height);
#endif

#endif /* __STATUSBAR_SKINNED_H__ */
