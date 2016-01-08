/*
 * xrick/e_sbonus.h
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

#ifndef _E_SBONUS_H
#define _E_SBONUS_H

#include "xrick/system/basic_types.h"

extern bool e_sbonus_counting;
extern U8 e_sbonus_counter;
extern U16 e_sbonus_bonus;

extern void e_sbonus_start(U8);
extern void e_sbonus_stop(U8);

#endif /* ndef _E_SBONUS_H */

/* eof */
