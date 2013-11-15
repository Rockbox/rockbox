/*****************************************************************************

   This file is part of x2600, the Atari 2600 Emulator
   ===================================================
   
   Copyright 1996 Alex Hornby. For contributions see the file CREDITS.

   This software is distributed under the terms of the GNU General Public
   License. This is free software with ABSOLUTELY NO WARRANTY.
   
   See the file COPYING for Details.
   
   $Id: memory.c,v 2.23 1997/04/06 02:19:12 ahornby Exp $
******************************************************************************/

/* 
 * Holds the memory access routines to both memory and memory mapped
 * i/o, hence memory.c
 *
 * Uses GNU C extensions.
 */
#include "rbconfig.h"
#include <stdio.h>
#include "types.h"
#include "address.h"
#include "vmachine.h"
#include "misc.h"
#include "display.h"
#include "raster.h"
#include "limiter.h"
#include "collision.h"
#include "col_mask.h"
#include "options.h"
#include "keyboard.h"
#include "sound.h"
#include "dbg_mess.h"

extern CLOCK clkcount;
extern CLOCK clk;
extern int beamadj;

/* Undecoded Read, for executable code etc. */
/* a: address to read */
/* returns: byte at address a */
BYTE
undecRead (ADDRESS a)
{
  if (a & 0x1000)
    return theRom[a & 0xfff];
  else
    return theRam[a & 0x7f];
}


inline void
bank_switch_write (ADDRESS a, BYTE b)
{
  a&=0xfff;
  switch (base_opts.bank)
    {

    case 1:
      /* Atari 8k F8 */
      switch (a)
	{
	case 0xff8:
	  theRom = &theCart[0];
	  break;
	case 0xff9:
	  theRom = &theCart[4096];
	  break;
	}
      break;

    case 2:
      /* Atari 16k F6 */
      switch (a)
	{
	case 0xff6:
	  theRom = &theCart[0];
	  break;
	case 0xff7:
	  theRom = &theCart[4096];
	  break;
	case 0xff8:
	  theRom = &theCart[8192];
	  break;
	case 0xff9:
	  theRom = &theCart[12288];
	  break;
	}
      break;

    case 3:
      /* Parker Brothers 8k E0 */
      {
	ADDRESS a1;
	if (a > 0xfdf && a < 0xfe8) 
	  {
	    a1=(a&0x07)<<10;
	    memcpy(&cartScratch[0],&theCart[a1],0x400);
	  }
	else if (a > 0xfe7 && a < 0xff0) 
	  {
	    a1=(a&0x07)<<10;
	    memcpy(&cartScratch[0x400],&theCart[a1],0x400);
	  } 
	else if (a > 0xfef && a < 0xff8) 
	  {
	    a1=(a&0x07)<<10;
	    memcpy(&cartScratch[0x800],&theCart[a1],0x400);
	  }
      }
      break;

    case 4:
      /* CBS Ram Plus FA */
      if (a < 0x100) 
	cartRam[a & 0xff]=b;
      else
	{	  
	  switch (a)
	    {
	    case 0xff8:
	      theRom = &theCart[0];
	      break;
	    case 0xff9:
	      theRom = &theCart[4096];
	      break;
	    case 0xffa:
	      theRom = &theCart[8192];
	      break;
	    }
	}
      break;

    case 5:
      /* Atari 16k + super chip ram F6SC */
      if (a < 0x80) 
	cartRam[a & 0x7f] = b;
      else
	{
	  switch (a)
	    {
	    case 0xff6:
	      theRom = &theCart[0];
	      break;
	    case 0xff7:
	      theRom = &theCart[4096];
	      break;
	    case 0xff8:
	      theRom = &theCart[8192];
	      break;
	    case 0xff9:
	      theRom = &theCart[12288];
	      break;
	    }
	}
      break;
    }
}

