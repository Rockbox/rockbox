/***************************************************************************
*             __________               __   ___.
*   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
*   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
*   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
*   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
*                     \/            \/     \/    \/            \/
* $Id$
*
* Bitmap graphics on player LCD!
*
* Copyright (C) 2005 Jens Arnold
*
* All files in this archive are subject to the GNU General Public License.
* See the file COPYING in the source tree root for full license agreement.
*
* This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
* KIND, either express or implied.
*
****************************************************************************/

#ifndef __PGFX_H__
#define __PGFX_H__

#include "plugin.h"

#ifdef HAVE_LCD_CHARCELLS /* Player only :) */

bool pgfx_init(struct plugin_api* newrb, int cwidth, int cheight);
void pgfx_release(void);
void pgfx_display(int cx, int cy);
void pgfx_clear_display(void);
void pgfx_drawpixel(int x, int y);
void pgfx_drawline(int x1, int y1, int x2, int y2);

#endif /* HAVE_LCD_CHARCELLS */
#endif /* __PGFX_H__ */
