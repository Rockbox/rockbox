/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 Vicentini Martin
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
#include "plugin.h"



/* variable button definitions */
#if (CONFIG_KEYPAD == IRIVER_H100_PAD) || \
      (CONFIG_KEYPAD == IRIVER_H300_PAD)
#define FLIPIT_LEFT         BUTTON_LEFT
#define FLIPIT_RIGHT        BUTTON_RIGHT
#define FLIPIT_UP           BUTTON_UP
#define FLIPIT_DOWN         BUTTON_DOWN
#define FLIPIT_QUIT         BUTTON_OFF
#define FLIPIT_SHUFFLE      BUTTON_MODE
#define FLIPIT_SOLVE        BUTTON_ON
#define FLIPIT_STEP_BY_STEP BUTTON_REC
#define FLIPIT_TOGGLE_PRE   BUTTON_SELECT
#define FLIPIT_TOGGLE       (BUTTON_SELECT | BUTTON_REL)

#define FLIPIT_RC_QUIT BUTTON_RC_STOP

#elif (CONFIG_KEYPAD == IPOD_4G_PAD) || \
      (CONFIG_KEYPAD == IPOD_3G_PAD) || \
      (CONFIG_KEYPAD == IPOD_1G2G_PAD)

#define FLIPIT_SCROLLWHEEL
#define FLIPIT_LEFT         BUTTON_LEFT
#define FLIPIT_RIGHT        BUTTON_RIGHT
#define FLIPIT_UP           BUTTON_MENU
#define FLIPIT_DOWN         BUTTON_PLAY
#define FLIPIT_NEXT         BUTTON_SCROLL_FWD
#define FLIPIT_PREV         BUTTON_SCROLL_BACK
#define FLIPIT_QUIT         (BUTTON_SELECT | BUTTON_MENU)
#define FLIPIT_SHUFFLE      (BUTTON_SELECT | BUTTON_LEFT)
#define FLIPIT_SOLVE        (BUTTON_SELECT | BUTTON_PLAY)
#define FLIPIT_STEP_BY_STEP (BUTTON_SELECT | BUTTON_RIGHT)
#define FLIPIT_TOGGLE_PRE   BUTTON_SELECT
#define FLIPIT_TOGGLE       (BUTTON_SELECT | BUTTON_REL)

#elif CONFIG_KEYPAD == IAUDIO_X5M5_PAD

#define FLIPIT_LEFT         BUTTON_LEFT
#define FLIPIT_RIGHT        BUTTON_RIGHT
#define FLIPIT_UP           BUTTON_UP
#define FLIPIT_DOWN         BUTTON_DOWN
#define FLIPIT_QUIT         BUTTON_POWER
#define FLIPIT_SHUFFLE      BUTTON_REC
#define FLIPIT_SOLVE_PRE    BUTTON_PLAY
#define FLIPIT_SOLVE        (BUTTON_PLAY | BUTTON_REPEAT)
#define FLIPIT_STEP_PRE     BUTTON_PLAY
#define FLIPIT_STEP_BY_STEP (BUTTON_PLAY | BUTTON_REL)
#define FLIPIT_TOGGLE       BUTTON_SELECT

#elif CONFIG_KEYPAD == GIGABEAT_PAD

#define FLIPIT_LEFT         BUTTON_LEFT
#define FLIPIT_RIGHT        BUTTON_RIGHT
#define FLIPIT_UP           BUTTON_UP
#define FLIPIT_DOWN         BUTTON_DOWN
#define FLIPIT_QUIT         BUTTON_POWER
#define FLIPIT_SHUFFLE      BUTTON_MENU
#define FLIPIT_SOLVE        BUTTON_VOL_UP
#define FLIPIT_STEP_BY_STEP BUTTON_VOL_DOWN
#define FLIPIT_TOGGLE       BUTTON_SELECT

#elif CONFIG_KEYPAD == SANSA_E200_PAD

#define FLIPIT_SCROLLWHEEL
#define FLIPIT_LEFT         BUTTON_LEFT
#define FLIPIT_RIGHT        BUTTON_RIGHT
#define FLIPIT_UP           BUTTON_UP
#define FLIPIT_DOWN         BUTTON_DOWN
#define FLIPIT_NEXT         BUTTON_SCROLL_FWD
#define FLIPIT_PREV         BUTTON_SCROLL_BACK
#define FLIPIT_QUIT         BUTTON_POWER
#define FLIPIT_SHUFFLE      (BUTTON_REC | BUTTON_LEFT)
#define FLIPIT_SOLVE        (BUTTON_REC | BUTTON_RIGHT)
#define FLIPIT_STEP_BY_STEP (BUTTON_REC | BUTTON_SELECT)
#define FLIPIT_TOGGLE       BUTTON_SELECT

#elif (CONFIG_KEYPAD == SANSA_FUZE_PAD)

#define FLIPIT_SCROLLWHEEL
#define FLIPIT_LEFT         BUTTON_LEFT
#define FLIPIT_RIGHT        BUTTON_RIGHT
#define FLIPIT_UP           BUTTON_UP
#define FLIPIT_DOWN         BUTTON_DOWN
#define FLIPIT_NEXT         BUTTON_SCROLL_FWD
#define FLIPIT_PREV         BUTTON_SCROLL_BACK
#define FLIPIT_QUIT         (BUTTON_HOME|BUTTON_REPEAT)
#define FLIPIT_SHUFFLE      (BUTTON_SELECT | BUTTON_LEFT)
#define FLIPIT_SOLVE        (BUTTON_SELECT | BUTTON_DOWN)
#define FLIPIT_STEP_BY_STEP (BUTTON_SELECT | BUTTON_RIGHT)
#define FLIPIT_TOGGLE_PRE   BUTTON_SELECT
#define FLIPIT_TOGGLE       (BUTTON_SELECT | BUTTON_REL)