inline BYTE
bank_switch_read (ADDRESS a)
{
  BYTE res;

  a&=0xfff;
  switch (base_opts.bank)
    {
    case 1:
      /* Atari 8k F8 */
      switch (a)
	{
	case 0xff8:
	  theRom = &theCart[0];
	  break;
	case 0xff9:
	  theRom = &theCart[4096];
	  break;
	}
      res=theRom[a];
      break;

    case 2:
      /* Atari 16k F6 */
      switch (a)
	{
	case 0xff6:
	  theRom = &theCart[0];
	  break;
	case 0xff7:
	  theRom = &theCart[4096];
	  break;
	case 0xff8:
	  theRom = &theCart[8192];
	  break;
	case 0xff9:
	  theRom = &theCart[12288];
	  break;
	}
      res=theRom[a];
      break;

    case 3:
      /* Parker Brothers 8k E0 */
      /* Parker Brothers 8k E0 */
      {
	ADDRESS a1;
	if (a > 0xfdf && a < 0xfe8) 
	  {
	    a1=(a&0x07)<<10;
	    memcpy(&cartScratch[0],&theCart[a1],0x400);
	  }
	else if (a > 0xfe7 && a < 0xff0) 
	  {
	    a1=(a&0x07)<<10;
	    memcpy(&cartScratch[0x400],&theCart[a1],0x400);
	  } 
	else if (a > 0xfef && a < 0xff8) 
	  {
	    a1=(a&0x07)<<10;
	    memcpy(&cartScratch[0x800],&theCart[a1],0x400);
	  }
      }
      res=theRom[a];
      break;

    case 4:
      /* CBS Ram Plus FA */
      if (a > 0xff && a < 0x200) 
	  res=cartRam[a & 0xff];
      else
	{	  
	  switch (a)
	    {
	    case 0xff8:
	      theRom = &theCart[0];
	      break;
	    case 0xff9:
	      theRom = &theCart[4096];
	      break;
	    case 0xffa:
	      theRom = &theCart[8192];
	      break;
	    }
	  res=theRom[a];
	}
      break;

    case 5:
      /* Atari 16k + super chip ram F6SC */
      if (a > 0x7f && a < 0x100) 
	res=cartRam[a & 0x7f];
      else
	{
	  switch (a)
	    {
	    case 0xff6:
	      theRom = &theCart[0];
	      break;
	    case 0xff7:
	      theRom = &theCart[4096];
	      break;
	    case 0xff8:
	      theRom = &theCart[8192];
	      break;
	    case 0xff9:
	      theRom = &theCart[12288];
	      break;
	    }
	  res=theRom[a];
	}
      break;
    default:
      res=theRom[a];
      break;
    }
  return res;
}


