/***************************************************************************
*             __________               __   ___.
*   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
*   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
*   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
*   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
*                     \/            \/     \/    \/            \/
* $Id$
*
* Additional LCD routines not present in the rockbox core
* Core functions
*
* Copyright (C) 2005 Jens Arnold
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

#include "plugin.h"

#ifdef HAVE_LCD_BITMAP
#include "xlcd.h"

/*** globals ***/

const struct plugin_api *_xlcd_rb = NULL; /* global api struct pointer */

/*** functions ***/

/* library init */
void xlcd_init(const struct plugin_api* newrb)
{
    _xlcd_rb = newrb;
}

#endif /* HAVE_LCD_BITMAP */

