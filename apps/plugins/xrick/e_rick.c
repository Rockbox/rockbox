/*
 * xrick/e_rick.c
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

#include "xrick/e_rick.h"

#include "xrick/system/system.h"
#include "xrick/config.h"
#include "xrick/game.h"
#include "xrick/ents.h"
#include "xrick/e_bullet.h"
#include "xrick/e_bomb.h"
#include "xrick/control.h"
#include "xrick/maps.h"
#include "xrick/util.h"

/*
 * public vars
 */
S16 e_rick_stop_x = 0;
S16 e_rick_stop_y = 0;
unsigned e_rick_state = 0;

/*
* public functions
*/
extern inline void e_rick_state_set(e_rick_state_t s);
extern inline void e_rick_state_clear(e_rick_state_t s);
extern inline bool e_rick_state_test(e_rick_state_t s);


/*
 * local vars
 */
static U8 scrawl;

static bool trigger = false;

static S8 offsx;
static U8 ylow;
static S16 offsy;

static U8 seq;

static U8 save_crawl, save_direction;
static U16 save_x, save_y;


/*
 * Box test
 *
 * ASM 113E (based on)
 *
 * e: entity to test against (corresponds to SI in asm code -- here DI
 *    is assumed to point to rick).
 * ret: true/intersect, false/not.
 */
bool
e_rick_boxtest(U8 e)
{
    /*
     * rick: x+0x05 to x+0x11, y+[0x08 if rick's crawling] to y+0x14
     * entity: x to x+w, y to y+h
     */

    if (E_RICK_ENT.x + 0x11 < ent_ents[e].x ||
        E_RICK_ENT.x + 0x05 > ent_ents[e].x + ent_ents[e].w ||
        E_RICK_ENT.y + 0x14 < ent_ents[e].y ||
        E_RICK_ENT.y + (e_rick_state_test(E_RICK_STCRAWL) ? 0x08 : 0x00) > ent_ents[e].y + ent_ents[e].h - 1)
        return false;
    else
        return true;
}




/*
 * Go zombie
 *
 * ASM 1851
 */
void
e_rick_gozombie(void)
{
#ifdef ENABLE_CHEATS
    if (game_cheat2) return;
#endif

    /* already zombie? */
    if (e_rick_state_test(E_RICK_STZOMBIE)) return;

#ifdef ENABLE_SOUND
    syssnd_play(soundDie, 1);
#endif

    e_rick_state_set(E_RICK_STZOMBIE);
    offsy = -0x0300;
    offsx = (E_RICK_ENT.x > 0x80 ? -3 : +3);
    ylow = 0;
    E_RICK_ENT.front = true;
}


/*
 * Action sub-function for e_rick when zombie
 *
 * ASM 17DC
 */
static void
e_rick_z_action(void)
{
    U32 i;

    /* sprite */
    E_RICK_ENT.sprite = (E_RICK_ENT.x & 0x04) ? 0x1A : 0x19;

    /* x */
    E_RICK_ENT.x += offsx;

    /* y */
    i = (E_RICK_ENT.y << 8) + offsy + ylow;
    E_RICK_ENT.y = i >> 8;
    offsy += 0x80;
    ylow = i;

    /* dead when out of screen */
    if (E_RICK_ENT.y < 0 || E_RICK_ENT.y > 0x0140)
    {
        e_rick_state_set(E_RICK_STDEAD);
    }
}


/*
 * Action sub-function for e_rick.
 *
 * ASM 13BE
 */
