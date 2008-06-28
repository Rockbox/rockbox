/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Profiling routines counts ticks and calls to each profiled function.
 *
 * Copyright (C) 2005 by Brandon Low
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 ****************************************************************************/

#ifndef _SYS_PROFILE_H
#define _SYS_PROFILE_H

/* Initialize and start profiling */
void profstart(int current_thread)
  NO_PROF_ATTR;

/* Clean up and write profile data */
void profstop (void)
  NO_PROF_ATTR;

/* Called every time a thread stops, we check if it's our thread and store
 * temporary timing data if it is */
void profile_thread_stopped(int current_thread)
  NO_PROF_ATTR;
/* Called when a thread starts, we check if it's our thread and resume timing */
void profile_thread_started(int current_thread)
  NO_PROF_ATTR;

void profile_func_exit(void *this_fn, void *call_site)
  NO_PROF_ATTR ICODE_ATTR;
void profile_func_enter(void *this_fn, void *call_site)
  NO_PROF_ATTR ICODE_ATTR;

#endif /*_SYS_PROFILE_H*/
