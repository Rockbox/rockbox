/*
 * xrick/scroller.h
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

#ifndef _SCROLLER_H
#define _SCROLLER_H

#define SCROLL_RUNNING 1
#define SCROLL_DONE 0

#define SCROLL_PERIOD 24

#include "xrick/system/basic_types.h"

extern U8 scroll_up(void);
extern U8 scroll_down(void);

#endif /* ndef _SCROLLER_H */

/* eof */


