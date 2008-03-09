/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2008 by Jonathan Gordon
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include "config.h"
#include "lcd.h"
#include "font.h"
#include "sprintf.h"
#include "string.h"
#include "settings.h"
#include "kernel.h"
#include "system.h"
#include "misc.h"
#include "atoi.h"
#include "viewport.h"
#include "statusbar.h"
#include "screen_access.h"

int viewport_get_nb_lines(struct viewport *vp)
{
#ifdef HAVE_LCD_BITMAP
    return vp->height/font_get(vp->font)->height;
#else
    (void)vp;
    return 2;
#endif
}


void viewport_set_defaults(struct viewport *vp, enum screen_type screen)
{
    vp->x = 0;
    vp->width = screens[screen].width;
    
    vp->y = global_settings.statusbar?STATUSBAR_HEIGHT:0;
    vp->height = screens[screen].height - vp->y
#ifdef HAS_BUTTONBAR
                - (screens[screen].has_buttonbar?BUTTONBAR_HEIGHT:0)
#endif
                ;
#ifdef HAVE_LCD_BITMAP
    vp->drawmode = DRMODE_SOLID;
    vp->font = FONT_UI; /* default to UI to discourage SYSFONT use */
#endif
    if (screens[screen].depth > 1)
    {
#ifdef HAVE_LCD_COLOR
        vp->fg_pattern = global_settings.fg_color;
        vp->bg_pattern = global_settings.bg_color;
        vp->lss_pattern = global_settings.lss_color;
        vp->lse_pattern = global_settings.lse_color;
        vp->lst_pattern = global_settings.lst_color;
#elif LCD_DEPTH > 1
        vp->fg_pattern = LCD_DEFAULT_FG;
        vp->bg_pattern = LCD_DEFAULT_BG;
#endif
    }
}
