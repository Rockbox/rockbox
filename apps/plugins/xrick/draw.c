/*
 * xrick/draw.c
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

/*
 * NOTES
 *
 * This is the only file which accesses the video. Anything calling d_*
 * function should be video-independant.
 *
 * draw.c draws into a 320x200 or 0x0140x0xc8 8-bits depth frame buffer,
 * using the CGA 2 bits color codes. It is up to the video to figure out
 * how to display the frame buffer. Whatever draw.c does, does not show
 * until the screen is explicitely refreshed.
 *
 * The "screen" is the whole 0x0140 by 0x00c8 screen, coordinates go from
 * 0x0000,0x0000 to 0x013f,0x00c7.
 *
 * The "map" is a 0x0100 by 0x0140 rectangle that represents the active
 * game area.
 *
 * Relative to the screen, the "map" is located at 0x0020,-0x0040 : the
 * "map" is composed of two hidden 0x0100 by 0x0040 rectangles (one at the
 * top and one at the bottom) and one visible 0x0100 by 0x00c0 rectangle (in
 * the middle).
 *
 * The "map screen" is the visible rectangle ; it is a 0x0100 by 0xc0
 * rectangle located at 0x0020,0x00.
 *
 * Coordinates can be relative to the screen, the map, or the map screen.
 *
 * Coordinates can be expressed in pixels. When relative to the map or the
 * map screen, they can also be expressed in tiles, the map being composed
 * of rows of 0x20 tiles of 0x08 by 0x08 pixels.
 */

#include "xrick/system/system.h"

#include "xrick/game.h"
#include "xrick/draw.h"

#include "xrick/data/sprites.h"
#include "xrick/data/tiles.h"

#include "xrick/maps.h"
#include "xrick/rects.h"
#include "xrick/data/img.h"


/*
 * counters positions (pixels, screen)
 */
#ifdef GFXPC
#define DRAW_STATUS_SCORE_X 0x28
#define DRAW_STATUS_LIVES_X 0xE8
#define DRAW_STATUS_Y 0x08
#endif
#define DRAW_STATUS_BULLETS_X 0x68
#define DRAW_STATUS_BOMBS_X 0xA8
#ifdef GFXST
#define DRAW_STATUS_SCORE_X 0x20
#define DRAW_STATUS_LIVES_X 0xF0
#define DRAW_STATUS_Y 0
#endif


/*
 * public vars
 */
U8 *draw_tllst;    /* pointer to tiles list */
#ifdef GFXPC
U16 draw_filter;   /* CGA colors filter */
#endif
U8 draw_tilesBank; /* tile number offset */

rect_t draw_STATUSRECT = {
  DRAW_STATUS_SCORE_X, DRAW_STATUS_Y,
  DRAW_STATUS_LIVES_X + 6 * 8 - DRAW_STATUS_SCORE_X, 8,
  NULL
};
const rect_t draw_SCREENRECT = { 0, 0, SYSVID_WIDTH, SYSVID_HEIGHT, NULL };

size_t game_color_count = 0;
img_color_t *game_colors = NULL;

/*
 * private vars
 */
static U8 *fb;     /* frame buffer pointer */


/*
 * Set the frame buffer pointer
 *
 * x, y: position (pixels, screen)
 */
void
draw_setfb(U16 x, U16 y)
{
  fb = sysvid_fb + x + y * SYSVID_WIDTH;
}


/*
 * Clip to map screen
 *
 * x, y: position (pixels, map) CHANGED clipped
 * width, height: dimension CHANGED clipped
 * return: true if fully clipped, false if still (at least partly) visible
 */
bool
draw_clipms(S16 *x, S16 *y, U16 *width, U16 *height)
{
  if (*x < 0) {
    if (*x + *width < 0)
      return true;
    else {
      *width += *x;
      *x = 0;
    }
  }
  else {
    if (*x > 0x0100)
      return true;
    else if (*x + *width > 0x0100) {
      *width = 0x0100 - *x;
    }
  }

  if (*y < DRAW_XYMAP_SCRTOP) {
    if ((*y + *height) < DRAW_XYMAP_SCRTOP)
      return true;
    else {
      *height += *y - DRAW_XYMAP_SCRTOP;
      *y = DRAW_XYMAP_SCRTOP;
    }
  }
  else {
    if (*y >= DRAW_XYMAP_HBTOP)
      return true;
    else if (*y + *height > DRAW_XYMAP_HBTOP)
      *height = DRAW_XYMAP_HBTOP - *y;
  }

  return false;
}