#elif CONFIG_KEYPAD == SANSA_C200_PAD

#define FLIPIT_LEFT         BUTTON_LEFT
#define FLIPIT_RIGHT        BUTTON_RIGHT
#define FLIPIT_UP           BUTTON_UP
#define FLIPIT_DOWN         BUTTON_DOWN
#define FLIPIT_QUIT         BUTTON_POWER
#define FLIPIT_SHUFFLE      (BUTTON_REC | BUTTON_LEFT)
#define FLIPIT_SOLVE        (BUTTON_REC | BUTTON_RIGHT)
#define FLIPIT_STEP_BY_STEP (BUTTON_REC | BUTTON_SELECT)
#define FLIPIT_TOGGLE       BUTTON_SELECT

#elif CONFIG_KEYPAD == SANSA_CLIP_PAD

#define FLIPIT_LEFT         BUTTON_LEFT
#define FLIPIT_RIGHT        BUTTON_RIGHT
#define FLIPIT_UP           BUTTON_UP
#define FLIPIT_DOWN         BUTTON_DOWN
#define FLIPIT_QUIT         BUTTON_POWER
#define FLIPIT_SHUFFLE      (BUTTON_HOME | BUTTON_LEFT)
#define FLIPIT_SOLVE        (BUTTON_HOME | BUTTON_RIGHT)
#define FLIPIT_STEP_BY_STEP (BUTTON_HOME | BUTTON_SELECT)
#define FLIPIT_TOGGLE       BUTTON_SELECT

#elif CONFIG_KEYPAD == SANSA_M200_PAD

#define FLIPIT_LEFT         BUTTON_LEFT
#define FLIPIT_RIGHT        BUTTON_RIGHT
#define FLIPIT_UP           BUTTON_UP
#define FLIPIT_DOWN         BUTTON_DOWN
#define FLIPIT_QUIT         BUTTON_POWER
#define FLIPIT_SHUFFLE      (BUTTON_SELECT | BUTTON_LEFT)
#define FLIPIT_SOLVE        (BUTTON_SELECT | BUTTON_RIGHT)
#define FLIPIT_STEP_BY_STEP (BUTTON_SELECT | BUTTON_UP)
#define FLIPIT_TOGGLE       (BUTTON_SELECT | BUTTON_REL)

#elif CONFIG_KEYPAD == SONY_NWZ_PAD
#define FLIPIT_LEFT         BUTTON_LEFT
#define FLIPIT_RIGHT        BUTTON_RIGHT
#define FLIPIT_UP           BUTTON_UP
#define FLIPIT_DOWN         BUTTON_DOWN
#define FLIPIT_QUIT         (BUTTON_BACK|BUTTON_REPEAT)
#define FLIPIT_SHUFFLE      BUTTON_BACK
#define FLIPIT_SOLVE        (BUTTON_PLAY|BUTTON_REPEAT)
#define FLIPIT_STEP_BY_STEP BUTTON_POWER
#define FLIPIT_TOGGLE       BUTTON_PLAY

#elif CONFIG_KEYPAD == CREATIVE_ZEN_PAD
#define FLIPIT_LEFT         BUTTON_LEFT
#define FLIPIT_RIGHT        BUTTON_RIGHT
#define FLIPIT_UP           BUTTON_UP
#define FLIPIT_DOWN         BUTTON_DOWN
#define FLIPIT_QUIT         BUTTON_BACK
#define FLIPIT_SHUFFLE      BUTTON_SHORTCUT
#define FLIPIT_SOLVE        BUTTON_PLAYPAUSE
#define FLIPIT_STEP_BY_STEP BUTTON_MENU
#define FLIPIT_TOGGLE       BUTTON_SELECT

#elif CONFIG_KEYPAD == IRIVER_H10_PAD

#define FLIPIT_LEFT         BUTTON_LEFT
#define FLIPIT_RIGHT        BUTTON_RIGHT
#define FLIPIT_UP           BUTTON_SCROLL_UP
#define FLIPIT_DOWN         BUTTON_SCROLL_DOWN
#define FLIPIT_QUIT         BUTTON_POWER
#define FLIPIT_SHUFFLE      (BUTTON_PLAY | BUTTON_LEFT)
#define FLIPIT_SOLVE        (BUTTON_PLAY | BUTTON_RIGHT)
#define FLIPIT_STEP_BY_STEP (BUTTON_PLAY | BUTTON_SCROLL_UP)
#define FLIPIT_TOGGLE_PRE   BUTTON_REW
#define FLIPIT_TOGGLE       (BUTTON_REW | BUTTON_REL)

#elif CONFIG_KEYPAD == GIGABEAT_S_PAD

#define FLIPIT_LEFT         BUTTON_LEFT
#define FLIPIT_RIGHT        BUTTON_RIGHT
#define FLIPIT_UP           BUTTON_UP
#define FLIPIT_DOWN         BUTTON_DOWN
#define FLIPIT_QUIT         BUTTON_BACK
#define FLIPIT_SHUFFLE      BUTTON_MENU
#define FLIPIT_SOLVE        BUTTON_VOL_UP
#define FLIPIT_STEP_BY_STEP BUTTON_VOL_DOWN
#define FLIPIT_TOGGLE       BUTTON_SELECT

#elif CONFIG_KEYPAD == MROBE100_PAD

