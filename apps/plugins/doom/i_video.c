/* Emacs style mode select   -*- C++ -*-
 *-----------------------------------------------------------------------------
 *
 * $Id$
 *
 * Copyright (C) 1993-1996 by id Software, Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * $Log: i_video.c,v $
 * Revision 1.26  2006-12-13 04:44:17  kkurbjun
 * Dehacked and BEX support for Doom - currently only supports a DEHACKED file in a WAD (not as a standalone file yet).
 *
 * Revision 1.25  2006-10-26 13:38:04  barrywardell
 * Allow the Sansa e200 UI simulator to be built. Thanks to Andre Smith for the nice image of the Sansa. Lots more to be done including testing and tweaking the keymaps and modifying the plugins for the Sansa's 176x220 LCD.
 *
 * Revision 1.24  2006-09-05 00:23:06  barrywardell
 * Fix not being able to exit Doom on the H10.
 *
 * Revision 1.23  2006-08-10 18:34:43  amiconn
 * Correct calculation of necessary buffer size to give maximum number of shades on iPod Mini.
 *
 * Revision 1.22  2006-08-07 02:44:18  amiconn
 * Use striped buffering for grayscale targets to make the buffer fit on iPod g3/g4. Also slightly faster (at least on H1x0) with the buffer in IRAM.
 *
 * Revision 1.21  2006-08-07 01:57:29  amiconn
 * Fix red iPod g3 build.
 *
 * Revision 1.20  2006-08-07 01:46:41  amiconn
 * Grayscale library ported to the grayscale iPods, first version. Added C reference versions of gray_update_rect() for both horizontal and vertical pixel packing. gray_update_rect() and gray_ub_gray_bitmap_part() not yet assembler optimised. Grayscale screendump doesn't work yet. * Fixed button assignments for iPod in grayscale.c
 *
 * Revision 1.19  2006-08-03 20:17:22  bagder
 * Barry Wardell's keymappings for H10
 *
 * Revision 1.18  2006-08-02 00:21:59  amiconn
 * Grayscale library: LCD linearisation and gamma correction.
 *
 * Revision 1.17  2006-04-22 03:48:15  kkurbjun
 * Better video update, add options to startup menu, change default screensize
 *
 * Revision 1.16  2006-04-20 19:39:56  kkurbjun
 * Optimizations for doom: coldfire asm drawspan routine = not much, fixed point multiply changes = not much, H300 asm lcd update = some, IRAM sound updates and simplifications = more
 *
 * Revision 1.15  2006-04-16 23:14:04  kkurbjun
 * Fix run so that it stays enabled across level loads.  Removed some unused code and added some back in for hopeful future use.
 *
 * Revision 1.14  2006-04-15 22:08:36  kkurbjun
 * Slight code cleanups, fixed sound parameter - now it saves.  Old configurations will be reset.
 *
 * Revision 1.13  2006-04-06 21:31:49  kkurbjun
 * Scaling code fixed by clamping down the width to a max of SCREENWIDTH.  Removed some #ifdefs for glprboom
 *
 * Revision 1.12  2006-04-05 06:37:37  kkurbjun
 * Fix finale text and try and prevent some data corruption due to the scaling code.  Also allows the non-standard GP32 mods to work with some bounds checking.  More comments are in v_video.c
 *
 * Revision 1.11  2006-04-04 19:39:31  amiconn
 * Doom on H1x0: Don't waste memory, the grayscale lib doesn't need that much, but properly tell the lib how much memory it may use.
 *
 * Revision 1.10  2006-04-04 12:00:53  dave
 * iPod: Make the hold switch bring up the in-game menu.
 *
 * Revision 1.9  2006-04-03 20:03:02  kkurbjun
 * Updates doom menu w/ new graphics, now requires rockdoom.wad: http://alamode.mines.edu/~kkurbjun/rockdoom.wad
 *
 * Revision 1.8  2006-04-03 17:11:42  kkurbjun
 * Finishing touches
 *
 * Revision 1.7  2006-04-03 16:30:12  kkurbjun
 * Fix #if
 *
 * Revision 1.5  2006-04-03 08:51:08  bger
 * Patch #4864 by Jonathan Gordon: text editor plugin, with some changes by me.
 * Also correct a var clash between the rockbox's gui api and doom plugin
 *
 * Revision 1.4  2006-04-02 20:45:24  kkurbjun
 * Properly ifdef H300 video code, fix commented line handling rockbox volume
 *
 * Revision 1.3  2006-04-02 01:52:44  kkurbjun
 * Update adds prboom's high resolution support, also makes the scaling for
 * platforms w/ resolution less then 320x200 much nicer.  IDoom's lookup table
 * code has been removed.  Also fixed a pallete bug.  Some graphic errors are
 * present in menu and status bar.  Also updates some headers and output
 * formatting.
 *
 * Revision 1.2  2006-03-28 17:20:49  christian
 * added good (tm) button mappings for x5, and added ifdef for HAS_BUTTON_HOLD
 *
 * Revision 1.1  2006-03-28 15:44:01  dave
 * Patch #2969 - Doom!  Currently only working on the H300.
 *
 *
 * DESCRIPTION:
 * DOOM graphics and buttons. H300 Port by Karl Kurbjun
 *      H100 Port by Dave Chapman, Karl Kurbjun and Jens Arnold
 *      IPOD port by Dave Chapman and Paul Louden
 *      Additional work by Thom Johansen
 *
 *-----------------------------------------------------------------------------
 */

