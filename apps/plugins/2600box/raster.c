/*****************************************************************************

   This file is part of x2600, the Atari 2600 Emulator
   ===================================================
   
   Copyright 1996 Alex Hornby. For contributions see the file CREDITS.

   This software is distributed under the terms of the GNU General Public
   License. This is free software with ABSOLUTELY NO WARRANTY.
   
   See the file COPYING for Details.
   
   $Id: raster.c,v 1.30 1997/11/22 14:27:47 ahornby Exp $
******************************************************************************/

/* Raster graphics procedures */

#include "rbconfig.h"

#include <stdio.h>
#include "types.h"
#include "address.h"
#include "vmachine.h"
#include "display.h"
#include "options.h"
#include "debug.h"
#include "dbg_mess.h"
#include "collision.h"

/* Color lookup tables. Used to speed up rendering */
/* The current colour lookup table */
unsigned int *colour_lookup;  

/* Colour table */
#define P0M0_COLOUR 0
#define P1M1_COLOUR 1
#define PFBL_COLOUR 2
#define BK_COLOUR 3

unsigned int colour_table[4];

/* normal/alternate, not scores/scores*/
int norm_val, scores_val;
int *colour_ptrs[2][3];

/* Normal priority */
static int colour_normal[64];
static int colour_normscoresl[64];
static int colour_normscoresr[64];

/* Alternate priority */
static int colour_alternate[64];
static int colour_altscoresl[64];
static int colour_altscoresr[64];

/* Playfield screen position */
UINT32 *pf_pos;
BYTE *line_ptr;

/* Draw playfield register PF0 */
/* pf: playfield structure */
/* dir: 1=normal, 0=mirrored */
inline void
draw_pf0 (struct PlayField *pf, int dir)
{
  int pfm;			/* playfield mask */
  /* 1=forward */
  if (dir)
    {
      for (pfm = 0x10; pfm < 0x100; pfm <<= 1)
	{
	  if (pf->pf0 & pfm)
	    *(pf_pos++) = PF_MASK32;
	  else
	    pf_pos++;
	}
    }
  else
    {
      for (pfm = 0x80; pfm > 0x08; pfm >>= 1)
	{
	  if (pf->pf0 & pfm)
	    *(pf_pos++) = PF_MASK32;
	  else
	    pf_pos++;
	}
    }
}

/* Draw playfield register PF1 */
/* pf: playfield structure */
/* dir: 1=normal, 0=mirrored */
inline void
draw_pf1 (struct PlayField *pf, int dir)
{
  int pfm;			/* playfield mask */
  /* 1=forward */
  if (dir)
    {
      /* do PF1 */
      for (pfm = 0x80; pfm > 0; pfm >>= 1)
	{
	  if (pf->pf1 & pfm)
	    *(pf_pos++) = PF_MASK32;
	  else
	    pf_pos ++;
	}
    }
  else
    {
      /* do PF1 */
      for (pfm = 0x01; pfm < 0x100; pfm <<= 1)
	{
	  if (pf->pf1 & pfm)
	    *(pf_pos++) = PF_MASK32;
	  else
	    pf_pos ++;
	}
    }
}

/* Draw playfield register PF2 */
/* pf: playfield structure */
/* dir: 1=normal, 0=mirrored */
inline void
draw_pf2 (struct PlayField *pf, int dir)
{
  int pfm;			/* playfield mask */
  /* 1=forward */
  if (dir)
    {
      /* do PF2 */
      for (pfm = 0x01; pfm < 0x100; pfm <<= 1)
	{
	  if (pf->pf2 & pfm)
	    *(pf_pos++) = PF_MASK32;
	  else
	    pf_pos ++;
	}
    }
  else
    {
      for (pfm = 0x80; pfm > 0; pfm >>= 1)
	{
	  if (pf->pf2 & pfm)
	    *(pf_pos++) = PF_MASK32;
	  else
	    pf_pos ++;
	}
    }
}

/* Update from the playfield display list */
/* num: playfield to use. Now depreciated as only pf[0] is used */
/* nextx: the start position of the next playfield element */
/* pfc: the number of the next playfield change structure */
/* pf_max: the highest playfield change structure */
inline void
pf_update (int num, int nextx, int *pfc, int pf_max)
{
  for (; (*pfc < pf_max) && (nextx + 3 > pf_change[num][*pfc].x); (*pfc)++)
    {
      use_pfraster_change (&pf[num], &pf_change[num][*pfc]);
    }
}