#define FLIPIT_LEFT         BUTTON_LEFT
#define FLIPIT_RIGHT        BUTTON_RIGHT
#define FLIPIT_UP           BUTTON_UP
#define FLIPIT_DOWN         BUTTON_DOWN
#define FLIPIT_QUIT         BUTTON_POWER
#define FLIPIT_SHUFFLE      BUTTON_MENU
#define FLIPIT_SOLVE        BUTTON_PLAY
#define FLIPIT_STEP_BY_STEP BUTTON_DISPLAY
#define FLIPIT_TOGGLE       BUTTON_SELECT

#elif CONFIG_KEYPAD == IAUDIO_M3_PAD

#define FLIPIT_LEFT         BUTTON_RC_REW
#define FLIPIT_RIGHT        BUTTON_RC_FF
#define FLIPIT_UP           BUTTON_RC_VOL_UP
#define FLIPIT_DOWN         BUTTON_RC_VOL_DOWN
#define FLIPIT_QUIT         BUTTON_RC_REC
#define FLIPIT_SHUFFLE      BUTTON_RC_MODE
#define FLIPIT_SOLVE_PRE    BUTTON_RC_MENU
#define FLIPIT_SOLVE        (BUTTON_RC_MENU|BUTTON_REPEAT)
#define FLIPIT_STEP_PRE     BUTTON_RC_MENU
#define FLIPIT_STEP_BY_STEP (BUTTON_RC_MENU|BUTTON_REL)
#define FLIPIT_TOGGLE       BUTTON_RC_PLAY

#define FLIPIT_RC_QUIT      BUTTON_REC

#elif CONFIG_KEYPAD == COWON_D2_PAD

#define FLIPIT_QUIT         BUTTON_POWER

#elif CONFIG_KEYPAD == CREATIVEZVM_PAD

#define FLIPIT_LEFT         BUTTON_LEFT
#define FLIPIT_RIGHT        BUTTON_RIGHT
#define FLIPIT_UP           BUTTON_UP
#define FLIPIT_DOWN         BUTTON_DOWN
#define FLIPIT_QUIT         BUTTON_BACK
#define FLIPIT_SHUFFLE      BUTTON_CUSTOM
#define FLIPIT_SOLVE        BUTTON_PLAY
#define FLIPIT_STEP_BY_STEP BUTTON_MENU
#define FLIPIT_TOGGLE       BUTTON_SELECT

#elif CONFIG_KEYPAD == CREATIVE_ZENXFI3_PAD
#define FLIPIT_LEFT         BUTTON_BACK
#define FLIPIT_RIGHT        BUTTON_MENU
#define FLIPIT_UP           BUTTON_UP
#define FLIPIT_DOWN         BUTTON_DOWN
#define FLIPIT_NEXT         BUTTON_VOL_UP
#define FLIPIT_PREV         BUTTON_VOL_DOWN
#define FLIPIT_QUIT         BUTTON_POWER
#define FLIPIT_SHUFFLE      (BUTTON_PLAY | BUTTON_BACK)
#define FLIPIT_SOLVE        (BUTTON_PLAY | BUTTON_DOWN)
#define FLIPIT_STEP_BY_STEP (BUTTON_PLAY | BUTTON_MENU)
#define FLIPIT_TOGGLE_PRE   BUTTON_PLAY
#define FLIPIT_TOGGLE       (BUTTON_PLAY | BUTTON_REL)

#elif CONFIG_KEYPAD == PHILIPS_HDD1630_PAD

#define FLIPIT_LEFT         BUTTON_LEFT
#define FLIPIT_RIGHT        BUTTON_RIGHT
#define FLIPIT_UP           BUTTON_UP
#define FLIPIT_DOWN         BUTTON_DOWN
#define FLIPIT_QUIT         BUTTON_POWER
#define FLIPIT_SHUFFLE      BUTTON_MENU
#define FLIPIT_SOLVE        BUTTON_VIEW
#define FLIPIT_STEP_BY_STEP BUTTON_PLAYLIST
#define FLIPIT_TOGGLE       BUTTON_SELECT

#elif CONFIG_KEYPAD == PHILIPS_HDD6330_PAD

#define FLIPIT_LEFT         BUTTON_LEFT
#define FLIPIT_RIGHT        BUTTON_RIGHT
#define FLIPIT_UP           BUTTON_UP
#define FLIPIT_DOWN         BUTTON_DOWN
#define FLIPIT_QUIT         BUTTON_POWER
#define FLIPIT_SHUFFLE      BUTTON_MENU
#define FLIPIT_SOLVE        BUTTON_VIEW
#define FLIPIT_STEP_BY_STEP BUTTON_NEXT
#define FLIPIT_TOGGLE       BUTTON_PLAY

#elif CONFIG_KEYPAD == PHILIPS_SA9200_PAD

#define FLIPIT_LEFT         BUTTON_PREV
#define FLIPIT_RIGHT        BUTTON_NEXT
#define FLIPIT_UP           BUTTON_UP
#define FLIPIT_DOWN         BUTTON_DOWN
#define FLIPIT_QUIT         BUTTON_POWER
#define FLIPIT_SHUFFLE      BUTTON_MENU
#define FLIPIT_SOLVE        BUTTON_RIGHT
#define FLIPIT_STEP_BY_STEP BUTTON_LEFT
#define FLIPIT_TOGGLE       BUTTON_PLAY

#elif CONFIG_KEYPAD == ONDAVX747_PAD || CONFIG_KEYPAD == MROBE500_PAD

#define FLIPIT_QUIT         BUTTON_POWER

#elif CONFIG_KEYPAD == ONDAVX777_PAD

#define FLIPIT_QUIT         BUTTON_POWER

#elif (CONFIG_KEYPAD == SAMSUNG_YH820_PAD) || \
      (CONFIG_KEYPAD == SAMSUNG_YH92X_PAD)

