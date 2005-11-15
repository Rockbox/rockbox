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

// BITMAPINFO256
typedef struct
{
	BITMAPINFOHEADER bmiHeader;
	RGBQUAD bmiColors[256];
} BITMAPINFO256;

#if LCD_DEPTH >= 16
extern unsigned long   bitmap[LCD_HEIGHT][LCD_WIDTH]; // the ui display
#else
extern unsigned char   bitmap[LCD_HEIGHT][LCD_WIDTH]; // the ui display
#endif
extern BITMAPINFO256   bmi; // bitmap information

#ifdef HAVE_REMOTE_LCD
extern unsigned char   remote_bitmap[LCD_REMOTE_HEIGHT][LCD_REMOTE_WIDTH];
extern BITMAPINFO256   remote_bmi; // bitmap information
#endif

void simlcdinit(void);

#endif // #ifndef __LCDWIN32_H__
