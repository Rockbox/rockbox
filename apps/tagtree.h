/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2005 by Miika Pekkarinen
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#ifndef _TAGTREE_H
#define _TAGTREE_H

#include "tagcache.h"
#include "tree.h"

enum table { 
    invalid, root, navibrowse,
    chunked_next };

void tagtree_init(void);
int tagtree_enter(struct tree_context* c);
void tagtree_exit(struct tree_context* c);
int tagtree_load(struct tree_context* c);
#ifdef HAVE_LCD_BITMAP
const char* tagtree_get_icon(struct tree_context* c);
#else
int   tagtree_get_icon(struct tree_context* c);
#endif
int tagtree_get_filename(struct tree_context* c, char *buf, int buflen);

#endif

