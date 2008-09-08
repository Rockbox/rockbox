/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 *   Copyright (C) 2007 by Rostilav Checkan
 *   $Id$
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
 
#ifndef PROXY_H
#define PROXY_h

#include <stdio.h>

#include "screen_access.h"
#include "api.h"
#include "defs.h"

#define DEBUGF  dbgf
#define DEBUGF1 dbgf
#define DEBUGF2(...)
#define DEBUGF3(...)
#define DEBUGF4(...)

EXPORT int checkwps(const char *filename, int verbose);
EXPORT int wps_init(const char* filename,struct proxy_api *api,bool isfile);
EXPORT int wps_display();
EXPORT int wps_refresh();
EXPORT const char* get_model_name();

extern struct screen screens[NB_SCREENS];
extern bool   debug_wps;
extern int    wps_verbose_level;


#endif
