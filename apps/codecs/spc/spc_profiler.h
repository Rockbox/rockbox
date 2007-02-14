/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2006-2007 Adam Gashlin (hcs)
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
 
/* a fun simple elapsed time profiler */

#if defined(SPC_PROFILE) && defined(USEC_TIMER)

#define CREATE_TIMER(name) static uint32_t spc_timer_##name##_start,\
    spc_timer_##name##_total
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

static void reset_profile_timers(void) {
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

static int logfd=-1;

static void print_timers(char * path) {
    logfd = ci->open("/spclog.txt",O_WRONLY|O_CREAT|O_APPEND);
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

#else

#define CREATE_TIMER(name)
#define ENTER_TIMER(name)
#define EXIT_TIMER(name)
#define READ_TIMER(name)
#define RESET_TIMER(name)
#define print_timers(path)
#define reset_profile_timers()

#endif
