/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Felix Arends
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#ifndef __LCDWIN32_H__
#define __LCDWIN32_H__

#include "uisw32.h"
#include "lcd.h"

// BITMAPINFO2
typedef struct
{
	BITMAPINFOHEADER bmiHeader;
	RGBQUAD bmiColors[2];
} BITMAPINFO2;

#ifdef HAVE_LCD_BITMAP

extern unsigned char        display[LCD_WIDTH][LCD_HEIGHT/8]; // the display
#else
#define DISP_X              112
#define DISP_Y              64
#endif


extern char                 bitmap[LCD_HEIGHT][LCD_WIDTH]; // the ui display
extern BITMAPINFO2          bmi; // bitmap information


#endif // #ifndef __LCDWIN32_H__
