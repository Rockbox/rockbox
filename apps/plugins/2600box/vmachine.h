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

extern int rom_size;
extern BYTE theCart[16384];
extern BYTE *theRom;
extern BYTE cartScratch[4096];
extern BYTE cartRam[1024];
extern BYTE theRam[128];
extern BYTE riotRead[0x298];
extern BYTE riotWrite[0x298];
extern BYTE tiaWrite[0x2d];
extern BYTE tiaRead[0x0e];
extern BYTE keypad[2][4];

extern int reset_flag;

extern int ebeamx, ebeamy, sbeamx;

#define VSYNCSTATE 1
#define VBLANKSTATE 2
#define HSYNCSTATE 4
#define DRAWSTATE 8
#define OVERSTATE 16 

extern int vbeam_state, hbeam_state;

extern int tv_width, tv_height, tv_vsync, tv_vblank,
    tv_overscan, tv_frame, tv_hertz, tv_hsync;

extern int timer_res, timer_count, timer_clks; 

extern struct PlayField {
    BYTE pf0,pf1,pf2;
    BYTE ref;
} pf[2];  

extern struct Paddle {
    int pos;
    int val;
} paddle[4];

extern struct Player {
  int x;
  BYTE grp;
  BYTE hmm;
  BYTE vdel;
  BYTE vdel_flag;
  BYTE nusize;
  BYTE reflect;
  BYTE mask;
} pl[2];

struct RasterChange {
  int x;     /* Position at which change happened */
  int type;  /* Type of change */
  int val;   /* Value of change */
} ;

extern struct RasterChange pl_change[2][80], pf_change[1][80], unified[80];

extern int pl_change_count[2], pf_change_count[1], unified_count;

/* The missile and ball positions */
extern struct Missile {
    int x;
    BYTE hmm;
    BYTE locked;
    BYTE enabled;
    BYTE width;
    BYTE vdel;
    BYTE vdel_flag;
    BYTE mask;
} ml[3];

/****************************************************************************
                              functions.
*****************************************************************************/

inline void 
do_plraster_change(int i, int type, int val);

inline void
do_unified_change(int type, int val);

inline void
use_unified_change( struct RasterChange *rc);

inline void 
use_plraster_change( struct Player *pl, struct RasterChange *rc);

inline void
do_pfraster_change(int i, int type, int val);

inline void
use_pfraster_change( struct PlayField *pf, struct RasterChange *rc);

void init_hardware(void);
void init_banking(void);

inline void set_timer(int res, int count, int clkadj);
inline BYTE do_timer(int clkadj);
inline void do_vblank(BYTE b);
inline void do_hsync(void);

int  do_paddle(int padnum);
BYTE do_keypad(int pad, int col);
inline void do_screen(int clks);

#endif