#include "doomstat.h"
#include "i_system.h"
#include "v_video.h"
#include "m_argv.h"
#include "d_main.h"

#include "doomdef.h"

#include "rockmacros.h"

#ifndef HAVE_LCD_COLOR
#include "../lib/grey.h"
GREY_INFO_STRUCT_IRAM
static unsigned char greybuffer[LCD_WIDTH] IBSS_ATTR; /* off screen buffer */
static unsigned char *gbuf;
#if LCD_PIXELFORMAT == HORIZONTAL_PACKING
#define GREYBUFSIZE (((LCD_WIDTH+7)/8)*LCD_HEIGHT*16+200)
#else
#define GREYBUFSIZE (LCD_WIDTH*((LCD_HEIGHT+7)/8)*16+200)
#endif
#endif

#if defined(CPU_COLDFIRE)
static char fastscreen[LCD_WIDTH*LCD_HEIGHT] IBSS_ATTR;
#endif

static fb_data palette[256] IBSS_ATTR;
static fb_data *paldata=NULL;

//
// I_ShutdownGraphics
//
void I_ShutdownGraphics(void)
{
#ifndef HAVE_LCD_COLOR
   grey_release();
#endif
   noprintf=0;
}

//
// I_StartTic
//

#if (CONFIG_KEYPAD == IPOD_4G_PAD) || (CONFIG_KEYPAD == IPOD_3G_PAD) || \
    (CONFIG_KEYPAD == IPOD_1G2G_PAD)
