/***************************************************************************
*             __________               __   ___.
*   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
*   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
*   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
*   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
*                     \/            \/     \/    \/            \/
* $Id$
*
* Copyright (C) 2010 Thomas Martitz, Andree Buschmann
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

PLUGIN_HEADER

#define BUF_SIZE (1<<13) /* 32 KB = (1<<13)*sizeof(int) */

#define LOOP_REPEAT_DRAM 256
static volatile int buf_dram[BUF_SIZE];

#if defined(PLUGIN_USE_IRAM)
#define LOOP_REPEAT_IRAM 1024
static volatile int buf_iram[BUF_SIZE] IBSS_ATTR;
#endif

/* (Byte per loop * loops)>>20 * ticks per s * 10 / ticks = dMB per s */
#define dMB_PER_SEC(cnt, delta) ((((BUF_SIZE*sizeof(int)*cnt)>>20)*HZ*10)/delta)

void write_test(volatile int *buf, int buf_size, int loop_cnt, 
                int line, char *ramtype)
{
    int delta, dMB;
    int last_tick = *rb->current_tick;
    
#if defined(CPU_ARM)
    asm volatile (
            "mov r0, #0           \n"
            "mov r1, #1           \n"
            "mov r2, #2           \n"
            "mov r3, #3           \n"
            "mov r6, %[loops]     \n"
        ".outer_loop_read:        \n"
            "mov r4, %[buf_p]     \n"
            "mov r5, %[size]      \n"
        ".inner_loop_read:        \n"
            "stmia r4!, {r0-r3}   \n"
            "stmia r4!, {r0-r3}   \n"
            "subs r5, r5, #8      \n"
            "bgt .inner_loop_read \n"
            "subs r6, r6, #1      \n"
            "bgt .outer_loop_read \n"
            :
            : [loops] "r" (loop_cnt), [size] "r" (buf_size), [buf_p] "r" (buf)
            : "r0", "r1", "r2", "r3", "r4", "r5", "r6"
        );
#else
    int i, j;
    for(i = 0; i < loop_cnt; i++)
    {
        for (j = 0; j < buf_size; j+=4)
        {
            buf[j  ] = j;
            buf[j+1] = j+1;
            buf[j+2] = j+2;
            buf[j+3] = j+3;
        }
    }
#endif
    delta = *rb->current_tick - last_tick;
    delta = delta>0 ? delta : delta+1;
    dMB   = dMB_PER_SEC(loop_cnt, delta);
    rb->screens[0]->putsf(0, line, "%s wr: %3d.%d MB/s (%2d ticks for %d MB)", 
                          ramtype, dMB/10, dMB%10, delta,
                          (loop_cnt*BUF_SIZE*4)>>20);
}

void read_test(volatile int *buf, int buf_size, int loop_cnt, 
               int line, char *ramtype)
{
    int delta, dMB;
    int last_tick = *rb->current_tick;
    
#if defined(CPU_ARM)
    asm volatile (
            "mov r6, %[loops]      \n"
        ".outer_loop_write:        \n"
            "mov r4, %[buf_p]      \n"
            "mov r5, %[size]       \n"
        ".inner_loop_write:        \n"
            "ldmia r4!, {r0-r3}    \n"
            "ldmia r4!, {r0-r3}    \n"
            "subs r5, r5, #8       \n"
            "bgt .inner_loop_write \n"
            "subs r6, r6, #1       \n"
            "bgt .outer_loop_write \n"
            :
            : [loops] "r" (loop_cnt), [size] "r" (buf_size), [buf_p] "r" (buf)
            : "r0", "r1", "r2", "r3", "r4", "r5", "r6"
        );
#else
    int i, j, x;
    for(i = 0; i < loop_cnt; i++)
    {
        for(j = 0; j < buf_size; j+=4)
        {
            x = buf[j  ];
            x = buf[j+2];
            x = buf[j+3];
            x = buf[j+4];
        }
    }
#endif
    delta = *rb->current_tick - last_tick;
    delta = delta>0 ? delta : delta+1;
    dMB   = dMB_PER_SEC(loop_cnt, delta);
    rb->screens[0]->putsf(0, line, "%s rd: %3d.%d MB/s (%2d ticks for %d MB)", 
                          ramtype, dMB/10, dMB%10, delta, 
                          (loop_cnt*BUF_SIZE*4)>>20);
}


enum plugin_status plugin_start(const void* parameter)
{
    (void)parameter;
    bool done = false;
    bool boost = false;
    int count = 0;

    rb->lcd_setfont(FONT_SYSFIXED);
    
    rb->screens[0]->clear_display();
    rb->screens[0]->putsf(0, 0, "patience, may take some seconds...");
    rb->screens[0]->update();

    while (!done)
    {
        int line = 0;

        rb->screens[0]->clear_display();
        rb->screens[0]->putsf(0, line++, "%s", boost?"boosted":"unboosted");
#ifndef SIMULATOR
        rb->screens[0]->putsf(0, line++, "clock: %d Hz", *rb->cpu_frequency);
#endif
        rb->screens[0]->putsf(0, line++, "loop#: %d", ++count);

        read_test (buf_dram, BUF_SIZE, LOOP_REPEAT_DRAM, line++, "DRAM");
        write_test(buf_dram, BUF_SIZE, LOOP_REPEAT_DRAM, line++, "DRAM");
#if defined(PLUGIN_USE_IRAM)
        read_test (buf_iram, BUF_SIZE, LOOP_REPEAT_IRAM, line++, "IRAM");
        write_test(buf_iram, BUF_SIZE, LOOP_REPEAT_IRAM, line++, "IRAM");
#endif

        rb->screens[0]->update();

        switch (rb->get_action(CONTEXT_STD, HZ/5))
        {
#ifdef HAVE_ADJUSTABLE_CPU_FREQ
            case ACTION_STD_PREV:
                if (!boost)
                {
                    rb->cpu_boost(true);
                    boost = true;
                }
                break;

            case ACTION_STD_NEXT:
                if (boost)
                {
                    rb->cpu_boost(false);
                    boost = false;
                }
                break;
#endif
            case ACTION_STD_CANCEL:
                done = true;
                break;
        }
    }

    return PLUGIN_OK;
}
