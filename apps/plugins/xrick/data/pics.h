/*
 * xrick/data/pics.h
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

#ifndef _PICS_H
#define _PICS_H

#include "xrick/config.h"

#ifdef GFXST

#include "xrick/system/basic_types.h"

typedef struct {
  U16 width;
  U16 height;
  U16 xPos;
  U16 yPos;
  U32 *pixels;
} pic_t;

extern pic_t *pic_haf;
extern pic_t *pic_congrats;
extern pic_t *pic_splash;

#endif /* GFXST */

#endif /* ndef _PICS_H */

/* eof */


