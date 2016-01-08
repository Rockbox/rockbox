/*
 * xrick/scr_getname.c
 *
 * Copyright (C) 1998-2002 BigOrno (bigorno@bigorno.net).
 * Copyright (C) 2008-2014 Pierluigi Vicinanza.
 * All rights reserved.
 *
 * The use and distribution terms for this software are contained in the file
 * named README, which can be found in the root of this distribution. By
 * using this software in any fashion, you are agreeing to be bound by the
 * terms of this license.
 *
 * You must not remove this notice, or any other, from this software.
 */

#include "xrick/screens.h"

#include "xrick/game.h"
#include "xrick/draw.h"
#include "xrick/control.h"
#include "xrick/data/pics.h"
#include "xrick/system/system.h"

/*
 * local vars
 */
static U8 seq = 0;
static U8 x, y, p;
static U8 player_name[HISCORE_NAME_SIZE];

#define TILE_POINTER '\072'
#define TILE_CURSOR '\073'
#define TOPLEFT_X 116
#define TOPLEFT_Y 64
#define NAMEPOS_X 120
#define NAMEPOS_Y 160
#define AUTOREPEAT_TMOUT 100


/*
 * prototypes
 */
static void pointer_show(bool);
static void name_update(void);
static void name_draw(void);


/*
 * Get name
 *
 * return: 0 while running, 1 when finished.
 */
U8
screen_getname(void)
{
    static U32 tm = 0;
    U8 i, j;

    if (seq == 0)
    {
        /* figure out if this is a high score */
        if (game_score < screen_highScores[screen_nbr_hiscores - 1].score)
            return SCREEN_DONE;

        /* prepare */
        draw_tilesBank = 0;
#ifdef GFXPC
        draw_filter = 0xffff;
#endif
        for (i = 0; i < HISCORE_NAME_SIZE; i++)
        {
            player_name[i] = '@';
        }
        x = 5, y = 4, p = 0;
        game_rects = &draw_SCREENRECT;
        seq = 1;
    }

    switch (seq)
    {
        case 1:  /* prepare screen */
        {
            sysvid_clear();
#ifdef GFXPC
            draw_setfb(32, 8);
            draw_filter = 0xaaaa; /* red */
            draw_tilesListImm(screen_congrats);
#endif
#ifdef GFXST
            draw_pic(pic_congrats);
#endif
            draw_setfb(72, 40);
#ifdef GFXPC
            draw_filter = 0xffff; /* yellow */
#endif
            draw_tilesListImm((U8 *)"PLEASE@ENTER@YOUR@NAME\376");
#ifdef GFXPC
            draw_filter = 0x5555; /* green */
#endif
            for (i = 0; i < 6; i++)
            {
                for (j = 0; j < 4; j++)
                {
                    draw_setfb(TOPLEFT_X + i * 8 * 2, TOPLEFT_Y + j * 8 * 2);
                    draw_tile('A' + i + j * 6);
                }
            }
            draw_setfb(TOPLEFT_X, TOPLEFT_Y + 64);
#ifdef GFXST
            draw_tilesListImm((U8 *)"Y@Z@.@@@\074\373\374\375\376");
#endif
#ifdef GFXPC
            draw_tilesListImm((U8 *)"Y@Z@.@@@\074@\075@\376");
#endif
            name_draw();
            pointer_show(true);
            seq = 2;
            break;
        }
        case 2:  /* wait for key pressed */
        {
            if (control_test(Control_FIRE))
                seq = 3;
            if (control_test(Control_UP)) {
                if (y > 0) {
                    pointer_show(false);
                    y--;
                    pointer_show(true);
                    tm = sys_gettime();
                }
                seq = 4;
            }
            if (control_test(Control_DOWN)) {
                if (y < 4) {
                    pointer_show(false);
                    y++;
                    pointer_show(true);
                    tm = sys_gettime();
                }
                seq = 5;
            }
            if (control_test(Control_LEFT)) {
                if (x > 0) {
                    pointer_show(false);
                    x--;
                    pointer_show(true);
                    tm = sys_gettime();
                }
                seq = 6;
            }
            if (control_test(Control_RIGHT)) {
                if (x < 5) {
                    pointer_show(false);
                    x++;
                    pointer_show(true);
                    tm = sys_gettime();
                }
                seq = 7;
            }
            break;
        }
        case 3:  /* wait for FIRE released */
        {
            if (!(control_test(Control_FIRE)))
            {
                if (x == 5 && y == 4)
                {  /* end */
                    i = 0;
                    while (game_score < screen_highScores[i].score) i++;
                    j = 7;
                    while (j > i)
                    {
                        screen_highScores[j].score = screen_highScores[j - 1].score;
                        for (x = 0; x < HISCORE_NAME_SIZE; x++)
                        {
                            screen_highScores[j].name[x] = screen_highScores[j - 1].name[x];
                        }
                        j--;
                    }
                    screen_highScores[i].score = game_score;
                    for (x = 0; x < HISCORE_NAME_SIZE; x++)
                    {
                        screen_highScores[i].name[x] = player_name[x];
                    }
                    seq = 99;
                }
                else
                {
                    name_update();
                    name_draw();
                    seq = 2;
                }
            }
            break;
        }
        case 4:  /* wait for UP released */
        {
            if (!(control_test(Control_UP)) ||
                sys_gettime() - tm > AUTOREPEAT_TMOUT)
                seq = 2;
            break;
        }
        case 5:  /* wait for DOWN released */
        {
            if (!(control_test(Control_DOWN)) ||
                sys_gettime() - tm > AUTOREPEAT_TMOUT)
                seq = 2;
            break;
        }
        case 6:  /* wait for LEFT released */
        {
            if (!(control_test(Control_LEFT)) ||
                sys_gettime() - tm > AUTOREPEAT_TMOUT)
                seq = 2;
            break;
        }
        case 7:  /* wait for RIGHT released */
        {
            if (!(control_test(Control_RIGHT)) ||
                sys_gettime() - tm > AUTOREPEAT_TMOUT)
                seq = 2;
            break;
        }
    }

    if (control_test(Control_EXIT))  /* check for exit request */
        return SCREEN_EXIT;

    if (seq == 99) {  /* seq 99, we're done */
        sysvid_clear();
        seq = 0;
        return SCREEN_DONE;
    }
    else
        return SCREEN_RUNNING;
}


