/*
 * xrick/rects.h
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

#ifndef _RECTS_H
#define _RECTS_H

#include "xrick/system/basic_types.h"

typedef struct rect_s {
  U16 x, y;
  U16 width, height;
  struct rect_s *next;
} rect_t;

extern void rects_free(rect_t *);
extern rect_t *rects_new(U16, U16, U16, U16, rect_t *);

#endif /* ndef _RECTS_H */

/* eof */
