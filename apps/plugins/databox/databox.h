/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2005 by Michiel van der Kolk 
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#ifndef DATABOX_H
#define DATABOX_H
#include <stdio.h>
#include <stdlib.h>
#include <plugin.h>
#include <database.h>

#include "edittoken.h"
#include "editparser.h"

extern struct plugin_api* rb;

struct print {
#ifdef HAVE_LCD_BITMAP
   struct font *fontfixed;
   int font_w,font_h;
#endif
   int line;
   int position;
};

struct editor {
    struct token *token; /* tokenstream */
    int currentindex; /* index of the token to change.(+1, 1=0 2=1 3=2 etc.) */
    int tokencount; /* total amount of tokens */
    int editingmode; /* defined in databox.h */
    int valid; /* is the current tokenstream valid ? */
};

struct editing {
    int selection_candidates[30]; /* possible options for this selecting */
    struct token old_token; /* only set when selecting, stores old token */
    int currentselection; /* current selection index */
    int selectionmax;
    int selecting; /* boolean */
};

extern struct print printing;
extern struct editor editor;
extern struct editing editing;

#endif