/*
 * Draw a list of tiles onto the frame buffer
 * start at position indicated by fb ; at the end of each (sub)list,
 * perform a "carriage return + line feed" i.e. go back to the initial
 * position then go down one tile row (8 pixels)
 *
 * ASM 1e33
 * fb: CHANGED (see above)
 * draw_tllst: CHANGED points to the element following 0xfe/0xff end code
 */
void
draw_tilesList(void)
{
  U8 *t;

  t = fb;
  while (draw_tilesSubList() != 0xFE) {  /* draw sub-list */
    t += 8 * SYSVID_WIDTH;  /* go down one tile i.e. 8 lines */
    fb = t;
  }
}


/*
 * Draw a list of tiles onto the frame buffer -- same as draw_tilesList,
 * but accept an immediate string as parameter. Note that the string needs
 * to be properly terminated with 0xfe (\376) and 0xff (\377) chars.
 */
void
draw_tilesListImm(U8 *list)
{
  draw_tllst = list;
  draw_tilesList();
}


/*
 * Draw a sub-list of tiles onto the frame buffer
 * start at position indicated by fb ; leave fb pointing to the next
 * tile to the right of the last tile drawn
 *
 * ASM 1e41
 * fpb: CHANGED (see above)
 * draw_tllst: CHANGED points to the element following 0xfe/0xff end code
 * returns: end code (0xfe : end of list ; 0xff : end of sub-list)
 */
U8
draw_tilesSubList()
{
  U8 i;

  i = *(draw_tllst++);
  while (i != 0xFF && i != 0xFE) {  /* while not end */
    draw_tile(i);  /* draw tile */
    i = *(draw_tllst++);
  }
  return i;
}


/*
 * Draw a tile
 * at position indicated by fb ; leave fb pointing to the next tile
 * to the right of the tile drawn
 *
 * ASM 1e6c
 * tlnbr: tile number
 * draw_filter: CGA colors filter
 * fb: CHANGED (see above)
 */
void
draw_tile(U8 tileNumber)
{
  U8 i, k, *f;

#ifdef GFXPC
  U16 x;
#endif

#ifdef GFXST
  U32 x;
#endif

  f = fb;  /* frame buffer */
  for (i = 0; i < TILES_NBR_LINES; i++) {  /* for all 8 pixel lines */

#ifdef GFXPC
    x = tiles_data[draw_tilesBank * TILES_NBR_TILES + tileNumber][i] & draw_filter;
    /*
     * tiles / perform the transformation from CGA 2 bits
     * per pixel to frame buffer 8 bits per pixels
     */
    for (k = 8; k--; x >>= 2)
      f[k] = x & 3;
    f += SYSVID_WIDTH;  /* next line */
#endif

#ifdef GFXST
  x = tiles_data[draw_tilesBank * TILES_NBR_TILES + tileNumber][i];
  /*
   * tiles / perform the transformation from ST 4 bits
   * per pixel to frame buffer 8 bits per pixels
   */
  for (k = 8; k--; x >>= 4)
    f[k] = x & 0x0F;
  f += SYSVID_WIDTH;  /* next line */
#endif

  }

  fb += 8;  /* next tile */
}

/*
 * Draw a sprite
 *
 * ASM 1a09
 * nbr: sprite number
 * x, y: sprite position (pixels, screen)
 * fb: CHANGED
 */
#ifdef GFXPC
void
draw_sprite(U8 nbr, U16 x, U16 y)
{
    U8 i, j, k, *f;
    U16 xm = 0, xp = 0;

    draw_setfb(x, y);

    for (i = 0; i < SPRITES_NBR_COLS; i++) {  /* for each tile column */
        f = fb;  /* frame buffer */
        for (j = 0; j < SPRITES_NBR_ROWS; j++) {  /* for each pixel row */
            xm = sprites_data[nbr][i][j].mask;  /* mask */
            xp = sprites_data[nbr][i][j].pict;  /* picture */
            /*
            * sprites / perform the transformation from CGA 2 bits
            * per pixel to frame buffer 8 bits per pixels
            */
            for (k = 8; k--; xm >>= 2, xp >>= 2)
                f[k] = (f[k] & (xm & 3)) | (xp & 3);
            f += SYSVID_WIDTH;
        }
        fb += 8;
    }
}
#endif


