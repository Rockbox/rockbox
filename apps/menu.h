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

/* button definitions */
#if (CONFIG_KEYPAD == IRIVER_H100_PAD) || \
    (CONFIG_KEYPAD == IRIVER_H300_PAD)
#define MENU_EXIT       BUTTON_LEFT
#define MENU_EXIT2      BUTTON_OFF
#define MENU_EXIT_MENU  BUTTON_MODE
#define MENU_ENTER      BUTTON_RIGHT
#define MENU_ENTER2     BUTTON_SELECT

#define MENU_RC_EXIT        BUTTON_RC_STOP
#define MENU_RC_EXIT_MENU   BUTTON_RC_MODE
#define MENU_RC_ENTER       BUTTON_RC_ON
#define MENU_RC_ENTER2      BUTTON_RC_MENU


#elif CONFIG_KEYPAD == RECORDER_PAD

#define MENU_EXIT       BUTTON_LEFT
#define MENU_EXIT2      BUTTON_OFF
#define MENU_EXIT_MENU  BUTTON_F1
#define MENU_ENTER      BUTTON_RIGHT
#define MENU_ENTER2     BUTTON_PLAY

#define MENU_RC_EXIT    BUTTON_RC_STOP
#define MENU_RC_ENTER   BUTTON_RC_PLAY

#elif CONFIG_KEYPAD == PLAYER_PAD
#define MENU_EXIT       BUTTON_STOP
#define MENU_EXIT_MENU  BUTTON_MENU
#define MENU_ENTER      BUTTON_PLAY

#define MENU_RC_EXIT    BUTTON_RC_STOP
#define MENU_RC_ENTER   BUTTON_RC_PLAY

#elif CONFIG_KEYPAD == ONDIO_PAD
#define MENU_EXIT       BUTTON_LEFT
#define MENU_EXIT_MENU  BUTTON_MENU
#define MENU_ENTER      BUTTON_RIGHT

#elif CONFIG_KEYPAD == GMINI100_PAD
#define MENU_EXIT       BUTTON_LEFT
#define MENU_EXIT2      BUTTON_OFF
#define MENU_EXIT_MENU  BUTTON_MENU
#define MENU_ENTER      BUTTON_RIGHT
#define MENU_ENTER2     BUTTON_PLAY

#elif (CONFIG_KEYPAD == IPOD_3G_PAD) || (CONFIG_KEYPAD == IPOD_4G_PAD)

/* TODO: Check menu button assignments */

#define MENU_NEXT       BUTTON_DOWN
#define MENU_PREV       BUTTON_UP
#define MENU_EXIT       BUTTON_LEFT
#define MENU_EXIT_MENU  BUTTON_MENU
#define MENU_ENTER      BUTTON_RIGHT
#define MENU_ENTER2     BUTTON_SELECT

#elif CONFIG_KEYPAD == IRIVER_IFP7XX_PAD

#define MENU_NEXT       BUTTON_DOWN
#define MENU_PREV       BUTTON_UP
#define MENU_EXIT       BUTTON_LEFT
#define MENU_EXIT_MENU  BUTTON_PLAY
#define MENU_ENTER      BUTTON_RIGHT

#elif CONFIG_KEYPAD == IAUDIO_X5_PAD

#define MENU_NEXT       BUTTON_DOWN
#define MENU_PREV       BUTTON_UP
#define MENU_EXIT       BUTTON_LEFT
#define MENU_EXIT_MENU  BUTTON_REC
#define MENU_ENTER      BUTTON_RIGHT

#elif CONFIG_KEYPAD == GIGABEAT_PAD

#define MENU_EXIT       BUTTON_LEFT
#define MENU_EXIT2      BUTTON_A
#define MENU_EXIT_MENU  BUTTON_MENU
#define MENU_ENTER      BUTTON_RIGHT
#define MENU_ENTER2     BUTTON_SELECT
#define MENU_NEXT       BUTTON_DOWN
#define MENU_PREV       BUTTON_UP

#endif

struct menu_item {
    unsigned char *desc; /* string or ID */
    bool (*function) (void); /* return true if USB was connected */
};

int menu_init(const struct menu_item* mitems, int count, int (*callback)(int, int),
              const char *button1, const char *button2, const char *button3);
void menu_exit(int menu);

void put_cursorxy(int x, int y, bool on);

 /* Returns below define, or number of selected menu item*/
int menu_show(int m);
#define MENU_ATTACHED_USB -1
#define MENU_SELECTED_EXIT -2

bool menu_run(int menu);
int menu_cursor(int menu);
char* menu_description(int menu, int position);
void menu_delete(int menu, int position);
int menu_count(int menu);
bool menu_moveup(int menu);
bool menu_movedown(int menu);
void menu_draw(int menu);
void menu_insert(int menu, int position, char *desc, bool (*function) (void));
void menu_set_cursor(int menu, int position);
void menu_talk_selected(int m);

#endif /* End __MENU_H__ */
