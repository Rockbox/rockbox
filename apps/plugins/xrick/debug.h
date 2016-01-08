/*
 * xrick/debug.h
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

#ifndef _DEBUG_H
#define _DEBUG_H

#include "xrick/config.h"

/* define IFDEBUG macros */
#ifdef DEBUG_MEMORY
#define IFDEBUG_MEMORY(X); X
#else
#define IFDEBUG_MEMORY(X);
#endif

#ifdef DEBUG_ENTS
#define IFDEBUG_ENTS(X); X
#else
#define IFDEBUG_ENTS(X);
#endif

#ifdef DEBUG_SCROLLER
#define IFDEBUG_SCROLLER(X); X
#else
#define IFDEBUG_SCROLLER(X);
#endif

#ifdef DEBUG_MAPS
#define IFDEBUG_MAPS(X); X
#else
#define IFDEBUG_MAPS(X);
#endif

#ifdef DEBUG_JOYSTICK
#define IFDEBUG_JOYSTICK(X); X
#else
#define IFDEBUG_JOYSTICK(X);
#endif

#ifdef DEBUG_EVENTS
#define IFDEBUG_EVENTS(X); X
#else
#define IFDEBUG_EVENTS(X);
#endif

#ifdef DEBUG_AUDIO
#define IFDEBUG_AUDIO(X); X
#else
#define IFDEBUG_AUDIO(X);
#endif

#ifdef DEBUG_AUDIO2
#define IFDEBUG_AUDIO2(X); X
#else
#define IFDEBUG_AUDIO2(X);
#endif

#ifdef DEBUG_VIDEO
#define IFDEBUG_VIDEO(X); X
#else
#define IFDEBUG_VIDEO(X);
#endif

#ifdef DEBUG_VIDEO2
#define IFDEBUG_VIDEO2(X); X
#else
#define IFDEBUG_VIDEO2(X);
#endif

#endif /* ndef _DEBUG_H */

/* eof */