/*
 * Draw a sprite
 *
 * foobar
 */
#ifdef GFXST
void
draw_sprite(U8 number, U16 x, U16 y)
{
    U8 i, j, k, *f;
    U16 g;
    U32 d;

    draw_setfb(x, y);
    g = 0;
    for (i = 0; i < SPRITES_NBR_ROWS; i++) { /* rows */
        f = fb;
        for (j = 0; j < SPRITES_NBR_COLS; j++) { /* cols */
            d = sprites_data[number][g++];
            for (k = 8; k--; d >>= 4)
                if (d & 0x0F) f[k] = (f[k] & 0xF0) | (d & 0x0F);
            f += 8;
        }
        fb += SYSVID_WIDTH;
    }
}
#endif


/*
 * Draw a sprite
 *
 * NOTE re-using original ST graphics format
 */
#ifdef GFXST
void
draw_sprite2(U8 number, U16 x, U16 y, bool front)
{
  U32 d = 0;   /* sprite data */
  S16 x0, y0;  /* clipped x, y */
  U16 w, h;    /* width, height */
  S16 g,       /* sprite data offset*/
    r, c,      /* row, column */
    i,         /* frame buffer shifter */
    im;        /* tile flag shifter */
  U8 flg;      /* tile flag */

  x0 = x;
  y0 = y;
  w = SPRITES_NBR_COLS * 8; /* each tile column is 8 pixels */
  h = SPRITES_NBR_ROWS;

  if (draw_clipms(&x0, &y0, &w, &h))  /* return if not visible */
    return;

  g = 0;
  draw_setfb(x0 - DRAW_XYMAP_SCRLEFT, y0 - DRAW_XYMAP_SCRTOP + 8);

  for (r = 0; r < SPRITES_NBR_ROWS; r++) {
    if (r >= h || y + r < y0) continue;

    i = 0x1f;
    im = x - (x & 0xfff8);
    flg = map_eflg[map_map[(y + r) >> 3][(x + 0x1f)>> 3]];

#ifdef ENABLE_CHEATS
#define LOOP(N, C0, C1) \
    d = sprites_data[number][g + N]; \
    for (c = C0; c >= C1; c--, i--, d >>= 4, im--) { \
      if (im == 0) { \
    flg = map_eflg[map_map[(y + r) >> 3][(x + c) >> 3]]; \
    im = 8; \
      } \
      if (c >= w || x + c < x0) continue; \
      if (!front && !game_cheat3 && (flg & MAP_EFLG_FGND)) continue; \
      if (d & 0x0F) fb[i] = (fb[i] & 0xF0) | (d & 0x0F); \
      if (game_cheat3) fb[i] |= 0x10; \
    }
#else
#define LOOP(N, C0, C1) \
    d = sprites_data[number][g + N]; \
    for (c = C0; c >= C1; c--, i--, d >>= 4, im--) { \
      if (im == 0) { \
    flg = map_eflg[map_map[(y + r) >> 3][(x + c) >> 3]]; \
    im = 8; \
      } \
      if (!front && (flg & MAP_EFLG_FGND)) continue; \
      if (c >= w || x + c < x0) continue; \
      if (d & 0x0F) fb[i] = (fb[i] & 0xF0) | (d & 0x0F); \
    }
#endif
    LOOP(3, 0x1f, 0x18);
    LOOP(2, 0x17, 0x10);
    LOOP(1, 0x0f, 0x08);
    LOOP(0, 0x07, 0x00);

#undef LOOP

    fb += SYSVID_WIDTH;
    g += SPRITES_NBR_COLS;
  }
}

#endif


/*
 * Draw a sprite
 * align to tile column, determine plane automatically, and clip
 *
 * nbr: sprite number
 * x, y: sprite position (pixels, map).
 * fb: CHANGED
 */