#define FLIPIT_LEFT         BUTTON_LEFT
#define FLIPIT_RIGHT        BUTTON_RIGHT
#define FLIPIT_UP           BUTTON_UP
#define FLIPIT_DOWN         BUTTON_DOWN
#define FLIPIT_QUIT         (BUTTON_REW|BUTTON_REPEAT)
#define FLIPIT_SHUFFLE      (BUTTON_REW|BUTTON_REL)
#define FLIPIT_SOLVE        (BUTTON_FFWD|BUTTON_REPEAT)
#define FLIPIT_STEP_BY_STEP (BUTTON_FFWD|BUTTON_REL)
#define FLIPIT_TOGGLE       BUTTON_PLAY

#elif CONFIG_KEYPAD == PBELL_VIBE500_PAD

#define FLIPIT_LEFT         BUTTON_PREV
#define FLIPIT_RIGHT        BUTTON_NEXT
#define FLIPIT_UP           BUTTON_UP
#define FLIPIT_DOWN         BUTTON_DOWN
#define FLIPIT_QUIT         BUTTON_REC
#define FLIPIT_SHUFFLE      BUTTON_PLAY
#define FLIPIT_SOLVE        BUTTON_MENU
#define FLIPIT_STEP_BY_STEP BUTTON_CANCEL
#define FLIPIT_TOGGLE       BUTTON_OK

#elif CONFIG_KEYPAD == MPIO_HD200_PAD

#define FLIPIT_LEFT         BUTTON_VOL_DOWN
#define FLIPIT_RIGHT        BUTTON_VOL_UP
#define FLIPIT_UP           BUTTON_REW
#define FLIPIT_DOWN         BUTTON_FF
#define FLIPIT_QUIT         (BUTTON_REC|BUTTON_PLAY)
#define FLIPIT_SHUFFLE      BUTTON_FUNC
#define FLIPIT_SOLVE        BUTTON_REC
#define FLIPIT_STEP_BY_STEP (BUTTON_PLAY|BUTTON_REW)
#define FLIPIT_TOGGLE       (BUTTON_PLAY|BUTTON_FF)

#elif CONFIG_KEYPAD == MPIO_HD300_PAD

#define FLIPIT_LEFT         BUTTON_REW
#define FLIPIT_RIGHT        BUTTON_FF
#define FLIPIT_UP           BUTTON_UP
#define FLIPIT_DOWN         BUTTON_DOWN
#define FLIPIT_QUIT         (BUTTON_MENU|BUTTON_REPEAT)
#define FLIPIT_SHUFFLE      BUTTON_PLAY
#define FLIPIT_SOLVE        BUTTON_MENU
#define FLIPIT_STEP_BY_STEP BUTTON_REC
#define FLIPIT_TOGGLE       BUTTON_ENTER

#elif CONFIG_KEYPAD == SANSA_FUZEPLUS_PAD

#define FLIPIT_LEFT         BUTTON_LEFT
#define FLIPIT_RIGHT        BUTTON_RIGHT
#define FLIPIT_UP           BUTTON_UP
#define FLIPIT_DOWN         BUTTON_DOWN
#define FLIPIT_QUIT         BUTTON_POWER
#define FLIPIT_SHUFFLE      BUTTON_PLAYPAUSE
#define FLIPIT_SOLVE        BUTTON_BACK
#define FLIPIT_STEP_BY_STEP BUTTON_BOTTOMLEFT
#define FLIPIT_TOGGLE       BUTTON_SELECT

#elif CONFIG_KEYPAD == SANSA_CONNECT_PAD

#define FLIPIT_LEFT         BUTTON_LEFT
#define FLIPIT_RIGHT        BUTTON_RIGHT
#define FLIPIT_UP           BUTTON_UP
#define FLIPIT_DOWN         BUTTON_DOWN
#define FLIPIT_QUIT         BUTTON_POWER
#define FLIPIT_SHUFFLE      BUTTON_VOL_UP
#define FLIPIT_SOLVE        BUTTON_PREV
#define FLIPIT_STEP_BY_STEP BUTTON_NEXT
#define FLIPIT_TOGGLE       BUTTON_SELECT

#elif CONFIG_KEYPAD == SAMSUNG_YPR0_PAD

#define FLIPIT_LEFT         BUTTON_LEFT
#define FLIPIT_RIGHT        BUTTON_RIGHT
#define FLIPIT_UP           BUTTON_UP
#define FLIPIT_DOWN         BUTTON_DOWN
#define FLIPIT_QUIT         BUTTON_BACK
#define FLIPIT_SHUFFLE      BUTTON_MENU
#define FLIPIT_SOLVE        BUTTON_USER
#define FLIPIT_STEP_BY_STEP BUTTON_POWER
#define FLIPIT_TOGGLE       BUTTON_SELECT

#elif CONFIG_KEYPAD == HM60X_PAD

#define FLIPIT_LEFT         BUTTON_LEFT
#define FLIPIT_RIGHT        BUTTON_RIGHT
#define FLIPIT_UP           BUTTON_UP
#define FLIPIT_DOWN         BUTTON_DOWN
#define FLIPIT_QUIT         BUTTON_POWER
#define FLIPIT_SHUFFLE      (BUTTON_UP|BUTTON_POWER)
#define FLIPIT_SOLVE        (BUTTON_DOWN|BUTTON_POWER)
#define FLIPIT_STEP_BY_STEP (BUTTON_LEFT|BUTTON_POWER)
#define FLIPIT_TOGGLE       BUTTON_SELECT

#elif CONFIG_KEYPAD == HM801_PAD