/* Draw the playfield */
void
draw_playfield (void)
{
  const int num = 0;		/* Stick to one playfield */
  int pfc = 0;
  int pf_max = pf_change_count[num];

  pf_pos = (UINT32 *)colvect;
  /* First half of playfield */

  pf_update (num, 0, &pfc, pf_max);
  draw_pf0 (&pf[0], 1);
  pf_update (num, 16, &pfc, pf_max);
  draw_pf1 (&pf[0], 1);
  pf_update (num, 48, &pfc, pf_max);
  draw_pf2 (&pf[0], 1);

  pf_update (num, 80, &pfc, pf_max);
  /* Second half of playfield */
  if (pf[0].ref)
    {
      draw_pf2 (&pf[0], 0);
      pf_update (num, 112, &pfc, pf_max);
      draw_pf1 (&pf[0], 0);
      pf_update (num, 144, &pfc, pf_max);
      draw_pf0 (&pf[0], 0);
    }
  else
    {
      draw_pf0 (&pf[0], 1);
      pf_update (num, 96, &pfc, pf_max);
      draw_pf1 (&pf[0], 1);
      pf_update (num, 128, &pfc, pf_max);
      draw_pf2 (&pf[0], 1);
    }
  /* Use last changes */
  for (; pfc < pf_max; pfc++)
    use_pfraster_change (&pf[num], &pf_change[num][pfc]);

  pf_change_count[num] = 0;
}

/* Draws a normal (8 clocks) sized player */
/* p: the player to draw */
/* x: the position to draw it */
inline void
pl_normal (struct Player *p, int x)
{
  /* Set pointer to start of player graphic */
  BYTE *ptr = colvect + x;
  BYTE mask;
  BYTE gr;

  if (p->vdel_flag)
    gr = p->vdel;
  else
    gr = p->grp;

  if (p->reflect)
    {
      /* Reflected: start with D0 of GRP on left */
      for (mask = 0x01; mask > 0; mask <<= 1)
	{
	  if (gr & mask)
	    {
	      *(ptr++) |= p->mask;
	    }
	  else
	    ptr++;
	}
    }
  else
    {
      /* Unreflected: start with D7 of GRP on left */
      for (mask = 0x80; mask > 0; mask >>= 1)
	{
	  if (gr & mask)
	    {
	      *(ptr++) |= p->mask;
	    }
	  else
	    ptr++;
	}
    }
}

/* Draws a double width ( 16 clocks ) player */
/* p: the player to draw */
/* x: the position to draw it */
inline void
pl_double (struct Player *p, int x)
{
  /* Set pointer to start of player graphic */
  BYTE *ptr = colvect + (x);
  BYTE mask;
  BYTE gr;

  if (p->vdel_flag)
    gr = p->vdel;
  else
    gr = p->grp;

  if (p->reflect)
    {
      for (mask = 0x01; mask > 0; mask <<= 1)
	{
	  if (gr & mask)
	    {
	      *(ptr++) |= p->mask;
	      *(ptr++) |= p->mask;
	    }
	  else
	    ptr += 2;
	}
    }
  else
    {
      for (mask = 0x80; mask > 0; mask >>= 1)
	{
	  if (gr & mask)
	    {
	      *(ptr++) |= p->mask;
	      *(ptr++) |= p->mask;
	    }
	  else
	    ptr += 2;
	}
    }
}

/* Draws a quad sized ( 32 clocks) player */
/* p: the player to draw */
/* x: the position to draw it */
inline void
pl_quad (struct Player *p, int x)
{
  /* Set pointer to start of player graphic */
  BYTE *ptr = colvect + x;
  BYTE mask;
  BYTE gr;

  if (p->vdel_flag)
    gr = p->vdel;
  else
    gr = p->grp;

  if (p->reflect)
    {
      for (mask = 0x01; mask > 0; mask <<= 1)
	{
	  if (gr & mask)
	    {
	      *(ptr++) |= p->mask;
	      *(ptr++) |= p->mask;
	      *(ptr++) |= p->mask;
	      *(ptr++) |= p->mask;
	    }
	  else
	    ptr += 4;
	}
    }
  else
    {
      for (mask = 0x80; mask > 0; mask >>= 1)
	{
	  if (gr & mask)
	    {
	      *(ptr++) |= p->mask;
	      *(ptr++) |= p->mask;
	      *(ptr++) |= p->mask;
	      *(ptr++) |= p->mask;
	    }
	  else
	    ptr += 4;
	}
    }
}

