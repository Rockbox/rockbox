/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 Dan Everton
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#ifndef __UISDL_H__
#define __UISDL_H__

#include <stdbool.h>
#include "SDL.h"

/* colour definitions are R, G, B */

#if defined(ARCHOS_RECORDER)
#define UI_TITLE                    "Jukebox Recorder"
#define UI_WIDTH                    270 // width of GUI window
#define UI_HEIGHT                   406 // height of GUI window
#define UI_LCD_BGCOLOR              90, 145, 90 // bkgnd color of LCD (no backlight)
#define UI_LCD_BGCOLORLIGHT         126, 229, 126 // bkgnd color of LCD (backlight)
#define UI_LCD_BLACK                0, 0, 0 // black
#define UI_LCD_POSX                 80 // x position of lcd
#define UI_LCD_POSY                 104 // y position of lcd (96 for real aspect)
#define UI_LCD_WIDTH                112
#define UI_LCD_HEIGHT               64 // (80 for real aspect)

#elif defined(ARCHOS_PLAYER)
#define UI_TITLE                    "Jukebox Player"
#define UI_WIDTH                    284 // width of GUI window
#define UI_HEIGHT                   420 // height of GUI window
#define UI_LCD_BGCOLOR              90, 145, 90 // bkgnd color of LCD (no backlight)
#define UI_LCD_BGCOLORLIGHT         126, 229, 126 // bkgnd color of LCD (backlight)
#define UI_LCD_BLACK                0, 0, 0 // black
#define UI_LCD_POSX                 75 // x position of lcd
#define UI_LCD_POSY                 111 // y position of lcd
#define UI_LCD_WIDTH                132
#define UI_LCD_HEIGHT               75

#elif defined(ARCHOS_FMRECORDER) || defined(ARCHOS_RECORDERV2)
#define UI_TITLE                    "Jukebox FM Recorder"
#define UI_WIDTH                    285 // width of GUI window
#define UI_HEIGHT                   414 // height of GUI window
#define UI_LCD_BGCOLOR              90, 145, 90 // bkgnd color of LCD (no backlight)
#define UI_LCD_BGCOLORLIGHT         126, 229, 126 // bkgnd color of LCD (backlight)
#define UI_LCD_BLACK                0, 0, 0 // black
#define UI_LCD_POSX                 87 // x position of lcd
#define UI_LCD_POSY                 77 // y position of lcd (69 for real aspect)
#define UI_LCD_WIDTH                112
#define UI_LCD_HEIGHT               64 // (80 for real aspect)

#elif defined(ARCHOS_ONDIOSP) || defined(ARCHOS_ONDIOFM)
#define UI_TITLE                    "Ondio"
#define UI_WIDTH                    155 // width of GUI window
#define UI_HEIGHT                   334 // height of GUI window
#define UI_LCD_BGCOLOR              90, 145, 90 // bkgnd color of LCD (no backlight)
#define UI_LCD_BGCOLORLIGHT         90, 145, 90 // bkgnd color of LCD (backlight)
#define UI_LCD_BLACK                0, 0, 0 // black
#define UI_LCD_POSX                 21 // x position of lcd
#define UI_LCD_POSY                 82 // y position of lcd (74 for real aspect)
#define UI_LCD_WIDTH                112
#define UI_LCD_HEIGHT               64 // (80 for real aspect)

#elif defined(IRIVER_H120) || defined(IRIVER_H100)
#define UI_TITLE                    "iriver H1x0"
#define UI_WIDTH                    379 // width of GUI window
#define UI_HEIGHT                   508 // height of GUI window
#define UI_LCD_BGCOLOR              90, 145, 90 // bkgnd color of LCD (no backlight)
#define UI_LCD_BGCOLORLIGHT         173, 216, 230 // bkgnd color of LCD (backlight)
#define UI_LCD_BLACK                0, 0, 0 // black
#define UI_LCD_POSX                 109 // x position of lcd
#define UI_LCD_POSY                 23 // y position of lcd
#define UI_LCD_WIDTH                160
#define UI_LCD_HEIGHT               128
#define UI_REMOTE_BGCOLOR           90, 145, 90 // bkgnd of remote lcd (no bklight)
#define UI_REMOTE_BGCOLORLIGHT      130, 180, 250 // bkgnd of remote lcd (bklight)
#define UI_REMOTE_POSX              50  // x position of remote lcd
#define UI_REMOTE_POSY              403 // y position of remote lcd
#define UI_REMOTE_WIDTH             128
#define UI_REMOTE_HEIGHT            64

