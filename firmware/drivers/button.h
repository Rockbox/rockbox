/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Daniel Stenberg
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#ifndef _BUTTON_H_
#define _BUTTON_H_

#include <stdbool.h>
#include "config.h"

extern struct event_queue button_queue;
extern long last_keypress;

void button_init (void);
int button_get (bool block);
int button_get_w_tmo(int ticks);

/* Shared button codes */
#define	BUTTON_NONE		0x0000
#define	BUTTON_ON		0x0001
#define	BUTTON_UP		0x0010
#define	BUTTON_DOWN		0x0020
#define	BUTTON_LEFT		0x0040
#define	BUTTON_RIGHT		0x0080

/* remote control buttons */
#define BUTTON_VOL_UP           0x1000
#define BUTTON_VOL_DOWN         0x1001

/* Button modifiers */
#define	BUTTON_REPEAT		0x4000
#define	BUTTON_REL		0x8000

/* Special message */
#define	BUTTON_LOCKED		0x2000

#ifdef HAVE_RECORDER_KEYPAD

/* Recorder specific button codes */
#define	BUTTON_OFF		0x0002
#define	BUTTON_PLAY		0x0004
#define	BUTTON_F1		0x0100
#define	BUTTON_F2		0x0200
#define	BUTTON_F3		0x0400

#define ALL_BUTTONS (BUTTON_ON | BUTTON_UP | BUTTON_DOWN | BUTTON_LEFT | \
                     BUTTON_RIGHT | BUTTON_OFF | BUTTON_PLAY | BUTTON_F1 | \
                     BUTTON_F2 | BUTTON_F3)

#elif HAVE_PLAYER_KEYPAD

/* Jukebox 6000 and Studio specific button codes */
#define	BUTTON_MENU		0x0002
#define	BUTTON_PLAY		BUTTON_UP
#define	BUTTON_STOP		BUTTON_DOWN

#define ALL_BUTTONS (BUTTON_ON | BUTTON_UP | BUTTON_DOWN | BUTTON_LEFT | \
                     BUTTON_RIGHT | BUTTON_MENU)

#endif /* HAVE_PLAYER_KEYPAD */

#endif

/* -----------------------------------------------------------------
 * local variables:
 * eval: (load-file "../rockbox-mode.el")
 * end:
 */
