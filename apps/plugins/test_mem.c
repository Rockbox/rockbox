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

#if PLUGIN_BUFFER_SIZE <= 0x8000
#define BUF_SIZE (1<<12) /* 16 KB = (1<<12)*sizeof(int) */
#elif PLUGIN_BUFFER_SIZE <= 0x10000
#define BUF_SIZE (1<<13) /* 32 KB = (1<<13)*sizeof(int) */
#elif PLUGIN_BUFFER_SIZE <= 0x20000
#define BUF_SIZE (1<<14) /* 64 KB = (1<<14)*sizeof(int) */
#else
#define BUF_SIZE (1<<15) /* 128 KB = (1<<15)*sizeof(int) */
#endif

#define LOOP_REPEAT_DRAM 256
#define MAX_REPEAT_DRAM  512
static int loop_repeat_dram = LOOP_REPEAT_DRAM;
static volatile int buf_dram[BUF_SIZE]           MEM_ALIGN_ATTR;

#if defined(PLUGIN_USE_IRAM)

#if PLUGIN_BUFFER_SIZE <= 0x8000
#define IBUF_SIZE (1<<12) /* 16 KB = (1<<12)*sizeof(int) */
#else
#define IBUF_SIZE (1<<13) /* 32 KB = (1<<13)*sizeof(int) */
#endif

#define LOOP_REPEAT_IRAM 256
#define MAX_REPEAT_IRAM  512
static int loop_repeat_iram = LOOP_REPEAT_DRAM;
static volatile int buf_iram[IBUF_SIZE] IBSS_ATTR MEM_ALIGN_ATTR;
#endif

/* (Byte per loop * loops)>>20 * ticks per s * 10 / ticks = dMB per s */
#define dMB_PER_SEC(buf_size, cnt, delta) ((((buf_size*sizeof(int)*cnt)>>20)*HZ*10)/delta)

static void memset_test(volatile int *buf, int buf_size, int loop_cnt)
{
    size_t buf_bytes = buf_size*sizeof(buf[0]);
    for(int i = 0; i < loop_cnt; i++)
    {
        memset((void*)buf, 0xff, buf_bytes);
    }
}

static void memcpy_test(volatile int *buf, int buf_size, int loop_cnt)
{
    /* half-size memcpy since memory regions must not overlap */
    void* half_buf = (void*)(&buf[buf_size/2]);
    size_t half_buf_bytes = buf_size * sizeof(buf[0]) / 2;

    /* double loop count to compensate for half size memcpy */
    for(int i = 0; i < loop_cnt*2; i++)
    {
        memcpy((void*)&buf[0], half_buf, half_buf_bytes);
    }
}

static void write_test(volatile int *buf, int buf_size, int loop_cnt)
{
#if defined(CPU_ARM)
    asm volatile (
            "mov r6, %[loops]      \n"
        ".outer_loop_write:        \n"
            "mov r4, %[buf_p]      \n"
            "mov r5, %[size]       \n"
        ".inner_loop_write:        \n"
            "ldmia r4!, {r0-r3}    \n"
            "subs r5, r5, #8       \n"
            "ldmia r4!, {r0-r3}    \n"
            "bgt .inner_loop_write \n"
            "subs r6, r6, #1       \n"
            "bgt .outer_loop_write \n"
            :
            : [loops] "r" (loop_cnt), [size] "r" (buf_size), [buf_p] "r" (buf)
            : "r0", "r1", "r2", "r3", "r4", "r5", "r6", "memory", "cc"
        );
#else
    for(int i = 0; i < loop_cnt; i++)
    {
        for(int j = 0; j < buf_size; j+=4)
        {
            buf[j  ] = j;
            buf[j+1] = j+1;
            buf[j+2] = j+2;
            buf[j+3] = j+3;
        }
    }
#endif
}

static void read_test(volatile int *buf, int buf_size, int loop_cnt)
{
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
            : "r0", "r1", "r2", "r3", "r4", "r5", "r6", "memory", "cc"
        );