//#define DOOMBUTTON_SCROLLWHEEL
#define DOOMBUTTON_UP         BUTTON_MENU
#define DOOMBUTTON_WEAPON     BUTTON_SELECT
#define DOOMBUTTON_LEFT       BUTTON_LEFT
#define DOOMBUTTON_RIGHT      BUTTON_RIGHT
#define DOOMBUTTON_SHOOT      BUTTON_PLAY
#define DOOMBUTTON_ENTER      BUTTON_SELECT
#define DOOMBUTTON_OPEN       BUTTON_MENU
#elif CONFIG_KEYPAD == IAUDIO_X5M5_PAD
#define DOOMBUTTON_UP      BUTTON_UP
#define DOOMBUTTON_DOWN    BUTTON_DOWN
#define DOOMBUTTON_LEFT    BUTTON_LEFT
#define DOOMBUTTON_RIGHT   BUTTON_RIGHT
#define DOOMBUTTON_SHOOT   BUTTON_SELECT
#define DOOMBUTTON_OPEN    BUTTON_PLAY
#define DOOMBUTTON_ESC     BUTTON_POWER
#define DOOMBUTTON_ENTER   BUTTON_SELECT
#define DOOMBUTTON_WEAPON  BUTTON_REC
#elif CONFIG_KEYPAD == IRIVER_H10_PAD
#define DOOMBUTTON_UP      BUTTON_SCROLL_UP
#define DOOMBUTTON_DOWN    BUTTON_SCROLL_DOWN
#define DOOMBUTTON_LEFT    BUTTON_LEFT
#define DOOMBUTTON_RIGHT   BUTTON_RIGHT
#define DOOMBUTTON_SHOOT   BUTTON_REW
#define DOOMBUTTON_OPEN    BUTTON_PLAY
#define DOOMBUTTON_ESC     BUTTON_POWER
#define DOOMBUTTON_ENTER   BUTTON_REW
#define DOOMBUTTON_WEAPON  BUTTON_FF
#elif CONFIG_KEYPAD == SANSA_E200_PAD
#define DOOMBUTTON_SCROLLWHEEL
#define DOOMBUTTON_SCROLLWHEEL_CC   BUTTON_SCROLL_BACK
#define DOOMBUTTON_SCROLLWHEEL_CW   BUTTON_SCROLL_FWD
#define DOOMBUTTON_UP      BUTTON_UP
#define DOOMBUTTON_DOWN    BUTTON_DOWN
#define DOOMBUTTON_LEFT    BUTTON_LEFT
#define DOOMBUTTON_RIGHT   BUTTON_RIGHT
#define DOOMBUTTON_SHOOT   BUTTON_SELECT
#define DOOMBUTTON_OPEN    BUTTON_REC
#define DOOMBUTTON_ESC     BUTTON_POWER
#define DOOMBUTTON_ENTER   BUTTON_SELECT
#define DOOMBUTTON_WEAPON  DOOMBUTTON_SCROLLWHEEL_CW
#elif CONFIG_KEYPAD == SANSA_C200_PAD
#define DOOMBUTTON_UP      BUTTON_UP
#define DOOMBUTTON_DOWN    BUTTON_DOWN
#define DOOMBUTTON_LEFT    BUTTON_LEFT
#define DOOMBUTTON_RIGHT   BUTTON_RIGHT
#define DOOMBUTTON_SHOOT   BUTTON_SELECT
#define DOOMBUTTON_OPEN    BUTTON_REC
#define DOOMBUTTON_ESC     BUTTON_POWER
#define DOOMBUTTON_ENTER   BUTTON_SELECT
#define DOOMBUTTON_WEAPON  BUTTON_VOL_UP
#elif CONFIG_KEYPAD == GIGABEAT_PAD
#define DOOMBUTTON_UP      BUTTON_UP
#define DOOMBUTTON_DOWN    BUTTON_DOWN
#define DOOMBUTTON_LEFT    BUTTON_LEFT
#define DOOMBUTTON_RIGHT   BUTTON_RIGHT
#define DOOMBUTTON_SHOOT   BUTTON_A
#define DOOMBUTTON_OPEN    BUTTON_MENU
#define DOOMBUTTON_ESC     BUTTON_POWER
#define DOOMBUTTON_ENTER   BUTTON_SELECT
#define DOOMBUTTON_WEAPON  BUTTON_VOL_DOWN
#define DOOMBUTTON_MAP     BUTTON_VOL_UP
#elif CONFIG_KEYPAD == GIGABEAT_S_PAD
#define DOOMBUTTON_UP      BUTTON_UP
#define DOOMBUTTON_DOWN    BUTTON_DOWN
#define DOOMBUTTON_LEFT    BUTTON_LEFT
#define DOOMBUTTON_RIGHT   BUTTON_RIGHT
#define DOOMBUTTON_SHOOT   BUTTON_PLAY
#define DOOMBUTTON_OPEN    BUTTON_MENU
#define DOOMBUTTON_ESC     BUTTON_POWER
#define DOOMBUTTON_ENTER   BUTTON_SELECT
#define DOOMBUTTON_WEAPON  BUTTON_VOL_DOWN
#define DOOMBUTTON_MAP     BUTTON_VOL_UP
#elif CONFIG_KEYPAD == MROBE500_PAD
#define DOOMBUTTON_ESC        BUTTON_POWER
#define DOOMBUTTON_UP         BUTTON_RC_PLAY
#define DOOMBUTTON_DOWN       BUTTON_RC_DOWN
#define DOOMBUTTON_LEFT       BUTTON_RC_REW
#define DOOMBUTTON_RIGHT      BUTTON_RC_FF
#define DOOMBUTTON_OPEN       BUTTON_RC_VOL_DOWN
#define DOOMBUTTON_SHOOT      BUTTON_RC_VOL_UP
#define DOOMBUTTON_ENTER      BUTTON_RC_MODE
#define DOOMBUTTON_WEAPON     BUTTON_RC_HEART
#elif CONFIG_KEYPAD == IRIVER_H100_PAD || \
      CONFIG_KEYPAD == IRIVER_H300_PAD
