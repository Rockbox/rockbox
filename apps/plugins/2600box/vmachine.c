/*****************************************************************************

   This file is part of x2600, the Atari 2600 Emulator
   ===================================================

   Copyright 1996 Alex Hornby. For contributions see the file CREDITS.

   This software is distributed under the terms of the GNU General Public
   License. This is free software with ABSOLUTELY NO WARRANTY.

   See the file COPYING for details.

   $Id: vmachine.c,v 2.22 1997/11/22 14:27:47 ahornby Exp $
******************************************************************************/


/*
   The virtual machine.
 */

#include "rbconfig.h"
#include "rb_test.h"

#include "address.h"
#include "options.h"
#include "raster.h"
#include "mos6502.h"
#include "keyboard.h"
#include "vmachine.h"

#include "memory.h"
#include "riot.h"
#include "tia_video.h"
#include "raster.h"

#include "rb_menu.h"
#include "rb_keymap.h"
#include "rb_lcd.h"

/* Aggregated clock count */
CLOCK clk;

/* a register number >=0 indicate tia write access inside the cpu loop.
 * tia register changes are processed immediately.
 */
int tia_register;
int tia_value;

unsigned sync_state;
int draw_x_min = 0;
int draw_end;

#ifdef TST_FRAME_CNT
unsigned frame_counter;
#endif

/* use VCS 8bit colours until rendering to RB frame buffer */
BYTE tv_screen[TV_SCREEN_HEIGHT][TV_SCREEN_WIDTH];


BYTE keypad[2][4];

int tia_delay;

/* tia delay in pixel. A negative number indicates that we don't need to tabellenform
 * render graphic. */
int16_t tia_delay_tab[0x40] = {
     0,     /* VSYNC   0x00 */
     0,     /* VBLANK  0x01 */
     0,     /* WSYNC   0x02 */
    -1,     /* RSYNC   0x03 */
     0,     /* NUSIZ0  0x04 */
     0,     /* NUSIZ1  0x05 */
     0,     /* COLUP0  0x06 */
     0,     /* COLUP1  0x07 */
     0,     /* COLUPF  0x08 */
     0,     /* COLUBK  0x09 */
     0,     /* CTRLPF  0x0A */
     1,     /* REFP0   0x0B */
     1,     /* REFP1   0x0C */
     2,     /* PF0     0x0D */
     2,     /* PF1     0x0E */
     2,     /* PF2     0x0F */
   13,//  9,     /* RESP0   0x10 */ /* NOTE: the actual pixel delay is 5, but I take a */
    13,// 9,     /* RESP1   0x11 */ /* higher number to hide the first copy of the player sprite */
     0,     /* RESM0   0x12 */
     0,     /* RESM1   0x13 */
     0,     /* RESBL   0x14 */
    -1,     /* AUDC0   0x15 */
    -1,     /* AUDC1   0x16 */
    -1,     /* AUDF0   0x17 */
    -1,     /* AUDF1   0x18 */
    -1,     /* AUDV0   0x19 */
    -1,     /* AUDV1   0x1A */
     1,     /* GRP0    0x1B */
     1,     /* GRP1    0x1C */
     1,     /* ENAM0   0x1D */
     1,     /* ENAM1   0x1E */
     1,     /* ENABL   0x1F */
    -1,     /* HMP0    0x20 */
    -1,     /* HMP1    0x21 */
    -1,     /* HMM0    0x22 */
    -1,     /* HMM1    0x23 */
    -1,     /* HMBL    0x24 */
     0,     /* VDELP0  0x25 */
     0,     /* VDELP1  0x26 */
     0,     /* VDELBL  0x27 */
     0,     /* RESMP0  0x28 */
     0,     /* RESMP1  0x29 */
     0,     /* HMOVE   0x2A */
    -1,     /* HMCLR   0x2B */
    -1,     /* CXCLR   0x2C */
    -1,     /* 0x2D */
    -1,     /* 0x2E */
    -1,     /* 0x2F */
    -1,     /* 0x30 */
    -1,     /* 0x31 */
    -1,     /* 0x32 */
    -1,     /* 0x33 */
    -1,     /* 0x34 */
    -1,     /* 0x35 */
    -1,     /* 0x36 */
    -1,     /* 0x37 */
    -1,     /* 0x38 */
    -1,     /* 0x39 */
    -1,     /* 0x3A */
    -1,     /* 0x3B */
    -1,     /* 0x3C */
    -1,     /* 0x3D */
    -1,     /* 0x3E */
    -1,     /* 0x3F */
};

