/*
 * xrick/util.c
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

#include "xrick/util.h"

#include "xrick/config.h"
#include "xrick/game.h"
#include "xrick/ents.h"
#include "xrick/e_rick.h"
#include "xrick/maps.h"
#include "xrick/system/system.h"

#include <string.h> /* memcpy */

/*
 * Full box test.
 *
 * ASM 1199
 *
 * e: entity to test against.
 * x,y: coordinates to test.
 * ret: true/(x,y) is within e's space, false/not.
 */
bool
u_fboxtest(U8 e, S16 x, S16 y)
{
  if (ent_ents[e].x >= x ||
      ent_ents[e].x + ent_ents[e].w < x ||
      ent_ents[e].y >= y ||
      ent_ents[e].y + ent_ents[e].h < y)
    return false;
  else
    return true;
}




/*
 * Box test (then whole e2 is checked agains the center of e1).
 *
 * ASM 113E
 *
 * e1: entity to test against (corresponds to DI in asm code).
 * e2: entity to test (corresponds to SI in asm code).
 * ret: true/intersect, false/not.
 */
bool
u_boxtest(U8 e1, U8 e2)
{
  /* rick is special (may be crawling) */
  if (e1 == E_RICK_NO)
    return e_rick_boxtest(e2);

  /*
   * entity 1: x+0x05 to x+0x011, y to y+0x14
   * entity 2: x to x+ .w, y to y+ .h
   */
  if (ent_ents[e1].x + 0x11 < ent_ents[e2].x ||
      ent_ents[e1].x + 0x05 > ent_ents[e2].x + ent_ents[e2].w ||
      ent_ents[e1].y + 0x14 < ent_ents[e2].y ||
      ent_ents[e1].y > ent_ents[e2].y + ent_ents[e2].h - 1)
    return false;
  else
    return true;
}


/*
 * Compute the environment flag.
 *
 * ASM 0FBC if !crawl, else 103E
 *
 * x, y: coordinates where to compute the environment flag
 * crawl: is rick crawling?
 * rc0: anything CHANGED to the environment flag for crawling (6DBA)
 * rc1: anything CHANGED to the environment flag (6DAD)
 */