#else
    int x;
    for(int i = 0; i < loop_cnt; i++)
    {
        for(int j = 0; j < buf_size; j+=4)
        {
            x = buf[j  ];
            x = buf[j+2];
            x = buf[j+3];
            x = buf[j+4];
        }
    }
    (void)x;
#endif
}

enum test_type {
    READ,
    WRITE,
    MEMSET,
    MEMCPY,
};

static const char tests[][7] = {
    [READ] = "read  ",
    [WRITE] = "write ",
    [MEMSET] = "memset",
    [MEMCPY] = "memcpy",
};

static int line;
#define TEST_MEM_PRINTF(...) rb->screens[0]->putsf(0, line++, __VA_ARGS__)

static int test(volatile int *buf, int buf_size, int loop_cnt,
                enum test_type type)
{    
    int delta, dMB;
    int last_tick = *rb->current_tick;
    int ret = 0;

    switch(type)
    {
        case READ:     read_test(buf, buf_size, loop_cnt); break;
        case WRITE:   write_test(buf, buf_size, loop_cnt); break;
        case MEMSET: memset_test(buf, buf_size, loop_cnt); break;
        case MEMCPY: memcpy_test(buf, buf_size, loop_cnt); break;
    }
    
    delta = *rb->current_tick - last_tick;

    if (delta <= 20)
    {
        /* The loop_cnt will be increased for the next measurement set until 
         * each measurement at least takes 10 ticks. This is to ensure a
         * minimum accuracy. */
        ret = 1;
    }

    delta = delta>0 ? delta : delta+1;
    dMB   = dMB_PER_SEC(buf_size, loop_cnt, delta);
    TEST_MEM_PRINTF("%s: %3d.%d MB/s (%3d ms)", 
        tests[type], dMB/10, dMB%10, delta*10);

    return ret;
}

enum plugin_status plugin_start(const void* parameter)
{
    (void)parameter;
    bool done = false;
#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    bool boost = false;
#endif
    int count = 0;

    rb->lcd_setfont(FONT_SYSFIXED);

    rb->screens[0]->clear_display();
    TEST_MEM_PRINTF("patience, may take some seconds...");
    rb->screens[0]->update();

    while (!done)
    {
        line = 0;
        int ret;
        rb->screens[0]->clear_display();
#ifdef HAVE_ADJUSTABLE_CPU_FREQ
        TEST_MEM_PRINTF("%s", boost?"boosted":"unboosted");
        TEST_MEM_PRINTF("clock: %3d.%d MHz", (*rb->cpu_frequency)/1000000, (*rb->cpu_frequency)%1000000);
#endif
        TEST_MEM_PRINTF("loop#: %d", ++count);

        TEST_MEM_PRINTF("DRAM cnt: %d size: %d MB", loop_repeat_dram, 
            (loop_repeat_dram*BUF_SIZE*sizeof(buf_dram[0]))>>20);
        ret = 0;
        ret |= test(buf_dram, BUF_SIZE, loop_repeat_dram, READ);
        ret |= test(buf_dram, BUF_SIZE, loop_repeat_dram, WRITE);
        ret |= test(buf_dram, BUF_SIZE, loop_repeat_dram, MEMSET);
        ret |= test(buf_dram, BUF_SIZE, loop_repeat_dram, MEMCPY);
        if (ret != 0 && loop_repeat_dram < MAX_REPEAT_DRAM) loop_repeat_dram *= 2;
#if defined(PLUGIN_USE_IRAM)
        TEST_MEM_PRINTF("IRAM cnt: %d size: %d MB", loop_repeat_iram, 
            (loop_repeat_iram*BUF_SIZE*sizeof(buf_iram[0]))>>20);
        ret = 0;
        ret |= test(buf_iram, BUF_SIZE, loop_repeat_iram, READ);
        ret |= test(buf_iram, BUF_SIZE, loop_repeat_iram, WRITE);
        ret |= test(buf_iram, BUF_SIZE, loop_repeat_iram, MEMSET);
        ret |= test(buf_iram, BUF_SIZE, loop_repeat_iram, MEMCPY);
        if (ret != 0 && loop_repeat_iram < MAX_REPEAT_IRAM) loop_repeat_iram *= 2;
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
