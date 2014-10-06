/*
 * xrick/e_them.c
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

#include "xrick/e_them.h"

#include "xrick/game.h"
#include "xrick/ents.h"
#include "xrick/e_rick.h"
#include "xrick/e_bomb.h"
#include "xrick/e_bullet.h"
#include "xrick/maps.h"
#include "xrick/util.h"

#define TYPE_1A (0x00)
#define TYPE_1B (0xff)

/*
 * public vars
 */
U32 e_them_rndseed = 0;

/*
 * local vars
 */
static U16 e_them_rndnbr = 0;

/*
 * Check if entity boxtests with a lethal e_them i.e. something lethal
 * in slot 0 and 4 to 8.
 *
 * ASM 122E
 *
 * e: entity slot number.
 * ret: true/boxtests, false/not
 */
static bool
u_themtest(U8 e)
{
  U8 i;

  if ((ent_ents[0].n & ENT_LETHAL) && u_boxtest(e, 0))
    return true;

  for (i = 4; i < 9; i++)
    if ((ent_ents[i].n & ENT_LETHAL) && u_boxtest(e, i))
      return true;

  return false;
}


/*
 * Go zombie
 *
 * ASM 237B
 */
void
e_them_gozombie(U8 e)
{
#define offsx c1
  ent_ents[e].n = 0x47;  /* zombie entity */
  ent_ents[e].front = true;
  ent_ents[e].offsy = -0x0400;
#ifdef ENABLE_SOUND
  syssnd_play(soundDie, 1);
#endif
  game_score += 50;
  if (ent_ents[e].flags & ENT_FLG_ONCE) {
    /* make sure entity won't be activated again */
    map_marks[ent_ents[e].mark].ent |= MAP_MARK_NACT;
  }
  ent_ents[e].offsx = (ent_ents[e].x >= 0x80 ? -0x02 : 0x02);
#undef offsx
}


/*
 * Action sub-function for e_them _t1a and _t1b
 *
 * Those two types move horizontally, and fall if they have to.
 * Type 1a moves horizontally over a given distance and then
 * u-turns and repeats; type 1b is more subtle as it does u-turns
 * in order to move horizontally towards rick.
 *
 * ASM 2242
 */
