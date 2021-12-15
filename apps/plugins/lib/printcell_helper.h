/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2021 William Wilgus
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

#ifndef _PRINTCELL_LIST_H_
#define _PRINTCELL_LIST_H_
#define PRINTCELL_MAXLINELEN MAX_PATH

/*  sets the printcell function enabled */ 
void  printcell_enable(struct gui_synclist *gui_list, bool enable, bool separator);

/* sets title and calculates cell widths each column is identified by '$' character
 ex 3 columns title = "Col1$Col2$Col3" also accepts $*WIDTH$
 ex 3 columns varying width title = "$*64$Col1$*128$Col2$Col3
 returns number of columns
*/
int   printcell_set_columns(struct gui_synclist *gui_list,
                            char * title, enum themable_icons icon);

/* increments the current selected column negative increment is allowed
   returns the selected column 
   range: -1(no selection) to ncols - 1 */
int   printcell_increment_column(struct gui_synclist *gui_list, int increment, bool wrap);

/* return the text of currently selected column buffer should be sized
 * for max item len, buf[PRINTCELL_MAXLINELEN] is a safe bet */
char *printcell_get_selected_column_text(struct gui_synclist *gui_list, char *buf, size_t bufsz);
#endif /*_PRINTCELL_LIST_H_*/