/* Consume the player display list */
inline void
pl_update (int num, int nextx, int *plc, int pl_max)
{
  for (; (*plc < pl_max) && (nextx > pl_change[num][*plc].x); (*plc)++)
    {
      use_plraster_change (&pl[num], &pl_change[num][*plc]);
    }
}

/* Draw a player graphic */
/* line: the vertical position of the raster */
/* num: the number of player to draw, current 0 or 1 for P0 and P1 */
static inline void
pl_draw (int num)
{
  int plc = 0;
  int pl_max = pl_change_count[num];
  int nextx;

  pl_update (num, pl[num].x, &plc, pl_max);
  if (pl[num].x >= 0 && pl[num].x < tv_width)
    {

      /*if(pl_max > plc)
         use_plraster_change( &pl[num], &pl_change[num][plc++]); */
      switch (pl[num].nusize)
	{
	case 0:
	  /* One copy */
	  pl_normal (&pl[num], pl[num].x);
	  break;
	case 1:
	  /* Two copies close */
	  pl_normal (&pl[num], pl[num].x);
	  nextx = pl[num].x + 8 + 8;
	  pl_update (num, nextx, &plc, pl_max);
	  pl_normal (&pl[num], nextx);
	  break;
	case 2:
	  /* Two copies medium */
	  pl_normal (&pl[num], pl[num].x);
	  nextx = pl[num].x + 8 + 24;
	  pl_update (num, nextx, &plc, pl_max);
	  pl_normal (&pl[num], nextx);
	  break;
	case 3:
	  /* Three copies close */
	  /* Pacman score line */
	  pl_normal (&pl[num], pl[num].x);

	  nextx = pl[num].x + 16;
	  pl_update (num, nextx, &plc, pl_max);
	  pl_normal (&pl[num], nextx);

	  nextx = pl[num].x + 32;
	  pl_update (num, nextx, &plc, pl_max);

	  pl_normal (&pl[num], nextx);
	  break;
	case 4:
	  /* Two copies wide */
	  pl_normal (&pl[num], pl[num].x);
	  nextx = pl[num].x + 8 + 56;
	  pl_update (num, nextx, &plc, pl_max);
	  pl_normal (&pl[num], nextx);
	  break;
	case 5:
	  /* Double sized player */
	  pl_double (&pl[num], pl[num].x);
	  break;
	case 6:
	  /* Three copies medium */
	  pl_normal (&pl[num], pl[num].x);
	  nextx = pl[num].x + 8 + 24;
	  pl_update (num, nextx, &plc, pl_max);
	  pl_normal (&pl[num], nextx);
	  nextx = pl[num].x + 8 + 56;
	  pl_update (num, nextx, &plc, pl_max);
	  pl_normal (&pl[num], nextx);
	  break;
	case 7:
	  /* Quad sized player */
	  pl_quad (&pl[num], pl[num].x);
	  break;
	}
    }
  /* Use last changes */
  for (; plc < pl_max; plc++)
    use_plraster_change (&pl[num], &pl_change[num][plc]);
  pl_change_count[num] = 0;
}


/* Draw the ball graphic */
/* line: the vertical position of the raster */
static inline void
draw_ball (void)
{
  int i;
  BYTE *blptr;
  BYTE e;

  if (ml[2].vdel_flag)
    e = ml[2].vdel;
  else
    e = ml[2].enabled;

  if (e && ml[2].x >= 0)
    {
      blptr = colvect + (ml[2].x);
      switch (tiaWrite[CTRLPF] >> 4)
	{
	case 3:
	  /* Eight clocks */
	  for (i = 0; i < 8; i++)
	    *(blptr++) |= BL_MASK;
	  break;
	case 2:
	  /* Four clocks */
	  for (i = 0; i < 4; i++)
	    *(blptr++) |= BL_MASK;
	  break;
	case 1:
	  /* Two clocks */
	  for (i = 0; i < 2; i++)
	    *(blptr++) |= BL_MASK;
	  break;
	case 0:
	  /* One clock */
	  *(blptr++) |= BL_MASK;
	  break;
	}
    }
}


