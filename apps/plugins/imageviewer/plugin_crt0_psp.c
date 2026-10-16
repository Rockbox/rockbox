/***************************************************************************
*             __________               __   ___.
*   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
*   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
*   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
*   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
*                     \/            \/     \/    \/            \/
* $Id$
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


#include "plugin.h"
#include <setjmp.h>

#ifdef SONY_PSP
#include <psploadexec.h>
#include <pspmoduleinfo.h>

PSP_MODULE_INFO("plugin", 0, 0, 1);
PSP_NO_CREATE_MAIN_THREAD();
PSP_HEAP_SIZE_KB( 256 );

int module_start (SceSize argc, void* argp)
{
    return 0;
    (void) argc;
    (void) argp;
}

/* pluginlib.S export */
extern const struct imgdec_header __header;
void *plugin_get_header(void)
{
    return (void *) &__header;
}

/* Unresolved symbols */
int abs (int i) { return (i < 0) ? -i : i; }
long labs (long i) { return (i < 0) ? -i : i; }
long long llabs (long long i) { return (i < 0) ? -i : i; }
#endif
