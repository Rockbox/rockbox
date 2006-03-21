
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
    (CONFIG_KEYPAD == IRIVER_H300_PAD) || \
    (CONFIG_KEYPAD == IAUDIO_X5_PAD)
#define HAS_BUTTON_HOLD
#define HAS_REMOTE_BUTTON_HOLD
#elif (CONFIG_KEYPAD == IPOD_4G_PAD) || \
      (CONFIG_KEYPAD == IPOD_3G_PAD) || \
      (CONFIG_KEYPAD == IRIVER_IFP7XX_PAD)
#define HAS_BUTTON_HOLD
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

#ifdef HAS_BUTTON_HOLD
bool button_hold(void);
#endif
#ifdef HAS_REMOTE_BUTTON_HOLD
bool remote_button_hold(void);
#endif


#define  BUTTON_NONE        0x00000000

        /* Button modifiers */
#define BUTTON_REL          0x02000000
#define BUTTON_REPEAT       0x04000000


  /* Target specific button codes */

#if (CONFIG_KEYPAD == IRIVER_H100_PAD)\
    || (CONFIG_KEYPAD == IRIVER_H300_PAD)

/* iRiver H100/H300 specific button codes */

        /* Main unit's buttons */
#define BUTTON_ON           0x00000001
#define BUTTON_OFF          0x00000002

#define BUTTON_LEFT         0x00000004
#define BUTTON_RIGHT        0x00000008
#define BUTTON_UP           0x00000010
#define BUTTON_DOWN         0x00000020

#define BUTTON_REC          0x00000040
#define BUTTON_MODE         0x00000080

#define BUTTON_SELECT       0x00000100

#define BUTTON_MAIN (BUTTON_ON|BUTTON_OFF|BUTTON_LEFT|BUTTON_RIGHT|\
                BUTTON_UP|BUTTON_DOWN|BUTTON_REC|BUTTON_MODE|BUTTON_SELECT)

    /* Remote control's buttons */
#define BUTTON_RC_ON        0x00100000
#define BUTTON_RC_STOP      0x00080000

#define BUTTON_RC_REW       0x00040000
#define BUTTON_RC_FF        0x00020000
#define BUTTON_RC_VOL_UP    0x00010000
#define BUTTON_RC_VOL_DOWN  0x00008000

#define BUTTON_RC_REC       0x00004000
#define BUTTON_RC_MODE      0x00002000

#define BUTTON_RC_MENU      0x00001000

#define BUTTON_RC_BITRATE   0x00000800
#define BUTTON_RC_SOURCE    0x00000400

#define BUTTON_REMOTE (BUTTON_RC_ON|BUTTON_RC_STOP|BUTTON_RC_REW|BUTTON_RC_FF\
                |BUTTON_RC_VOL_UP|BUTTON_RC_VOL_DOWN|BUTTON_RC_REC\
                |BUTTON_RC_MODE|BUTTON_RC_MENU|BUTTON_RC_BITRATE\
                |BUTTON_RC_SOURCE)

#elif CONFIG_KEYPAD == RECORDER_PAD

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

/* Jukebox 6000 and Studio specific button codes */

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

#elif ((CONFIG_KEYPAD == IPOD_4G_PAD) || (CONFIG_KEYPAD == IPOD_3G_PAD))

  /* iPod specific button codes */

#define BUTTON_SELECT       0x00000001
#define BUTTON_MENU         0x00000002

#define BUTTON_LEFT         0x00000004
#define BUTTON_RIGHT        0x00000008
#define BUTTON_SCROLL_FWD   0x00000010
#define BUTTON_SCROLL_BACK  0x00000020

#define BUTTON_PLAY         0x00000040

#if CONFIG_KEYPAD == IPOD_3G_PAD
#define  BUTTON_HOLD        0x00000100
/* TODO Does the BUTTON_HOLD need to be here? */
#define BUTTON_MAIN (BUTTON_SELECT|BUTTON_MENU\
                |BUTTON_LEFT|BUTTON_RIGHT\
                |BUTTON_SCROLL_FWD|BUTTON_SCROLL_BACK\
                |BUTTON_PLAY|BUTTON_HOLD)
