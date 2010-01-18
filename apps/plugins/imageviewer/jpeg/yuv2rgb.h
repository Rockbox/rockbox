/***************************************************************************
*             __________               __   ___.
*   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
*   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
*   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
*   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
*                     \/            \/     \/    \/            \/
* $Id$
*
* JPEG image viewer
* (This is a real mess if it has to be coded in one single C file)
*
* File scrolling addition (C) 2005 Alexander Spyridakis
* Copyright (C) 2004 Jörg Hohensohn aka [IDC]Dragon
* Heavily borrowed from the IJG implementation (C) Thomas G. Lane
* Small & fast downscaling IDCT (C) 2002 by Guido Vollbeding  JPEGclub.org
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

#ifndef _JPEG_YUV2RGB_H
#define _JPEG_YUV2RGB_H

enum color_modes
{
    COLOURMODE_COLOUR = 0,
    COLOURMODE_GRAY,
    COLOUR_NUM_MODES
};

enum dither_modes
{
    DITHER_NONE = 0,    /* No dithering */
    DITHER_ORDERED,     /* Bayer ordered */
    DITHER_DIFFUSION,   /* Floyd/Steinberg error diffusion */
    DITHER_NUM_MODES
};

void yuv_bitmap_part(unsigned char *src[3], int csub_x, int csub_y,
                     int src_x, int src_y, int stride,
                     int x, int y, int width, int height,
                     int colour_mode, int dither_mode);

#endif