void
u_envtest(S16 x, S16 y, bool crawl, U8 *rc0, U8 *rc1)
{
  U8 i, xx;

  /* prepare for ent #0 test */
  ent_ents[ENT_ENTSNUM].x = x;
  ent_ents[ENT_ENTSNUM].y = y;

  i = 1;
  if (!crawl) i++;
  if (y & 0x0004) i++;

  x += 4;
  xx = (U8)x; /* FIXME? */

  x = x >> 3;  /* from pixels to tiles */
  y = y >> 3;  /* from pixels to tiles */

  *rc0 = *rc1 = 0;

  if (xx & 0x07) {  /* tiles columns alignment */
    if (crawl) {
      *rc0 |= (map_eflg[map_map[y][x]] &
       (MAP_EFLG_VERT|MAP_EFLG_SOLID|MAP_EFLG_SPAD|MAP_EFLG_WAYUP));
      *rc0 |= (map_eflg[map_map[y][x + 1]] &
       (MAP_EFLG_VERT|MAP_EFLG_SOLID|MAP_EFLG_SPAD|MAP_EFLG_WAYUP));
      *rc0 |= (map_eflg[map_map[y][x + 2]] &
       (MAP_EFLG_VERT|MAP_EFLG_SOLID|MAP_EFLG_SPAD|MAP_EFLG_WAYUP));
      y++;
    }
    do {
      *rc1 |= (map_eflg[map_map[y][x]] &
           (MAP_EFLG_SOLID|MAP_EFLG_SPAD|MAP_EFLG_FGND|
        MAP_EFLG_LETHAL|MAP_EFLG_01));
      *rc1 |= (map_eflg[map_map[y][x + 1]] &
           (MAP_EFLG_SOLID|MAP_EFLG_SPAD|MAP_EFLG_FGND|
        MAP_EFLG_LETHAL|MAP_EFLG_CLIMB|MAP_EFLG_01));
      *rc1 |= (map_eflg[map_map[y][x + 2]] &
           (MAP_EFLG_SOLID|MAP_EFLG_SPAD|MAP_EFLG_FGND|
        MAP_EFLG_LETHAL|MAP_EFLG_01));
      y++;
    } while (--i > 0);

    *rc1 |= (map_eflg[map_map[y][x]] &
         (MAP_EFLG_SOLID|MAP_EFLG_SPAD|MAP_EFLG_WAYUP|MAP_EFLG_FGND|
          MAP_EFLG_LETHAL|MAP_EFLG_01));
    *rc1 |= (map_eflg[map_map[y][x + 1]]);
    *rc1 |= (map_eflg[map_map[y][x + 2]] &
         (MAP_EFLG_SOLID|MAP_EFLG_SPAD|MAP_EFLG_WAYUP|MAP_EFLG_FGND|
          MAP_EFLG_LETHAL|MAP_EFLG_01));
  }
  else {
    if (crawl) {
      *rc0 |= (map_eflg[map_map[y][x]] &
       (MAP_EFLG_VERT|MAP_EFLG_SOLID|MAP_EFLG_SPAD|MAP_EFLG_WAYUP));
      *rc0 |= (map_eflg[map_map[y][x + 1]] &
       (MAP_EFLG_VERT|MAP_EFLG_SOLID|MAP_EFLG_SPAD|MAP_EFLG_WAYUP));
      y++;
    }
    do {
      *rc1 |= (map_eflg[map_map[y][x]] &
           (MAP_EFLG_SOLID|MAP_EFLG_SPAD|MAP_EFLG_FGND|
        MAP_EFLG_LETHAL|MAP_EFLG_CLIMB|MAP_EFLG_01));
      *rc1 |= (map_eflg[map_map[y][x + 1]] &
           (MAP_EFLG_SOLID|MAP_EFLG_SPAD|MAP_EFLG_FGND|
        MAP_EFLG_LETHAL|MAP_EFLG_CLIMB|MAP_EFLG_01));
      y++;
    } while (--i > 0);

    *rc1 |= (map_eflg[map_map[y][x]]);
    *rc1 |= (map_eflg[map_map[y][x + 1]]);
  }

  /*
   * If not lethal yet, and there's an entity on slot zero, and (x,y)
   * boxtests this entity, then raise SOLID flag. This is how we make
   * sure that no entity can move over the entity that is on slot zero.
   *
   * Beware! When game_cheat2 is set, this means that a block can
   * move over rick without killing him -- but then rick is trapped
   * because the block is solid.
   */
  if (!(*rc1 & MAP_EFLG_LETHAL)
      && ent_ents[0].n
      && u_boxtest(ENT_ENTSNUM, 0)) {
    *rc1 |= MAP_EFLG_SOLID;
  }

  /* When game_cheat2 is set, the environment can not be lethal. */
#ifdef ENABLE_CHEATS
  if (game_cheat2) *rc1 &= ~MAP_EFLG_LETHAL;
#endif
}


/*
 * Check if x,y is within e trigger box.
 *
 * ASM 126F
 * return: false if not in box, true if in box.
 */
bool
u_trigbox(U8 e, S16 x, S16 y)
{
  U16 xmax, ymax;

  xmax = ent_ents[e].trig_x + (ent_entdata[ent_ents[e].n & 0x7F].trig_w << 3);
  ymax = ent_ents[e].trig_y + (ent_entdata[ent_ents[e].n & 0x7F].trig_h << 3);

  if (xmax > 0xFF) xmax = 0xFF;

  if (x <= ent_ents[e].trig_x || x > xmax ||
      y <= ent_ents[e].trig_y || y > ymax)
    return false;
  else
    return true;
}

/*
 * Custom implementation of strdup function
 */
char *
u_strdup(const char *sourceStr)
{
    char *destStr;
    size_t length;

    length = sys_strlen(sourceStr) + 1;
    destStr = sysmem_push(length);
    if (!destStr)
    {
        return NULL;
    }
    memcpy(destStr, sourceStr, length);
    return destStr;
}


/* eof */