void
e_them_t1_action2(U8 e, U8 type)
{
#define offsx c1
#define step_count c2
  U32 i;
  S16 x, y;
  U8 env0, env1;

  /* by default, try vertical move. calculate new y */
  i = (ent_ents[e].y << 8) + ent_ents[e].offsy + ent_ents[e].ylow;
  y = i >> 8;

  /* deactivate if outside vertical boundaries */
  /* no need to test zero since e_them _t1a/b don't go up */
  /* FIXME what if they got scrolled out ? */
  if (y > 0x140) {
    ent_ents[e].n = 0;
    return;
  }

  /* test environment */
  u_envtest(ent_ents[e].x, y, false, &env0, &env1);

  if (!(env1 & (MAP_EFLG_VERT|MAP_EFLG_SOLID|MAP_EFLG_SPAD|MAP_EFLG_WAYUP))) {
    /* vertical move possible: falling */
    if (env1 & MAP_EFLG_LETHAL) {
      /* lethal entities kill e_them */
      e_them_gozombie(e);
      return;
    }
    /* save, cleanup and return */
    ent_ents[e].y = y;
    ent_ents[e].ylow = i;
    ent_ents[e].offsy += 0x0080;
    if (ent_ents[e].offsy > 0x0800)
      ent_ents[e].offsy = 0x0800;
    return;
  }

  /* vertical move not possible. calculate new sprite */
  ent_ents[e].sprite = ent_ents[e].sprbase
    + ent_sprseq[(ent_ents[e].x & 0x1c) >> 3]
    + (ent_ents[e].offsx < 0 ? 0x03 : 0x00);

  /* reset offsy */
  ent_ents[e].offsy = 0x0080;

  /* align to ground */
  ent_ents[e].y &= 0xfff8;
  ent_ents[e].y |= 0x0003;

  /* latency: if not zero then decrease and return */
  if (ent_ents[e].latency > 0) {
    ent_ents[e].latency--;
    return;
  }

  /* horizontal move. calculate new x */
  if (ent_ents[e].offsx == 0)  /* not supposed to move -> don't */
    return;

  x = ent_ents[e].x + ent_ents[e].offsx;
  if (ent_ents[e].x < 0 || ent_ents[e].x > 0xe8) {
    /*  U-turn and return if reaching horizontal boundaries */
    ent_ents[e].step_count = 0;
    ent_ents[e].offsx = -ent_ents[e].offsx;
    return;
  }

  /* test environment */
  u_envtest(x, ent_ents[e].y, false, &env0, &env1);

  if (env1 & (MAP_EFLG_VERT|MAP_EFLG_SOLID|MAP_EFLG_SPAD|MAP_EFLG_WAYUP)) {
    /* horizontal move not possible: u-turn and return */
    ent_ents[e].step_count = 0;
    ent_ents[e].offsx = -ent_ents[e].offsx;
    return;
  }

  /* horizontal move possible */
  if (env1 & MAP_EFLG_LETHAL) {
    /* lethal entities kill e_them */
    e_them_gozombie(e);
    return;
  }

  /* save */
  ent_ents[e].x = x;

  /* depending on type, */
  if (type == TYPE_1B) {
    /* set direction to move horizontally towards rick */
    if ((ent_ents[e].x & 0x1e) != 0x10)  /* prevents too frequent u-turns */
      return;
    ent_ents[e].offsx = (ent_ents[e].x < E_RICK_ENT.x) ? 0x02 : -0x02;
    return;
  }
  else {
    /* set direction according to step counter */
    ent_ents[e].step_count++;
    /* FIXME why trig_x (b16) ?? */
    if ((ent_ents[e].trig_x >> 1) > ent_ents[e].step_count)
      return;
  }

  /* type is 1A and step counter reached its limit: u-turn */
  ent_ents[e].step_count = 0;
  ent_ents[e].offsx = -ent_ents[e].offsx;
#undef offsx
#undef step_count
}


/*
 * ASM 21CF
 */
void
e_them_t1_action(U8 e, U8 type)
{
  e_them_t1_action2(e, type);

  /* lethal entities kill them */
  if (u_themtest(e)) {
    e_them_gozombie(e);
    return;
  }

  /* bullet kills them */
  if (E_BULLET_ENT.n &&
      u_fboxtest(e, E_BULLET_ENT.x + (e_bullet_offsx < 0 ? 0 : 0x18),
         E_BULLET_ENT.y)) {
    E_BULLET_ENT.n = 0;
    e_them_gozombie(e);
    return;
  }

  /* bomb kills them */
  if (e_bomb_lethal && e_bomb_hit(e)) {
    e_them_gozombie(e);
    return;
  }

  /* rick stops them */
  if (e_rick_state_test(E_RICK_STSTOP) &&
      u_fboxtest(e, e_rick_stop_x, e_rick_stop_y))
    ent_ents[e].latency = 0x14;

  /* they kill rick */
  if (e_rick_boxtest(e))
    e_rick_gozombie();
}


/*
 * Action function for e_them _t1a type (stays within boundaries)
 *
 * ASM 2452
 */
void
e_them_t1a_action(U8 e)
{
  e_them_t1_action(e, TYPE_1A);
}


/*
 * Action function for e_them _t1b type (runs for rick)
 *
 * ASM 21CA
 */
void
e_them_t1b_action(U8 e)
{
  e_them_t1_action(e, TYPE_1B);
}


/*
 * Action function for e_them _z (zombie) type
 *
 * ASM 23B8
 */
