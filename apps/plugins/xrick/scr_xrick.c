/*
 * xrick/scr_xrick.c
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
#include "xrick/data/img.h"
#include "xrick/system/system.h"

/*
 * global vars
 */
size_t screen_nbr_imapsl = 0;
U8 *screen_imapsl = NULL;

size_t screen_nbr_imapstesps = 0;
screen_imapsteps_t *screen_imapsteps = NULL;

size_t screen_nbr_imapsofs = 0;
U8 *screen_imapsofs = NULL;

size_t screen_nbr_imaptext = 0;
U8 **screen_imaptext = NULL;

size_t screen_nbr_hiscores = 0;
hiscore_t *screen_highScores = NULL;

#ifdef GFXPC
U8 *screen_imainhoft = NULL;
U8 *screen_imainrdt = NULL;
U8 *screen_imaincdc = NULL;
U8 *screen_congrats = NULL;
#endif
U8 *screen_gameovertxt = NULL;
U8 *screen_pausedtxt = NULL;


/*
 * Display XRICK splash screen
 *
 * return: SCREEN_RUNNING, SCREEN_DONE, SCREEN_EXIT
 */
U8
screen_xrick(void)
{
    static U8 seq = 0;
    static U8 wait = 0;

    if (seq == 0) {
        sysvid_clear();
        draw_img(img_splash);
        game_rects = &draw_SCREENRECT;
        seq = 1;
    }

    switch (seq) {
    case 1:  /* wait */
        if (wait++ > 0x2) {
#ifdef ENABLE_SOUND
            game_setmusic(soundBullet, 1);
#endif
            seq = 2;
            wait = 0;
        }
        break;

    case 2:  /* wait */
        if (wait++ > 0x20) {
            seq = 99;
            wait = 0;
        }
    }

    if (control_test(Control_EXIT))  /* check for exit request */
        return SCREEN_EXIT;

    if (seq == 99) {  /* we're done */
        sysvid_clear();
        sysvid_setGamePalette();
        seq = 0;
        return SCREEN_DONE;
    }

    return SCREEN_RUNNING;
}

/* eof */

