/***************************************************************************
*             __________               __   ___.
*   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
*   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
*   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
*   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
*                     \/            \/     \/    \/            \/
* $Id$
*
* Passthrough routines for plugin profiling.
*
* Copyright (C) 2005 Brandon Low
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

#ifndef __PROFILE_PLUGIN_H__
#define __PROFILE_PLUGIN_H__

#include "plugin.h"

void profile_init(const struct plugin_api* pa);

void __cyg_profile_func_enter(void *this_fn, void *call_site)
    NO_PROF_ATTR ICODE_ATTR;
void __cyg_profile_func_exit(void *this_fn, void *call_site)
    NO_PROF_ATTR ICODE_ATTR;

#endif /* __PROFILE_PLUGIN_H__ */

