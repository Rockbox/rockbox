/*
 * xrick/data/img.h
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

#ifndef _IMG_H
#define _IMG_H

#include "xrick/system/basic_types.h"

typedef struct {
  U8 r, g, b, nothing;
} img_color_t;

typedef struct {
  U16 width;
  U16 height;
  U16 xPos;
  U16 yPos;
  U16 ncolors;
  img_color_t *colors;
  U8 *pixels;
} img_t;

extern img_t *img_splash;

#endif /* ndef _IMG_H */

/* eof */
