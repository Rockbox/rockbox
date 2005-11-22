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

#ifndef _GUI_LOGO_H_
#define _GUI_LOGO_H_
#include "screen_access.h"

struct logo{
#ifdef HAVE_LCD_BITMAP
    const unsigned char * bitmap;
    int width;
    int height;
#else
    const char * text;
#endif
};

extern struct logo usb_logos[];

/*
 * Draws the given logo at the center of the given screen
 *  - logo : the logo
 *  - display : the screen to draw on
 */
void gui_logo_draw(struct logo * logo, struct screen * display);

#endif /* _GUI_LOGO_H_ */