#define FLIPIT_LEFT         BUTTON_LEFT
#define FLIPIT_RIGHT        BUTTON_RIGHT
#define FLIPIT_UP           BUTTON_UP
#define FLIPIT_DOWN         BUTTON_DOWN
#define FLIPIT_QUIT         BUTTON_POWER
#define FLIPIT_SHUFFLE      BUTTON_PREV
#define FLIPIT_SOLVE        BUTTON_NEXT
#define FLIPIT_STEP_BY_STEP BUTTON_PLAY
#define FLIPIT_TOGGLE       BUTTON_SELECT

#elif CONFIG_KEYPAD == DX50_PAD

#define FLIPIT_LEFT         BUTTON_LEFT
#define FLIPIT_RIGHT        BUTTON_RIGHT
#define FLIPIT_UP           BUTTON_VOL_UP
#define FLIPIT_DOWN         BUTTON_VOL_DOWN
#define FLIPIT_QUIT         (BUTTON_POWER|BUTTON_REL)
#define FLIPIT_SHUFFLE      (BUTTON_VOL_UP|BUTTON_POWER)
#define FLIPIT_SOLVE        (BUTTON_VOL_DOWN|BUTTON_POWER)
#define FLIPIT_STEP_BY_STEP (BUTTON_LEFT|BUTTON_POWER)
#define FLIPIT_TOGGLE       BUTTON_PLAY

#elif CONFIG_KEYPAD == CREATIVE_ZENXFI2_PAD

#define FLIPIT_QUIT         BUTTON_POWER
#define FLIPIT_TOGGLE       BUTTON_MENU

#elif CONFIG_KEYPAD == AGPTEK_ROCKER_PAD

#define FLIPIT_LEFT         BUTTON_LEFT
#define FLIPIT_RIGHT        BUTTON_RIGHT
#define FLIPIT_UP           BUTTON_UP
#define FLIPIT_DOWN         BUTTON_DOWN
#define FLIPIT_QUIT         (BUTTON_POWER|BUTTON_REL)
#define FLIPIT_SHUFFLE      (BUTTON_VOLDOWN)
#define FLIPIT_SOLVE        (BUTTON_SELECT|BUTTON_VOLUP)
#define FLIPIT_STEP_BY_STEP (BUTTON_LEFT|BUTTON_VOLUP)
#define FLIPIT_TOGGLE       BUTTON_SELECT

#elif CONFIG_KEYPAD == XDUOO_X3_PAD || CONFIG_KEYPAD == XDUOO_X3II_PAD ||  CONFIG_KEYPAD == XDUOO_X20_PAD

#define FLIPIT_LEFT         BUTTON_PREV
#define FLIPIT_RIGHT        BUTTON_NEXT
#define FLIPIT_UP           BUTTON_HOME
#define FLIPIT_DOWN         BUTTON_OPTION
#define FLIPIT_QUIT         BUTTON_POWER
#define FLIPIT_SHUFFLE      (BUTTON_HOME | BUTTON_PREV)
#define FLIPIT_SOLVE        (BUTTON_HOME | BUTTON_NEXT)
#define FLIPIT_STEP_BY_STEP (BUTTON_HOME | BUTTON_PLAY)
#define FLIPIT_TOGGLE       BUTTON_PLAY

#elif CONFIG_KEYPAD == FIIO_M3K_PAD

#define FLIPIT_LEFT         BUTTON_PREV
#define FLIPIT_RIGHT        BUTTON_NEXT
#define FLIPIT_UP           BUTTON_HOME
#define FLIPIT_DOWN         BUTTON_OPTION
#define FLIPIT_QUIT         BUTTON_POWER
#define FLIPIT_SHUFFLE      (BUTTON_HOME | BUTTON_PREV)
#define FLIPIT_SOLVE        (BUTTON_HOME | BUTTON_NEXT)
#define FLIPIT_STEP_BY_STEP (BUTTON_HOME | BUTTON_PLAY)
#define FLIPIT_TOGGLE       BUTTON_PLAY

#elif CONFIG_KEYPAD == IHIFI_770_PAD || CONFIG_KEYPAD == IHIFI_800_PAD

#define FLIPIT_LEFT         BUTTON_HOME
#define FLIPIT_RIGHT        BUTTON_VOL_DOWN
#define FLIPIT_UP           BUTTON_PREV
#define FLIPIT_DOWN         BUTTON_NEXT
#define FLIPIT_QUIT         BUTTON_POWER
#define FLIPIT_SHUFFLE      (BUTTON_POWER | BUTTON_PREV)
#define FLIPIT_SOLVE        (BUTTON_POWER | BUTTON_NEXT)
#define FLIPIT_STEP_BY_STEP (BUTTON_POWER | BUTTON_PLAY)
#define FLIPIT_TOGGLE       BUTTON_PLAY

#elif CONFIG_KEYPAD == EROSQ_PAD

#define FLIPIT_LEFT         BUTTON_SCROLL_BACK
#define FLIPIT_RIGHT        BUTTON_SCROLL_FWD
#define FLIPIT_UP           BUTTON_PREV
#define FLIPIT_DOWN         BUTTON_NEXT
#define FLIPIT_QUIT         BUTTON_POWER
#define FLIPIT_SHUFFLE      BUTTON_MENU
#define FLIPIT_SOLVE        BUTTON_VOL_DOWN
#define FLIPIT_STEP_BY_STEP BUTTON_VOL_UP
#define FLIPIT_TOGGLE       BUTTON_PLAY

#else
#error No keymap defined!
#endif

