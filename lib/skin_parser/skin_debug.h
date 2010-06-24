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


#ifndef SKIN_DEBUG_H
#define SKIN_DEBUG_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "skin_parser.h"
#ifndef ROCKBOX
/* Debugging functions */
void skin_error(enum skin_errorcode error);
int skin_error_line(void);
char* skin_error_message(void);
void skin_clear_errors(void);
void skin_debug_tree(struct skin_element* root);

/* Auxiliary debug functions */
void skin_debug_params(int count, struct skin_tag_parameter params[]);
void skin_debug_indent(void);
#else

#define skin_error(...)
#define skin_clear_errors()

#endif /* !ROCKBOX */


#ifdef __cplusplus
}
#endif

#endif // SKIN_DEBUG_H