#ifdef GFXPC
void
draw_sprite2(U8 number, U16 x, U16 y, bool front)
{
  U8 k, *f, c, r, dx;
  U16 cmax, rmax;
  U16 xm = 0, xp = 0;
  S16 xmap, ymap;

  /* align to tile column, prepare map coordinate and clip */
  xmap = x & 0xFFF8;
  ymap = y;
  cmax = SPRITES_NBR_COLS * 8;  /* width, 4 tile columns, 8 pixels each */
  rmax = SPRITES_NBR_ROWS;  /* height, 15 pixels */
  dx = (x - xmap) * 2;
  if (draw_clipms(&xmap, &ymap, &cmax, &rmax))  /* return if not visible */
    return;

  /* get back to screen */
  draw_setfb(xmap - DRAW_XYMAP_SCRLEFT, ymap - DRAW_XYMAP_SCRTOP);
  xmap >>= 3;
  cmax >>= 3;

  /* draw */
  for (c = 0; c < cmax; c++) {  /* for each tile column */
    f = fb;
    for (r = 0; r < rmax; r++) {  /* for each pixel row */
      /* check that tile is not hidden behind foreground */
#ifdef ENABLE_CHEATS
      if (front || game_cheat3 ||
      !(map_eflg[map_map[(ymap + r) >> 3][xmap + c]] & MAP_EFLG_FGND)) {
#else
      if (front ||
      !(map_eflg[map_map[(ymap + r) >> 3][xmap + c]] & MAP_EFLG_FGND)) {
#endif
    xp = xm = 0;
    if (c > 0) {
      xm |= sprites_data[number][c - 1][r].mask << (16 - dx);
      xp |= sprites_data[number][c - 1][r].pict << (16 - dx);
    }
    else
      xm |= 0xFFFF << (16 - dx);
    if (c < cmax) {
      xm |= sprites_data[number][c][r].mask >> dx;
      xp |= sprites_data[number][c][r].pict >> dx;
    }
    else
      xm |= 0xFFFF >> dx;
    /*
     * sprites / perform the transformation from CGA 2 bits
     * per pixel to frame buffer 8 bits per pixels
     */
    for (k = 8; k--; xm >>= 2, xp >>= 2) {
      f[k] = ((f[k] & (xm & 3)) | (xp & 3));
#ifdef ENABLE_CHEATS
      if (game_cheat3) f[k] |= 4;
#endif
    }
      }
      f += SYSVID_WIDTH;
    }
    fb += 8;
  }
}
#endif


/*
 * Redraw the map behind a sprite
 * align to tile column and row, and clip
 *
 * x, y: sprite position (pixels, map).
 */
void
draw_spriteBackground(U16 x, U16 y)
{
  U8 r, c;
  U16 rmax, cmax;
  S16 xmap, ymap;
  U16 xs, ys;

  /* aligne to column and row, prepare map coordinate, and clip */
  xmap = x & 0xFFF8;
  ymap = y & 0xFFF8;
  cmax = (x - xmap == 0 ? 0x20 : 0x28);  /* width, 4 tl cols, 8 pix each */
  rmax = (y & 0x04) ? 0x20 : 0x18;  /* height, 3 or 4 tile rows */
  if (draw_clipms(&xmap, &ymap, &cmax, &rmax))  /* don't draw if fully clipped */
    return;

  /* get back to screen */
  xs = xmap - DRAW_XYMAP_SCRLEFT;
  ys = ymap - DRAW_XYMAP_SCRTOP;
  xmap >>= 3;
  ymap >>= 3;
  cmax >>= 3;
  rmax >>= 3;

  /* draw */
  for (r = 0; r < rmax; r++) {  /* for each row */
#ifdef GFXPC
    draw_setfb(xs, ys + r * 8);
#endif
#ifdef GFXST
    draw_setfb(xs, 8 + ys + r * 8);
#endif
    for (c = 0; c < cmax; c++) {  /* for each column */
      draw_tile(map_map[ymap + r][xmap + c]);
    }
  }
}


/*
 * Draw entire map screen background tiles onto frame buffer.
 *
 * ASM 0af5, 0a54
 */
void
draw_map(void)
{
    U8 i, j;

    draw_tilesBank = map_tilesBank;

    for (i = 0; i < 0x18; i++) /* 0x18 rows */
    {
#ifdef GFXPC
        draw_setfb(-DRAW_XYMAP_SCRLEFT, (i * 8));
#endif
#ifdef GFXST
        draw_setfb(-DRAW_XYMAP_SCRLEFT, 8 + (i * 8));
#endif
        for (j = 0; j < 0x20; j++)  /* 0x20 tiles per row */
        {
            draw_tile(map_map[i + 8][j]);
        }
    }
}


/*
 * Draw status indicators
 *
 * ASM 0309
 */
void
draw_drawStatus(void)
{
  S8 i;
  U32 sv;
  static U8 s[7] = {0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0xfe};

  draw_tilesBank = 0;

  for (i = 5, sv = game_score; i >= 0; i--) {
    s[i] = 0x30 + (U8)(sv % 10);
    sv /= 10;
  }
  draw_tllst = s;

  draw_setfb(DRAW_STATUS_SCORE_X, DRAW_STATUS_Y);
  draw_tilesList();

  draw_setfb(DRAW_STATUS_BULLETS_X, DRAW_STATUS_Y);
  for (i = 0; i < game_bullets; i++)
    draw_tile(TILES_BULLET);

  draw_setfb(DRAW_STATUS_BOMBS_X, DRAW_STATUS_Y);
  for (i = 0; i < game_bombs; i++)
    draw_tile(TILES_BOMB);

  draw_setfb(DRAW_STATUS_LIVES_X, DRAW_STATUS_Y);
  for (i = 0; i < game_lives; i++)
    draw_tile(TILES_RICK);
}


/*
 * Draw info indicators
 */
#ifdef ENABLE_CHEATS
void
draw_infos(void)
{
  draw_tilesBank = 0;

#ifdef GFXPC
  draw_filter = 0xffff;
#endif

  draw_setfb(0x00, DRAW_STATUS_Y);
  draw_tile(game_cheat1 ? 'T' : '@');
  draw_setfb(0x08, DRAW_STATUS_Y);
  draw_tile(game_cheat2 ? 'N' : '@');
  draw_setfb(0x10, DRAW_STATUS_Y);
  draw_tile(game_cheat3 ? 'V' : '@');
}
#endif


/*
 * Clear status indicators
 */
void
draw_clearStatus(void)
{
  U8 i;

#ifdef GFXPC
  draw_tilesBank = map_tilesBank;
#endif
#ifdef GFXST
  draw_tilesBank = 0;
#endif
  draw_setfb(DRAW_STATUS_SCORE_X, DRAW_STATUS_Y);
  for (i = 0; i < DRAW_STATUS_LIVES_X/8 + 6 - DRAW_STATUS_SCORE_X/8; i++) {
#ifdef GFXPC
    draw_tile(map_map[MAP_ROW_SCRTOP + (DRAW_STATUS_Y / 8)][i]);
#endif
#ifdef GFXST
    draw_tile('@');
#endif
  }
}

/*
 * Draw a picture
 */
#ifdef GFXST
void
draw_pic(const pic_t * picture)
{
    U8 *f;
    U16 i, j, k, pp;
    U32 v;

    draw_setfb(picture->xPos, picture->yPos);
    pp = 0;

    for (i = 0; i < picture->height; i++) { /* rows */
        f = fb;
        for (j = 0; j < picture->width; j += 8) {  /* cols */
            v = picture->pixels[pp++];
            for (k = 8; k--; v >>= 4)
                f[k] = v & 0x0F;
            f += 8;
        }
        fb += SYSVID_WIDTH;
    }
}
#endif


/*
 * Draw a bitmap
 */
void
draw_img(img_t *image)
{
    U8 *f;
    U16 i, j, pp;

    sysvid_setPalette(image->colors, image->ncolors);

    draw_setfb(image->xPos, image->yPos);
    pp = 0;

    for (i = 0; i < image->height; i++) /* rows */
    {
        f = fb;
        for (j = 0; j < image->width; j++) /* cols */
        {
            f[j] = image->pixels[pp++];
        }
        fb += SYSVID_WIDTH;
    }
}


/* eof */
