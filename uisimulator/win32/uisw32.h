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

#ifdef _MSC_VER
typedef unsigned short wchar_t;
#define _WCHAR_T_DEFINED
#endif
#include <windows.h>
#include "lcd-win32.h"

#define UI_WIDTH                    240 // width of GUI window
#define UI_HEIGHT                   360 // height of GUI window
#define UI_LCD_BGCOLOR              46, 67, 49 // bkgnd color of LCD (no backlight)
//#define UI_LCD_BGCOLORLIGHT         56, 77, 59 // bkgnd color of LCD (backlight)
#define UI_LCD_BGCOLORLIGHT         109, 212, 68 // bkgnd color of LCD (backlight)
#define UI_LCD_BLACK                0, 0, 0 // black
#define UI_LCD_POSX                 59 // x position of lcd
#define UI_LCD_POSY                 95 // y position of lcd
#define UI_LCD_WIDTH                112
#define UI_LCD_HEIGHT               64

#define TM_YIELD                    WM_USER + 101 // thread message for yield
#define TIMER_EVENT                 0x34928340

extern HWND                         hGUIWnd; // the GUI window handle
extern unsigned int                 uThreadID; // id of mod thread
extern bool                         bActive;

// typedefs
typedef unsigned char               uchar;
typedef unsigned int                uint32;

#endif // #ifndef __UISW32_H__
