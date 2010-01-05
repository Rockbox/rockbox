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

#include "config.h"
#ifndef SIMULATOR
#include "../../codecs/lib/setjmp.h"
#else
#include <setjmp.h>
#endif

#define _PLUGINLIB_EXIT_INIT(atexit) switch(setjmp(__exit_env))   \
                                     {                            \
                                         case 1:                  \
                                             atexit               \
                                             return PLUGIN_OK;    \
                                         case 2:                  \
                                             atexit               \
                                             return PLUGIN_ERROR; \
                                         case 0:                  \
                                         default:                 \
                                             break;               \
                                     }

/* Either PLUGINLIB_EXIT_INIT or PLUGINLIB_EXIT_INIT_ATEXIT needs to be placed
 * as the first line in plugin_start. The _ATEXIT version will call the named
 * no-argument function when exit() is called before exiting the plugin, to
 * allow for cleanup.
 */
#define PLUGINLIB_EXIT_INIT _PLUGINLIB_EXIT_INIT()
#define PLUGINLIB_EXIT_INIT_ATEXIT(atexit) _PLUGINLIB_EXIT_INIT(atexit();)

extern jmp_buf __exit_env;
#define exit(status) longjmp(__exit_env, status != 0 ? 2 : 1)

#endif /*  __PLUGINLIB_EXIT_H__ */
