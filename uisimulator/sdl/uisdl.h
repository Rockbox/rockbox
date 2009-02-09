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
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#ifndef __UISDL_H__
#define __UISDL_H__

#include <stdbool.h>
#include <SDL.h>

/* colour definitions are R, G, B */

#if defined(ARCHOS_RECORDER)
#define UI_TITLE                    "Jukebox Recorder"
#define UI_WIDTH                    270 /* width of GUI window */
#define UI_HEIGHT                   406 /* height of GUI window */
#define UI_LCD_POSX                 80 /* x position of lcd */
#define UI_LCD_POSY                 104 /* y position of lcd */

#elif defined(ARCHOS_PLAYER)
#define UI_TITLE                    "Jukebox Player"
#define UI_WIDTH                    284 /* width of GUI window */
#define UI_HEIGHT                   420 /* height of GUI window */
#define UI_LCD_POSX                 75 /* x position of lcd */
#define UI_LCD_POSY                 116 /* y position of lcd */

#elif defined(ARCHOS_FMRECORDER) || defined(ARCHOS_RECORDERV2)
#define UI_TITLE                    "Jukebox FM Recorder"
#define UI_WIDTH                    285 /* width of GUI window */
#define UI_HEIGHT                   414 /* height of GUI window */
#define UI_LCD_POSX                 87 /* x position of lcd */
#define UI_LCD_POSY                 77 /* y position of lcd */

#elif defined(ARCHOS_ONDIOSP) || defined(ARCHOS_ONDIOFM)
#define UI_TITLE                    "Ondio"
#define UI_WIDTH                    155 /* width of GUI window */
#define UI_HEIGHT                   334 /* height of GUI window */
#define UI_LCD_POSX                 21 /* x position of lcd */
#define UI_LCD_POSY                 82 /* y position of lcd */

#elif defined(IRIVER_H120) || defined(IRIVER_H100)
#define UI_TITLE                    "iriver H1x0"
#define UI_WIDTH                    379 /* width of GUI window */
#define UI_HEIGHT                   508 /* height of GUI window */
#define UI_LCD_POSX                 109 /* x position of lcd */
#define UI_LCD_POSY                 23 /* y position of lcd */
#define UI_REMOTE_POSX              50  /* x position of remote lcd */
#define UI_REMOTE_POSY              403 /* y position of remote lcd */

#elif defined(IRIVER_H300)
#define UI_TITLE                    "iriver H300"
#define UI_WIDTH                    288 /* width of GUI window */
#define UI_HEIGHT                   581 /* height of GUI window */
#define UI_LCD_POSX                 26 /* x position of lcd */
#define UI_LCD_POSY                 36 /* y position of lcd */
#define UI_REMOTE_POSX              12  /* x position of remote lcd */
#define UI_REMOTE_POSY              478 /* y position of remote lcd */

#elif defined(IPOD_1G2G)
#define UI_TITLE                    "iPod 1G/2G"
#define UI_WIDTH                    224 /* width of GUI window */
#define UI_HEIGHT                   382 /* height of GUI window */
#define UI_LCD_POSX                 32 /* x position of lcd */
#define UI_LCD_POSY                 12 /* y position of lcd */

#elif defined(IPOD_3G)
#define UI_TITLE                    "iPod 3G"
#define UI_WIDTH                    218 /* width of GUI window */
#define UI_HEIGHT                   389 /* height of GUI window */
#define UI_LCD_POSX                 29 /* x position of lcd */
#define UI_LCD_POSY                 16 /* y position of lcd */

#elif defined(IPOD_4G)
#define UI_TITLE                    "iPod 4G"
#define UI_WIDTH                    196 /* width of GUI window */
#define UI_HEIGHT                   370 /* height of GUI window */
#define UI_LCD_POSX                 19 /* x position of lcd */
#define UI_LCD_POSY                 14 /* y position of lcd */

#elif defined(IPOD_MINI) || defined(IPOD_MINI2G)
#define UI_TITLE                    "iPod mini"
#define UI_WIDTH                    191 /* width of GUI window */
#define UI_HEIGHT                   365 /* height of GUI window */
#define UI_LCD_POSX                 24 /* x position of lcd */
#define UI_LCD_POSY                 17 /* y position of lcd */

#elif defined(IPOD_COLOR)
#define UI_TITLE                    "iPod Color"
#define UI_WIDTH                    261 /* width of GUI window */
#define UI_HEIGHT                   493 /* height of GUI window */
#define UI_LCD_POSX                 21 /* x position of lcd */
#define UI_LCD_POSY                 16 /* y position of lcd */