/* Decoded write to memory */
/* a: address written to */
/* b: byte value written */
void
decWrite (ADDRESS a, BYTE b)
{
  int i;

  /* A Write to the ROM area */
  if (a & 0x1000)
    {
      bank_switch_write (a,b);
    }
  /* A Write to the RAM area in Page 0 and 1 */
  else if ((a & 0x280) == 0x80)
    {
      theRam[a & 0x7f] = b;
    }
  /* TIA */
  else if (!(a & 0x80))
    {
      switch (a & 0x7f)
	{
	case VSYNC:
	  if (b & 0x02)
	    {
	      /* Start vertical sync */
	      vbeam_state = VSYNCSTATE;
	    }
	  break;
	case VBLANK:
	  do_vblank (b);
	  /* Ground paddle pots */
	  if (b & 0x80)
	    {
	      /* Grounded ports */
	      tiaRead[INPT0] = 0x00;
	      tiaRead[INPT1] = 0x00;
	    }
	  else
	    {
	      /* Processor now measures time for a logic 1 to appear
	         at each paddle port */
	      tiaRead[INPT0] = 0x80;
	      tiaRead[INPT1] = 0x80;
	      paddle[0].val = clk;
	      paddle[1].val = clk;
	    }
	  /* Logic for dumped input ports */
	  if (b & 0x40)
	    {
	      if (tiaWrite[VBLANK] & 0x40)
		{
		  tiaRead[INPT4] = 0x80;
		  tiaRead[INPT5] = 0x80;
		}
	      else
		{
		  read_trigger ();
		}
	    }
	  tiaWrite[VBLANK] = b;
	  break;
	case WSYNC:
	  /* Skip to HSYNC pulse */
	  do_hsync ();
	  break;
	case RSYNC:
	  /* used in chip testing */
	  dbg_message(DBG_LOTS,"ARGHH an undocumented RSYNC!\n");
	  break;
	case NUSIZ0:
	  /*
	     printf("P0 nusize: ebeamx=%d, ebeamy=%d, nusize=%02x\n",
	     ebeamx, ebeamy, (int)b);
	   */
	  pl[0].nusize = b & 0x07;
	  ml[0].width = (b & 0x30) >> 4;
	  break;
	case NUSIZ1:
	  /*
	     printf("P1 nusize: ebeamx=%d, ebeamy=%d, nusize=%02x\n",
	     ebeamx, ebeamy, (int)b);
	   */
	  pl[1].nusize = b & 0x07;
	  ml[1].width = (b & 0x30) >> 4;
	  break;
	case COLUP0:
	  do_unified_change (0, tv_color (b));
	  break;
	case COLUP1:
	  do_unified_change (1, tv_color (b));
	  break;
	case COLUPF:
	  do_unified_change (2, tv_color (b));
	  break;
	case COLUBK:
	  /*printf("BKcolour = %d, line=%d\n", (int)(b>>1), ebeamy); */
	  do_unified_change (3, tv_color (b));
	  break;
	case CTRLPF:
	  tiaWrite[CTRLPF] = b & 0x37;	/* Bitmask 00110111 */
	  do_pfraster_change (0, 3, b & 0x01);	/* Reflection */
	  
	  /* Normal/Alternate priority */
	  do_unified_change(4, (b & 0x04));
	  
	  /* Scores/Not scores */
	  do_unified_change(5, (b & 0x02));

	  break;
	case REFP0:
	  pl[0].reflect = (b & 0x08) >> 3;
	  break;
	case REFP1:
	  pl[1].reflect = (b & 0x08) >> 3;
	  break;
	case PF0:
	  do_pfraster_change (0, 0, b & 0xf0);
	  break;
	case PF1:
	  do_pfraster_change (0, 1, b);
	  break;
	case PF2:
	  do_pfraster_change (0, 2, b);
	  break;
	case RESP0:
	  /* Ghost in pacman!
	     if(beamadj == 0) {
	     printf("RESP0: ebeamx=%d, ebeamy=%d\n",
	     ebeamx, ebeamy); 
	     show();
	     } */
	  pl[0].x = ebeamx + beamadj;
	  /* As per page 20 Stella Programmers Guide */
	  if (pl[0].x < 0)
	    pl[0].x = 0;
	  break;
	case RESP1:
	  /*if(beamadj == 0) {
	     printf("RESP1: ebeamx=%d, ebeamy=%d\n",
	     ebeamx, ebeamy);
	     show(); 
	     } */
	  pl[1].x = ebeamx + beamadj;
	  /* As per page 20 Stella Programmers Guide */
	  if (pl[1].x < 0)
	    pl[1].x = 0;
	  break;
	case RESM0:
	  ml[0].x = ebeamx + beamadj;
	  /* As per page 20 Stella Programmers Guide */
	  if (ml[0].x < 0)
	    ml[0].x = 0;
	  break;
	case RESM1:
	  ml[1].x = ebeamx + beamadj;
	  /* As per page 20 Stella Programmers Guide */
	  if (ml[1].x < 0)
	    ml[1].x = 0;
	  break;
	case RESBL:
	  ml[2].x = ebeamx + beamadj;
	  /* As per page 20 Stella Programmers Guide */
	  if (ml[2].x < 0)
	    ml[2].x = 0;
	  break;
	case AUDC0:
	  sound_waveform (0, b & 0x0f);
	  break;
	case AUDC1:
	  sound_waveform (1, b & 0x0f);
	  break;
	case AUDF0:
	  sound_freq (0, b & 0x1f);
	  break;
	case AUDF1:
	  sound_freq (1, b & 0x1f);
	  break;
	case AUDV0:
	  sound_volume (0, b & 0x0f);
	  break;
	case AUDV1:
	  sound_volume (1, b & 0x0f);
	  break;
	case GRP0:
	  do_plraster_change (0, 0, b);
	  do_plraster_change (1, 1, b);
	  break;
	case GRP1:
	  do_plraster_change (1, 0, b);
	  do_plraster_change (0, 1, b);
	  ml[2].vdel = ml[2].enabled;
	  break;
	case ENAM0:
	  ml[0].enabled = b & 0x02;
	  if (tiaWrite[RESMP0])
	    ml[0].enabled = 0;
	  break;
	case ENAM1:
	  ml[1].enabled = b & 0x02;
	  if (tiaWrite[RESMP1])
	    ml[1].enabled = 0;
	  break;
	case ENABL:
	  ml[2].enabled = b & 0x02;
	  break;
	case HMP0:
	  pl[0].hmm = (b >> 4);
	  break;
	case HMP1:
	  pl[1].hmm = (b >> 4);
	  break;
	case HMM0:
	  ml[0].hmm = (b >> 4);
	  break;
	case HMM1:
	  ml[1].hmm = (b >> 4);
	  break;
	case HMBL:
	  ml[2].hmm = (b >> 4);
	  break;
	case VDELP0:
	  pl[0].vdel_flag = b & 0x01;
	  break;
	case VDELP1:
	  pl[1].vdel_flag = b & 0x01;
	  break;
	case VDELBL:
	  ml[2].vdel_flag = b & 0x01;
	  break;
	case RESMP0:
	  tiaWrite[RESMP0] = b & 0x02;
	  if (b & 0x02)
	    {
	      ml[0].x = pl[0].x + 4;
	      ml[0].enabled = 0;
	    }
	  break;
	case RESMP1:
	  tiaWrite[RESMP1] = b & 0x02;
	  if (b & 0x02)
	    {
	      ml[1].x = pl[1].x + 4;
	      ml[1].enabled = 0;
	    }
	  break;
	case HMOVE:
	  /* Player 0 */
	  if (pl[0].hmm & 0x08)
	    pl[0].x += ((pl[0].hmm ^ 0x0f) + 1);
	  else
	    pl[0].x -= pl[0].hmm;
	  if (pl[0].x > 160)
	    pl[0].x = -68;
	  else if (pl[0].x < -68)
	    pl[0].x = 160;

	  /* Player 2 */
	  if (pl[1].hmm & 0x08)
	    pl[1].x += ((pl[1].hmm ^ 0x0f) + 1);
	  else
	    pl[1].x -= pl[1].hmm;
	  if (pl[1].x > 160)
	    pl[1].x = -68;
	  else if (pl[1].x < -68)
	    pl[1].x = 160;

	  /* Missiles */
	  for (i = 0; i < 3; i++)
	    {
	      if (ml[i].hmm & 0x08)
		ml[i].x += ((ml[i].hmm ^ 0x0f) + 1);
	      else
		ml[i].x -= ml[i].hmm;
	      if (ml[i].x > 160)
		ml[i].x = -68;
	      else if (ml[i].x < -68)
		ml[i].x = 160;
	    }
	  break;
	case HMCLR:
	  pl[0].hmm = 0;
	  pl[1].hmm = 0;
	  for (i = 0; i < 3; i++)
	    ml[i].hmm = 0;
	  break;
	case CXCLR:
	  col_state=0;
	  break;
	}
    }
  else
    {
      switch (a & 0x2ff)
	{
	  /* RIOT I/O ports */
	case SWCHA:
	  riotWrite[SWCHA] = b;
	  break;
	case SWACNT:
	  riotWrite[SWACNT] = b;
	  break;
	case SWCHB:
	case SWBCNT:
	  /* Do nothing */
	  break;

	  /* Timer ports */
	case TIM1T:
	  set_timer (0, b, clkcount);
	  break;
	case TIM8T:
	  set_timer (3, b, clkcount);
	  break;
	case TIM64T:
	  set_timer (6, b, clkcount);
	  break;
	case T1024T:
	  set_timer (10, b, clkcount);
	  break;
	default:
	  printf ("Unknown write %x\n", a);
	  show ();
	  break;
	}
    }
}


