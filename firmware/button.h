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

#include "config.h"

#ifdef HAVE_RECORDER_KEYPAD
/*
 * Archos Jukebox Recorder button functions
 */

/* Button codes */
#define	BUTTON_NONE		0x0000
#define	BUTTON_ON		0x0001
#define	BUTTON_OFF		0x0002
#define	BUTTON_PLAY		0x0004
#define	BUTTON_UP		0x0010
#define	BUTTON_DOWN		0x0020
#define	BUTTON_LEFT		0x0040
#define	BUTTON_RIGHT		0x0080
#define	BUTTON_F1		0x0100
#define	BUTTON_F2		0x0200
#define	BUTTON_F3		0x0400

/* Button modifiers */
#define	BUTTON_HELD		0x4000
#define	BUTTON_REL		0x8000

void button_init (void);
int button_get (void);

#endif