#elif defined(IRIVER_H300)
#define UI_TITLE                    "iriver H300"
#define UI_WIDTH                    288 // width of GUI window
#define UI_HEIGHT                   581 // height of GUI window
/* high-colour */
#define UI_LCD_POSX                 26 // x position of lcd
#define UI_LCD_POSY                 36 // y position of lcd
#define UI_LCD_WIDTH                220
#define UI_LCD_HEIGHT               176
#define UI_REMOTE_BGCOLOR           90, 145, 90 // bkgnd of remote lcd (no bklight)
#define UI_REMOTE_BGCOLORLIGHT      130, 180, 250 // bkgnd of remote lcd (bklight)
#define UI_REMOTE_POSX              12  // x position of remote lcd
#define UI_REMOTE_POSY              478 // y position of remote lcd
#define UI_REMOTE_WIDTH             128
#define UI_REMOTE_HEIGHT            64

#elif defined(IPOD_4G)
#define UI_TITLE                    "iPod 4G"
#define UI_WIDTH                    196 // width of GUI window
#define UI_HEIGHT                   370 // height of GUI window
#define UI_LCD_BGCOLOR              90, 145, 90 // bkgnd color of LCD (no backlight)
#define UI_LCD_BGCOLORLIGHT         173, 216, 230 // bkgnd color of LCD (backlight)
#define UI_LCD_BLACK                0, 0, 0 // black
#define UI_LCD_POSX                 19 // x position of lcd
#define UI_LCD_POSY                 14 // y position of lcd
#define UI_LCD_WIDTH                160
#define UI_LCD_HEIGHT               128

#elif defined(IPOD_COLOR)
#define UI_TITLE                    "iPod Color"
#define UI_WIDTH                    261 // width of GUI window
#define UI_HEIGHT                   493 // height of GUI window
/* high-colour */
#define UI_LCD_POSX                 21 // x position of lcd
#define UI_LCD_POSY                 16 // y position of lcd
#define UI_LCD_WIDTH                220
#define UI_LCD_HEIGHT               176

#elif defined(IPOD_NANO)
#define UI_TITLE                    "iPod Nano"
#define UI_WIDTH                    199 // width of GUI window
#define UI_HEIGHT                   421 // height of GUI window
/* high-colour */
#define UI_LCD_POSX                 13 // x position of lcd
#define UI_LCD_POSY                 14 // y position of lcd
#define UI_LCD_WIDTH                176
#define UI_LCD_HEIGHT               132

#elif defined(IPOD_VIDEO)
#define UI_TITLE                    "iPod Video"
#define UI_WIDTH                    320 // width of GUI window
#define UI_HEIGHT                   240 // height of GUI window
/* high-colour */
#define UI_LCD_POSX                 0 // x position of lcd
#define UI_LCD_POSY                 0 // y position of lcd
#define UI_LCD_WIDTH                320
#define UI_LCD_HEIGHT               240

#elif defined(ARCHOS_GMINI120)
#define UI_TITLE                    "Gmini 120"
#define UI_WIDTH                    370 // width of GUI window
#define UI_HEIGHT                   264 // height of GUI window
#define UI_LCD_BGCOLOR              90, 145, 90 // bkgnd color of LCD (no backlight)
#define UI_LCD_BGCOLORLIGHT         60, 160, 230 // bkgnd color of LCD (backlight)
#define UI_LCD_BLACK                0, 0, 0 // black
#define UI_LCD_POSX                 85 // x position of lcd
#define UI_LCD_POSY                 61 // y position of lcd (74 for real aspect)
#define UI_LCD_WIDTH                192 // * 1.5
#define UI_LCD_HEIGHT               96  // * 1.5

#elif defined(IAUDIO_X5)
#define UI_TITLE                    "iAudio X5"
#define UI_WIDTH                    300 // width of GUI window
#define UI_HEIGHT                   462 // height of GUI window
#define UI_LCD_BGCOLOR              90, 145, 90 // bkgnd color of LCD (no backlight)
#define UI_LCD_BGCOLORLIGHT         230, 160, 60 // bkgnd color of LCD (backlight)
#define UI_LCD_BLACK                0, 0, 0 // black
#define UI_LCD_POSX                 55 // x position of lcd
#define UI_LCD_POSY                 61 // y position of lcd (74 for real aspect)
#define UI_LCD_WIDTH                LCD_WIDTH // * 1.5
#define UI_LCD_HEIGHT               LCD_HEIGHT  // * 1.5

#define UI_REMOTE_POSX              12  // x position of remote lcd
#define UI_REMOTE_POSY              478 // y position of remote lcd
#define UI_REMOTE_WIDTH             128
#define UI_REMOTE_HEIGHT            64

#define UI_REMOTE_BGCOLORLIGHT      250, 180, 130 // bkgnd of remote lcd (bklight)
#endif

extern SDL_Surface *gui_surface;
extern bool background;  /* True if the background image is enabled */
extern int display_zoom; 

#endif // #ifndef __UISDL_H__