/* Decoded read from memory */
/* a: address to read */
/* returns: byte value from address a */
BYTE
decRead (ADDRESS a)
{
  BYTE res = 65;

  if (a & 0x1000)
    {
      a = a & 0xfff;
      if (base_opts.bank != 0)
	res= bank_switch_read (a);
      else
	res = theRom[a];
    }
  else if ((a & 0x280) == 0x80)
    {
      res = theRam[a & 0x7f];
    }
  else if (!(a & 0x80))
    {
      switch (a & 0x0f)
	{
	  /* TIA */
	case CXM0P:
	  res = (col_state & CXM0P_MASK) << 6;
	  break;
	case CXM1P:
	  res = (col_state & CXM1P_MASK) << 4;
	  break;
	case CXP0FB:
	  res = (col_state & CXP0FB_MASK) << 2;
	  break;
	case CXP1FB:
	  res = (col_state & CXP1FB_MASK);
	  break;
	case CXM0FB:
	  res = (col_state & CXM0FB_MASK) >> 2;
	  break;
	case CXM1FB:
	  res = (col_state & CXM1FB_MASK) >> 4;
	  break;
	case CXBLPF:
	  res = (col_state & CXBLPF_MASK) >> 5;
	  break;
	case CXPPMM:
	  res = (col_state & CXPPMM_MASK) >> 7;
	  break;
	case INPT0:
	  if (base_opts.lcon == PADDLE)
	    {
	      tiaRead[INPT0] = do_paddle (0);
	    }
	  else if (base_opts.lcon == KEYPAD)
	    do_keypad (0, 0);
	  res = tiaRead[INPT0];
	  break;
	case INPT1:
	  if (base_opts.lcon == PADDLE)
	    {
	      tiaRead[INPT1] = do_paddle (1);
	    }
	  if (base_opts.lcon == KEYPAD)
	    tiaRead[INPT1]=do_keypad (0, 1);
	  res = tiaRead[INPT1];
	  break;
	case INPT2:
	  if (base_opts.rcon == KEYPAD)
	    do_keypad (1, 0);
	  res = tiaRead[INPT2];
	  break;
	case INPT3:
	  if (base_opts.rcon == KEYPAD)
	     tiaRead[INPT3]=do_keypad ( 1, 1);
	  res = tiaRead[INPT3];
	  break;
	case INPT4:
	  switch (base_opts.lcon)
	    {
	    case KEYPAD:
	      tiaRead[INPT4]=do_keypad ( 0, 2);
	      break;
	    case STICK:
	      read_trigger ();
	      break;
	    }
	  res =tiaRead[INPT4];
	  break;
	case INPT5:
	  switch (base_opts.rcon)
	    {
	    case KEYPAD:
	      tiaRead[INPT5]=do_keypad (1, 2);
	      break;
	    case STICK:
	      read_trigger ();
	      break;
	    }
	  res = tiaRead[INPT5];
	  break;
	case 0x0e:
	case 0x0f:
	  res = 0x0f;
	  /* RAM, mapped to page 0 and 1 */
	}
    }
  else
    {
      switch (a & 0x2ff)
	{
	  /* Timer output */
	case INTIM:
	case 0x285:
	case 0x286:
	case TIM1T:
	case TIM8T:
	case TIM64T:
	case T1024T:
	  res = do_timer (clkcount);
	  /*printf("Timer is %d res is %d\n", res, timer_res); */
	  break;
	case SWCHA:
	  switch (base_opts.lcon)
	    {
	    case PADDLE:
	      if (base_opts.lcon == PADDLE)
		{
		  if (mouse_button ())
		    riotRead[SWCHA] &= 0x7f;
		  else
		    riotRead[SWCHA] |= 0x80;
		}
	      else if (base_opts.rcon == PADDLE)
		{
		  if (mouse_button ())
		    riotRead[SWCHA] &= 0xbf;
		  else
		    riotRead[SWCHA] |= 0x40;
		}
	      break;
	    case STICK:
	      read_stick ();
	      break;
	    }
	  res = riotRead[SWCHA];
	  break;
	  /* Switch B is hardwired to input */
	case SWCHB:
	case SWCHB + 0x100:
	  read_console ();
	  res = riotRead[SWCHB];
	  break;
	default:
	  printf ("Unknown read 0x%x\n", a & 0x2ff);
	  show ();
	  res = 65;
	  break;
	}
    }
  return res;
}