#define DOOMBUTTON_UP      BUTTON_UP
#define DOOMBUTTON_DOWN    BUTTON_DOWN
#define DOOMBUTTON_LEFT    BUTTON_LEFT
#define DOOMBUTTON_RIGHT   BUTTON_RIGHT
#define DOOMBUTTON_SHOOT   BUTTON_REC
#define DOOMBUTTON_OPEN    BUTTON_MODE
#define DOOMBUTTON_ESC     BUTTON_OFF
#define DOOMBUTTON_ENTER   BUTTON_SELECT
#define DOOMBUTTON_WEAPON  BUTTON_ON
#elif CONFIG_KEYPAD == IAUDIO_M3_PAD
#define DOOMBUTTON_ESC        BUTTON_RC_REC
#define DOOMBUTTON_UP         BUTTON_RC_VOL_UP
#define DOOMBUTTON_DOWN       BUTTON_RC_VOL_DOWN
#define DOOMBUTTON_LEFT       BUTTON_RC_REW
#define DOOMBUTTON_RIGHT      BUTTON_RC_FF
#define DOOMBUTTON_OPEN       BUTTON_RC_PLAY
#define DOOMBUTTON_SHOOT      BUTTON_RC_MODE
#define DOOMBUTTON_ENTER      BUTTON_RC_PLAY
#define DOOMBUTTON_WEAPON     BUTTON_RC_MENU
#elif CONFIG_KEYPAD == COWOND2_PAD
#define DOOMBUTTON_ESC        BUTTON_POWER
#elif CONFIG_KEYPAD == MROBE100_PAD
#define DOOMBUTTON_UP      BUTTON_UP
#define DOOMBUTTON_DOWN    BUTTON_DOWN
#define DOOMBUTTON_LEFT    BUTTON_LEFT
#define DOOMBUTTON_RIGHT   BUTTON_RIGHT
#define DOOMBUTTON_SHOOT   BUTTON_SELECT
#define DOOMBUTTON_OPEN    BUTTON_PLAY
#define DOOMBUTTON_ESC     BUTTON_POWER
#define DOOMBUTTON_ENTER   BUTTON_MENU
#define DOOMBUTTON_WEAPON  BUTTON_DISPLAY
#else
#error Keymap not defined!
#endif

