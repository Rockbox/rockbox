/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 Björn Stenberg
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include "logo.h"

#ifdef HAVE_LCD_BITMAP

#include <bitmaps/usblogo.h>
#if NB_SCREENS==2
#include <bitmaps/remote_usblogo.h>
#endif

struct logo usb_logos[]=
{
    [SCREEN_MAIN]={usblogo, BMPWIDTH_usblogo, BMPHEIGHT_usblogo},
#if NB_SCREENS==2
    [SCREEN_REMOTE]={remote_usblogo, BMPWIDTH_remote_usblogo, BMPHEIGHT_remote_usblogo}
#endif

};
#else
struct logo usb_logos[]=
{
    [SCREEN_MAIN]={"[USB Mode]"}
};
#endif

void gui_logo_draw(struct logo * logo, struct screen * display)
{
    display->clear_display();

#ifdef HAVE_LCD_BITMAP
    /* Center bitmap on screen */
    display->bitmap(logo->bitmap,
                display->width/2-logo->width/2,
                display->height/2-logo->height/2,
                logo->width,
                logo->height);
    display->update();
#else
    display->double_height(false);
    display->puts_scroll(0, 0, logo->text);
#ifdef SIMULATOR
    display->update();
#endif /* SIMULATOR */
#endif /* HAVE_LCD_BITMAP */
}
