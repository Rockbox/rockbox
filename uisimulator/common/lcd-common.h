/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Robert E. Hak <rhak@ramapo.edu>
 *
 * Windows Version Copyright (C) 2002 by Felix Arends 
 * X11 Version Copyright (C) 2002 by Daniel Stenberg <daniel@haxx.se>
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include "lcd.h"

#ifdef WIN32
 #ifndef __LCDWIN32_H__
 #define __LCDWIN32_H__

 #include "uisw32.h"
 #include "lcd.h"

 typedef struct
 {
     BITMAPINFOHEADER bmiHeader;
     RGBQUAD bmiColors[2];
 } BITMAPINFO2;

 #ifdef HAVE_LCD_BITMAP
  /* the display */
  extern unsigned char        display[LCD_WIDTH][LCD_HEIGHT/8];
 #else
  #define DISP_X              112
  #define DISP_Y              64
 #endif /* HAVE_LCD_BITMAP */

 /* the ui display */
 extern char                 bitmap[LCD_HEIGHT][LCD_WIDTH];
 /* bitmap information */
 extern BITMAPINFO2          bmi;
 #endif /* __LCDWIN32_H__ */

#else
 /* X11 */
 #define MARGIN_X 3
 #define MARGIN_Y 3

#endif /* WIN32 */