#ifdef HAVE_TOUCHSCREEN
#ifndef DOOMBUTTON_UP
#define DOOMBUTTON_UP      BUTTON_TOPMIDDLE
#endif
#ifndef DOOMBUTTON_DOWN
#define DOOMBUTTON_DOWN    BUTTON_BOTTOMMIDDLE
#endif
#ifndef DOOMBUTTON_LEFT
#define DOOMBUTTON_LEFT    BUTTON_MIDLEFT
#endif
#ifndef DOOMBUTTON_RIGHT
#define DOOMBUTTON_RIGHT   BUTTON_MIDRIGHT
#endif
#ifndef DOOMBUTTON_SHOOT
#define DOOMBUTTON_SHOOT   BUTTON_CENTER
#endif
#ifndef DOOMBUTTON_OPEN
#define DOOMBUTTON_OPEN    BUTTON_TOPRIGHT
#endif
#ifndef DOOMBUTTON_ESC
#define DOOMBUTTON_ESC     BUTTON_TOPLEFT
#endif
#ifndef DOOMBUTTON_ENTER
#define DOOMBUTTON_ENTER   BUTTON_BOTTOMLEFT
#endif
#ifndef DOOMBUTTON_WEAPON
#define DOOMBUTTON_WEAPON  BUTTON_BOTTOMRIGHT
#endif
#endif

#ifdef DOOMBUTTON_SCROLLWHEEL
/* Scrollwheel events are posted directly and not polled by the button
   driver - synthesize polling */
static inline unsigned int read_scroll_wheel(void)
{
    unsigned int buttons = BUTTON_NONE;
    unsigned int btn;

    /* Empty out the button queue and see if any scrollwheel events were
       posted */
    do
    {
        btn = rb->button_get_w_tmo(0);
        buttons |= btn;
    }
    while (btn != BUTTON_NONE);

    return buttons & (DOOMBUTTON_SCROLLWHEEL_CC | DOOMBUTTON_SCROLLWHEEL_CW);
}
#endif

