/*****************************************************************************

   This file is part of x2600, the Atari 2600 Emulator
   ===================================================

   Copyright 1996 Alex Hornby. For contributions see the file CREDITS.

   This software is distributed under the terms of the GNU General Public
   License. This is free software with ABSOLUTELY NO WARRANTY.

   See the file COPYING for details.

   $Id: vmachine.h,v 2.13 1996/11/24 16:55:40 ahornby Exp $
******************************************************************************/


/*
   External definitions of the 2600 virtual machine.
*/


#ifndef VMACHINE_H
#define VMACHINE_H


extern CLOCK clk;       /* Aggregate */

extern bool tia_access;
extern bool tia_wsync;
extern int tia_register;
extern int tia_value;

extern BYTE keypad[2][4];

extern int ebeamx, ebeamy;


enum tv_system {
    TV_UNCHANGED = -1, /* keep current setting */
    TV_NTSC = 0,
    TV_PAL,
    TV_SECAM,
};

struct tv {
    int width,
        height;
    int vsync,
        vblank,
        overscan;
    int frame;
    int hertz;
    int hsync;
};
extern struct tv tv;

extern struct Paddle {
    int pos;
    int val;
} paddle[4];

extern int ebeamx;
extern int ebeamy;
extern int sync_state;

// sync-states can be ORed together
#define SYNC_STATE_NORMAL           0
#define SYNC_STATE_VSYNC_WRITTEN    1   // sync has just been set
#define SYNC_STATE_VSYNC_RUNNING    2
#define SYNC_STATE_VBLANK           4

extern int draw_x_min;
extern bool render_enable;

/* frame height (pal) =  vsync(3)+vblank(45)+visible(228)+overscan(36) = 312
   add a few lines for safety */
#define TV_SCREEN_HEIGHT    320
#define TV_SCREEN_WIDTH     160     /* visible pixels */
#define TV_SCREEN_HSYNC      68     /* pixels within HSYNC */

/* use VCS 8bit colours until rendering to RB frame buffer */
extern BYTE tv_screen[TV_SCREEN_HEIGHT][TV_SCREEN_WIDTH];

#ifdef TST_FRAME_CNT
extern unsigned frame_counter;
#endif


/****************************************************************************
                              functions.
*****************************************************************************/


extern void init_hardware(void);
extern void init_banking(void);
void init_screen(int tv_system);

extern int  do_paddle(int padnum);

extern void start_machine (void);

#endif
