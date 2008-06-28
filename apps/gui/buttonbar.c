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
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
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
2008 Jonathan Gordon
 - redone to use viewports, items will NOT scroll in their vp.
   Bar is always drawn at the bottom of the screen. This may be changed later.
   Callers need to remember to adjust their viewports to not be overwitten
*/
#include "config.h"
#include "buttonbar.h"
#include "viewport.h"
#include "lcd.h"
#include "font.h"
#include "string.h"
#include "settings.h"

static struct viewport bb_vp[NB_SCREENS];
void gui_buttonbar_init(struct gui_buttonbar * buttonbar)
{
    int i;
    gui_buttonbar_unset(buttonbar);
    FOR_NB_SCREENS(i)
    {
        viewport_set_defaults(&bb_vp[i], i);
        bb_vp[i].font = FONT_SYSFIXED;
        bb_vp[i].y = screens[i].height - BUTTONBAR_HEIGHT;
        bb_vp[i].height = BUTTONBAR_HEIGHT;
        bb_vp[i].drawmode = DRMODE_COMPLEMENT;
    }
}

void gui_buttonbar_set_display(struct gui_buttonbar * buttonbar,
                               struct screen * display)
{
    buttonbar->display = display;
}

static void gui_buttonbar_draw_button(struct gui_buttonbar * buttonbar, int num)
{
    int button_width;
    int fh, fw;
    struct screen * display = buttonbar->display;
    struct viewport vp = bb_vp[display->screen_type];

    button_width = display->width/BUTTONBAR_MAX_BUTTONS;
    vp.width = button_width;
    vp.x = button_width * num;
    display->set_viewport(&vp);
    display->fillrect(0, 0, button_width - 1, vp.height);
    if(buttonbar->caption[num][0] != 0)
    {
        display->getstringsize(buttonbar->caption[num], &fw, &fh);
        display->putsxy((button_width - fw)/2,
                        (vp.height-fh)/2, buttonbar->caption[num]);
    }
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
    if(!global_settings.buttonbar || !gui_buttonbar_isset(buttonbar))
        return;
    int i;
    display->set_viewport(&bb_vp[display->screen_type]);
    display->clear_viewport();
    for(i = 0;i < BUTTONBAR_MAX_BUTTONS;i++)
        gui_buttonbar_draw_button(buttonbar, i);
    display->set_viewport(&bb_vp[display->screen_type]);
    display->update_viewport();
    display->set_viewport(NULL);
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
