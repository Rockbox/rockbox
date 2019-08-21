/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2019 Sebastian Leonhardt
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


#ifndef RB_TEST_H
#define RB_TEST_H


/*
 * Change defines to 1 or 0 to enable or disable specific test/debugging options
 */


/*
 * usec timer
 * (not available on simulator and probably some targets)
 */
#ifdef SIMULATOR
# define TST_TIME_USEC           0
#else
# define TST_TIME_USEC           1
# if !defined(USEC_TIMER)
#  warning No USEC_TIMER! Deactivated time measurement.
#  define TST_TIME_USEC 0
# endif
#endif

/*
 * display warning/error stats on LCD
 */
#ifdef SIMULATOR
#define TST_DISPLAY_ALERTS   1
#else
#define TST_DISPLAY_ALERTS   0
#endif

/*
 * gather frame statistics,
 * i.e. number of lines VSYNCed/VBLANKed/drawn etc.
 */
#ifdef SIMULATOR
#define TST_FRAME_STAT_ENABLE   1
#else
#define TST_FRAME_STAT_ENABLE   0
#endif

/*
 * profile function calls
 * display as DEBUGF
 */
#ifdef SIMULATOR
# define TST_PROFILE_FUNC        1
#else
# define TST_PROFILE_FUNC        0
#endif

/*
 * make some extra sanity test. exits on fail.
 */
#ifdef SIMULATOR
# define TST_USE_ASSERT         1
#else
# define TST_USE_ASSERT         0
#endif

/*
 * time measurement
 * especially usefull on real target to see if a "optimization" really helps
 */
#if TST_TIME_USEC
# define TST_TIME_MEASURE       0   /* deactivate if we can use USEC_TIMER */
#else
# define TST_TIME_MEASURE       1
#endif

/*
 * apply global frame counter for debugging purposes
 */
#ifdef SIMULATOR
#define TST_FRAME_CNT
#endif


// -----------------------------------------------

/*
 * display alerts on LCD
 */

#if TST_DISPLAY_ALERTS

/* time (in frames) to display an alert */
#define TST_ALERT_TIME      40

/* whenever an alert occurs, this timer is set to the specific frame count */
extern unsigned tst_alert_timer;

enum {
    ALERT_CPU_ILLEGAL=0,
    ALERT_CPU_UNDOC,
    ALERT_VID_NOSYNC,
    ALERT_TIA_ADDR,
    ALERT_TIA_HMOVE,
    NUM_ALERTS
};
#define TST_ALERT_TOKENS \
{ \
    "cpuIll", \
    "cpuUnd", \
    "noVsync", \
    "tiaAddr", \
    "tiaHmove", \
}

extern uint8_t tst_alert[NUM_ALERTS];

char *tst_get_alert_tokens(void);

/* if expr is false, the alert id will be triggert */
#define TST_ALERT(expr, id) \
    do { \
        if (!(expr)) { \
            tst_alert_timer = tst_alert[id] = TST_ALERT_TIME; \
        } \
    } while (0)

#else

#define TST_ALERT(expr, id)

#endif /* TST_DISPLAY_ALERTS */

// -----------------------------------------------


#if TST_USE_ASSERT

#include <setjmp.h>

extern jmp_buf tst_assert_jmpbuf;
void tst_assert_print(void);
extern char tst_assert_msg[40];

/*
 * use this macro in your 'main' function.
 */
#define TST_INIT_ASSERT()   \
    do { \
        if (setjmp(tst_assert_jmpbuf)) { \
            tst_assert_print(); \
            exit(0); /* or return */ \
        } \
    } while (0)

/*
 * use this macro to make sanity checks in your code.
 * if (x) is false, the variadic arguments will be printed.
 */
#define TST_ASSERT(x, ...) \
    do { \
        if (!(x)) { \
            rb->snprintf(tst_assert_msg, 40, __VA_ARGS__); \
            longjmp(tst_assert_jmpbuf, 1); \
        } \
    } while (0)

#else

#define TST_INIT_ASSERT()
#define TST_ASSERT(...)     do { } while (0)

