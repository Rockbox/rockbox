/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2005 by Kevin Ferrare
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/


#ifndef _GUI_SELECT_H_
#define _GUI_SELECT_H_
#include "screen_access.h"
#include "settings.h"

#if (CONFIG_KEYPAD == IRIVER_H100_PAD) || \
  (CONFIG_KEYPAD == IRIVER_H300_PAD)
#define SELECT_INC     BUTTON_UP
#define SELECT_DEC     BUTTON_DOWN
#define SELECT_OK      BUTTON_SELECT
#define SELECT_OK2     BUTTON_LEFT
#define SELECT_CANCEL  BUTTON_OFF
#define SELECT_CANCEL2 BUTTON_MODE

#elif CONFIG_KEYPAD == RECORDER_PAD
#define SELECT_INC     BUTTON_UP
#define SELECT_DEC     BUTTON_DOWN
#define SELECT_OK      BUTTON_PLAY
#define SELECT_OK2     BUTTON_LEFT
#define SELECT_CANCEL  BUTTON_OFF
#define SELECT_CANCEL2 BUTTON_F1

#elif CONFIG_KEYPAD == PLAYER_PAD
#define SELECT_INC     BUTTON_RIGHT
#define SELECT_DEC     BUTTON_LEFT
#define SELECT_OK      BUTTON_PLAY
#define SELECT_CANCEL  BUTTON_STOP
#define SELECT_CANCEL2 BUTTON_MENU

#elif CONFIG_KEYPAD == ONDIO_PAD
#define SELECT_INC     BUTTON_UP
#define SELECT_DEC     BUTTON_DOWN
#define SELECT_OK      BUTTON_RIGHT
#define SELECT_OK2     BUTTON_LEFT
#define SELECT_CANCEL  BUTTON_MENU
#define SELECT_CANCEL2 BUTTON_OFF

#elif CONFIG_KEYPAD == GMINI100_PAD
#define SELECT_INC     BUTTON_UP
#define SELECT_DEC     BUTTON_DOWN
#define SELECT_OK      BUTTON_PLAY
#define SELECT_OK2     BUTTON_LEFT
#define SELECT_CANCEL  BUTTON_OFF
#define SELECT_CANCEL2 BUTTON_MENU

#endif

struct gui_select
{
    bool canceled;
    bool validated;
    const char * title;
    int min_value;
    int max_value;
    int step;
    int option;
    const char * extra_string;
    /* In the case the option is a number */
    void (*formatter)(char* dest,
                      int dest_length,
                      int variable,
                      const char* unit);
    const struct opt_items * items;
};

extern void gui_select_init_numeric(struct gui_select * select,
        const char * title,
        int init_value,
        int min_value,
        int max_value,
        int step,
        const char * unit,
        void (*formatter)(char* dest,
                          int dest_length,
                          int variable,
                          const char* unit));

extern void gui_select_init_items(struct gui_select * select,
        const char * title,
        int selected,
        const struct opt_items * items,
        int nb_items
        );

extern void gui_select_next(struct gui_select * select);

extern void gui_select_prev(struct gui_select * select);

extern void gui_select_draw(struct gui_select * select, struct screen * display);

#define gui_select_get_selected(select) \
    (select)->option

#define gui_select_cancel(select) \
    (select)->canceled=true

#define gui_select_is_canceled(select) \
    (select)->canceled

#define gui_select_validate(select) \
    (select)->validated=true

#define gui_select_is_validated(select) \
    (select)->validated

extern void gui_syncselect_draw(struct gui_select * select);

extern bool gui_syncselect_do_button(struct gui_select * select, int button);

#endif /* _GUI_SELECT_H_ */