/* Electron beam position */
int ebeamx, ebeamy;


/* The tv size, varies with PAL/NTSC */
struct tv tv;

struct Paddle
/*  {
    int pos;
    int val;
  } */
paddle[4];



/***************************************************************************
                           Let the functions begin!
****************************************************************************/

/* Device independent screen initialisations */
void init_tv(const int tv_system) COLD_ATTR;
void init_tv(const int tv_system)
{
    /* Set the electron beam to the top left */
    ebeamx = -TV_SCREEN_HSYNC;
    ebeamy = 0;

    //base_opts.tvtype = NTSC;
    base_opts.tvtype = tv_system;

    //tv.vsync = 3; // not used.
    //tv.hsync = 68; // not used. use #define instead!
    //tv.width = 160; // now only the define is used
    switch (tv_system) {
    /* the recommended numbers, see Stella Programmer's Guide */
    case TV_NTSC:
        tv.height = 192;    /* visible height, "Kernal" */
        tv.vblank = 37;     /* excluding VSYNC (3 lines) */
        tv.overscan = 30;
        tv.frame = 262;     /* vsync+vblank+height+overscan */
        tv.hertz = 60;
        break;
    case TV_PAL:
    case TV_SECAM:
        tv.height = 228;
        tv.vblank = 45;          /* excluding VSYNC (3 lines) */
        tv.overscan = 36;
        tv.frame = 312;
        tv.hertz = 50;
        break;
    }

    screen.render_start = 0;
    screen.render_end = tv.frame;
}



/* Main hardware startup */
void init_hardware (void) COLD_ATTR;
void init_hardware (void)
{
    init_tv(TV_NTSC);
    init_riot();
    init_tia();
    init_raster();
    init_memory();
    init_banking();
    init_cpu();
#ifdef TST_FRAME_CNT
    frame_counter = 0;
#endif
    clk = 0;        /* reset global clock counter */
}


extern inline int do_paddle (int padnum)
{
  int res = 0x80;
  int x;

  (void) padnum;
  (void) x;

  if ((tiaWrite[VBLANK] & 0x80) == 0)
    {
#if 0
      x = 320 - mouse_position ();
      x = x * 45;
      x += paddle[padnum].val;
      if ((unsigned)x > clk)
        res = 0x00;
#endif
    }
  return res;
}


/* ============================================================= */


/*
 * return: 1 if "Quit" is choosen in the menu
 */