/* Debugging read from memory */
/* a: address to read from */
/* returns: value at address a, WITHOUT side effects */
BYTE
dbgRead (ADDRESS a)
{
  BYTE res;

  if (a & 0x1000)
    {
      a = a & 0xfff;
      res = theRom[a];
      if (base_opts.bank != 0)
	bank_switch_read (a);
    }
  else if ((a & 0x280) == 0x80)
    {
      res = theRam[a & 0x7f];
    }
  else if (!(a & 0x80))
    {
      switch (a & 0x0f)
	{
	  /* TIA */
	case COLUP0:
	  res = colour_table[P0M0_COLOUR] & 0xff;
	  break;
	case COLUP1:
	  res = colour_table[P1M1_COLOUR] & 0xff;
	  break;
	case COLUPF:
	  res = colour_table[PFBL_COLOUR] & 0xff;
	  break;
	case COLUBK:
	  res = colour_table[BK_COLOUR] & 0xff;
	  break;
	case CTRLPF:
	  res = tiaWrite[CTRLPF];	/* Bitmask 00110111 */
	  break;
	case REFP0:
	  res = tiaWrite[REFP0];
	  break;
	case REFP1:
	  res = tiaWrite[REFP1];
	  break;
	case PF0:
	  res = pf[0].pf0;
	  break;
	case PF1:
	  res = pf[0].pf1;
	  break;
	case PF2:
	  res = pf[0].pf2;
	  break;
	default:
	  res = 0;
	  break;
	  /*  case CXM0P:
	     break;
	     case CXM1P:
	     break;
	     case CXP0FB:
	     break;  
	     case CXP1FB:
	     break;  
	     case CXM0FB:
	     break; 
	     case CXM1FB:
	     break; 
	     case CXBLPF:
	     break;  
	     case CXPPMM:
	     break;
	     case INPT0:
	     break;
	     case INPT1:
	     break;
	     case INPT2:
	     break;
	     case INPT3:
	     break;
	     case INPT4:
	     break;
	     case INPT5:
	     break;
	   */
	}
    }
  else
    {
      switch (a & 0x2ff)
	{
	  /* Timer output */
	case INTIM:
	case 0x285:
	case 0x29d:
	case INTIM + 0x100:
	  res = riotRead[INTIM];
	  break;
	case SWCHA:
	case SWCHA + 0x100:
	  res = riotRead[SWCHA];
	  break;
	  /* Switch B is hardwired to input */
	case SWCHB:
	case SWCHB + 0x100:
	  res = riotRead[SWCHB];
	  break;
	default:
	  res = 0;
	  break;
	}
    }
  return res;
}
