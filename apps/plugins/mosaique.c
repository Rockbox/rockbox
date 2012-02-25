/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 Itai Shaked 
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 **************************************************************************/
#include "plugin.h"
#include "lib/playergfx.h"
#include "lib/mylcd.h"
#include "lib/pluginlib_actions.h"


#ifdef HAVE_LCD_BITMAP
#define GFX_X (LCD_WIDTH/2-1)
#define GFX_Y (LCD_HEIGHT/2-1)
#if LCD_WIDTH != LCD_HEIGHT
#define GFX_WIDTH  GFX_X
#define GFX_HEIGHT GFX_Y
#else
#define GFX_WIDTH  GFX_X
#define GFX_HEIGHT (4*GFX_Y/5)
#endif
#else
#define GFX_X 9
#define GFX_Y 6
#define GFX_WIDTH  9
#define GFX_HEIGHT 6
#endif

/* this set the context to use with PLA */
static const struct button_mapping *plugin_contexts[] = { pla_main_ctx };

#define MOSAIQUE_QUIT        PLA_EXIT
#define MOSAIQUE_QUIT2       PLA_CANCEL
#define MOSAIQUE_SPEED       PLA_UP
#define MOSAIQUE_RESTART     PLA_SELECT

enum plugin_status plugin_start(const void* parameter)
{
    int button;
    int timer = 10;
    int x=0;
    int y=0;
    int sx = 3;
    int sy = 3;
    (void)parameter;

#ifdef HAVE_LCD_CHARCELLS
    if (!pgfx_init(4, 2))
    {
        rb->splash(HZ*2, "Old LCD :(");
        return PLUGIN_OK;
    }
    pgfx_display(3, 0);
#endif
    mylcd_clear_display();
    mylcd_set_drawmode(DRMODE_COMPLEMENT);
    while (1) {

        x+=sx;
        if (x>GFX_WIDTH) 
        {
            x = 2*GFX_WIDTH-x;
            sx=-sx;
        }

        if (x<0) 
        {
            x = -x;
            sx = -sx;
        }

        y+=sy;
        if (y>GFX_HEIGHT) 
        {
            y = 2*GFX_HEIGHT-y;
            sy=-sy;
        }

        if (y<0) 
        {
            y = -y;
            sy = -sy;
        }

        mylcd_fillrect(GFX_X-x, GFX_Y-y, 2*x+1, 1);
        mylcd_fillrect(GFX_X-x, GFX_Y+y, 2*x+1, 1);
        mylcd_fillrect(GFX_X-x, GFX_Y-y+1, 1, 2*y-1);
        mylcd_fillrect(GFX_X+x, GFX_Y-y+1, 1, 2*y-1);
        mylcd_update();

        rb->sleep(HZ/timer);

        /*We get button from PLA this way */
        button = pluginlib_getaction(TIMEOUT_NOBLOCK, plugin_contexts,
                               ARRAYLEN(plugin_contexts));

        switch (button)
        {
#ifdef MOSAIQUE_RC_QUIT
            case MOSAIQUE_RC_QUIT:
#endif
            case MOSAIQUE_QUIT:
            case MOSAIQUE_QUIT2:
                mylcd_set_drawmode(DRMODE_SOLID);
#ifdef HAVE_LCD_CHARCELLS
                pgfx_release();
#endif
                return PLUGIN_OK;

            case MOSAIQUE_SPEED:
                timer = timer+5;
                if (timer>20)
                    timer=5;
                break;

            case MOSAIQUE_RESTART:

                sx = rb->rand() % (GFX_HEIGHT/2) + 1;
                sy = rb->rand() % (GFX_HEIGHT/2) + 1;
                x=0;
                y=0;
                mylcd_clear_display();
                break;


            default:
                if (rb->default_event_handler(button) == SYS_USB_CONNECTED)
                {
                    mylcd_set_drawmode(DRMODE_SOLID);
#ifdef HAVE_LCD_CHARCELLS
                    pgfx_release();
#endif
                    return PLUGIN_USB_CONNECTED;
                }
                break;
        }
    }
}
