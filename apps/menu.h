/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 Robert E. Hak
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#ifndef __MENU_H__
#define __MENU_H__

#include <stdbool.h>

typedef enum {
  MENU_OK,
  MENU_DISK_CHANGED, /* any file/directory contents need to be re-read */
  MENU_LAST /* don't use as return code, only for number of return codes
               available */
} Menu;

struct menu_items {
    unsigned char *desc;
    Menu (*function) (void);
};

int menu_init(struct menu_items* items, int count);
void menu_exit(int menu);

void put_cursorxy(int x, int y, bool on);

Menu menu_run(int menu);

#endif /* End __MENU_H__ */
