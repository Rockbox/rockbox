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

#include "rbconfig.h"
#include "rb_lcd.h"
#include "rb_test.h"


/*
 * Various stuff to test and debug the Atari 2600 emulator
 */

// ----------------------------------------------------------

#if TST_TIME_USEC

int tst_time_usec_start;
unsigned long tst_usec[TST_USEC_NUM];

#endif

// ----------------------------------------------------------

#if TST_TIME_MEASURE

unsigned tst_time_measure;
unsigned tst_time_measure_fc;     /* frame counter */
unsigned long tst_time_measure_time;

#endif

// ----------------------------------------------------------

#if TST_PROFILE_FUNC

bool start_profiling = false;
struct prof_functions prof_functions[NUM_OF_PROF_FUNC];

unsigned long prof_tia_write[0x30];
unsigned long prof_tia_read[0x10];
unsigned long prof_riot_write[0x20];
unsigned long prof_riot_read[0x20];
unsigned long prof_cpu_instruction[0x100];

#if 0
static void tst_format_number(unsigned long num, char txt[], int txtlen)
{
    int s=0;    /* thousands separator */
    txt[--txtlen] = '\0';
    for (;;) {
        ++s;
        txt[--txtlen] = '0' + num % 10;
        num /= 10;
        if (!num)
            break;
        if (!txtlen) {
            txt[txtlen] = '?';
            break;
        }
        if (s%3 == 0) {
            txt[--txtlen] = ',';
            if (!txtlen)
                break;
        }
    }

    while (txtlen > 0) {
        txt[--txtlen] = ' ';
    }
}
#endif

/*
 * format number as frame average
 * and output frame average (frame counter MUST be prof_functions[0])
 */
static void tst_format_num_frame(unsigned long num, char txt[], int txtlen)
{
    int s=0;    /* thousands separator */

    txt[--txtlen] = '\0';
    /* frame average (rounded) */
    int av = (num*10+prof_functions[0].num/2) / prof_functions[0].num;
    num = av;

    txt[--txtlen] = '0' + num % 10;
    num /= 10;
    txt[--txtlen] = '.';

    for (;;) {
        ++s;
        txt[--txtlen] = '0' + num % 10;
        num /= 10;
        if (!num)
            break;
        if (!txtlen) {
            txt[txtlen] = '>';
            break;
        }
        if (s%3 == 0) {
            txt[--txtlen] = ',';
            if (!txtlen)
                break;
        }
    }

    while (txtlen > 0) {
        txt[--txtlen] = ' ';
    }
}