inline void getkey()
{
   event_t event;
   /* Same button handling as rockboy */
   static unsigned int oldbuttonstate IDATA_ATTR = 0;

   unsigned int released, pressed, newbuttonstate;

#ifdef HAS_BUTTON_HOLD
   static unsigned int holdbutton IDATA_ATTR=0;
   static bool hswitch IDATA_ATTR=0;
   if (rb->button_hold()&~holdbutton)
   {
      if(hswitch==0)
      {
         event.type = ev_keydown;
         hswitch=1;
      }
      else
      {
         event.type = ev_keyup;
         hswitch=0;
      }
#if (CONFIG_KEYPAD == IPOD_4G_PAD) || (CONFIG_KEYPAD == IPOD_3G_PAD) || \
    (CONFIG_KEYPAD == IPOD_1G2G_PAD)
      /* Bring up the menu */
      event.data1=KEY_ESCAPE;
#else
      /* Enable run */
      event.data1=KEY_CAPSLOCK;
#endif
      D_PostEvent(&event);
   }
   holdbutton=rb->button_hold();
#endif

   newbuttonstate = rb->button_status();
#ifdef DOOMBUTTON_SCROLLWHEEL
   newbuttonstate |= read_scroll_wheel();
#endif

   if(newbuttonstate==oldbuttonstate) /* Don't continue, nothing left to do */
      return;
   released = ~newbuttonstate & oldbuttonstate;
   pressed = newbuttonstate & ~oldbuttonstate;
   oldbuttonstate = newbuttonstate;
   if(released)
   {
      event.type = ev_keyup;
      if(released & DOOMBUTTON_LEFT)
      {
         event.data1=KEY_LEFTARROW;
         D_PostEvent(&event);
      }
      if(released & DOOMBUTTON_RIGHT)
      {
         event.data1=KEY_RIGHTARROW;
         D_PostEvent(&event);
      }
#ifdef DOOMBUTTON_DOWN
      if(released & DOOMBUTTON_DOWN)
      {
         event.data1=KEY_DOWNARROW;
         D_PostEvent(&event);
      }
#endif
      if(released & DOOMBUTTON_UP)
      {
         event.data1=KEY_UPARROW;
         D_PostEvent(&event);
      }
      if(released & DOOMBUTTON_SHOOT)
      {
         event.data1=KEY_RCTRL;
         D_PostEvent(&event);
      }
      if(released & DOOMBUTTON_OPEN)
      {
         event.data1=' ';
         D_PostEvent(&event);
      }
#ifdef DOOMBUTTON_ESC
      if(released & DOOMBUTTON_ESC)
      {
         event.data1=KEY_ESCAPE;
         D_PostEvent(&event);
      }
#endif
      if(released & DOOMBUTTON_ENTER)
      {
         event.data1=KEY_ENTER;
         D_PostEvent(&event);
      }
#ifdef DOOMBUTTON_WEAPON
      if(released & DOOMBUTTON_WEAPON)
      {
         event.data1 ='w';
         D_PostEvent(&event);
      }
#endif
#ifdef DOOMBUTTON_MAP
      if(released & DOOMBUTTON_MAP)
      {
         event.data1 =KEY_TAB;
         D_PostEvent(&event);
      }
#endif
   }
   if(pressed)
   {
      event.type = ev_keydown;
      if(pressed & DOOMBUTTON_LEFT)
      {
         event.data1=KEY_LEFTARROW;
         D_PostEvent(&event);
      }
      if(pressed & DOOMBUTTON_RIGHT)
      {
         event.data1=KEY_RIGHTARROW;
         D_PostEvent(&event);
      }
#ifdef DOOMBUTTON_DOWN
      if(pressed & DOOMBUTTON_DOWN)
      {
         event.data1=KEY_DOWNARROW;
         D_PostEvent(&event);
      }
#endif
      if(pressed & DOOMBUTTON_UP)
      {
         event.data1=KEY_UPARROW;
         D_PostEvent(&event);
      }
      if(pressed & DOOMBUTTON_SHOOT)
      {
         event.data1=KEY_RCTRL;
         D_PostEvent(&event);
      }
      if(pressed & DOOMBUTTON_OPEN)
      {
         event.data1=' ';
         D_PostEvent(&event);
      }
#ifdef DOOMBUTTON_ESC
      if(pressed & DOOMBUTTON_ESC)
      {
         event.data1=KEY_ESCAPE;
         D_PostEvent(&event);
      }
#endif
#ifdef DOOMBUTTON_ENTER
      if(pressed & DOOMBUTTON_ENTER)
      {
         event.data1=KEY_ENTER;
         D_PostEvent(&event);
      }
#endif
#ifdef DOOMBUTTON_WEAPON
      if(pressed & DOOMBUTTON_WEAPON)
      {
         event.data1='w';
         D_PostEvent(&event);
      }
#endif
#ifdef DOOMBUTTON_MAP
      if(pressed & DOOMBUTTON_MAP)
      {
         event.data1 =KEY_TAB;
         D_PostEvent(&event);
      }
#endif
   }
}

inline void I_StartTic (void)
{
   getkey();
}


///////////////////////////////////////////////////////////
// Palette stuff.
//
static void I_UploadNewPalette(int pal)
{
  // This is used to replace the current 256 colour cmap with a new one
  // Used by 256 colour PseudoColor modes
  static int cachedgamma;
  static size_t num_pals;

  if ((paldata == NULL) || (cachedgamma != usegamma)) {
    int lump = W_GetNumForName("PLAYPAL");
    const byte *pall = W_CacheLumpNum(lump);
    register const byte *const gtable = gammatable[cachedgamma = usegamma];
    register int i;

    num_pals = W_LumpLength(lump) / (3*256);
    num_pals *= 256;

    if (!paldata) {
      // First call - allocate and prepare colour array
      paldata = malloc(sizeof(*paldata)*num_pals);
    }

    // set the colormap entries
    for (i=0 ; (size_t)i<num_pals ; i++) {
      int r = gtable[pall[0]];
      int g = gtable[pall[1]];
      int b = gtable[pall[2]];
      pall+=3;
#ifndef HAVE_LCD_COLOR
      paldata[i]=(3*r+6*g+b)/10;
#else
      paldata[i] = LCD_RGBPACK(r,g,b);
#endif
    }

    W_UnlockLumpNum(lump);
    num_pals/=256;
  }

#ifdef RANGECHECK
  if ((size_t)pal >= num_pals)
    I_Error("I_UploadNewPalette: Palette number out of range (%d>=%d)",
      pal, num_pals);
#endif
   memcpy(palette,paldata+256*pal,256*sizeof(fb_data));
}