void
e_them_z_action(U8 e)
{
#define offsx c1
  U32 i;

  /* calc new sprite */
  ent_ents[e].sprite = ent_ents[e].sprbase
    + ((ent_ents[e].x & 0x04) ? 0x07 : 0x06);

  /* calc new y */
  i = (ent_ents[e].y << 8) + ent_ents[e].offsy + ent_ents[e].ylow;

  /* deactivate if out of vertical boundaries */
  if (ent_ents[e].y < 0 || ent_ents[e].y > 0x0140) {
    ent_ents[e].n = 0;
    return;
  }

  /* save */
  ent_ents[e].offsy += 0x0080;
  ent_ents[e].ylow = i;
  ent_ents[e].y = i >> 8;

  /* calc new x */
  ent_ents[e].x += ent_ents[e].offsx;

  /* must stay within horizontal boundaries */
  if (ent_ents[e].x < 0)
    ent_ents[e].x = 0;
  if (ent_ents[e].x > 0xe8)
    ent_ents[e].x = 0xe8;
#undef offsx
}


/*
 * Action sub-function for e_them _t2.
 *
 * Must document what it does.
 *
 * ASM 2792
 */
void
e_them_t2_action2(U8 e)
{
#define flgclmb c1
#define offsx c2
  U32 i;
  S16 x, y, yd;
  U8 env0, env1;

  /*
   * vars required by the Black Magic (tm) performance at the
   * end of this function.
   */
  static U16 bx;
  static U8 *bl = (U8 *)&bx;
  static U8 *bh = (U8 *)&bx + 1;
  static U16 cx;
  static U8 *cl = (U8 *)&cx;
  static U8 *ch = (U8 *)&cx + 1;
  static U16 *sl = (U16 *)&e_them_rndseed;
  static U16 *sh = (U16 *)&e_them_rndseed + 2;

  /*sys_printf("e_them_t2 ------------------------------\n");*/

  /* latency: if not zero then decrease */
  if (ent_ents[e].latency > 0) ent_ents[e].latency--;

  /* climbing? */
  if (!ent_ents[e].flgclmb) goto climbing_not;

  /* CLIMBING */

  /*sys_printf("e_them_t2 climbing\n");*/

  /* latency: if not zero then return */
  if (ent_ents[e].latency > 0) return;

  /* calc new sprite */
  ent_ents[e].sprite = ent_ents[e].sprbase + 0x08 +
    (((ent_ents[e].x ^ ent_ents[e].y) & 0x04) ? 1 : 0);

  /* reached rick's level? */
  if ((ent_ents[e].y & 0xfe) != (E_RICK_ENT.y & 0xfe)) goto ymove;

  xmove:
    /* calc new x and test environment */
    ent_ents[e].offsx = (ent_ents[e].x < E_RICK_ENT.x) ? 0x02 : -0x02;
    x = ent_ents[e].x + ent_ents[e].offsx;
    u_envtest(x, ent_ents[e].y, false, &env0, &env1);
    if (env1 & (MAP_EFLG_SOLID|MAP_EFLG_SPAD|MAP_EFLG_WAYUP))
      return;
    if (env1 & MAP_EFLG_LETHAL) {
      e_them_gozombie(e);
      return;
    }
    ent_ents[e].x = x;
    if (env1 & (MAP_EFLG_VERT|MAP_EFLG_CLIMB))  /* still climbing */
      return;
    goto climbing_not;  /* not climbing anymore */

  ymove:
    /* calc new y and test environment */
    yd = ent_ents[e].y < E_RICK_ENT.y ? 0x02 : -0x02;
    y = ent_ents[e].y + yd;
    if (y < 0 || y > 0x0140) {
      ent_ents[e].n = 0;
      return;
    }
    u_envtest(ent_ents[e].x, y, false, &env0, &env1);
    if (env1 & (MAP_EFLG_SOLID|MAP_EFLG_SPAD|MAP_EFLG_WAYUP)) {
      if (yd < 0)
    goto xmove;  /* can't go up */
      else
    goto climbing_not;  /* can't go down */
    }
    /* can move */
    ent_ents[e].y = y;
    if (env1 & (MAP_EFLG_VERT|MAP_EFLG_CLIMB))  /* still climbing */
      return;

    /* NOT CLIMBING */

 climbing_not:
    /*sys_printf("e_them_t2 climbing NOT\n");*/

    ent_ents[e].flgclmb = false;  /* not climbing */

    /* calc new y (falling) and test environment */
    i = (ent_ents[e].y << 8) + ent_ents[e].offsy + ent_ents[e].ylow;
    y = i >> 8;
    u_envtest(ent_ents[e].x, y, false, &env0, &env1);
    if (!(env1 & (MAP_EFLG_SOLID|MAP_EFLG_SPAD|MAP_EFLG_WAYUP))) {
      /*sys_printf("e_them_t2 y move OK\n");*/
      /* can go there */
      if (env1 & MAP_EFLG_LETHAL) {
    e_them_gozombie(e);
    return;
      }
      if (y > 0x0140) {  /* deactivate if outside */
    ent_ents[e].n = 0;
    return;
      }
      if (!(env1 & MAP_EFLG_VERT)) {
    /* save */
    ent_ents[e].y = y;
    ent_ents[e].ylow = i;
    ent_ents[e].offsy += 0x0080;
    if (ent_ents[e].offsy > 0x0800)
      ent_ents[e].offsy = 0x0800;
    return;
      }
      if (((ent_ents[e].x & 0x07) == 0x04) && (y < E_RICK_ENT.y)) {
    /*sys_printf("e_them_t2 climbing00\n");*/
    ent_ents[e].flgclmb = true;  /* climbing */
    return;
      }
    }

    /*sys_printf("e_them_t2 ymove nok or ...\n");*/
    /* can't go there, or ... */
    ent_ents[e].y = (ent_ents[e].y & 0xf8) | 0x03;  /* align to ground */
    ent_ents[e].offsy = 0x0100;
    if (ent_ents[e].latency != 00)
      return;

    if ((env1 & MAP_EFLG_CLIMB) &&
    ((ent_ents[e].x & 0x0e) == 0x04) &&
    (ent_ents[e].y > E_RICK_ENT.y)) {
      /*sys_printf("e_them_t2 climbing01\n");*/
      ent_ents[e].flgclmb = true;  /* climbing */
      return;
    }

    /* calc new sprite */
    ent_ents[e].sprite = ent_ents[e].sprbase +
      ent_sprseq[(ent_ents[e].offsx < 0 ? 4 : 0) +
        ((ent_ents[e].x & 0x0e) >> 3)];
    /*sys_printf("e_them_t2 sprite %02x\n", ent_ents[e].sprite);*/


    /* */
    if (ent_ents[e].offsx == 0)
      ent_ents[e].offsx = 2;
    x = ent_ents[e].x + ent_ents[e].offsx;
    /*sys_printf("e_them_t2 xmove x=%02x\n", x);*/
    if (x < 0xe8) {
      u_envtest(x, ent_ents[e].y, false, &env0, &env1);
      if (!(env1 & (MAP_EFLG_VERT|MAP_EFLG_SOLID|MAP_EFLG_SPAD|MAP_EFLG_WAYUP))) {
    ent_ents[e].x = x;
    if ((x & 0x1e) != 0x08)
      return;

    /*
     * Black Magic (tm)
     *
     * this is obviously some sort of randomizer to define a direction
     * for the entity. it is an exact copy of what the assembler code
     * does but I can't explain.
     */
    bx = e_them_rndnbr + *sh + *sl + 0x0d;
    cx = *sh;
    *bl ^= *ch;
    *bl ^= *cl;
    *bl ^= *bh;
    e_them_rndnbr = bx;

    ent_ents[e].offsx = (*bl & 0x01) ? -0x02 : 0x02;

    /* back to normal */

    return;

      }
    }

    /* U-turn */
    /*sys_printf("e_them_t2 u-turn\n");*/
    if (ent_ents[e].offsx == 0)
      ent_ents[e].offsx = 2;
    else
      ent_ents[e].offsx = -ent_ents[e].offsx;
#undef offsx
}

