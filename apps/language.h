#ifndef __LANGUAGE_H
#define __LANGUAGE_H
/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002, 2008 Daniel Stenberg
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

/* Initialize language array with the builtin strings */
void lang_init(const unsigned char *builtin, unsigned char **dest, int count);

/* load a given language file */
int lang_core_load(const char *filename);

int lang_load(const char *filename, const unsigned char *builtin, 
              unsigned char **dest, unsigned char *buffer, 
              unsigned int user_num, int max_lang_size,
              unsigned int max_id);

/* get the ID of an english string so it can be localised */
int lang_english_to_id(const char *english);

/* returns whether the loaded language is a right-to-left language */
int lang_is_rtl(void);
#endif
