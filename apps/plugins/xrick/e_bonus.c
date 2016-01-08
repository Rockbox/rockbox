/*
 * xrick/e_bonus.c
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

#include "xrick/e_bonus.h"

#include "xrick/game.h"
#include "xrick/ents.h"

#include "xrick/e_rick.h"
#include "xrick/maps.h"


/*
 * Entity action
 *
 * ASM 242C
 */
void
e_bonus_action(U8 e)
{
#define seq c1

  if (ent_ents[e].seq == 0) {
    if (e_rick_boxtest(e)) {
      game_score += 500;
#ifdef ENABLE_SOUND
      syssnd_play(soundBonus, 1);
#endif
      map_marks[ent_ents[e].mark].ent |= MAP_MARK_NACT;
      ent_ents[e].seq = 1;
      ent_ents[e].sprite = 0xad;
      ent_ents[e].front = true;
      ent_ents[e].y -= 0x08;
    }
  }

  else if (ent_ents[e].seq > 0 && ent_ents[e].seq < 10) {
    ent_ents[e].seq++;
    ent_ents[e].y -= 2;
  }

  else {
    ent_ents[e].n = 0;
  }
}


/* eof */


