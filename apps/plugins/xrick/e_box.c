/*
 * xrick/e_box.c
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

#include "xrick/e_box.h"

#include "xrick/game.h"
#include "xrick/ents.h"
#include "xrick/e_bullet.h"
#include "xrick/e_bomb.h"
#include "xrick/e_rick.h"
#include "xrick/maps.h"
#include "xrick/util.h"

/*
 * FIXME this is because the same structure is used
 * for all entities. Need to replace this w/ an inheritance
 * solution.
 */
#define cnt c1

/*
 * Constants
 */
#define SEQ_INIT 0x0A

/*
 * Prototypes
 */
static void explode(U8);

/*
 * Entity action
 *
 * ASM 245A
 */
void
e_box_action(U8 e)
{
    static U8 sp[] = {0x24, 0x25, 0x26, 0x27, 0x28};  /* explosion sprites sequence */

    if (ent_ents[e].n & ENT_LETHAL) {
        /*
         * box is lethal i.e. exploding
         * play sprites sequence then stop
         */
        ent_ents[e].sprite = sp[ent_ents[e].cnt >> 1];
        if (--ent_ents[e].cnt == 0) {
            ent_ents[e].n = 0;
            map_marks[ent_ents[e].mark].ent |= MAP_MARK_NACT;
        }
    } else {
        /*
         * not lethal: check to see if triggered
         */
        if (e_rick_boxtest(e)) {
            /* rick: collect bombs or bullets and stop */
#ifdef ENABLE_SOUND
            syssnd_play(soundBox, 1);
#endif
            if (ent_ents[e].n == 0x10)
                game_bombs = GAME_BOMBS_INIT;
            else  /* 0x11 */
                game_bullets = GAME_BULLETS_INIT;
            ent_ents[e].n = 0;
            map_marks[ent_ents[e].mark].ent |= MAP_MARK_NACT;
        }
        else if (e_rick_state_test(E_RICK_STSTOP) &&
                u_fboxtest(e, e_rick_stop_x, e_rick_stop_y)) {
            /* rick's stick: explode */
            explode(e);
        }
        else if (E_BULLET_ENT.n && u_fboxtest(e, e_bullet_xc, e_bullet_yc)) {
            /* bullet: explode (and stop bullet) */
            E_BULLET_ENT.n = 0;
            explode(e);
        }
        else if (e_bomb_lethal && e_bomb_hit(e)) {
            /* bomb: explode */
            explode(e);
        }
    }
}


/*
 * Explode when
 */
static void explode(U8 e)
{
    ent_ents[e].cnt = SEQ_INIT;
    ent_ents[e].n |= ENT_LETHAL;
#ifdef ENABLE_SOUND
    syssnd_play(soundExplode, 1);
#endif
}

/* eof */


