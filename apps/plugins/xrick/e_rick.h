/*
 * xrick/e_rick.h
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

#ifndef _E_RICK_H
#define _E_RICK_H

#include "xrick/system/basic_types.h"

#define E_RICK_NO 1
#define E_RICK_ENT ent_ents[E_RICK_NO]

typedef enum
{
    E_RICK_STSTOP = (1 << 0),
    E_RICK_STSHOOT = (1 << 1),
    E_RICK_STCLIMB = (1 << 2),
    E_RICK_STJUMP = (1 << 3),
    E_RICK_STZOMBIE = (1 << 4),
    E_RICK_STDEAD = (1 << 5),
    E_RICK_STCRAWL = (1 << 6),
} e_rick_state_t;

extern unsigned e_rick_state;
inline void e_rick_state_set(e_rick_state_t s) { e_rick_state |= s; }
inline void e_rick_state_clear(e_rick_state_t s) { e_rick_state &= ~s; }
inline bool e_rick_state_test(e_rick_state_t s) { return e_rick_state & s; }

extern S16 e_rick_stop_x, e_rick_stop_y;

extern void e_rick_save(void);
extern void e_rick_restore(void);
extern void e_rick_action(U8);
extern void e_rick_gozombie(void);
extern bool e_rick_boxtest(U8);

#endif /* ndef _E_RICK_H */

/* eof */


