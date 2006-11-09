
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
#if (CONFIG_KEYPAD == IRIVER_H100_PAD) || \
    (CONFIG_KEYPAD == IRIVER_H300_PAD)
#define HAS_BUTTON_HOLD
#define HAS_REMOTE_BUTTON_HOLD
#endif
extern struct event_queue button_queue;

void button_init (void);
long button_get (bool block);
long button_get_w_tmo(int ticks);
int button_status(void);
void button_clear_queue(void);
#ifdef HAVE_LCD_BITMAP
void button_set_flip(bool flip); /* turn 180 degrees */
#endif
#ifdef CONFIG_BACKLIGHT
void set_backlight_filter_keypress(bool value);
#ifdef HAVE_REMOTE_LCD
void set_remote_backlight_filter_keypress(bool value);
#endif
#endif

#ifdef HAS_BUTTON_HOLD
bool button_hold(void);
#endif
#ifdef HAS_REMOTE_BUTTON_HOLD
bool remote_button_hold(void);
#endif
#ifdef HAVE_HEADPHONE_DETECTION
bool headphones_inserted(void);
#endif
#ifdef HAVE_WHEEL_POSITION
int wheel_status(void);
void wheel_send_events(bool send);
#endif

#define  BUTTON_NONE        0x00000000

        /* Button modifiers */
#define BUTTON_REL          0x02000000
#define BUTTON_REPEAT       0x04000000


#ifdef TARGET_TREE
#include "button-target.h"
#else

  /* Target specific button codes */
#if CONFIG_KEYPAD == RECORDER_PAD

 /* Recorder specific button codes */

      /* Main unit's buttons */
#define BUTTON_ON           0x00000001
#define BUTTON_OFF          0x00000002

#define BUTTON_LEFT         0x00000004
#define BUTTON_RIGHT        0x00000008
#define BUTTON_UP           0x00000010
#define BUTTON_DOWN         0x00000020

#define BUTTON_PLAY         0x00000040

#define BUTTON_F1           0x00000080
#define BUTTON_F2           0x00000100
#define BUTTON_F3           0x00000200

#define BUTTON_MAIN (BUTTON_ON|BUTTON_OFF|BUTTON_LEFT|BUTTON_RIGHT\
                |BUTTON_UP|BUTTON_DOWN|BUTTON_PLAY\
                |BUTTON_F1|BUTTON_F2|BUTTON_F3)
                
    /* Remote control's buttons */
#define BUTTON_RC_PLAY      0x00100000
#define BUTTON_RC_STOP      0x00080000

#define BUTTON_RC_LEFT      0x00040000
#define BUTTON_RC_RIGHT     0x00020000
#define BUTTON_RC_VOL_UP    0x00010000
#define BUTTON_RC_VOL_DOWN  0x00008000

#define BUTTON_REMOTE (BUTTON_RC_PLAY|BUTTON_RC_STOP\
                |BUTTON_RC_LEFT|BUTTON_RC_RIGHT\
                |BUTTON_RC_VOL_UP|BUTTON_RC_VOL_DOWN)

#elif CONFIG_KEYPAD == PLAYER_PAD

        /* Main unit's buttons */
#define BUTTON_ON           0x00000001
#define BUTTON_STOP         0x00000002

#define BUTTON_LEFT         0x00000004
#define BUTTON_RIGHT        0x00000008
#define BUTTON_PLAY         0x00000010
#define BUTTON_MENU         0x00000020

#define BUTTON_MAIN (BUTTON_ON|BUTTON_STOP|BUTTON_LEFT|BUTTON_RIGHT\
                |BUTTON_PLAY|BUTTON_MENU)

    /* Remote control's buttons */
#define BUTTON_RC_PLAY      0x00100000
#define BUTTON_RC_STOP      0x00080000

#define BUTTON_RC_LEFT      0x00040000
#define BUTTON_RC_RIGHT     0x00020000
#define BUTTON_RC_VOL_UP    0x00010000
#define BUTTON_RC_VOL_DOWN  0x00008000

#define BUTTON_REMOTE (BUTTON_RC_PLAY|BUTTON_RC_STOP\
                |BUTTON_RC_LEFT|BUTTON_RC_RIGHT\
                |BUTTON_RC_VOL_UP|BUTTON_RC_VOL_DOWN)


#elif CONFIG_KEYPAD == ONDIO_PAD

    /* Ondio specific button codes */

#define BUTTON_OFF          0x00000001
#define BUTTON_MENU         0x00000002

#define BUTTON_LEFT         0x00000004
#define BUTTON_RIGHT        0x00000008
#define BUTTON_UP           0x00000010
#define BUTTON_DOWN         0x00000020

#define BUTTON_MAIN (BUTTON_OFF|BUTTON_MENU|BUTTON_LEFT|BUTTON_RIGHT\
                |BUTTON_UP|BUTTON_DOWN)

#define BUTTON_REMOTE 0

#elif CONFIG_KEYPAD == GMINI100_PAD

  /* Gmini specific button codes */

#define BUTTON_ON           0x00000001
#define BUTTON_OFF          0x00000002

#define BUTTON_LEFT         0x00000004
#define BUTTON_RIGHT        0x00000008
#define BUTTON_UP           0x00000010
#define BUTTON_DOWN         0x00000020

#define BUTTON_PLAY         0x00000040
#define BUTTON_MENU         0x00000080

#define BUTTON_MAIN (BUTTON_ON|BUTTON_OFF|BUTTON_LEFT|BUTTON_RIGHT\
                |BUTTON_UP|BUTTON_DOWN|BUTTON_PLAY|BUTTON_MENU)

#define BUTTON_REMOTE 0


#elif 0

/* 
 * Please, add the next Rockbox target's button defines here,
 * using: 
 *  - bits 0 and up:  for main unit's buttons
 *  - bits 20 (0x00100000) and downwards for the remote buttons (if applicable)
 * Don't forget to add BUTTON_MAIN and BUTTON_REMOTE masks
 * Currently, main buttons on all targets are up to bit 9 (0x00000200),
 * and remote buttons are down to bit 10 (0x00000400)
 */ 



#endif /* RECORDER/PLAYER/ONDIO/GMINI KEYPAD */

#endif /* TARGET_TREE */

#endif /* _BUTTON_H_ */

