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
#ifndef PRINTCELL_MAX_COLUMNS
#define PRINTCELL_MAX_COLUMNS 16 /* Max 32 (hidecol_flags)*/
#endif

#define PRINTCELL_MAXLINELEN MAX_PATH
#define PC_COL_FLAG(col) ((uint32_t)(col >= 0 \
        && col < PRINTCELL_MAX_COLUMNS) ? 1u<<col : -1u)

#define PRINTCELL_COLUMN_IS_VISIBLE(flag, col) ((flag & PC_COL_FLAG(col)) == 0)
#define PRINTCELL_COLUMN_FLAG(col) (PC_COL_FLAG(col))

struct printcell_settings
{
   bool cell_separator;
   char title_delimeter;
   char text_delimeter;
   uint32_t hidecol_flags;
};

/* Printcell initialization - Sets title and calculates cell widths
*  by default each column is identified by '$' character
*  ex 3 columns title = "Col1$Col2$Col3" also accepts $*WIDTH$
*  ex 3 columns varying width title = "$*64$Col1$*128$Col2$Col3
*  supplying struct printcell_settings pcs allows changing default settings
*  supply NULL to use defaults
*
* Returns number of columns
*/
int printcell_set_columns(struct gui_synclist *gui_list,
                          struct printcell_settings * pcs,
                          char * title, enum themable_icons icon);

/* Sets the printcell function enabled (use after initializing with set_column)
 * Note you should call printcell_enable(false) if the list might be reused */
void printcell_enable(bool enable);

/* Increments the current selected column negative increment is allowed
   returns the selected column
   range: -1(no selection) to ncols - 1 */
int printcell_increment_column(int increment, bool wrap);

/* Return index of the currently selected column (-1 to ncols - 1) */
int printcell_get_column_selected(void);

/* Return the text of currently selected column buffer should be sized
 * for max item len, buf[PRINTCELL_MAXLINELEN] is a safe bet */
char *printcell_get_column_text(int selcol, char *buf, size_t bufsz);

/* Return the text of currently selected column title should be sized
 * for max item len, buf[PRINTCELL_MAXLINELEN] is a safe bet */
char *printcell_get_title_text(int selcol, char *buf, size_t bufsz);


/* Hide or show a specified column - supply col = -1 to affect all columns */
void printcell_set_column_visible(int col, bool visible);

/* Return visibility of a specified column
*  returns (1 visible or 0 hidden)
*  if supply col == -1 a flag with visibility of all columns will be returned
*  NOTE: flag denotes a hidden column by a 1 in the column bit (1 << col#)
*  PRINTCELL_COLUMN_IS_VISIBLE(flag,col) macro will convert to bool
*/
uint32_t printcell_get_column_visibility(int col);
#endif /*_PRINTCELL_LIST_H_*/
