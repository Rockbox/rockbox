/*
 * xrick/e_bullet.h
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

#ifndef _E_BULLET_H
#define _E_BULLET_H

#include "xrick/system/basic_types.h"

#define E_BULLET_NO 2
#define E_BULLET_ENT ent_ents[E_BULLET_NO]

extern S8 e_bullet_offsx;
extern S16 e_bullet_xc, e_bullet_yc;

extern void e_bullet_init(U16, U16);
extern void e_bullet_action(U8);

#endif /* ndef _E_BULLET_H */

/* eof */
