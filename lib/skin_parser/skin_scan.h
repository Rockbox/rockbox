/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2010 Robert Bieber
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

#ifndef SCANNING_H
#define SCANNING_H

#ifdef __cplusplus
extern "C"
{
#endif


/* Scanning functions */
void skip_tag(const char** document);
void skip_comment(const char** document);
void skip_arglist(const char** document);
void skip_enumlist(const char** document);
char* scan_string(const char** document);
int scan_int(const char** document);
int check_viewport(const char* document); /* Checks for a viewport declaration */

#ifdef __cplusplus
}
#endif

#endif // SCANNING_H
