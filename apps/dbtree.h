/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2005 by Björn Stenberg
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#ifndef DBTREE_H
#define DBTREE_H

#include "tree.h"

enum table { invalid, allsongs, allalbums, allartists, albums, songs };

int db_init(void);
void db_enter(struct tree_context* c);
void db_exit(struct tree_context* c);
int db_load(struct tree_context* c,
            bool* dir_buffer_full);
#ifdef HAVE_LCD_BITMAP
const char* db_get_icon(struct tree_context* c);
#else
int   db_get_icon(struct tree_context* c);
#endif


#endif

