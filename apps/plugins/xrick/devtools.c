/*
 * xrick/devtools.c
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

#include "xrick/config.h"

#ifdef ENABLE_DEVTOOLS

#include "xrick/devtools.h"
#include "xrick/game.h"
#include "xrick/control.h"
#include "xrick/screens.h"
#include "xrick/draw.h"
#include "xrick/data/sprites.h"
#include "xrick/maps.h"
#include "xrick/system/system.h"

/*
 * DevTools
 */

U8
devtools_run(void)
{
  static U8 seq = 0;
  static U8 pos = 0;
  static U8 pos2 = 0;
  U8 i, j, k, l;
  U8 s[128];

  if (seq == 0) {
    sysvid_clear();
    game_rects = &draw_SCREENRECT;
#ifdef GFXPC
    draw_filter = 0xffff;
#endif
    seq = 1;
  }

  switch (seq) {
  case 1:  /* draw tiles */
    sysvid_clear();
    draw_tilesBank = 0;
    sys_snprintf(s, sizeof(s), "TILES@BANK@%d\376", pos);
    draw_setfb(4, 4);
    draw_tilesListImm(s);
    k = 0;
    for (i = 0; i < 0x10; i++) {
      draw_setfb(80 + i * 0x0a, 14);
      draw_tile((i<10?0x30:'A'-10) + i);
      draw_setfb(64, 30 + i * 0x0a);
      draw_tile((i<10?0x30:'A'-10) + i);
    }
    draw_tilesBank = pos;
    for (i = 0; i < 0x10; i++)
      for (j = 0; j < 0x10; j++) {
    draw_setfb(80 + j * 0x0a, 30 + i * 0x0a);
    draw_tile(k++);
      }
    seq = 10;
    break;
  case 10:  /* wait for key pressed */
    if (control_test(Control_FIRE))
      seq = 98;
    if (control_test(Control_UP))
      seq = 12;
    if (control_test(Control_DOWN))
      seq = 13;
    if (control_test(Control_RIGHT))
      seq = 11;
    break;
  case 11:  /* wait for key released */
    if (!(control_test(Control_RIGHT))) {
      pos = 0;
      seq = 21;
    }
    break;
  case 12:  /* wait for key released */
    if (!(control_test(Control_UP))) {
      if (pos < 4) pos++;
      seq = 1;
    }
    break;
  case 13:  /* wait for key released */
    if (!(control_test(Control_DOWN))) {
      if (pos > 0) pos--;
      seq = 1;
    }
    break;
  case 21:  /* draw sprites */
    sysvid_clear();
    draw_tilesBank = 0;
    sys_snprintf(s, sizeof(s), "SPRITES\376");
    draw_setfb(4, 4);
    draw_tilesListImm(s);
    for (i = 0; i < 8; i++) {
      draw_setfb(0x08 + 0x20 + i * 0x20, 0x30 - 26);
      draw_tile((i<10?0x30:'A'-10) + i);
      draw_setfb(0x08 + 0x20 + i * 0x20, 0x30 - 16);
      draw_tile((i+8<10?0x30:'A'-10) + i+8);
    }
    for (i = 0; i < 4; i++) {
      k = pos + i * 8;
      draw_setfb(0x20 - 16, 0x08 + 0x30 + i * 0x20);
      j = k%16;
      k /= 16;
      draw_tile((j<10?0x30:'A'-10) + j);
      draw_setfb(0x20 - 26, 0x08 + 0x30 + i * 0x20);
      j = k%16;
      draw_tile((j<10?0x30:'A'-10) + j);
    }
    k = pos;
    for (i = 0; i < 4; i++)
      for (j = 0; j < 8; j++) {
      draw_sprite(k++, 0x20 + j * 0x20, 0x30 + i * 0x20);
      }
    seq = 30;
    break;
  case 30:  /* wait for key pressed */
    if (control_test(Control_FIRE))
      seq = 98;
    if (control_test(Control_UP))
      seq = 32;
    if (control_test(Control_DOWN))
      seq = 33;
    if (control_test(Control_LEFT))
      seq = 31;
    if (control_test(Control_RIGHT))
      seq = 40;
    break;
  case 31:  /* wait for key released */
    if (!(control_test(Control_LEFT))) {
      pos = 0;
      seq = 1;
    }
    break;
  case 32:  /* wait for key released */
    if (!(control_test(Control_UP))) {
      if (pos < sprites_nbr_sprites - 32) pos += 32;
      seq = 21;
    }
    break;
  case 33:  /* wait for key released */
    if (!(control_test(Control_DOWN))) {
      if (pos > 0) pos -= 32;
      seq = 21;
    }
    break;
  case 40:
    sysvid_clear();
#ifdef GFXPC
    if (pos2 == 0) pos2 = 2;
#endif
#ifdef GFXST
    if (pos2 == 0) pos2 = 1;
#endif
    sys_snprintf(s, sizeof(s), "BLOCKS@%#04X@TO@%#04X@WITH@BANK@%d\376",
        pos, pos + 4*8-1, pos2);
    draw_setfb(4, 4);
    draw_tilesBank = 0;
    draw_tilesListImm(s);
    draw_tilesBank = pos2;
    for (l = 0; l < 8; l++)
      for (k = 0; k < 4; k++)
    for (i = 0; i < 4; i++)
      for (j = 0; j < 4; j++) {
        draw_setfb(20 + j * 8 + l * 36, 30 + i * 8 + k * 36);
        draw_tile(map_blocks[pos + l + k * 8][i * 4 + j]);
      }
    seq = 41;
    break;
  case 41:
    if (control_test(Control_FIRE))
      seq = 98;
    if (control_test(Control_UP))
      seq = 42;
    if (control_test(Control_DOWN))
      seq = 43;
    if (control_test(Control_LEFT))
      seq = 44;
    if (control_test(Control_PAUSE))
      seq = 45;
    break;
  case 42:
    if (!(control_test(Control_UP))) {
      if (pos < map_nbr_blocks - 8*4) pos += 8 * 4;
      seq = 40;
    }
    break;
  case 43:
    if (!(control_test(Control_DOWN))) {
      if (pos > 0) pos -= 8 * 4;
      seq = 40;
    }
    break;
  case 44:
    if (!(control_test(Control_LEFT))) {
      pos = 0;
      pos2 = 0;
      seq = 21;
    }
    break;
  case 45:
    if (!(control_test(Control_PAUSE))) {
#ifdef GFXPC
      if (pos2 == 2) pos2 = 3;
      else pos2 = 2;
#endif
#ifdef GFXST
      if (pos2 == 1) pos2 = 2;
      else pos2 = 1;
#endif
      seq = 40;
    }
    break;
  case 98:  /* wait for key released */
    if (!(control_test(Control_FIRE)))
      seq = 99;
    break;
  }

  if (control_test(Control_EXIT))  /* check for exit request */
    return SCREEN_EXIT;

  if (seq == 99) {  /* we're done */
    sysvid_clear();
    seq = 0;
    return SCREEN_DONE;
  }

  return SCREEN_RUNNING;
}

#endif /* ENABLE_DEVTOOLS */

/* eof */

