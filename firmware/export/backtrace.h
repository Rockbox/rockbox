/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2017 by Amaury Pouly
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
#ifndef __ROCKBOX_BACKTRACE_H__
#define __ROCKBOX_BACKTRACE_H__

#include "config.h"
#ifdef BACKTRACE_UNWARMINDER
#include "backtrace-unwarminder.h"
#endif
#ifdef BACKTRACE_MIPSUNWINDER
#include "backtrace-mipsunwinder.h"
#endif

/* Print a backtrace using lcd_* functions, starting at the given line and updating
 * the line number. On targets that support it (typically native targets), the
 * backtrace will start at the given value of PC and using the stack frame given
 * by PC. On hosted targets, it will typically ignore those values and backtrace
 * from the caller */
void rb_backtrace(int pcAddr, int spAddr, unsigned *line);

#endif /* __ROCKBOX_BACKTRACE_H__ */