#ifdef HAVE_TOUCHSCREEN
#ifndef FLIPIT_LEFT
#define FLIPIT_LEFT         BUTTON_MIDLEFT
#endif
#ifndef FLIPIT_RIGHT
#define FLIPIT_RIGHT        BUTTON_MIDRIGHT
#endif
#ifndef FLIPIT_UP
#define FLIPIT_UP           BUTTON_TOPMIDDLE
#endif
#ifndef FLIPIT_DOWN
#define FLIPIT_DOWN         BUTTON_BOTTOMMIDDLE
#endif
#ifndef FLIPIT_QUIT
#define FLIPIT_QUIT         BUTTON_TOPLEFT
#endif
#ifndef FLIPIT_SHUFFLE
#define FLIPIT_SHUFFLE      BUTTON_TOPRIGHT
#endif
#ifndef FLIPIT_SOLVE
#define FLIPIT_SOLVE        BUTTON_BOTTOMLEFT
#endif
#ifndef FLIPIT_STEP_BY_STEP
#define FLIPIT_STEP_BY_STEP BUTTON_BOTTOMRIGHT
#endif
#ifndef FLIPIT_TOGGLE
#define FLIPIT_TOGGLE       BUTTON_CENTER
#endif
#endif

static int spots[20];
static int toggle[20];
static int cursor_pos, moves;

#include "pluginbitmaps/flipit_cursor.h"
#include "pluginbitmaps/flipit_tokens.h"

#define PANEL_HEIGHT 12
#define TK_WIDTH    BMPWIDTH_flipit_cursor
#define TK_HEIGHT   BMPHEIGHT_flipit_cursor
#define TK_SPACE    MAX(0, MIN((LCD_WIDTH - 5*TK_WIDTH)/4, \
                               (LCD_HEIGHT - PANEL_HEIGHT - 4*TK_HEIGHT)/4))
#define GRID_WIDTH  (5*TK_WIDTH + 4*TK_SPACE)
#define GRID_LEFT   ((LCD_WIDTH - GRID_WIDTH)/2)
#define GRID_HEIGHT (4*TK_HEIGHT + 4*TK_SPACE) /* includes grid-panel space */
#define GRID_TOP    MAX(0, ((LCD_HEIGHT - PANEL_HEIGHT - GRID_HEIGHT)/2))

/* draw a spot at the coordinates (x,y), range of p is 0-19 */
static void draw_spot(int p)
{
    rb->lcd_bitmap_part( flipit_tokens, 0, spots[p] * TK_HEIGHT,
                         STRIDE(SCREEN_MAIN, BMPWIDTH_flipit_tokens,
                            BMPHEIGHT_flipit_tokens),
                         GRID_LEFT + (p%5) * (TK_WIDTH+TK_SPACE),
                         GRID_TOP + (p/5) * (TK_HEIGHT+TK_SPACE),
                         TK_WIDTH, TK_HEIGHT );
}

/* draw the cursor at the current cursor position */
static void draw_cursor(void)
{
#ifdef HAVE_LCD_COLOR
    rb->lcd_bitmap_transparent( flipit_cursor,
                                GRID_LEFT + (cursor_pos%5) * (TK_WIDTH+TK_SPACE),
                                GRID_TOP + (cursor_pos/5) * (TK_HEIGHT+TK_SPACE),
                                TK_WIDTH, TK_HEIGHT );
#else
    rb->lcd_set_drawmode(DRMODE_FG);
    rb->lcd_mono_bitmap( flipit_cursor,
                         GRID_LEFT + (cursor_pos%5) * (TK_WIDTH+TK_SPACE),
                         GRID_TOP + (cursor_pos/5) * (TK_HEIGHT+TK_SPACE),
                         TK_WIDTH, TK_HEIGHT );
    rb->lcd_set_drawmode(DRMODE_SOLID);
#endif
}

/* draw the info panel ... duh */
static void draw_info_panel(void)
{
    char s[32];

    rb->lcd_set_drawmode( DRMODE_SOLID|DRMODE_INVERSEVID );
    rb->lcd_fillrect( GRID_LEFT, GRID_TOP + 4*(TK_HEIGHT+TK_SPACE),
                      GRID_WIDTH, PANEL_HEIGHT );
    rb->lcd_set_drawmode( DRMODE_SOLID );
    rb->lcd_drawrect( GRID_LEFT, GRID_TOP + 4*(TK_HEIGHT+TK_SPACE),
                      GRID_WIDTH, PANEL_HEIGHT );

    rb->snprintf( s, sizeof(s), "Flips: %d", moves );
    rb->lcd_putsxy( (LCD_WIDTH - rb->lcd_getstringsize(s, NULL, NULL)) / 2,
                    GRID_TOP + 4*(TK_HEIGHT+TK_SPACE) + 2, s );
}

/* clear the cursor where it is */
static inline void clear_cursor(void)
{
    draw_spot( cursor_pos );
}

/* check if the puzzle is finished */
static bool flipit_finished(void)
{
    int i;
    for (i=0; i<20; i++)
    if (!spots[i])
        return false;
    clear_cursor();
    return true;
}

/* draws the toggled spots */
static void flipit_toggle(void)
{
    spots[cursor_pos] = 1-spots[cursor_pos];
    toggle[cursor_pos] = 1-toggle[cursor_pos];
    draw_spot(cursor_pos);
    if (cursor_pos%5 > 0) {
        spots[cursor_pos-1] = 1-spots[cursor_pos-1];
        draw_spot(cursor_pos-1);
    }
    if (cursor_pos%5 < 4) {
        spots[cursor_pos+1] = 1-spots[cursor_pos+1];
        draw_spot(cursor_pos+1);
    }
    if (cursor_pos/5 > 0) {
        spots[cursor_pos-5] = 1-spots[cursor_pos-5];
        draw_spot(cursor_pos-5);
    }
    if (cursor_pos/5 < 3) {
        spots[cursor_pos+5] = 1-spots[cursor_pos+5];
        draw_spot(cursor_pos+5);
    }
    moves++;

    draw_info_panel();

    if (flipit_finished())
        clear_cursor();
}

