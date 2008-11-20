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

/* lovingly ripped off from Game_Music_Emu 0.5.2. http://www.slack.net/~ant/ */
/* DSP Based on Brad Martin's OpenSPC DSP emulator */
/* tag reading from sexyspc by John Brawn (John_Brawn@yahoo.com) and others */

#if defined(SPC_PROFILE) && defined(USEC_TIMER)

#include "codeclib.h"
#include "spc_codec.h"
#define SPC_DEFINE_PROFILER_TIMERS
#include "spc_profiler.h"

void reset_profile_timers(void)
{
    RESET_TIMER(total);
    RESET_TIMER(render);
#if 0
    RESET_TIMER(cpu);
    RESET_TIMER(dsp);
    RESET_TIMER(dsp_pregen);
    RESET_TIMER(dsp_gen);
    RESET_TIMER(dsp_mix);
#endif
}

void print_timers(char * path)
{
    int logfd = ci->open("/spclog.txt",O_WRONLY|O_CREAT|O_APPEND);
    ci->fdprintf(logfd,"%s:\t",path);
    ci->fdprintf(logfd,"%10ld total\t",READ_TIMER(total));
    PRINT_TIMER_PCT(render,total,"render");
#if 0
    PRINT_TIMER_PCT(cpu,total,"CPU");
    PRINT_TIMER_PCT(dsp,total,"DSP");
    ci->fdprintf(logfd,"(");
    PRINT_TIMER_PCT(dsp_pregen,dsp,"pregen");
    PRINT_TIMER_PCT(dsp_gen,dsp,"gen");
    PRINT_TIMER_PCT(dsp_mix,dsp,"mix");
#endif
    ci->fdprintf(logfd,"\n");
    
    ci->close(logfd);
    logfd=-1;
}

#endif /* #if defined(SPC_PROFILE) && defined(USEC_TIMER) */
