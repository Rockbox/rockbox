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

#ifndef __UISW32_H__
#define __UISW32_H__

#include <windows.h>
#include "lcd-win32.h"

#define UI_WIDTH                    240 // width of GUI window
#define UI_HEIGHT                   360 // height of GUI window
#define UI_LCD_COLOR                46, 67, 49 // bkgnd color of LCD
#define UI_LCD_BLACK                0, 0, 0 // black
#define UI_LCD_POSX                 59 // x position of lcd
#define UI_LCD_POSY                 95 // y position of lcd

extern HWND                         hGUIWnd; // the GUI window handle
extern unsigned int                 uThreadID; // id of mod thread

// typedefs
typedef unsigned char               uchar;
typedef unsigned int                uint32;

#endif // #ifndef __UISW32_H__