/* move the cursor in any direction */
static void move_cursor(int x, int y)
{
    if (!(flipit_finished())) {
        clear_cursor();
        cursor_pos =     ( x + 5 + cursor_pos%5 )%5
                     + ( ( y + 4 + cursor_pos/5 )%4 )*5;
        draw_cursor();
    }
    rb->lcd_update();
}

/* initialize the board */
static void flipit_init(void)
{
    int i;

    rb->lcd_clear_display();
    for (i=0; i<20; i++) {
        spots[i]=1;
        toggle[i]=1;
        draw_spot(i);
    }
    rb->lcd_update();

    for (i=0; i<20; i++) {
        cursor_pos = (rb->rand() % 20);
        flipit_toggle();
    }

    cursor_pos = 0;
    draw_cursor();
    moves = 0;
    draw_info_panel();
    rb->lcd_update();
}

/* the main game loop */
static bool flipit_loop(void)
{
    int i;
    int button;
#if defined(FLIPIT_SOLVE_PRE) || defined(FLIPIT_STEP_PRE) || \
    defined(FLIPIT_TOGGLE_PRE) || defined(FLIPIT_UP_PRE)
    int lastbutton = BUTTON_NONE;
#endif

    flipit_init();
    while(true) {
        button = rb->button_get(true);
        switch (button) {
#ifdef FLIPIT_RC_QUIT
            case FLIPIT_RC_QUIT:
#endif
            case FLIPIT_QUIT:
                /* get out of here */
                return PLUGIN_OK;

            case FLIPIT_SHUFFLE:
                /* mix up the pieces */
                flipit_init();
                break;

            case FLIPIT_SOLVE:
#ifdef FLIPIT_SOLVE_PRE
                if (lastbutton != FLIPIT_SOLVE_PRE)
                    break;
#endif
                /* solve the puzzle */
                if (!flipit_finished()) {
                    for (i=0; i<20; i++)
                        if (!toggle[i]) {
                            clear_cursor();
                            cursor_pos = i;
                            flipit_toggle();
                            draw_cursor();
                            rb->lcd_update();
                            rb->sleep(HZ*2/3);
                        }
                }
                break;

            case FLIPIT_STEP_BY_STEP:
#ifdef FLIPIT_STEP_PRE
                if (lastbutton != FLIPIT_STEP_PRE)
                    break;
#endif
                if (!flipit_finished()) {
                    for (i=0; i<20; i++)
                        if (!toggle[i]) {
                            clear_cursor();
                            cursor_pos = i;
                            flipit_toggle();
                            draw_cursor();
                            rb->lcd_update();
                            break;
                        }
                }
                break;

            case FLIPIT_TOGGLE:
#ifdef FLIPIT_TOGGLE_PRE
                if (lastbutton != FLIPIT_TOGGLE_PRE)
                    break;
#endif
                /* toggle the pieces */
                if (!flipit_finished()) {
                    flipit_toggle();
                    draw_cursor();
                    rb->lcd_update();
                }
                break;

            case FLIPIT_LEFT:
                move_cursor(-1, 0);
                break;

            case FLIPIT_RIGHT:
                move_cursor(1, 0);
                break;
            /*move cursor though the entire field*/
#ifdef FLIPIT_SCROLLWHEEL
            case FLIPIT_PREV:
            case FLIPIT_PREV|BUTTON_REPEAT:
                if ((cursor_pos)%5 == 0) {
                    move_cursor(-1, -1);
                }
                else {
                    move_cursor(-1, 0);
                }
                break;

            case FLIPIT_NEXT:
            case FLIPIT_NEXT|BUTTON_REPEAT:
                if ((cursor_pos+1)%5 == 0) {
                    move_cursor(1, 1);
                }
                else {
                    move_cursor(1, 0);
                }
                break;
#endif
            case FLIPIT_UP:
#ifdef FLIPIT_UP_PRE
                if (lastbutton != FLIPIT_UP_PRE)
                    break;
#endif
                move_cursor(0, -1);
                break;

            case FLIPIT_DOWN:
                move_cursor(0, 1);
                break;

            default:
                if (rb->default_event_handler(button) == SYS_USB_CONNECTED)
                    return PLUGIN_USB_CONNECTED;
                break;
        }
#if defined(FLIPIT_SOLVE_PRE) || defined(FLIPIT_STEP_PRE) || \
    defined(FLIPIT_TOGGLE_PRE) || defined(FLIPIT_UP_PRE)
        if (button != BUTTON_NONE)
            lastbutton = button;
#endif
    }
}

/* called function from outside */
enum plugin_status plugin_start(const void* parameter)
{
    int i, rc;

    (void)parameter;

#ifdef HAVE_LCD_COLOR
    rb->lcd_set_background(LCD_WHITE);
    rb->lcd_set_foreground(LCD_BLACK);
#endif

#if LCD_DEPTH > 1
    rb->lcd_set_backdrop(NULL);
#endif

    rb->splash(HZ, "FlipIt!");