/* Draw a missile graphic */
static inline void
do_missile (int num, BYTE * misptr)
{
  int i;

  switch (ml[num].width)
    {
    case 0:
      /* one clock */
      *(misptr++) |= ml[num].mask;
      break;
    case 1:
      /* two clocks */
      for (i = 0; i < 2; i++)
	*(misptr++) |= ml[num].mask;
      break;
    case 2:
      /* four clocks */
      for (i = 0; i < 4; i++)
	*(misptr++) |= ml[num].mask;
      break;
    case 3:
      /* Eight clocks */
      for (i = 0; i < 8; i++)
	*(misptr++) |= ml[num].mask;
      break;
    }				/* switch */
}

/* Draw a missile taking into account the player's position. */
/* line: the vertical position of the raster */
/* num: 0 for M0, 1 for M1 */
static inline void
draw_missile (int num)
{
  BYTE *misptr;

  if (ml[num].enabled && ml[num].x >= 0)
    {
      switch (pl[num].nusize)
	{
	case 0:
	  misptr = colvect + (ml[num].x);
	  do_missile (num, misptr);
	  break;
	case 1:
	  misptr = colvect + (ml[num].x);
	  do_missile (num, misptr);
	  misptr = misptr + 16;
	  do_missile (num, misptr);
	  break;
	case 2:
	  misptr = colvect + (ml[num].x);
	  do_missile (num, misptr);
	  misptr = misptr + 32;
	  do_missile (num, misptr);
	  break;
	case 3:
	  misptr = colvect + (ml[num].x);
	  do_missile (num, misptr);
	  misptr = misptr + 16;
	  do_missile (num, misptr);
	  misptr = misptr + 16;
	  do_missile (num, misptr);
	  break;
	case 4:
	  misptr = colvect + (ml[num].x);
	  do_missile (num, misptr);
	  misptr = misptr + 64;
	  do_missile (num, misptr);
	  break;
	case 5:
	  misptr = colvect + (ml[num].x);
	  do_missile (num, misptr);
	  break;
	case 6:
	  misptr = colvect + (ml[num].x);
	  do_missile (num, misptr);
	  misptr = misptr + 32;
	  do_missile (num, misptr);
	  misptr = misptr + 32;
	  do_missile (num, misptr);
	  break;
	case 7:
	  misptr = colvect + (ml[num].x);
	  do_missile (num, misptr);
	  break;

	}

    }				/* If */
}


/* Construct one tv raster line colvect */
/* line: the vertical position of the raster */
void
tv_rasterise (int line)
{
  line_ptr = vscreen + line * vwidth * base_opts.magstep * tv_bytes_pp;
  /* Draw the playfield first */
  draw_playfield (); 
  
  /* Do the ball */
  draw_ball ();
  
  /* Do the player 1 graphics */
  draw_missile (1);
  pl_draw (1);
  
  /* Do the player 0 graphics */
  draw_missile (0);
  pl_draw (0);
}

/* Reset the collision vector */
void
reset_vector (void)
{
  int i;
  UINT32 *cpos=(UINT32 *)colvect;
  for (i = 0; i < 40; i++)
    cpos[i] = 0; 
}


/* draw the collision vector */
/* Quick version with no magnification */
void
draw_vector_q (void)
{
  int i;
  int uct = 0;
  int colind, colval;
  unsigned int pad;
  unsigned short *tv_ptr = (short *) line_ptr;
  
  /* Check for scores */
  if(scores_val ==2) 
    {
      scores_val=1;
      colour_lookup=colour_ptrs[norm_val][scores_val];
    }

  /* Use starting changes */
  while (uct < unified_count && unified[uct].x < 0)
    use_unified_change (&unified[uct++]);

  for (i = 0; i < 80; i++)
    {
      if (uct < unified_count && unified[uct].x == i)
	use_unified_change (&unified[uct++]);
      
      if((colval=colvect[i])){
	
	/* Collision detection */
	col_state|=col_table[colval];
	
	colind=colour_lookup[colval];
	pad=colour_table[colind];
      } else
	pad=colour_table[BK_COLOUR];      

      tv_ptr[i] = pad;
    }

  /* Check for scores */
  if(scores_val ==1)
    {
      scores_val=2;
      colour_lookup=colour_ptrs[norm_val][scores_val];
    }
  for (i = 80; i < 160; i++)
    {
      if (uct < unified_count && unified[uct].x == i)
	use_unified_change (&unified[uct++]);
      
      if((colval=colvect[i])){
	
	/* Collision detection */
	col_state|=col_table[colval];
	
	colind=colour_lookup[colval];
	pad=colour_table[colind];
      } else
	pad=colour_table[BK_COLOUR];      

      tv_ptr[i] = pad;
    }

  while (uct < unified_count)
    use_unified_change (&unified[uct++]);
  unified_count = 0;
}

