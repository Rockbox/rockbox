/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2010 Jonathan Gordon
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

#include "config.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "file.h"
#include "settings.h"
#include "font.h"
#include "skin_buffer.h"

#ifndef _SKINFONTS_H_
#define _SKINFONTS_H_

#if LCD_HEIGHT > 160
#define SKIN_FONT_SIZE (1024*10)
#else
#define SKIN_FONT_SIZE (1024*3)
#endif

void skin_font_init(void);

/* load a font into the skin buffer. return the font id. */
int skin_font_load(char* font_name);

/* unload a skin font. If a font has been loaded more than once it wont actually
 * be unloaded untill all references have been unloaded */
void skin_font_unload(int font_id);
#endif