    /* print instructions */
    rb->lcd_clear_display();
    rb->lcd_setfont(FONT_SYSFIXED);
#if (CONFIG_KEYPAD == IRIVER_H100_PAD) || \
    (CONFIG_KEYPAD == IRIVER_H300_PAD)
    rb->lcd_putsxy(2, 8, "[STOP] to stop");
    rb->lcd_putsxy(2, 18, "[SELECT] toggle");
    rb->lcd_putsxy(2, 28, "[MODE] shuffle");
    rb->lcd_putsxy(2, 38, "[PLAY] solution");
    rb->lcd_putsxy(2, 48, "[REC] step by step");
#elif (CONFIG_KEYPAD == IPOD_4G_PAD) || \
      (CONFIG_KEYPAD == IPOD_3G_PAD) || \
      (CONFIG_KEYPAD == IPOD_1G2G_PAD)
    rb->lcd_putsxy(2, 8, "[S-MENU] to stop");
    rb->lcd_putsxy(2, 18, "[SELECT] toggle");
    rb->lcd_putsxy(2, 28, "[S-LEFT] shuffle");
    rb->lcd_putsxy(2, 38, "[S-PLAY] solution");
    rb->lcd_putsxy(2, 48, "[S-RIGHT] step by step");
#elif CONFIG_KEYPAD == IAUDIO_X5M5_PAD
    rb->lcd_putsxy(2, 8, "[POWER] to stop");
    rb->lcd_putsxy(2, 18, "[SELECT] toggle");
    rb->lcd_putsxy(2, 28, "[REC] shuffle");
    rb->lcd_putsxy(2, 38, "[PLAY..] solution");
    rb->lcd_putsxy(2, 48, "[PLAY] step by step");
#elif CONFIG_KEYPAD == GIGABEAT_PAD
    rb->lcd_putsxy(2, 8, "[POWER] to stop");
    rb->lcd_putsxy(2, 18, "[SELECT] toggle");
    rb->lcd_putsxy(2, 28, "[MENU] shuffle");
    rb->lcd_putsxy(2, 38, "[VOL+] solution");
    rb->lcd_putsxy(2, 48, "[VOL-] step by step");
#elif CONFIG_KEYPAD == IRIVER_H10_PAD
    rb->lcd_putsxy(2, 8, "[POWER] to stop");
    rb->lcd_putsxy(2, 18, "[REW] toggle");
    rb->lcd_putsxy(2, 28, "[PL-LEFT] shuffle");
    rb->lcd_putsxy(2, 38, "[PL-RIGHT] solution");
    rb->lcd_putsxy(2, 48, "[PL-UP] step by step");
#elif CONFIG_KEYPAD == GIGABEAT_S_PAD
    rb->lcd_putsxy(2, 8, "[BACK] to stop");
    rb->lcd_putsxy(2, 18, "[SELECT] toggle");
    rb->lcd_putsxy(2, 28, "[MENU] shuffle");
    rb->lcd_putsxy(2, 38, "[VOL+] solution");
    rb->lcd_putsxy(2, 48, "[VOL-] step by step");
#elif (CONFIG_KEYPAD == SANSA_E200_PAD) || \
      (CONFIG_KEYPAD == SANSA_C200_PAD)
    rb->lcd_putsxy(2, 8, "[POWER] to stop");
    rb->lcd_putsxy(2, 18, "[SELECT] toggle");
    rb->lcd_putsxy(2, 28, "[REC-LEFT] shuffle");
    rb->lcd_putsxy(2, 38, "[REC-RIGHT] solution");
    rb->lcd_putsxy(2, 48, "[REC-SEL] step by step");
#elif CONFIG_KEYPAD == IAUDIO_M3_PAD
    rb->lcd_putsxy(2, 8, "[REC] to stop");
    rb->lcd_putsxy(2, 18, "[PLAY] toggle");
    rb->lcd_putsxy(2, 28, "[MODE] shuffle");
    rb->lcd_putsxy(2, 38, "[MENU..] solution");
    rb->lcd_putsxy(2, 48, "[MENU] step by step");
#elif CONFIG_KEYPAD == SANSA_CONNECT_PAD
    rb->lcd_putsxy(2, 8, "[POWER] to stop");
    rb->lcd_putsxy(2, 18, "[SELECT] toggle");
    rb->lcd_putsxy(2, 28, "[VOL+] shuffle");
    rb->lcd_putsxy(2, 38, "[PREV] solution");
    rb->lcd_putsxy(2, 48, "[NEXT] step by step");
#elif (CONFIG_KEYPAD == SAMSUNG_YH92X_PAD) || \
      (CONFIG_KEYPAD == SAMSUNG_YH820_PAD)
    rb->lcd_putsxy(2, 8, "Long [REW] to stop");
    rb->lcd_putsxy(2, 18, "[PLAY] toggle");
    rb->lcd_putsxy(2, 28, "[REW] shuffle");
    rb->lcd_putsxy(2, 38, "Long [FFWD] solution");
    rb->lcd_putsxy(2, 48, "[FFWD] step by step");
#endif

#ifdef HAVE_TOUCHSCREEN
    rb->lcd_putsxy(2, 8, "[BOTTOMLEFT]  to stop");
    rb->lcd_putsxy(2, 18, "[CENTRE]      toggle");
    rb->lcd_putsxy(2, 28, "[TOPRIGHT]    shuffle");
    rb->lcd_putsxy(2, 38, "[BOTTOMLEFT]  solution");
    rb->lcd_putsxy(2, 48, "[BOTTOMRIGHT] step by step");
#endif

    rb->lcd_update();

    rb->button_get_w_tmo(HZ*3);

    rb->lcd_clear_display();
    draw_info_panel();
    for (i=0; i<20; i++) {
        spots[i]=1;
        draw_spot(i);
    }
    rb->lcd_update();
    rb->sleep(HZ*3/2);
    rb->srand(*rb->current_tick);

    rc = flipit_loop();
    return rc;
}