/*
 * Action function for e_them _t2 type
 *
 * ASM 2718
 */
void
e_them_t2_action(U8 e)
{
  e_them_t2_action2(e);

  /* they kill rick */
  if (e_rick_boxtest(e))
    e_rick_gozombie();

  /* lethal entities kill them */
  if (u_themtest(e)) {
    e_them_gozombie(e);
    return;
  }

  /* bullet kills them */
  if (E_BULLET_ENT.n &&
      u_fboxtest(e, E_BULLET_ENT.x + (e_bullet_offsx < 0 ? 00 : 0x18),
         E_BULLET_ENT.y)) {
    E_BULLET_ENT.n = 0;
    e_them_gozombie(e);
    return;
  }

  /* bomb kills them */
  if (e_bomb_lethal && e_bomb_hit(e)) {
    e_them_gozombie(e);
    return;
  }

  /* rick stops them */
  if (e_rick_state_test(E_RICK_STSTOP) &&
      u_fboxtest(e, e_rick_stop_x, e_rick_stop_y))
    ent_ents[e].latency = 0x14;
}


/*
 * Action sub-function for e_them _t3
 *
 * FIXME always starts asleep??
 *
 * Waits until triggered by something, then execute move steps from
 * ent_mvstep with sprite from ent_sprseq. When done, either restart
 * or disappear.
 *
 * Not always lethal ... but if lethal, kills rick.
 *
 * ASM: 255A
 */