#endif /* TST_USE_ASSERT */


// -----------------------------------------------


void init_tests(void);
int menu_test(void);



// -----------------------------------------------


/*
 * gather frame statistics,
 * i.e. number of lines VSYNCed/VBLANKed/drawn etc.
 */

#if TST_FRAME_STAT_ENABLE

struct tst_frame_stat {
    unsigned int    vsynced,
                    vblanked0,
                    drawn,
                    vblanked1,
                    wsynced;
    unsigned int    show;
};
extern struct tst_frame_stat tst_frame_stat;

#define TST_FRAME_STAT_INIT()               \
    do {                                    \
        tst_frame_stat.show = 1;            \
        TST_FRAME_STAT_ZERO();              \
    } while(0)

#define TST_FRAME_STAT(line)                \
    do {                                    \
        ++tst_frame_stat.line;              \
    } while(0)

/* special handling for vblank */
#define TST_FRAME_STAT_VBLANK()              \
    do {                                    \
        if (tst_frame_stat.drawn)           \
            ++tst_frame_stat.vblanked1;     \
        else                                \
            ++tst_frame_stat.vblanked0;     \
    } while(0)

#define TST_FRAME_STAT_ZERO()               \
    do {                                    \
        tst_frame_stat.vsynced = 1;         \
        tst_frame_stat.vblanked0 = 0;       \
        tst_frame_stat.vblanked1 = 0;       \
        tst_frame_stat.drawn = 0;           \
        tst_frame_stat.wsynced = 0;         \
    } while(0)


#else /* ! TST_FRAME_STAT_ENABLE */

#define TST_FRAME_STAT_INIT()
#define TST_FRAME_STAT_VBLANK()
#define TST_FRAME_STAT(line)
#define TST_FRAME_STAT_ZERO()

#endif /* TST_FRAME_STAT_ENABLE */

// ------------------------------------------------

#if TST_PROFILE_FUNC

/*
 * profile functions (number of calls)
 * and number of writes/reads to tia and riot registers.
 * Usage: put macro... at beginning of function
 */

/* profiled functions */
enum {
    PRF_DRAW_FRAME = 0,
    PRF_HSYNC,
    PRF_CPU,
    PRF_CPU_UNDOC_OPCODE,
    PRF_MEM_BSWCOPY,
    PRF_TIA_ACC,
    PRF_UPD_PF,
    PRF_DEL_PL,
    PRF_UPD_PL,
    PRF_DEL_MI,
    PRF_UPD_MI,
    PRF_UPD_BL,
    PRF_MASK_RENDER,
    PRF_SETTIMER,
    PRF_DOTIMER,

    NUM_OF_PROF_FUNC    /* end-of-list marker */
};
/* Usage:
 * Put identifier in the above list, then add the following line to your code:
 * TST_PROFILE(PRF_YOUR_IDENTIFIER, "description");
 */


extern bool start_profiling;

struct prof_functions { unsigned long num; char * name;};
extern struct prof_functions prof_functions[NUM_OF_PROF_FUNC];

extern unsigned long prof_tia_write[0x30];
extern unsigned long prof_tia_read[0x10];
extern unsigned long prof_riot_write[0x20];
extern unsigned long prof_riot_read[0x20];
extern unsigned long prof_cpu_instruction[0x100];
extern unsigned long prof_tia_w_direct, prof_tia_w_ind;

void tst_profiling_result(void);    /* print result */

/* profile any function
 * id must be an unique number, see defines or enum
 */
#define TST_PROFILE(id,name_)               \
    do {                                    \
        if (start_profiling) {              \
            ++prof_functions[id].num;       \
            prof_functions[id].name = name_; \
        }                                   \
    } while(0)


/* x MUST be 0..0xff! */
#define TST_PRF_CPU_INSTR(x)                \
    do {                                    \
        if (start_profiling) {              \
            ++prof_cpu_instruction[x];      \
        }                                   \
    } while(0)


