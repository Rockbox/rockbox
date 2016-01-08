/*
 * xrick/util.h
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

#ifndef _UTIL_H
#define _UTIL_H

#include "xrick/system/basic_types.h"

extern void u_envtest(S16, S16, bool, U8 *, U8 *);
extern bool u_boxtest(U8, U8);
extern bool u_fboxtest(U8, S16, S16);
extern bool u_trigbox(U8, S16, S16);
extern char * u_strdup(const char *);

#endif /* ndef _UTIL_H */

/* eof */
