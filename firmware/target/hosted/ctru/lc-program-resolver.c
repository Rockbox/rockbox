/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2025 Mauricio G.
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

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <setjmp.h>
#include "debug.h"

#define RESOLVER_ENTRY(name) \
    if (!strcmp(symName, #name)) \
        return (void *)name;

/* void* ctrdlProgramResolver(const char* symName) */
void* programResolver(const char* symName, void *userData)
{
    DEBUGF("programResolver(name=\"%s\")\n", symName);

    /* codecs, etc */
    RESOLVER_ENTRY(abs);
    RESOLVER_ENTRY(labs);
    RESOLVER_ENTRY(llabs);
    RESOLVER_ENTRY(printf);

    /* plugins */
    RESOLVER_ENTRY(_ctype_);
    RESOLVER_ENTRY(__errno);
    RESOLVER_ENTRY(longjmp);
    RESOLVER_ENTRY(setjmp);

    return NULL;
    (void) userData;
}