/* draw the collision vector */
/* Quick version with no magnification */
void
draw_vector_2 (void)
{
  int i;
  int uct = 0;
  int colind, colval;
  unsigned int pad;
  unsigned int *tv_ptr = (int *) line_ptr;
  
  /* Check for scores */
  if(scores_val ==2) 
    {
      scores_val=1;
      colour_lookup=colour_ptrs[norm_val][scores_val];
    }

  /* Use starting changes */
  while (uct < unified_count && unified[uct].x < 0)
    use_unified_change (&unified[uct++]);

  for (i = 0; i < 80; i++)
    {
      if (uct < unified_count && unified[uct].x == i)
	use_unified_change (&unified[uct++]);
      
      if((colval=colvect[i])){
	
	/* Collision detection */
	col_state|=col_table[colval];
	
	colind=colour_lookup[colval];
	pad=colour_table[colind];
      } else
	pad=colour_table[BK_COLOUR];      

      tv_ptr[i] = pad;
    }

  /* Check for scores */
  if(scores_val ==1)
    {
      scores_val=2;
      colour_lookup=colour_ptrs[norm_val][scores_val];
    }
  for (i = 80; i < 160; i++)
    {
      if (uct < unified_count && unified[uct].x == i)
	use_unified_change (&unified[uct++]);
      
      if((colval=colvect[i])){
	
	/* Collision detection */
	col_state|=col_table[colval];
	
	colind=colour_lookup[colval];
	pad=colour_table[colind];
      } else
	pad=colour_table[BK_COLOUR];      

      tv_ptr[i] = pad;
    }

  while (uct < unified_count)
    use_unified_change (&unified[uct++]);
  unified_count = 0;
}


/* draw the collision vector */
/* const agrument ensures that constant propagation occurs, meaning
   less slow down for the magstep=1 case */
void
draw_vector(const int magstep)
{
  int i,j;
  int uct = 0;
  int colind, colval;
  unsigned int pad;
  unsigned short *tv_ptr = (short *) line_ptr;
  
  /* Check for scores */
  if(scores_val ==2) 
    {
      scores_val=1;
      colour_lookup=colour_ptrs[norm_val][scores_val];
    }

  /* Use starting changes */
  while (uct < unified_count && unified[uct].x < 0)
    use_unified_change (&unified[uct++]);

  for (i = 0; i < 80; i++)
    {
      if (uct < unified_count && unified[uct].x == i)
	use_unified_change (&unified[uct++]);
      
      if((colval=colvect[i])){
	
	/* Collision detection */
	col_state|=col_table[colval];
	
	colind=colour_lookup[colval];
	pad=colour_table[colind];
      } else
	pad=colour_table[BK_COLOUR];      
      
      for(j=0; j<magstep; j++)
	{
	  *(tv_ptr)++ = pad;
	}
    }

  /* Check for scores */
  if(scores_val ==1)
    {
      scores_val=2;
      colour_lookup=colour_ptrs[norm_val][scores_val];
    }
  for (i = 80; i < 160; i++)
    {
      if (uct < unified_count && unified[uct].x == i)
	use_unified_change (&unified[uct++]);
      
      if((colval=colvect[i])){
	
	/* Collision detection */
	col_state|=col_table[colval];
	
	colind=colour_lookup[colval];
	pad=colour_table[colind];
      } else
	pad=colour_table[BK_COLOUR];      

      for(j=0; j<magstep; j++)
	{
	  *(tv_ptr)++ = pad;
	}
    }

  while (uct < unified_count)
    use_unified_change (&unified[uct++]);
  unified_count = 0;
}

/* draw the collision vector */
/* const agrument ensures that constant propagation occurs, meaning
   less slow down for the magstep=1 case */
