/***************************************************************************
 *             __________               __   ___.                  
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___  
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /  
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <   
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \  
 *                     \/            \/     \/    \/            \/ 
 *
 * Copyright (C) 2015 by Lorenzo Miori
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

#include <stdlib.h>
#include <inttypes.h>
#include <unistd.h>
#include <sys/mount.h>
#include <errno.h>

#include "system.h"
#include "kernel.h"
#include "thread.h"

#include "string-extra.h"
#include "panic.h"
#include "debug.h"
#include "storage.h"
#include "mv.h"
#include "backlight.h"
#include "rbunicode.h"
#include "logdiskf.h"

void power_off(void)
{
    exit(EXIT_SUCCESS);
}

uintptr_t *stackbegin;
uintptr_t *stackend;
void system_init(void)
{
    int *s;
    /* fake stack, OS manages size (and growth) */
    stackbegin = stackend = (uintptr_t*)&s;
}


void system_reboot(void)
{
    power_off();
}

void system_exception_wait(void)
{
    system_reboot();
}