//
// I_FinishUpdate
//

void I_FinishUpdate (void)
{
    int count;
    byte *src = d_screens[0];
#if (CONFIG_LCD == LCD_H300) && !defined(SIMULATOR)
    count = SCREENWIDTH*SCREENHEIGHT;

    /* ASM screen update (drops ~300 tics) */
    asm volatile (
        "move.w  #33, (%[LCD])                   \n" /* Setup the LCD controller */
        "nop                                     \n"
        "clr.w   (%[LCD2])                       \n"
        "nop                                     \n"
        "move.w  #34, (%[LCD])                   \n" /* End LCD controller setup */
        "clr.l   %%d1                            \n"
    ".loop:                                      \n"
        "move.l  (%[scrp])+, %%d0                \n"
        "swap.w  %%d0                            \n"
        "move.w  %%d0, %%d1                      \n"
        "lsr.l   #8,%%d1                         \n"
        "move.w  (%[pal], %%d1.l:2), (%[LCD2])   \n"
        "move.b  %%d0,%%d1                       \n"
        "swap.w  %%d0                            \n"
        "nop                                     \n"
        "move.w  (%[pal], %%d1.l:2), (%[LCD2])   \n"
        "move.w  %%d0, %%d1                      \n"
        "lsr.l   #8,%%d1                         \n"
        "nop                                     \n"
        "move.w  (%[pal], %%d1.l:2), (%[LCD2])   \n"
        "move.b  %%d0,%%d1                       \n"
        "nop                                     \n"
        "move.w  (%[pal], %%d1.l:2), (%[LCD2])   \n"
        "subq.l  #4,%[cnt]                       \n"
        "bne.b   .loop                           \n"
        : /* outputs */
        [scrp]"+a"(src),
        [cnt] "+d"(count)
        : /* inputs */
        [pal] "a" (palette),
        [LCD] "a" (0xf0000000),
        [LCD2]"a" (0xf0000002)
        : /* clobbers */
        "d0", "d1"
   );
#elif (CONFIG_LCD == LCD_X5) && !defined(SIMULATOR) \
   && defined(CPU_COLDFIRE) /* protect from using it on e200 (sic!) */
    count = SCREENWIDTH*SCREENHEIGHT;

    /* ASM screen update (drops ~230 tics) */
    asm volatile (
        "clr.w   (%[LCD])                    \n" /* Setup the LCD controller */
        "move.w  #(33<<1), (%[LCD])          \n" 
        "clr.w   (%[LCD2])                   \n"
        "clr.w   (%[LCD2])                   \n"
        "clr.w   (%[LCD])                    \n" /* End LCD controller setup */
        "move.w  #(34<<1), (%[LCD])          \n" 
        "clr.l   %%d1                        \n"
    ".loop:                                  \n"
        "move.l  (%[scrp])+, %%d0            \n"
        "swap.w  %%d0                        \n"
        "move.w  %%d0, %%d1                  \n"
        "lsr.l   #8,%%d1                     \n"
        "move.w  (%[pal], %%d1.l:2), %%d2    \n"
        "move.l  %%d2, %%d3                  \n"
        "lsr.l   #7, %%d3                    \n"
        "move.w  %%d3, (%[LCD2])             \n"
        "lsl.l   #1, %%d2                    \n"
        "move.w  %%d2, (%[LCD2])             \n"
        "move.b  %%d0,%%d1                   \n"
        "move.w  (%[pal], %%d1.l:2), %%d2    \n"
        "move.l  %%d2, %%d3                  \n"
        "lsr.l   #7, %%d3                    \n"
        "move.w  %%d3, (%[LCD2])             \n"
        "lsl.l   #1, %%d2                    \n"
        "move.w  %%d2, (%[LCD2])             \n"
        "swap.w  %%d0                        \n"
        "move.w  %%d0, %%d1                  \n"
        "lsr.l   #8,%%d1                     \n"
        "move.w  (%[pal], %%d1.l:2), %%d2    \n"
        "move.l  %%d2, %%d3                  \n"
        "lsr.l   #7, %%d3                    \n"
        "move.w  %%d3, (%[LCD2])             \n"
        "lsl.l   #1, %%d2                    \n"
        "move.w  %%d2, (%[LCD2])             \n"
        "move.b  %%d0,%%d1                   \n"
        "move.w  (%[pal], %%d1.l:2), %%d2    \n"
        "move.l  %%d2, %%d3                  \n"
        "lsr.l   #7, %%d3                    \n"
        "move.w  %%d3, (%[LCD2])             \n"
        "lsl.l   #1, %%d2                    \n"
        "move.w  %%d2, (%[LCD2])             \n"
        "subq.l  #4,%[cnt]                   \n"
        "bne.b   .loop                       \n"
        : /* outputs */
        [scrp]"+a"(src),
        [cnt] "+d"(count)
        : /* inputs */
        [pal] "a" (palette),
        [LCD] "a" (0xf0008000),
        [LCD2]"a" (0xf0008002)
        : /* clobbers */
        "d0", "d1", "d2", "d3"
   );
#else
#ifdef HAVE_LCD_COLOR
#if(LCD_HEIGHT>LCD_WIDTH)
    if(rotate_screen)
    {
        int y;
        
        for (y = 1; y <= SCREENHEIGHT; y++)
        {
            fb_data *dst = rb->lcd_framebuffer + LCD_WIDTH - y;
            count = SCREENWIDTH;

            do
            {
                *dst = palette[*src++];
                dst += LCD_WIDTH;
            }
            while (--count);
        }
    }
    else
#endif
    {
        fb_data *dst = rb->lcd_framebuffer;
        count = SCREENWIDTH*SCREENHEIGHT;

        do
            *dst++ = palette[*src++];
        while (--count);
    }
    rb->lcd_update();
#else /* !HAVE_LCD_COLOR */
    unsigned char *dst;
    int y;

    for (y = 0; y < SCREENHEIGHT; y++)
    {
        dst = greybuffer;
        count = SCREENWIDTH;

        do
            *dst++ = palette[*src++];
        while (--count);

        grey_ub_gray_bitmap(greybuffer, 0, y, SCREENWIDTH, 1);
    }
#endif /* !HAVE_LCD_COLOR */
#endif
}

//
// I_ReadScreen
//
void I_ReadScreen (byte* scr)
{
   memcpy (scr, d_screens[0], LCD_WIDTH*LCD_HEIGHT);
}

//
// I_SetPalette
//
void I_SetPalette (int pal)
{
   I_UploadNewPalette(pal);
}

//
// I_InitGraphics
//
void I_InitGraphics(void)
{
   printf("Starting Graphics engine\n");

   noprintf=1;

   /* Note: The other screens are allocated as needed */

#ifndef HAVE_LCD_COLOR
   gbuf=malloc(GREYBUFSIZE);
   grey_init(rb, gbuf, GREYBUFSIZE, GREY_ON_COP, LCD_WIDTH, LCD_HEIGHT, NULL);
   /* switch on greyscale overlay */
   grey_show(true);
#endif

#ifdef CPU_COLDFIRE
   coldfire_set_macsr(EMAC_FRACTIONAL | EMAC_SATURATE);
   d_screens[0] = fastscreen;
#else
   // Don't know if this will fit in other IRAMs
   d_screens[0] = malloc (LCD_WIDTH * LCD_HEIGHT * sizeof(unsigned char));
#endif
}
