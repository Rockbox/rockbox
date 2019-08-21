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


#include "rb_menu.h"
#include "rb_keymap.h"
#include "rb_lcd.h"

/* Aggregated clock count */
CLOCK clk;

/* this flag indicates tia register access inside the cpu loop.
 * tia register changes are processed immediately.
 */
bool tia_access = false;
bool tia_wsync = false;     /* tia triggered WSYNC (TIA register 0x02) */
int tia_register;
int tia_value;

int sync_state;
int draw_x_min = 0;
int draw_end;

bool render_enable = true;

#ifdef TST_FRAME_CNT
unsigned frame_counter;
#endif

/* use VCS 8bit colours until rendering to RB frame buffer */
BYTE tv_screen[TV_SCREEN_HEIGHT][TV_SCREEN_WIDTH];


BYTE keypad[2][4];


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
void init_screen (int tv_system) COLD_ATTR;
void init_screen (int tv_system)
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
    init_screen(TV_NTSC);
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
        goto new_frame;
    }

    if (sync_state & SYNC_STATE_VSYNC_WRITTEN) {
        sync_state &= ~SYNC_STATE_VSYNC_WRITTEN;
        sync_state |= SYNC_STATE_VSYNC_RUNNING; // should have been done!
#if TST_DUMP_TIA_ACCESS
        ++xyz; // debug message frame counter
#endif
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
#if defined(PLUGIN_QUIT)
        if (buttons & PLUGIN_QUIT) {
            int quit = do_a2600_menu();
            set_fps_timer();
            if (quit==USER_MENU_QUIT)
                return 1;
        }
#endif
#if defined(HOLD_IS_MENU)
        if (rb->button_hold()) {
            int quit = do_a2600_menu();
            set_fps_timer();
            if (quit==USER_MENU_QUIT)
                return 1;
        }
#endif

    }
    else if (sync_state & SYNC_STATE_VSYNC_RUNNING) {
        TST_FRAME_STAT(vsynced);
    }
    else if (sync_state & SYNC_STATE_VBLANK) {
        ++ebeamy;
        TST_FRAME_STAT_VSYNC();
    }
    else if (sync_state == SYNC_STATE_NORMAL) {
        ++ebeamy;
        TST_FRAME_STAT(drawn);
    }

    return 0;
}

extern int div3tab[];
extern int div3pixel[];

/* The main emulation loop, performs the CPU emulation */
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

        if (tia_access) {
            tia_access = false;

            if (ebeamx >= 160) {
                // end of line
                ebeamx -= 228;
                draw_x_min = 0;
                if (do_hsync())
                    return; // selected "quit" in the menu
                if (sync_state == SYNC_STATE_NORMAL
                        && ebeamy >= screen.render_start && ebeamy <= screen.render_end)
                    render_enable = true;
                else
                    render_enable = false;
            }

#  if TST_TIME_USEC
            //unsigned long tmp_usec;
            if (tst_time_usec_start > 0) {
                tmp_usec = USEC_TIMER;
            }
#  endif
            do_tia_write(tia_register, tia_value);
            tia_register = 255; tia_value=0; // only for debugging
#  if TST_TIME_USEC
            if (tst_time_usec_start > 0) {
                tmp_usec = USEC_TIMER - tmp_usec;
                tst_usec[TST_USEC_TIA] += tmp_usec;
            }
#  endif
        }

        else if (ebeamx >= 160) {
            if (render_enable) {
#if TST_DUMP_TIA_ACCESS
                if ((frame_counter==1  && ebeamy>-1)||(frame_counter==2 && ebeamy<50) ) {
                    DEBUGF("Draw Line %d from %d to %d (tiaReg=0x%x / val=0x%x) --hsync w/o wait--\n",
                      ebeamy, draw_x_min, ebeamx, tia_register, tia_value);
                }
#endif
                do_raster(draw_x_min, ebeamx);
                TST_FRAME_STAT(unsynced);
            }
            // End of line
            ebeamx -= 228;
            draw_x_min = 0;
            if (do_hsync())
                return; // "Quit" was selected in the menu
            if (sync_state == SYNC_STATE_NORMAL
                    && ebeamy >= screen.render_start && ebeamy <= screen.render_end)
                render_enable = true;
            else
                render_enable = false;
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