void
e_them_t3_action2(U8 e)
{
#define sproffs c1
#define step_count c2
  U8 i;
  S16 x, y;
  int wav_index;

  while (1) {

    /* calc new sprite */
    i = ent_sprseq[ent_ents[e].sprbase + ent_ents[e].sproffs];
    if (i == 0xff)
      i = ent_sprseq[ent_ents[e].sprbase];
    ent_ents[e].sprite = i;

    if (ent_ents[e].sproffs != 0) {  /* awake */

      /* rotate sprseq */
      if (ent_sprseq[ent_ents[e].sprbase + ent_ents[e].sproffs] != 0xff)
    ent_ents[e].sproffs++;
      if (ent_sprseq[ent_ents[e].sprbase + ent_ents[e].sproffs] == 0xff)
    ent_ents[e].sproffs = 1;

      if (ent_ents[e].step_count < ent_mvstep[ent_ents[e].step_no].count) {
    /*
     * still running this step: try to increment x and y while
     * checking that they remain within boudaries. if so, return.
     * else switch to next step.
     */
    ent_ents[e].step_count++;
    x = ent_ents[e].x + ent_mvstep[ent_ents[e].step_no].dx;

    /* check'n save */
    if (x > 0 && x < 0xe8) {
      ent_ents[e].x = x;
      /*FIXME*/
      /*
      y = ent_mvstep[ent_ents[e].step_no].dy;
      if (y < 0)
        y += 0xff00;
      y += ent_ents[e].y;
      */
      y = ent_ents[e].y + ent_mvstep[ent_ents[e].step_no].dy;
      if (y > 0 && y < 0x0140) {
        ent_ents[e].y = y;
        return;
      }
    }
      }

      /*
       * step is done, or x or y is outside boundaries. try to
       * switch to next step
       */
      ent_ents[e].step_no++;
      if (ent_mvstep[ent_ents[e].step_no].count != 0xff) {
    /* there is a next step: init and loop */
    ent_ents[e].step_count = 0;
      }
      else {
    /* there is no next step: restart or deactivate */
    if (!e_rick_state_test(E_RICK_STZOMBIE) &&
        !(ent_ents[e].flags & ENT_FLG_ONCE)) {
      /* loop this entity */
      ent_ents[e].sproffs = 0;
      ent_ents[e].n &= ~ENT_LETHAL;
      if (ent_ents[e].flags & ENT_FLG_LETHALR)
        ent_ents[e].n |= ENT_LETHAL;
      ent_ents[e].x = ent_ents[e].xsave;
      ent_ents[e].y = ent_ents[e].ysave;
      if (ent_ents[e].y < 0 || ent_ents[e].y > 0x140) {
        ent_ents[e].n = 0;
        return;
      }
    }
    else {
      /* deactivate this entity */
      ent_ents[e].n = 0;
      return;
    }
      }
    }
    else {  /* ent_ents[e].sprseq1 == 0 -- waiting */

        /* ugly GOTOs */

        if (ent_ents[e].flags & ENT_FLG_TRIGRICK) {  /* reacts to rick */
            /* wake up if triggered by rick */
            if (u_trigbox(e, E_RICK_ENT.x + 0x0C, E_RICK_ENT.y + 0x0A))
                goto wakeup;
        }

        if (ent_ents[e].flags & ENT_FLG_TRIGSTOP) {  /* reacts to rick "stop" */
            /* wake up if triggered by rick "stop" */
            if (e_rick_state_test(E_RICK_STSTOP) &&
                u_trigbox(e, e_rick_stop_x, e_rick_stop_y))
                goto wakeup;
        }

        if (ent_ents[e].flags & ENT_FLG_TRIGBULLET) {  /* reacts to bullets */
            /* wake up if triggered by bullet */
            if (E_BULLET_ENT.n && u_trigbox(e, e_bullet_xc, e_bullet_yc)) {
                E_BULLET_ENT.n = 0;
                goto wakeup;
            }
        }

        if (ent_ents[e].flags & ENT_FLG_TRIGBOMB) {  /* reacts to bombs */
            /* wake up if triggered by bomb */
            if (e_bomb_lethal && u_trigbox(e, e_bomb_xc, e_bomb_yc))
                goto wakeup;
        }

        /* not triggered: keep waiting */
        return;

        /* something triggered the entity: wake up */
        /* initialize step counter */
    wakeup:
        if (e_rick_state_test(E_RICK_STZOMBIE))
        {
            return;
        }
#ifdef ENABLE_SOUND
        /*
        * FIXME the sound should come from a table, there are 10 of them
        * but I dont have the table yet. must rip the data off the game...
        * FIXME is it 8 of them, not 10?
        * FIXME testing below...
        */

        /* FIXME this is defensive, need to figure out whether there
                 is simply missing sound (and possibly rip it)
                 or wrong data in sumbmap 47 (when making the switch explode)
                 and submap 13 (when touching jewel) */
        wav_index = (ent_ents[e].trigsnd & 0x1F) - 0x14;
        if((0 <= wav_index) && (wav_index < SOUNDS_NBR_ENTITIES - 1))
        {
            syssnd_play(soundEntity[wav_index], 1);
        }
        /*syssnd_play(WAV_ENTITY[0], 1);*/
#endif /* ENABLE_SOUND */
      ent_ents[e].n &= ~ENT_LETHAL;
      if (ent_ents[e].flags & ENT_FLG_LETHALI)
    ent_ents[e].n |= ENT_LETHAL;
      ent_ents[e].sproffs = 1;
      ent_ents[e].step_count = 0;
      ent_ents[e].step_no = ent_ents[e].step_no_i;
      return;
    }
  }
#undef step_count
}


/*
 * Action function for e_them _t3 type
 *
 * ASM 2546
 */
void
e_them_t3_action(U8 e)
{
  e_them_t3_action2(e);

  /* if lethal, can kill rick */
  if ((ent_ents[e].n & ENT_LETHAL) &&
      !e_rick_state_test(E_RICK_STZOMBIE) && e_rick_boxtest(e)) {  /* CALL 1130 */
    e_rick_gozombie();
  }
}

/* eof */


