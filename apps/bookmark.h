/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2003 by Benjamin Metzler
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#ifndef __BOOKMARK_H__
#define __BOOKMARK_H__

#include <stdbool.h>

#if CONFIG_KEYPAD == ONDIO_PAD
#define BOOKMARK_SELECT_PRE BUTTON_RIGHT
#define BOOKMARK_SELECT (BUTTON_RIGHT | BUTTON_REL)
#define BOOKMARK_DELETE (BUTTON_RIGHT | BUTTON_REPEAT)
#define BOOKMARK_DOWN BUTTON_DOWN

#elif (CONFIG_KEYPAD == IRIVER_H100_PAD) || \
      (CONFIG_KEYPAD == IRIVER_H300_PAD)
#define BOOKMARK_SELECT BUTTON_SELECT
#define BOOKMARK_DELETE (BUTTON_ON | BUTTON_SELECT)
#define BOOKMARK_DOWN BUTTON_DOWN

#elif (CONFIG_KEYPAD == IPOD_4G_PAD)
#define BOOKMARK_SELECT BUTTON_SELECT
#define BOOKMARK_DELETE (BUTTON_RIGHT | BUTTON_REPEAT)
#define BOOKMARK_DOWN BUTTON_SCROLL_FWD

#elif CONFIG_KEYPAD == IRIVER_IFP7XX_PAD
#define BOOKMARK_SELECT BUTTON_SELECT
#define BOOKMARK_DELETE (BUTTON_PLAY | BUTTON_SELECT)
#define BOOKMARK_DOWN BUTTON_DOWN

#else /* player, recorder, gmini */
#define BOOKMARK_SELECT BUTTON_PLAY
#define BOOKMARK_DELETE (BUTTON_ON | BUTTON_PLAY)
#define BOOKMARK_DOWN BUTTON_DOWN

#endif

bool bookmark_load_menu(void);
bool bookmark_autobookmark(void);
bool bookmark_create_menu(void);
bool bookmark_mrb_load(void);
bool bookmark_autoload(const char* file);
bool bookmark_load(const char* file, bool autoload);
void bookmark_play(char* resume_file, int index, int offset, int seed,
                   char *filename);
bool bookmark_exist(void);

#endif /* __BOOKMARK_H__ */