#else
#define BUTTON_MAIN (BUTTON_SELECT|BUTTON_MENU\
                |BUTTON_LEFT|BUTTON_RIGHT|BUTTON_SCROLL_FWD\
                |BUTTON_SCROLL_BACK|BUTTON_PLAY)
#endif

#define BUTTON_REMOTE 0

/* This is for later
#define  BUTTON_SCROLL_TOUCH 0x00000200
*/

#elif CONFIG_KEYPAD == IRIVER_IFP7XX_PAD

/* iriver IFP7XX specific button codes */

#define BUTTON_PLAY         0x00000001
#define BUTTON_SELECT       0x00000002

#define BUTTON_LEFT         0x00000004
#define BUTTON_RIGHT        0x00000008
#define BUTTON_UP           0x00000010
#define BUTTON_DOWN         0x00000020

#define BUTTON_MODE         0x00000040
#define BUTTON_EQ           0x00000080

#define BUTTON_MAIN (BUTTON_PLAY|BUTTON_SELECT\
                |BUTTON_LEFT|BUTTON_RIGHT|BUTTON_UP|BUTTON_DOWN\
                |BUTTON_MODE|BUTTON_EQ)

#define BUTTON_REMOTE 0

#elif CONFIG_KEYPAD == IAUDIO_X5_PAD

/* iaudio X5 specific button codes */

    /* Main unit's buttons */
#define BUTTON_POWER        0x00000001
#define BUTTON_PLAY         0x00000002

#define BUTTON_LEFT         0x00000004
#define BUTTON_RIGHT        0x00000008
#define BUTTON_UP           0x00000010
#define BUTTON_DOWN         0x00000020

#define BUTTON_REC          0x00000040
#define BUTTON_SELECT       0x00000080

#define BUTTON_MAIN (BUTTON_POWER|BUTTON_PLAY|BUTTON_LEFT|BUTTON_RIGHT\
                |BUTTON_UP|BUTTON_DOWN|BUTTON_REC|BUTTON_SELECT)

    /* Remote control's buttons */
#define BUTTON_RC_PLAY      0x00100000

#define BUTTON_RC_REW       0x00080000
#define BUTTON_RC_FF        0x00040000
#define BUTTON_RC_VOL_UP    0x00020000
#define BUTTON_RC_VOL_DOWN  0x00010000

#define BUTTON_RC_REC       0x00008000
#define BUTTON_RC_MENU      0x00004000

#define BUTTON_RC_MODE      0x00002000

#define BUTTON_REMOTE (BUTTON_RC_PLAY|BUTTON_RC_VOL_UP|BUTTON_RC_VOL_DOWN\
                |BUTTON_RC_REW|BUTTON_RC_FF\
                |BUTTON_RC_REC|BUTTON_RC_MENU|BUTTON_RC_MODE)



#elif CONFIG_KEYPAD == GIGABEAT_PAD
/* Toshiba Gigabeat specific button codes */

#define BUTTON_POWER        0x00000001
#define BUTTON_MENU         0x00000002

#define BUTTON_LEFT         0x00000004
#define BUTTON_RIGHT        0x00000008
#define BUTTON_UP           0x00000010
#define BUTTON_DOWN         0x00000020

#define BUTTON_VOL_UP       0x00000040
#define BUTTON_VOL_DOWN     0x00000080

#define BUTTON_SELECT       0x00000100
#define BUTTON_A            0x00000200


#define BUTTON_MAIN (BUTTON_POWER|BUTTON_MENU|BUTTON_LEFT|BUTTON_RIGHT\
                |BUTTON_UP|BUTTON_DOWN|BUTTON_VOL_UP|BUTTON_VOL_DOWN\
                |BUTTON_SELECT|BUTTON_A)


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

#endif /* _BUTTON_H_ */