static void
pointer_show(bool show)
{
  draw_setfb(TOPLEFT_X + x * 8 * 2, TOPLEFT_Y + y * 8 * 2 + 8);
#ifdef GFXPC
  draw_filter = 0xaaaa; /* red */
#endif
  draw_tile(show? TILE_POINTER:'@');
}

static void
name_update(void)
{
  U8 i;

  i = x + y * 6;
  if (i < 26 && p < 10)
    player_name[p++] = 'A' + i;
  if (i == 26 && p < 10)
    player_name[p++] = '.';
  if (i == 27 && p < 10)
    player_name[p++] = '@';
  if (i == 28 && p > 0) {
    p--;
  }
}

static void
name_draw(void)
{
  U8 i;

  draw_setfb(NAMEPOS_X, NAMEPOS_Y);
#ifdef GFXPC
  draw_filter = 0xaaaa; /* red */
#endif
  for (i = 0; i < p; i++)
    draw_tile(player_name[i]);
  for (i = p; i < 10; i++)
    draw_tile(TILE_CURSOR);

#ifdef GFXST
  draw_setfb(NAMEPOS_X, NAMEPOS_Y + 8);
  for (i = 0; i < 10; i++)
    draw_tile('@');
  draw_setfb(NAMEPOS_X + 8 * (p < 9 ? p : 9), NAMEPOS_Y + 8);
  draw_tile(TILE_POINTER);
#endif
}


/* eof */