void
e_rick_action2(void)
{
    U8 env0, env1;
    S16 x, y;
    U32 i;

    e_rick_state_clear(E_RICK_STSTOP | E_RICK_STSHOOT);

    /* if zombie, run dedicated function and return */
    if (e_rick_state_test(E_RICK_STZOMBIE))
    {
        e_rick_z_action();
        return;
    }

    /* climbing? */
    if (e_rick_state_test(E_RICK_STCLIMB))
    {
        goto climbing;
    }
    /*
    * NOT CLIMBING
    */
    e_rick_state_clear(E_RICK_STJUMP);
    /* calc y */
    i = (E_RICK_ENT.y << 8) + offsy + ylow;
    y = i >> 8;
    /* test environment */
    u_envtest(E_RICK_ENT.x, y, e_rick_state_test(E_RICK_STCRAWL), &env0, &env1);
    /* stand up, if possible */
    if (e_rick_state_test(E_RICK_STCRAWL) && !env0)
    {
        e_rick_state_clear(E_RICK_STCRAWL);
    }
    /* can move vertically? */
    if (env1 & (offsy < 0 ?
                    MAP_EFLG_VERT|MAP_EFLG_SOLID|MAP_EFLG_SPAD :
                    MAP_EFLG_VERT|MAP_EFLG_SOLID|MAP_EFLG_SPAD|MAP_EFLG_WAYUP))
        goto vert_not;

    /*
    * VERTICAL MOVE
    */
    e_rick_state_set(E_RICK_STJUMP);
    /* killed? */
    if (env1 & MAP_EFLG_LETHAL) {
        e_rick_gozombie();
        return;
    }
    /* save */
    E_RICK_ENT.y = y;
    ylow = i;
    /* climb? */
    if ((env1 & MAP_EFLG_CLIMB) && (control_test(Control_UP | Control_DOWN)))
    {
        offsy = 0x0100;
        e_rick_state_set(E_RICK_STCLIMB);
        return;
    }
    /* fall */
    offsy += 0x0080;
    if (offsy > 0x0800) {
        offsy = 0x0800;
        ylow = 0;
    }

    /*
    * HORIZONTAL MOVE
    */
    horiz:
    /* should move? */
    if (!(control_test(Control_LEFT | Control_RIGHT))) {
        seq = 2; /* no: reset seq and return */
        return;
    }
    if (control_test(Control_LEFT)) {  /* move left */
        x = E_RICK_ENT.x - 2;
        game_dir = LEFT;
        if (x < 0) {  /* prev submap */
            game_chsm = true;
            E_RICK_ENT.x = 0xe2;
            return;
        }
    } else {  /* move right */
        x = E_RICK_ENT.x + 2;
        game_dir = RIGHT;
        if (x >= 0xe8) {  /* next submap */
            game_chsm = true;
            E_RICK_ENT.x = 0x04;
            return;
        }
    }

    /* still within this map: test environment */
    u_envtest(x, E_RICK_ENT.y, e_rick_state_test(E_RICK_STCRAWL), &env0, &env1);

    /* save x-position if it is possible to move */
    if (!(env1 & (MAP_EFLG_SOLID|MAP_EFLG_SPAD|MAP_EFLG_WAYUP))) {
        E_RICK_ENT.x = x;
        if (env1 & MAP_EFLG_LETHAL) e_rick_gozombie();
    }

    /* end */
    return;

  /*
   * NO VERTICAL MOVE
   */
 vert_not:
  if (offsy < 0) {
    /* not climbing + trying to go _up_ not possible -> hit the roof */
    e_rick_state_set(E_RICK_STJUMP);  /* fall back to the ground */
    E_RICK_ENT.y &= 0xF8;
    offsy = 0;
    ylow = 0;
    goto horiz;
  }
  /* else: not climbing + trying to go _down_ not possible -> standing */
  /* align to ground */
  E_RICK_ENT.y &= 0xF8;
  E_RICK_ENT.y |= 0x03;
  ylow = 0;

  /* standing on a super pad? */
  if ((env1 & MAP_EFLG_SPAD) && offsy >= 0X0200) {
    offsy = (control_test(Control_UP)) ? 0xf800 : 0x00fe - offsy;
#ifdef ENABLE_SOUND
    syssnd_play(soundPad, 1);
#endif
    goto horiz;
  }

  offsy = 0x0100;  /* reset*/

  /* standing. firing ? */
  if (scrawl || !(control_test(Control_FIRE)))
    goto firing_not;

  /*
   * FIRING
   */
    if (control_test(Control_LEFT | Control_RIGHT)) {  /* stop */
        if (control_test(Control_RIGHT))
        {
            game_dir = RIGHT;
            e_rick_stop_x = E_RICK_ENT.x + 0x17;
        } else {
            game_dir = LEFT;
            e_rick_stop_x = E_RICK_ENT.x;
        }
        e_rick_stop_y = E_RICK_ENT.y + 0x000E;
        e_rick_state_set(E_RICK_STSTOP);
        return;
    }

  if (control_test(Control_UP)) {  /* bullet */
    e_rick_state_set(E_RICK_STSHOOT);
    /* not an automatic gun: shoot once only */
    if (trigger)
      return;
    else
      trigger = true;
    /* already a bullet in the air ... that's enough */
    if (E_BULLET_ENT.n)
      return;
    /* else use a bullet, if any available */
    if (!game_bullets)
      return;
#ifdef ENABLE_CHEATS
    if (!game_cheat1)
#endif
    {
      game_bullets--;
    }

    /* initialize bullet */
    e_bullet_init(E_RICK_ENT.x, E_RICK_ENT.y);
    return;
  }

  trigger = false; /* not shooting means trigger is released */
  seq = 0; /* reset */

  if (control_test(Control_DOWN)) {  /* bomb */
    /* already a bomb ticking ... that's enough */
    if (E_BOMB_ENT.n)
      return;
    /* else use a bomb, if any available */
    if (!game_bombs)
      return;
#ifdef ENABLE_CHEATS
    if (!game_cheat1)
#endif
    {
      game_bombs--;
    }

    /* initialize bomb */
    e_bomb_init(E_RICK_ENT.x, E_RICK_ENT.y);
    return;
  }

  return;

  /*
   * NOT FIRING
   */
 firing_not:
  if (control_test(Control_UP)) {  /* jump or climb */
    if (env1 & MAP_EFLG_CLIMB) {  /* climb */
      e_rick_state_set(E_RICK_STCLIMB);
      return;
    }
    offsy = -0x0580;  /* jump */
    ylow = 0;
#ifdef ENABLE_SOUND
    syssnd_play(soundJump, 1);
#endif
    goto horiz;
  }
  if (control_test(Control_DOWN)) {  /* crawl or climb */
    if ((env1 & MAP_EFLG_VERT) &&  /* can go down */
    !(control_test(Control_LEFT | Control_RIGHT)) &&  /* + not moving horizontaly */
    (E_RICK_ENT.x & 0x1f) < 0x0a) {  /* + aligned -> climb */
      E_RICK_ENT.x &= 0xf0;
      E_RICK_ENT.x |= 0x04;
      e_rick_state_set(E_RICK_STCLIMB);
    }
    else {  /* crawl */
      e_rick_state_set(E_RICK_STCRAWL);
      goto horiz;
    }

  }
  goto horiz;

    /*
    * CLIMBING
    */
    climbing:
        /* should move? */
        if (!(control_test(Control_UP | Control_DOWN | Control_LEFT | Control_RIGHT))) {
            seq = 0; /* no: reset seq and return */
            return;
        }

        if (control_test(Control_UP | Control_DOWN)) {
            /* up-down: calc new y and test environment */
            y = E_RICK_ENT.y + ((control_test(Control_UP)) ? -0x02 : 0x02);
            u_envtest(E_RICK_ENT.x, y, e_rick_state_test(E_RICK_STCRAWL), &env0, &env1);
            if (env1 & (MAP_EFLG_SOLID|MAP_EFLG_SPAD|MAP_EFLG_WAYUP) &&
                    !(control_test(Control_UP))) {
                /* FIXME what? */
                e_rick_state_clear(E_RICK_STCLIMB);
                return;
            }
            if (!(env1 & (MAP_EFLG_SOLID|MAP_EFLG_SPAD|MAP_EFLG_WAYUP)) ||
                    (env1 & MAP_EFLG_WAYUP)) {
                /* ok to move, save */
                E_RICK_ENT.y = y;
                if (env1 & MAP_EFLG_LETHAL) {
                    e_rick_gozombie();
                    return;
                }
                if (!(env1 & (MAP_EFLG_VERT|MAP_EFLG_CLIMB))) {
                    /* reached end of climb zone */
                    offsy = (control_test(Control_UP)) ? -0x0300 : 0x0100;
#ifdef ENABLE_SOUND
                    if (control_test(Control_UP))
                        syssnd_play(soundJump, 1);
#endif
                    e_rick_state_clear(E_RICK_STCLIMB);
                    return;
                }
            }
        }
  if (control_test(Control_LEFT | Control_RIGHT)) {
    /* left-right: calc new x and test environment */
    if (control_test(Control_LEFT)) {
      x = E_RICK_ENT.x - 0x02;
      if (x < 0) {  /* (i.e. negative) prev submap */
    game_chsm = true;
    /*6dbd = 0x00;*/
    E_RICK_ENT.x = 0xe2;
    return;
      }
    }
    else {
      x = E_RICK_ENT.x + 0x02;
      if (x >= 0xe8) {  /* next submap */
    game_chsm = true;
    /*6dbd = 0x01;*/
    E_RICK_ENT.x = 0x04;
    return;
      }
    }
    u_envtest(x, E_RICK_ENT.y, e_rick_state_test(E_RICK_STCRAWL), &env0, &env1);
    if (env1 & (MAP_EFLG_SOLID|MAP_EFLG_SPAD)) return;
    E_RICK_ENT.x = x;
    if (env1 & MAP_EFLG_LETHAL) {
      e_rick_gozombie();
      return;
    }

    if (env1 & (MAP_EFLG_VERT|MAP_EFLG_CLIMB)) return;
    e_rick_state_clear(E_RICK_STCLIMB);
    if (control_test(Control_UP))
      offsy = -0x0300;
  }
}


