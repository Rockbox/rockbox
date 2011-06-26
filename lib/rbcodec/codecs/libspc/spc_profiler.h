/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006-2007 Adam Gashlin (hcs)
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
 
/* a fun simple elapsed time profiler */
#ifndef _SPC_PROFILER_H_
#define _SPC_PROFILER_H_

#if defined(SPC_PROFILE) && defined(USEC_TIMER)

#ifdef SPC_DEFINE_PROFILER_TIMERS
#define CREATE_TIMER(name) uint32_t spc_timer_##name##_start,\
    spc_timer_##name##_total
#else
#define CREATE_TIMER(name) extern uint32_t spc_timer_##name##_start,\
    spc_timer_##name##_total
#endif

#define ENTER_TIMER(name) spc_timer_##name##_start=USEC_TIMER
#define EXIT_TIMER(name) spc_timer_##name##_total+=\
    (USEC_TIMER-spc_timer_##name##_start)
#define READ_TIMER(name) (spc_timer_##name##_total)
#define RESET_TIMER(name) spc_timer_##name##_total=0

#define PRINT_TIMER_PCT(bname,tname,nstr) ci->fdprintf( \
    logfd,"%10ld ",READ_TIMER(bname));\
    ci->fdprintf(logfd,"(%3d%%) " nstr "\t",\
    ((uint64_t)READ_TIMER(bname))*100/READ_TIMER(tname))

CREATE_TIMER(total);
CREATE_TIMER(render);
#if 0
CREATE_TIMER(cpu);
CREATE_TIMER(dsp);
CREATE_TIMER(dsp_pregen);
CREATE_TIMER(dsp_gen);
CREATE_TIMER(dsp_mix);
#endif

void reset_profile_timers(void);
void print_timers(char * path);

#else

#define CREATE_TIMER(name)
#define ENTER_TIMER(name)
#define EXIT_TIMER(name)
#define READ_TIMER(name)
#define RESET_TIMER(name)
#define print_timers(path)
#define reset_profile_timers()

#endif

#endif /* _SPC_PROFILER_H_ */