/* print results of profiling */
void tst_profiling_result(void)
{
    int i;
    char txt[16]; /* formatted number */
//  #define FRM_CNT(num)     tst_format_number(num, txt, ARRAY_LEN(txt))
    #define FRM_CNT(num)     tst_format_num_frame(num, txt, ARRAY_LEN(txt))
    DEBUGF("*** PROFILING RESULTS *** (All Numbers are average per Frame)\n");

    /* FUNCTIONS */
    DEBUGF("*** FUNCTION CALLS ***\n");
    i = 0;
    do {
        FRM_CNT(prof_functions[i].num);
        DEBUGF(txt);
        DEBUGF("   %s\n", prof_functions[i].name);
        ++i;
    } while (i < (int) ARRAY_LEN(prof_functions));

    /* print tia write access */
    DEBUGF("*** TIA WRITE ***\n");
    i = 0;
    do {
        if (i==0x00) DEBUGF("\t\t VSYNC  \t VBLANK \t WSYNC  \t RSYNC\n");
        if (i==0x04) DEBUGF("\t\t NUSIZ0 \t NUSIZ1 \t COLUP0 \t COLUP1\n");
        if (i==0x08) DEBUGF("\t\t COLUPF \t COLUBK \t CTRLPF \t REFP0\n");
        if (i==0x0c) DEBUGF("\t\t REFP1  \t PF0    \t PF1    \t PF2\n");
        if (i==0x10) DEBUGF("\t\t RESP0  \t RESP1  \t RESM0  \t RESM1\n");
        if (i==0x14) DEBUGF("\t\t RESBL  \t AUDC0  \t AUDC1  \t AUDF0\n");
        if (i==0x18) DEBUGF("\t\t AUDF1  \t AUDV0  \t AUDV1  \t GRP0\n");
        if (i==0x1C) DEBUGF("\t\t GRP1   \t ENAM0  \t ENAM1  \t ENABL\n");
        if (i==0x20) DEBUGF("\t\t HMP0   \t HMP1   \t HMM0   \t HMM1\n");
        if (i==0x24) DEBUGF("\t\t HMBL   \t VDELP0 \t VDELP1 \t VDELBL\n");
        if (i==0x28) DEBUGF("\t\t RESMP0 \t RESMP1 \t HMOVE  \t HMCLR\n");
        if (i==0x2c) DEBUGF("\t\t CXCLR  \t 0x2d   \t 0x2e   \t >=0x2f\n");
        DEBUGF("    %02x: ", i);
        for (int j=0; j<4; ++j) {
            FRM_CNT(prof_tia_write[i++]);
            DEBUGF(txt);
        }
        DEBUGF("\n");
    } while (i < (int) ARRAY_LEN(prof_tia_write));

    /* print tia read access */
    DEBUGF("*** TIA READ ***\n");
    i = 0;
    do {
        if (i==0x00) DEBUGF("\t\t CXM0P  \t CXM1P  \t CXP0FB \t CXP1FB\n");
        if (i==0x04) DEBUGF("\t\t CXM0FB \t CXM1FB \t CXBLPF \t CXPPMM\n");
        if (i==0x08) DEBUGF("\t\t INPT0  \t INPT1  \t INPT2  \t INPT3\n");
        if (i==0x0c) DEBUGF("\t\t INPT4  \t INPT5  \t 0x0e   \t >=0x0f\n");
        DEBUGF("    %02x: ", i);
        for (int j=0; j<4; ++j) {
            FRM_CNT(prof_tia_read[i++]);
            DEBUGF(txt);
        }
        DEBUGF("\n");
    } while (i < (int) ARRAY_LEN(prof_tia_read));

    /* print riot write access */
    DEBUGF("*** RIOT WRITE ***\n");
    i = 0;
    do {
        if (i==0x00) DEBUGF("\t\t SWCHA  \t SWACNT \t SWCHB  \t SWBCNT \n");
        if (i==0x04) DEBUGF("\t\t INTIM  \t -      \t -      \t - \n");
        if (i==0x14) DEBUGF("\t\t TIM1T  \t TIM8T  \t TIM64T \t T1024T \n");
        DEBUGF("   %03x: ", i+0x280);
        for (int j=0; j<4; ++j) {
            FRM_CNT(prof_riot_write[i++]);
            DEBUGF(txt);
        }
        DEBUGF("\n");
    } while (i < (int) ARRAY_LEN(prof_riot_write));

    /* print riot read access */
    DEBUGF("*** RIOT READ ***\n");
    i = 0;
    do {
        if (i==0x00) DEBUGF("\t\t SWCHA  \t SWACNT \t SWCHB  \t SWBCNT \n");
        if (i==0x04) DEBUGF("\t\t INTIM  \t -      \t -      \t - \n");
        if (i==0x14) DEBUGF("\t\t TIM1T  \t TIM8T  \t TIM64T \t T1024T \n");
        DEBUGF("   %03x: ", i+0x280);
        for (int j=0; j<4; ++j) {
            FRM_CNT(prof_riot_read[i++]);
            DEBUGF(txt);
        }
        DEBUGF("\n");
    } while (i < (int) ARRAY_LEN(prof_riot_read));

    /* print cpu instructions */
    DEBUGF("*** CPU INSTRUCTIONS ***\n");
    i = 0;
    do {
        DEBUGF("    %02x: ", i);
        for (int j=0; j<4; ++j) {
            FRM_CNT(prof_cpu_instruction[i++]);
            DEBUGF(txt);
        }
        DEBUGF("\n");
    } while (i < (int) ARRAY_LEN(prof_cpu_instruction));
}

#endif

// ----------------------------------------------------------

#if TST_FRAME_STAT_ENABLE
struct tst_frame_stat tst_frame_stat;
#endif

// ----------------------------------------------------------


#if TST_TIME_MEASURE