/*
 * Action function for e_rick
 *
 * ASM 12CA
 */
void e_rick_action(U8 e/*unused*/)
{
    static U8 stopped = false; /* is this the most elegant way? */

    (void)e;

    e_rick_action2();

    scrawl = e_rick_state_test(E_RICK_STCRAWL);

    if (e_rick_state_test(E_RICK_STZOMBIE))
    {
        return;
    }
    /*
     * set sprite
     */

    if (e_rick_state_test(E_RICK_STSTOP))
    {
        E_RICK_ENT.sprite = (game_dir ? 0x17 : 0x0B);
#ifdef ENABLE_SOUND
        if (!stopped)
        {
            syssnd_play(soundStick, 1);
            stopped = true;
        }
#endif
        return;
    }

    stopped = false;

    if (e_rick_state_test(E_RICK_STSHOOT))
    {
        E_RICK_ENT.sprite = (game_dir ? 0x16 : 0x0A);
        return;
    }

    if (e_rick_state_test(E_RICK_STCLIMB))
    {
        E_RICK_ENT.sprite = (((E_RICK_ENT.x ^ E_RICK_ENT.y) & 0x04) ? 0x18 : 0x0c);
#ifdef ENABLE_SOUND
        seq = (seq + 1) & 0x03;
        if (seq == 0) syssnd_play(soundWalk, 1);
#endif
        return;
    }

    if (e_rick_state_test(E_RICK_STCRAWL))
    {
        E_RICK_ENT.sprite = (game_dir ? 0x13 : 0x07);
        if (E_RICK_ENT.x & 0x04) E_RICK_ENT.sprite++;
#ifdef ENABLE_SOUND
        seq = (seq + 1) & 0x03;
        if (seq == 0) syssnd_play(soundCrawl, 1);
#endif
        return;
    }

    if (e_rick_state_test(E_RICK_STJUMP))
    {
        E_RICK_ENT.sprite = (game_dir ? 0x15 : 0x06);
        return;
    }

    seq++;

    if (seq >= 0x14)
    {
#ifdef ENABLE_SOUND
        syssnd_play(soundWalk, 1);
#endif
        seq = 0x04;
    }
#ifdef ENABLE_SOUND
    else
    {
        if (seq == 0x0C)
        {
            syssnd_play(soundWalk, 1);
        }
    }
#endif

    E_RICK_ENT.sprite = (seq >> 2) + 1 + (game_dir ? 0x0c : 0x00);
}


/*
 * Save status
 *
 * ASM part of 0x0BBB
 */
void e_rick_save(void)
{
    save_x = E_RICK_ENT.x;
    save_y = E_RICK_ENT.y;
    save_crawl = e_rick_state_test(E_RICK_STCRAWL);
    save_direction = game_dir;
    /* FIXME
     * save_C0 = E_RICK_ENT.b0C;
     * plus some 6DBC stuff?
     */
}


/*
 * Restore status
 *
 * ASM part of 0x0BDC
 */
void e_rick_restore(void)
{
    E_RICK_ENT.x = save_x;
    E_RICK_ENT.y = save_y;
    if (save_crawl)
    {
        e_rick_state_set(E_RICK_STCRAWL);
    }
    else
    {
         e_rick_state_clear(E_RICK_STCRAWL);
    }
    game_dir = save_direction;

    E_RICK_ENT.front = false;
    e_rick_state_clear(E_RICK_STCLIMB); /* should we clear other states? */
    /* FIXME
     * E_RICK_ENT.b0C = save_C0;
     * plus some 6DBC stuff?
     */
}




/* eof */