#elif defined(IPOD_NANO)
#define UI_TITLE                    "iPod Nano"
#define UI_WIDTH                    199 /* width of GUI window */
#define UI_HEIGHT                   421 /* height of GUI window */
#define UI_LCD_POSX                 13 /* x position of lcd */
#define UI_LCD_POSY                 14 /* y position of lcd */

#elif defined(IPOD_VIDEO)
#define UI_TITLE                    "iPod Video"
#define UI_WIDTH                    350 /* width of GUI window */
#define UI_HEIGHT                   591 /* height of GUI window */
#define UI_LCD_POSX                 14 /* x position of lcd */
#define UI_LCD_POSY                 12 /* y position of lcd */

#elif defined(IAUDIO_X5)
#define UI_TITLE                    "iAudio X5"
#define UI_WIDTH                    300 /* width of GUI window */
#define UI_HEIGHT                   558 /* height of GUI window */
#define UI_LCD_POSX                 55 /* x position of lcd */
#define UI_LCD_POSY                 61 /* y position of lcd */
#define UI_REMOTE_POSX              12  /* x position of remote lcd */
#define UI_REMOTE_POSY              462 /* y position of remote lcd */

#elif defined(IAUDIO_M5)
#define UI_TITLE                    "iAudio M5"
#define UI_WIDTH                    374 /* width of GUI window */
#define UI_HEIGHT                   650 /* height of GUI window */
#define UI_LCD_POSX                 82 /* x position of lcd */
#define UI_LCD_POSY                 74 /* y position of lcd */
#define UI_REMOTE_POSX              59  /* x position of remote lcd */
#define UI_REMOTE_POSY              509 /* y position of remote lcd */

#elif defined(IAUDIO_M3)
#define UI_TITLE                    "iAudio M3"
#define UI_WIDTH                    397 /* width of GUI window */
#define UI_HEIGHT                   501 /* height of GUI window */
#define UI_LCD_POSX                 92  /* x position of lcd */
#define UI_LCD_POSY                 348 /* y position of lcd */

#elif defined(GIGABEAT_F)
#define UI_TITLE                    "Toshiba Gigabeat"
#define UI_WIDTH                    401 /* width of GUI window */
#define UI_HEIGHT                   655 /* height of GUI window */
#define UI_LCD_POSX                 48 /* x position of lcd */
#define UI_LCD_POSY                 60 /* y position of lcd */

#elif defined(GIGABEAT_S)
#define UI_TITLE                    "Toshiba Gigabeat"
#define UI_WIDTH                    450 /* width of GUI window */
#define UI_HEIGHT                   688 /* height of GUI window */
#define UI_LCD_POSX                 96 /* x position of lcd */
#define UI_LCD_POSY                 90 /* y position of lcd */

#elif defined(MROBE_500)
#define UI_TITLE                    "Olympus M:Robe 500"
#define UI_WIDTH                    401 /* width of GUI window */
#define UI_HEIGHT                   655 /* height of GUI window */
#define UI_LCD_POSX                 48 /* x position of lcd */
#define UI_LCD_POSY                 60 /* y position of lcd */
#define UI_REMOTE_POSX              50  /* x position of remote lcd */
#define UI_REMOTE_POSY              403 /* y position of remote lcd */

#elif defined(IRIVER_H10)
#define UI_TITLE                    "iriver H10 20Gb"
#define UI_WIDTH                    392 /* width of GUI window */
#define UI_HEIGHT                   391 /* height of GUI window */
#define UI_LCD_POSX                 111 /* x position of lcd */
#define UI_LCD_POSY                 30 /* y position of lcd */

#elif defined(IRIVER_H10_5GB)
#define UI_TITLE                    "iriver H10 5/6Gb"
#define UI_WIDTH                    353 /* width of GUI window */
#define UI_HEIGHT                   460 /* height of GUI window */
#define UI_LCD_POSX                 112 /* x position of lcd */
#define UI_LCD_POSY                 45  /* y position of lcd */

#elif defined(SANSA_E200) || defined(SANSA_E200V2)
#ifdef SANSA_E200
#define UI_TITLE                    "Sansa e200"
#else
#define UI_TITLE                    "Sansa e200v2"
#endif
#define UI_WIDTH                    260 /* width of GUI window */
#define UI_HEIGHT                   502 /* height of GUI window */
#define UI_LCD_POSX                 42 /* x position of lcd */
#define UI_LCD_POSY                 37  /* y position of lcd */

#elif defined(SANSA_C200)
#define UI_TITLE                    "Sansa c200"
#define UI_WIDTH                    350 /* width of GUI window */
#define UI_HEIGHT                   152 /* height of GUI window */
#define UI_LCD_POSX                 42 /* x position of lcd */
#define UI_LCD_POSY                 35  /* y position of lcd */

