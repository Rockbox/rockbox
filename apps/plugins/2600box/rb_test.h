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


// -----------------------------------------------

/*
 * some things for optimization
 */


/*
 * use UINT32 writes on BYTE arrays in raster.c to optimize speed
 * NOTE: even without this define BYTE arrays can be zeroed with 32bit access
 */
#define RASTER_32BIT_OPT
/* define to not use 32bit access to zero byte arrays. RASTER_32BIT_OPT must
 not be defined to make this work */
//#define RASTER_NO_32Bit_ZEROS

// Note: optimizations for cpu.c are placed there

// -----------------------------------------------


/*
 * Change defines to 1 or 0 to enable or disable specific tests/debugging/optimizations
 */

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
# define TST_TIME_USEC           0
#else
# define TST_PROFILE_FUNC        0
# define TST_TIME_USEC           1
# if !defined(USEC_TIMER)
#  warning No USEC_TIMER! Deactivated time measurement.
#  define TST_TIME_USEC 0
# endif
#endif

/*
 * time measurement
 * especially usefull on real target to see if a "optimization" really helps
 */
#define TST_TIME_MEASURE        0   /* deactivated, because I have USEC_TIMER on my target */

/*
 * DEBUGF on tia write access for 1 1/2 frames
 * TODO: start dump via menu!
 */
#define TST_DUMP_TIA_ACCESS     0

/*
 * apply global frame counter for debugging purposes
 */
#ifdef SIMULATOR
#define TST_FRAME_CNT
#endif


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
                    unsynced;
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

/* special handling for vsync */
#define TST_FRAME_STAT_VSYNC()              \
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
        tst_frame_stat.unsynced = 0;        \
    } while(0)


#else /* ! TST_FRAME_STAT_ENABLE */

#define TST_FRAME_STAT_INIT()
#define TST_FRAME_STAT_VSYNC()
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

extern unsigned tst_time_measure;     /* 0=No Test, 1=Normal, 2= w/o Playfield,
                3= w/o Player, 4= without display, 5=only CPU/main loop */
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
      TST_USEC_CPU,
      TST_USEC_NUM  /* Last entry: Number of entries */
};
#define TST_USEC_DESCRIPTION \
    { \
        "Frame Time:", \
        "tv_display():", \
        "Frame w/o LCD:", \
        "do_raster():", \
        "do_tia_write():", \
        "cpu():", \
    }

extern unsigned long tst_usec[TST_USEC_NUM];
void tst_time_usec_results(void);

#endif

// ------------------------------------------------

#endif /* RB_TEST_H */