void
draw_vector_true_color16(const int magstep)
{
  int i,j;
  int uct = 0;
  int colind, colval;
  unsigned int pad;
  unsigned int *tv_ptr = (unsigned int *) line_ptr;
  /* Check for scores */
  if(scores_val ==2) 
    {
      scores_val=1;
      colour_lookup=colour_ptrs[norm_val][scores_val];
    }

  /* Use starting changes */
  while (uct < unified_count && unified[uct].x < 0)
    use_unified_change (&unified[uct++]);

  for (i = 0; i < 80; i++)
    {
      if (uct < unified_count && unified[uct].x == i)
	use_unified_change (&unified[uct++]);
      
      if((colval=colvect[i])){
	
	/* Collision detection */
	col_state|=col_table[colval];
	
	colind=colour_lookup[colval];
	pad=colour_table[colind];
      } else
	pad=colour_table[BK_COLOUR];      
      
      for(j=0; j<magstep; j++)
	{
	  *(tv_ptr)++ = pad;
	}
    }

  /* Check for scores */
  if(scores_val ==1)
    {
      scores_val=2;
      colour_lookup=colour_ptrs[norm_val][scores_val];
    }
  for (i = 80; i < 160; i++)
    {
      if (uct < unified_count && unified[uct].x == i)
	use_unified_change (&unified[uct++]);
      
      if((colval=colvect[i])){
	
	/* Collision detection */
	col_state|=col_table[colval];
	
	colind=colour_lookup[colval];
	pad=colour_table[colind];
      } else
	pad=colour_table[BK_COLOUR];      

      for(j=0; j<magstep; j++)
	{
	  *(tv_ptr)++ = pad;
	}
    }

  while (uct < unified_count)
    use_unified_change (&unified[uct++]);
  unified_count = 0;
}

/* draw the collision vector */
/* const agrument ensures that constant propagation occurs, meaning
   less slow down for the magstep=1 case */
void
draw_vector_true_color24(const int magstep)
{
  int i,j;
  int uct = 0;
  int colind, colval;
  unsigned int pad;
  unsigned int *tv_ptr = (unsigned int *) line_ptr;
  
  /* Check for scores */
  if(scores_val ==2) 
    {
      scores_val=1;
      colour_lookup=colour_ptrs[norm_val][scores_val];
    }

  /* Use starting changes */
  while (uct < unified_count && unified[uct].x < 0)
    use_unified_change (&unified[uct++]);

  for (i = 0; i < 80; i++)
    {
      if (uct < unified_count && unified[uct].x == i)
	use_unified_change (&unified[uct++]);
      
      if((colval=colvect[i])){
	
	/* Collision detection */
	col_state|=col_table[colval];
	
	colind=colour_lookup[colval];
	pad=colour_table[colind];
      } else
	pad=colour_table[BK_COLOUR];      
      
      for(j=0; j<magstep; j++)
	{
	  *(tv_ptr)++ = pad;
	}
    }

  /* Check for scores */
  if(scores_val ==1)
    {
      scores_val=2;
      colour_lookup=colour_ptrs[norm_val][scores_val];
    }
  for (i = 80; i < 160; i++)
    {
      if (uct < unified_count && unified[uct].x == i)
	use_unified_change (&unified[uct++]);
      
      if((colval=colvect[i])){
	
	/* Collision detection */
	col_state|=col_table[colval];
	
	colind=colour_lookup[colval];
	pad=colour_table[colind];
      } else
	pad=colour_table[BK_COLOUR];      

      for(j=0; j<magstep; j++)
	{
	  *(tv_ptr)++ = pad;
	}
    }

  while (uct < unified_count)
    use_unified_change (&unified[uct++]);
  unified_count = 0;
}

/* Used for when running in frame skipping mode */
static inline void
update_registers (void)
{
  int i, num;

  /* Playfield */
  for (i = 0; i < pf_change_count[0]; i++)
    use_pfraster_change (&pf[0], &pf_change[0][i]);
  pf_change_count[0] = 0;

  /* Player graphics */
  for (num = 0; num < 2; num++)
    {
      for (i = 0; i < pl_change_count[num]; i++)
	use_plraster_change (&pl[num], &pl_change[num][i]);
      pl_change_count[num] = 0;
    }

  /* Unified */
  for (i = 0; i < unified_count; i++)
    use_unified_change (&unified[i]);
  unified_count = 0;
}