#define TST_PRF_TIA_W(x)                    \
    do {                                    \
        if (start_profiling) {              \
            ++prof_tia_w_ind;               \
            ++prof_tia_write[               \
                (x) >= ARRAY_LEN(prof_tia_write) \
                     ? prof_tia_write[ARRAY_LEN(prof_tia_write)-1] : (x) \
                ];                          \
        }                                   \
    } while(0)

#define TST_PRF_TIA_W_DIRECT(x)                    \
    do {                                    \
        if (start_profiling) {              \
            ++prof_tia_w_direct;            \
            ++prof_tia_write[               \
                (x) >= ARRAY_LEN(prof_tia_write) \
                     ? prof_tia_write[ARRAY_LEN(prof_tia_write)-1] : (x) \
                ];                          \
        }                                   \
    } while(0)


#define TST_PRF_TIA_R(x)                    \
    do {                                    \
        if (start_profiling) {              \
            ++prof_tia_read[               \
                (x) >= ARRAY_LEN(prof_tia_read) \
                ? prof_tia_read[ARRAY_LEN(prof_tia_read)-1] : (x) \
                ];                          \
        }                                   \
    } while(0)


#define TST_PRF_RIOT_W(x)                   \
    do {                                    \
        if (start_profiling) {              \
            ++prof_riot_write[              \
                (x) >= ARRAY_LEN(prof_riot_write) \
                ? prof_riot_write[ARRAY_LEN(prof_riot_write)-1] : (x) \
                ];                          \
        }                                   \
    } while(0)


#define TST_PRF_RIOT_R(x)                   \
    do {                                    \
        if (start_profiling) {              \
            ++prof_riot_read[               \
                (x) >= ARRAY_LEN(prof_riot_read) \
                ? prof_riot_read[ARRAY_LEN(prof_riot_read)-1] : (x) \
                ];                          \
        }                                   \
    } while(0)


#else

#define TST_PROFILE(id, name)
#define TST_PRF_CPU_INSTR(x)
#define TST_PRF_TIA_W(x)
#define TST_PRF_TIA_W_DIRECT(x)
#define TST_PRF_TIA_R(x)
#define TST_PRF_RIOT_W(x)
#define TST_PRF_RIOT_R(x)

#endif /* TST_PROFILE_FUNC */

// ------------------------------------------------

#if TST_TIME_MEASURE

/* We measure time in RB ticks over a specific count of frames: */
#ifdef SIMULATOR
#define TST_TIME_MEASURE_FRAMES 2000
#define TST_TIME_MEASURE_FRAMES_S "2000" /* as string to print out in menu */
#else
#define TST_TIME_MEASURE_FRAMES 250
#define TST_TIME_MEASURE_FRAMES_S "250" /* as string to print out in menu */
#endif

extern unsigned tst_time_measure;     /* 0=No Test, 1=Normal, 2=without display */
extern unsigned tst_time_measure_fc;     /* frame counter */
extern unsigned long tst_time_measure_time;


#else

#endif

// ------------------------------------------------

#if TST_TIME_USEC

#include "cpu.h"

extern int tst_time_usec_start;
#define TST_USEC_NUMFRAMES     8   /* number of frames to measure */

enum {TST_USEC_FRAME=0,
      TST_USEC_LCD,
      TST_USEC_F_WO_LCD,
      TST_USEC_RASTER,
      TST_USEC_TIA,
      TST_USEC_PF,
      TST_USEC_PL,
      TST_USEC_MIBL,
      TST_USEC_MOVE,
      TST_USEC_CPU,
      TST_USEC_NUM  /* Last entry: Number of entries */
};
#define TST_USEC_DESCRIPTION \
    { \
        "Frame Time:", \
        "tv_display():", \
        "Frame w/o LCD:", \
        "do_raster():", \
        "tia write functs:", \
        "tia playfield:", \
        "tia player:", \
        "tia missile/ball:", \
        "tia move:", \
        "cpu():", \
    }

extern unsigned long tst_usec[TST_USEC_NUM];
void tst_time_usec_results(void);

#endif

// ------------------------------------------------

#endif /* RB_TEST_H */
