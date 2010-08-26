/***************************************************************************
*             __________               __   ___.
*   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
*   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
*   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
*   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
*                     \/            \/     \/    \/            \/
* $Id$
*
* Copyright (C) 2009 by Maurus Cuelenaere
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

#ifndef __PLUGINLIB_EXIT_H__
#define __PLUGINLIB_EXIT_H__

/* make sure we are in sync with the real definitions, especially on
 * hosted systems */
#include <stdlib.h>
#include "gcc_extensions.h"

/* these are actually implemented in plugin_crt0.c which all plugins link
 *
 * the cygwin/mingw shared library stub also defines atexit, so give our
 * implementation a prefix */
#define atexit rb_atexit
extern int rb_atexit(void (*func)(void));
extern void exit(int status) NORETURN_ATTR;
/* these don't call the exit handlers */
extern void _exit(int status) NORETURN_ATTR;
/* C99 version */
#define _Exit _exit

#ifndef EXIT_SUCCESS
#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1
#endif

/**
 * helper function to handle USB connected events coming from
 * button_get()
 *
 * it will exit the plugin if usb is detected, but it will call the atexit func
 * before actually showing the usb screen
 *
 * it additionally handles power off as well, with the same behavior
 */
extern void exit_on_usb(int button);

#endif /*  __PLUGINLIB_EXIT_H__ */