/* Main raster function, will have switches for different magsteps */
/* line: the vertical position of the raster */
void
tv_raster (int line)
{

  int j;
  if ((tv_counter % base_opts.rr != 0)  || (line > theight))
    {
      update_registers ();
    }
  else
    {
      reset_vector();
      tv_rasterise (line);
      switch( tv_depth )
	{
	case 8: 
	  {
	    switch (base_opts.magstep)
	      {
	      case 1:
		draw_vector_q ();
		break;
	      case 2:
		draw_vector_2 ();
		/* Duplicate line if magnifying */
		memcpy (line_ptr + vwidth,
			line_ptr, vwidth);
		
		break;
	      case 3:
		draw_vector (3);
		/* Duplicate line if magnifying */
		for (j = 0; j < 2; j++)
		  {
		    memcpy (line_ptr + vwidth,
			    line_ptr, vwidth);
		  }
		break;
	      }	    
	    break;
	  }
	case 16:
	  {	   
	    /* Magstep is not doubled */
		  
	    draw_vector_true_color16(base_opts.magstep);
	    break;
	  }
	case 32:
	  {
	    /* Magstep is doubled */
	    draw_vector_true_color24(base_opts.magstep*2);
	    break;
	  }	
	}
#ifdef XDEBUGGER
      if (debugf_raster)
	{
	  *(line_ptr + 320) = 0;
	  *(line_ptr + 321) = colour_table[P0M0_COLOUR];
	  tv_display ();
	  debugf_halt = 1;
	}
#endif
    }
}

void
init_raster (void)
{
  int i,val;

  init_collisions();

  /* Normal Priority */
  for (i=0; i<64; i++)
    {
      if (i & (PL0_MASK | ML0_MASK))
	val = P0M0_COLOUR;
      else if (i & (PL1_MASK | ML1_MASK))
	val = P1M1_COLOUR;
      else if (i & (BL_MASK | PF_MASK))
	val = PFBL_COLOUR;
      else
	val = BK_COLOUR;     
      colour_normal[i]=val;
    }

  /* Alternate Priority */
  for (i=0; i<64; i++)
    {  
      if (i & (BL_MASK | PF_MASK))
	val = PFBL_COLOUR;
      else if (i & (PL0_MASK | ML0_MASK))
	val = P0M0_COLOUR;
      else if (i & (PL1_MASK | ML1_MASK))
	val = P1M1_COLOUR;
      else
	val = BK_COLOUR;
      colour_alternate[i]=val;
    }

  /* Normal Scores Left */
  for (i=0; i<64; i++)
    {
      if (i & (PL0_MASK | ML0_MASK))
	val = P0M0_COLOUR;
      else if (i & (PL1_MASK | ML1_MASK))
	val = P1M1_COLOUR;
      else if (i & (BL_MASK | PF_MASK))
	/* Use P1 colour */
	val = P0M0_COLOUR;
      else
	val = BK_COLOUR;     
      colour_normscoresl[i]=val;
    }
  
  /* Normal Scores Right */
  for (i=0; i<64; i++)
    {
      if (i & (PL0_MASK | ML0_MASK))
	val = P0M0_COLOUR;
      else if (i & (PL1_MASK | ML1_MASK))
	val = P1M1_COLOUR;
      else if (i & (BL_MASK | PF_MASK))
	/* Use P1 colour */
	val = P1M1_COLOUR;
      else
	val = BK_COLOUR;     
      colour_normscoresr[i]=val;
    }

  /* Alternate Scores Left*/
  for (i=0; i<64; i++)
    {  
      if (i & (BL_MASK | PF_MASK))
	val = P0M0_COLOUR;
      else if (i & (PL0_MASK | ML0_MASK))
	val = P0M0_COLOUR;
      else if (i & (PL1_MASK | ML1_MASK))
	val = P1M1_COLOUR;
      else
	val = BK_COLOUR;
      colour_altscoresl[i]=val;
    }

  /* Alternate Scores Right*/
  for (i=0; i<64; i++)
    {  
      if (i & (BL_MASK | PF_MASK))
	val = P1M1_COLOUR;
      else if (i & (PL0_MASK | ML0_MASK))
	val = P0M0_COLOUR;
      else if (i & (PL1_MASK | ML1_MASK))
	val = P1M1_COLOUR;
      else
	val = BK_COLOUR;
      colour_altscoresr[i]=val;
    }
  
  colour_ptrs[0][0]=colour_normal;
  colour_ptrs[1][0]=colour_alternate;
  colour_ptrs[0][1]=colour_normscoresl;
  colour_ptrs[1][1]=colour_altscoresl;
  colour_ptrs[0][2]=colour_normscoresr;
  colour_ptrs[1][2]=colour_altscoresr;
  norm_val=0; scores_val=0;

  colour_lookup=colour_normal;
}
