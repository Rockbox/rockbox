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
#include "skin_fonts.h"

static struct skin_font_info {
    struct font font;
    int font_id;
    char name[MAX_PATH];
    char *buffer;
    int ref_count; /* how many times has this font been loaded? */
} font_table[MAXUSERFONTS];

/* need this to know if we should be closing font fd's on the next init */
static bool first_load = true;

void skin_font_init(void)
{
    int i;
    for(i=0;i<MAXUSERFONTS;i++)
    {
        if (!first_load)
            font_unload(font_table[i].font_id);
        font_table[i].font_id = -1;
        font_table[i].name[0] = '\0';
        font_table[i].buffer = NULL;
        font_table[i].ref_count = 0;
    }
    first_load = false;
}

/* load a font into the skin buffer. return the font id. */
int skin_font_load(char* font_name)
{
    int i;
    struct font *pf;
    struct skin_font_info *font = NULL;
    char filename[MAX_PATH];
    
    if (!strcmp(font_name, global_settings.font_file))
        return FONT_UI;
#ifdef HAVE_REMOTE_LCD
    if (!strcmp(font_name, global_settings.remote_font_file))
        return FONT_UI_REMOTE;
#endif
    for(i=0;i<MAXUSERFONTS;i++)
    {
        if (font_table[i].font_id >= 0 && !strcmp(font_table[i].name, font_name))
        {
            font_table[i].ref_count++;
            return font_table[i].font_id;
        }
        else if (!font && font_table[i].font_id == -1)
        {
            font = &font_table[i];
        }
    }
    if (!font)
        return -1; /* too many fonts loaded */
    
    pf = &font->font;
    if (!font->buffer)
    {
        pf->buffer_start = skin_buffer_alloc(SKIN_FONT_SIZE);
        if (!pf->buffer_start)
            return -1;
        font->buffer = pf->buffer_start;
    }
    else
    {
        pf->buffer_start = font->buffer;
    }
    pf->buffer_size = SKIN_FONT_SIZE;
    
    snprintf(filename, MAX_PATH, FONT_DIR "/%s.fnt", font_name);
    strcpy(font->name, font_name);
    
    pf->fd = -1;
    font->font_id = font_load(pf, filename);
    
    if (font->font_id < 0)
        return -1;
    font->ref_count = 1;
    
    return font->font_id;
}

/* unload a skin font. If a font has been loaded more than once it wont actually
 * be unloaded untill all references have been unloaded */
void skin_font_unload(int font_id)
{
    int i;
    for(i=0;i<MAXUSERFONTS;i++)
    {
        if (font_table[i].font_id == font_id)
        {
            if (--font_table[i].ref_count == 0)
            {
                font_unload(font_id);
                font_table[i].font_id = -1;
                font_table[i].name[0] = '\0';
            }
            return;
        }
    }
}