static inline int do_hsync(void)
{
    TST_PROFILE(PRF_HSYNC, "do_hsync()");
#if TST_TIME_USEC
    unsigned long tmp_usec;
#endif

    if (ebeamy > TV_SCREEN_HEIGHT) {
        TST_ALERT(0, ALERT_VID_NOSYNC);
        goto new_frame;
    }

    if (sync_state & SYNC_STATE_VSYNC_WRITTEN) {
        sync_state &= ~SYNC_STATE_VSYNC_WRITTEN;
        sync_state |= SYNC_STATE_VSYNC_RUNNING; // should have been done!
new_frame:
        /* first VSYNC line */
#if TST_TIME_USEC
        if (tst_time_usec_start > 0) {
            tmp_usec = USEC_TIMER;
        }
#endif
        tv_display ();
#if TST_TIME_USEC
        if (tst_time_usec_start > 0) {
            tmp_usec = USEC_TIMER - tmp_usec;
            tst_usec[TST_USEC_LCD] += tmp_usec;
        }
#endif
#if TST_TIME_USEC
        if (tst_time_usec_start) {
            if (tst_time_usec_start < 0) {
                /* armed; now start measurement! */
                tst_time_usec_start = TST_USEC_NUMFRAMES;
                tst_usec[TST_USEC_FRAME] = USEC_TIMER;
            }
            else {
                --tst_time_usec_start;
                if (tst_time_usec_start==0) {
                    /* measurement ended */
                    tst_usec[TST_USEC_FRAME] = USEC_TIMER - tst_usec[TST_USEC_FRAME];
                    tst_time_usec_results();
                }
            }
        }
#endif

#ifdef TST_FRAME_CNT
        ++frame_counter;
#endif
        ebeamy = 0;
        TST_FRAME_STAT_ZERO();

        /* "remove this when keybrd driver is ready" */
        int buttons = rb->button_status();
#ifdef SIMULATOR
        /* fix exit with "queue_post ovl" message in simulator */
        rb->button_clear_queue();
#endif
#if defined(PLUGIN_QUIT) && defined(HOLD_IS_MENU)
        if (buttons & PLUGIN_QUIT || rb->button_hold()) {
#elif defined(PLUGIN_QUIT)
        if (buttons & PLUGIN_QUIT) {
#elif defined(HOLD_IS_MENU)
        if (rb->button_hold()) {
#endif
            int quit = do_a2600_menu();
            set_fps_timer();
            if (quit==USER_MENU_QUIT)
                return 1;
        }
    }
    else if (sync_state & SYNC_STATE_VSYNC_RUNNING) {
        TST_FRAME_STAT(vsynced);
    }
    else if (sync_state & SYNC_STATE_VBLANK) {
        ++ebeamy;
        TST_FRAME_STAT_VBLANK();
    }
    else if (sync_state == SYNC_STATE_NORMAL) {
        ++ebeamy;
        if (ebeamy < screen.render_start || ebeamy > screen.render_end)
            sync_state = TV_STATE_BLANK;
        TST_FRAME_STAT(drawn);
    }
    else { // sync_state == TV_STATE_BLANK
        ++ebeamy;
        if (ebeamy >= screen.render_start && ebeamy <= screen.render_end)
            sync_state = SYNC_STATE_NORMAL;
        TST_FRAME_STAT_VBLANK(); // count as blank, even the arbitrary skipped ones
    }

    return 0;
}


void mainloop (void)
{
    for (;;) {
#if TST_TIME_USEC
        unsigned long tmp_usec;
        if (tst_time_usec_start > 0) {
            tmp_usec = USEC_TIMER;
        }
#endif
        do_cpu();
#if TST_TIME_USEC
        if (tst_time_usec_start > 0) {
            tmp_usec = USEC_TIMER - tmp_usec;
            tst_usec[TST_USEC_CPU] += tmp_usec;
        }
#endif

        if (tia_register >= 0) {
            // assure that tia_register is always < 0x40
            TST_ASSERT(tia_register<0x40, "vmachine:tia_reg>=0x40");
            if (tia_register==0x02) { /* TIA WSYNC */
                /* NOTE: 8bitworkshop's emulator set x to -62 after performing sta WSYNC!
                   stella counts differently, but behaves similar: if WSYNC is hit at pixel
                   pos 154 x is set to 0 (~-68), but an additional line is skipped. */
                TST_FRAME_STAT(wsynced);
# ifdef USE_DIV_3_TABLE
                clk += div3tab[ebeamx+TV_SCREEN_HSYNC]; // needed by timer
                ebeamx += div3pixel[ebeamx+TV_SCREEN_HSYNC];
# else
                int tmp = (160-ebeamx);
                tmp = tmp<0 ? tmp+228 : tmp;
                clk += tmp/3;
                ebeamx += tmp;
# endif
            }

            tia_delay = tia_delay_tab[tia_register];

            if (!sync_state && tia_delay >= 0) {
                do_raster(draw_x_min, ebeamx + tia_delay);
                draw_x_min = ebeamx + tia_delay;
            }

            if (ebeamx >= 160) {
                // end of line
                ebeamx -= 228;
                draw_x_min = 0;
                if (do_hsync())
                    return; // selected "quit" in the menu
            }

#  if TST_TIME_USEC
            if (tst_time_usec_start > 0) {
                tmp_usec = USEC_TIMER;
            }
#  endif

            (*tia_call_tab[tia_register])(tia_register, tia_value);

#  if TST_TIME_USEC
            if (tst_time_usec_start > 0) {
                tmp_usec = USEC_TIMER - tmp_usec;
                tst_usec[TST_USEC_TIA] += tmp_usec;
            }
#  endif
            tia_register = -1;
        }
        else if (ebeamx >= 160) {
            if (!sync_state) {
                do_raster(draw_x_min, ebeamx);
            }
            ebeamx -= 228;
            draw_x_min = 0;
            if (do_hsync())
                return; // "Quit" was selected in the menu
        }
    } /* for (;;) */
}


void start_machine(void)
{
    init_hardware ();
    ebeamx = -TV_SCREEN_HSYNC;
    draw_x_min = 0;

    set_fps_timer();
    mainloop();
    return;
}