#elif defined(IRIVER_IFP7XX)
#define UI_TITLE                    "iriver iFP7xx"
#define UI_WIDTH                    425 /* width of GUI window */
#define UI_HEIGHT                   183 /* height of GUI window */
#define UI_LCD_POSX                 115 /* x position of lcd */
#define UI_LCD_POSY                 54 /* y position of lcd */

#elif defined(ARCHOS_AV300)
#define UI_TITLE                    "Archos AV300"
/* We are temporarily using a 2bpp LCD driver and dummy bitmap */
#define UI_WIDTH                    420 /* width of GUI window */
#define UI_HEIGHT                   340 /* height of GUI window */
#define UI_LCD_POSX                 50 /* x position of lcd */
#define UI_LCD_POSY                 50 /* y position of lcd */

#elif defined(MROBE_100)
#define UI_TITLE                    "Olympus M:Robe 100"
#define UI_WIDTH                    247 /* width of GUI window */
#define UI_HEIGHT                   416 /* height of GUI window */
#define UI_LCD_POSX                 43 /* x position of lcd */
#define UI_LCD_POSY                 25 /* y position of lcd */

#elif defined(COWON_D2)
#define UI_TITLE                    "Cowon D2"
#define UI_WIDTH                    472 /* width of GUI window */
#define UI_HEIGHT                   368 /* height of GUI window */
#define UI_LCD_POSX                 58 /* x position of lcd */
#define UI_LCD_POSY                 67 /* y position of lcd */

#elif defined(IAUDIO_7)
#define UI_TITLE                    "iAudio7"
#define UI_WIDTH                    494 /* width of GUI window */
#define UI_HEIGHT                   214 /* height of GUI window */
#define UI_LCD_POSX                 131 /* x position of lcd */
#define UI_LCD_POSY                 38 /* y position of lcd */

#elif defined(CREATIVE_ZVM) || defined(CREATIVE_ZVM60GB)
#ifdef CREATIVE_ZVM
 #define UI_TITLE                    "Creative Zen Vision:M 30GB"
#else
 #define UI_TITLE                    "Creative Zen Vision:M 60GB"
#endif
#define UI_WIDTH                    383 /* width of GUI window */
#define UI_HEIGHT                   643 /* height of GUI window */
#define UI_LCD_POSX                 31 /* x position of lcd */
#define UI_LCD_POSY                 62 /* y position of lcd */

#elif defined(CREATIVE_ZV)
#define UI_TITLE                    "Creative Zen Vision"
#define UI_WIDTH                    1054 /* width of GUI window */
#define UI_HEIGHT                   643 /* height of GUI window */
#define UI_LCD_POSX                 129 /* x position of lcd */
#define UI_LCD_POSY                 85 /* y position of lcd */

#elif defined(MEIZU_M6SL)
#define UI_TITLE                    "Meizu M6"
#define UI_WIDTH                    512 /* width of GUI window */
#define UI_HEIGHT                   322 /* height of GUI window */
#define UI_LCD_POSX                 39 /* x position of lcd */
#define UI_LCD_POSY                 38 /* y position of lcd */

#elif defined(SANSA_FUZE)
#define UI_TITLE                    "Sansa Fuze"
#define UI_WIDTH                    279 /* width of GUI window */
#define UI_HEIGHT                   449 /* height of GUI window */
#define UI_LCD_POSX                 30 /* x position of lcd */
#define UI_LCD_POSY                 31 /* y position of lcd */

#elif defined(SANSA_CLIP)
#define UI_TITLE                    "Sansa Clip"
#define UI_WIDTH                    205 /* width of GUI window */
#define UI_HEIGHT                   325 /* height of GUI window */
#define UI_LCD_POSX                 38 /* x position of lcd */
#define UI_LCD_POSY                 38 /* y position of lcd */

#elif defined(PHILIPS_HDD1630)
#define UI_TITLE                    "Philips GoGear HDD1630"
#define UI_WIDTH                    407 /* width of GUI window */
#define UI_HEIGHT                   391 /* height of GUI window */
#define UI_LCD_POSX                 143 /* x position of lcd */
#define UI_LCD_POSY                 27   /* y position of lcd */

#elif defined(SANSA_M200V4)
#define UI_TITLE                    "sansa m200v4"
#define UI_WIDTH                    350 /* width of GUI window */
#define UI_HEIGHT                   168 /* height of GUI window */
#define UI_LCD_POSX                 42 /* x position of lcd */
#define UI_LCD_POSY                 55 /* y position of lcd */


#else
#error no UI defines
#endif
extern SDL_Surface *gui_surface;
extern bool background;  /* True if the background image is enabled */
extern int display_zoom; 

#endif /* #ifndef __UISDL_H__ */