int menu_test_tmeasure(void)
{
    bool exit_menu=false;
    int selected=0;
    int result;
    int ret = 0;

    MENUITEM_STRINGLIST(menu, "Time Measure Menu", NULL,
        "Average " TST_TIME_MEASURE_FRAMES_S " Frames",
        TST_TIME_MEASURE_FRAMES_S " Frames w/o Playfield",
        TST_TIME_MEASURE_FRAMES_S " Frames w/o Player",
        TST_TIME_MEASURE_FRAMES_S " Frames w/o Render",
        TST_TIME_MEASURE_FRAMES_S " Frames w/o LCD output",
        "CPU/Mainloop only (w/o LCD output & TIA access)",
    );

    while (!exit_menu) {
        result = rb->do_menu(&menu, &selected, NULL, false);
        switch (result) {
        case 0:
            tst_time_measure = 1;
            screen.lockfps = 0;    /* set framerate to "unlimited" */
            exit_menu = true;
            ret = 1;
            break;
        case 1:
            tst_time_measure = 2;
            screen.lockfps = 0;    /* set framerate to "unlimited" */
            exit_menu = true;
            ret = 1;
            break;
        case 2:
            tst_time_measure = 3;
            screen.lockfps = 0;    /* set framerate to "unlimited" */
            exit_menu = true;
            ret = 1;
            break;
        case 3:
            tst_time_measure = 4;
            screen.lockfps = 0;    /* set framerate to "unlimited" */
            exit_menu = true;
            ret = 1;
            break;
        case 4:
            tst_time_measure = 5;
            screen.lockfps = 0;    /* set framerate to "unlimited" */
            exit_menu = true;
            ret = 1;
            break;
        case 5:
            tst_time_measure = 6;
            screen.lockfps = 0;    /* set framerate to "unlimited" */
            exit_menu = true;
            ret = 1;
            break;
        default:
            exit_menu = true;
        }
    }
    return ret;
}

#endif /* TST_TIME_MEASURE */

// ----------------------------------------------------------

#if TST_TIME_USEC

void tst_time_usec_results(void)
{
    static char *usec_descr[TST_USEC_NUM] = TST_USEC_DESCRIPTION;

    /* special case: calculate frame time w/o LCD output */
    tst_usec[TST_USEC_F_WO_LCD] = tst_usec[TST_USEC_FRAME]-tst_usec[TST_USEC_LCD];

    /* time measurement ended: print results */
    for (int i=0; i<TST_USEC_NUM; ++i) {
        unsigned long time = tst_usec[i] / TST_USEC_NUMFRAMES;
        rb->splashf(HZ/2, "%s %lu.%03lu ms", usec_descr[i],
            time/1000,time%1000);
        rb->button_clear_queue();
        while (!rb->button_get(10))
            ;
    }
}

#endif /* TST_TIME_USEC */

// ----------------------------------------------------------

/*
 * test function menu.
 * Separated from rb menu for easier disabling.
 */
int menu_test(void)
{
    bool exit_menu=false;
    int  new_setting;
    int selected=0;
    int result;
    int ret = 0;

    (void) new_setting;

    static const struct opt_items noyes[] = {
        { "No", -1 },
        { "Yes", -1 },
    };


    MENUITEM_STRINGLIST(menu, "Test Menu", NULL,
#if TST_FRAME_STAT_ENABLE
                        "Show frame statistics",
#endif
#if TST_TIME_MEASURE
                        "Measure ms/frame Time",
#endif
#if TST_PROFILE_FUNC
                        "Start Function Profiling",
                        "End Function Profiling",
#endif
#if TST_TIME_USEC
                        "Measure Time (usec)",
#endif
    );

    enum {
        MENU_DUMMY_START = -1,   /* first entry must be 0! */
#if TST_FRAME_STAT_ENABLE
        MENU_FRAMESTAT,
#endif
#if TST_TIME_MEASURE
        MENU_TMEASURE,
#endif
#if TST_PROFILE_FUNC
        MENU_PROFILE_START,
        MENU_PROFILE_END,
#endif
#if TST_TIME_USEC
        MENU_TIME_USEC,
#endif
    };

    while (!exit_menu) {
        result = rb->do_menu(&menu, &selected, NULL, false);
        switch (result) {
#if TST_FRAME_STAT_ENABLE
            case MENU_FRAMESTAT:
                rb->set_option("Show frame statistics", &tst_frame_stat.show,
                    INT, noyes, 2, NULL);
                break;
#endif
#if TST_TIME_MEASURE
            case MENU_TMEASURE:
                ret = menu_test_tmeasure();
                if (ret)
                    exit_menu = true;
                break;
#endif
#if TST_PROFILE_FUNC
            case MENU_PROFILE_START:
                start_profiling = true;
                rb->splash(HZ, "Started profiling...");
                break;
            case MENU_PROFILE_END:
                start_profiling = false;    /* note: results don't get resetted! */
                tst_profiling_result();
                rb->splash(HZ, "Profiling results written.");
                break;
#endif
#if TST_TIME_USEC
            case MENU_TIME_USEC:
                /* clear old results */
                for (int i=0; i<TST_USEC_NUM; ++i)
                    tst_usec[i] = 0;
                /* arm time measurement */
                tst_time_usec_start = -1;
                ret = 1;
                exit_menu = true;
                break;
#endif
            default:
                exit_menu = true;
        }
    }
    return ret;
}

// --------------------------------------------

void init_tests(void) COLD_ATTR;
void init_tests(void)
{
    TST_FRAME_STAT_INIT();

}
