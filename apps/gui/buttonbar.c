/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) Linus Nielsen Feltzing (2002)
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
/*
2005 Kevin Ferrare :
 - Multi screen support
 - Rewrote a lot of code to avoid global vars and make it accept eventually
   more that 3 buttons on the bar (just the prototype of gui_buttonbar_set
   and the constant BUTTONBAR_MAX_BUTTONS to modify)
*/
#include "config.h"
#include "buttonbar.h"

#ifdef HAS_BUTTONBAR

#include "lcd.h"
#include "font.h"
#include "string.h"
#include "settings.h"

void gui_buttonbar_init(struct gui_buttonbar * buttonbar)
{
    gui_buttonbar_unset(buttonbar);
}

void gui_buttonbar_set_display(struct gui_buttonbar * buttonbar,
                               struct screen * display)
{
    buttonbar->display = display;
}

void gui_buttonbar_draw_button(struct gui_buttonbar * buttonbar, int num)
{
    int xpos, ypos, button_width, text_width;
    int fw, fh;
    struct screen * display = buttonbar->display;
    display->getstringsize("M", &fw, &fh);

    button_width = display->width/BUTTONBAR_MAX_BUTTONS;
    xpos = num * button_width;
    ypos = display->height - fh;

    if(buttonbar->caption[num][0] != 0)
    {
        /* center the text */
        text_width = fw * strlen(buttonbar->caption[num]);
        display->putsxy(xpos + (button_width - text_width)/2,
                        ypos, buttonbar->caption[num]);
    }

    display->set_drawmode(DRMODE_COMPLEMENT);
    display->fillrect(xpos, ypos, button_width - 1, fh);
    display->set_drawmode(DRMODE_SOLID);
}

void gui_buttonbar_set(struct gui_buttonbar * buttonbar,
                       const char *caption1,
                       const char *caption2,
                       const char *caption3)
{
    gui_buttonbar_unset(buttonbar);
    if(caption1)
    {
        strncpy(buttonbar->caption[0], caption1, 7);
        buttonbar->caption[0][7] = 0;
    }
    if(caption2)
    {
        strncpy(buttonbar->caption[1], caption2, 7);
        buttonbar->caption[1][7] = 0;
    }
    if(caption3)
    {
        strncpy(buttonbar->caption[2], caption3, 7);
        buttonbar->caption[2][7] = 0;
    }
}

void gui_buttonbar_unset(struct gui_buttonbar * buttonbar)
{
    int i;
    for(i = 0;i < BUTTONBAR_MAX_BUTTONS;i++)
        buttonbar->caption[i][0] = 0;
}

void gui_buttonbar_draw(struct gui_buttonbar * buttonbar)
{
    struct screen * display = buttonbar->display;
    screen_has_buttonbar(display, gui_buttonbar_isset(buttonbar));
    if(!global_settings.buttonbar || !display->has_buttonbar)
        return;
    int i;
    display->setfont(FONT_SYSFIXED);

    display->set_drawmode(DRMODE_SOLID|DRMODE_INVERSEVID);
    display->fillrect(0, display->height - BUTTONBAR_HEIGHT,
                      display->width, BUTTONBAR_HEIGHT);
    display->set_drawmode(DRMODE_SOLID);

    for(i = 0;i < BUTTONBAR_MAX_BUTTONS;i++)
        gui_buttonbar_draw_button(buttonbar, i);
    display->update_rect(0, display->height - BUTTONBAR_HEIGHT,
                         display->width, BUTTONBAR_HEIGHT);
    display->setfont(FONT_UI);
}

bool gui_buttonbar_isset(struct gui_buttonbar * buttonbar)
{
    /* If all buttons are unset, the button bar is considered disabled */
    int i;
    for(i = 0;i < BUTTONBAR_MAX_BUTTONS;i++)
        if(buttonbar->caption[i][0] != 0)
            return true;
    return false;
}

#endif /* HAS_BUTTONBAR */
