/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2013 Thomas Martitz
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

#ifndef __LINE_H__
#define __LINE_H__

#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>

#include "lcd.h"
#include "screens.h"

struct line_desc {
    /* height of the line (in pixels). -1 to inherit the height
     * from the font. The text will be centered if the height is larger,
     * but the decorations will span the entire height */
    int height;
    /* multiline support: For some decorations (e.g. gradient) to work
     * across multiple lines (e.g. to draw a line selector across 2 lines)
     * the line index and line count must be known. For normal, single
     * lines specify nlines=1 and line=0 */
    /* line count of a group */
    int16_t nlines;
    /* index of the line in the group */
    int16_t line;
    /* line text color if STYLE_COLORED is specified, in native
     * lcd format (convert with LCD_RGBPACK() if necessary) */
    fb_data text_color;
    /* line color if STYLE_COLORBAR or STYLE_GRADIENT is specified, in native
     * lcd format (convert with LCD_RGBPACK() if necessary) */
    fb_data line_color, line_end_color;
    /* line decorations, see STYLE_DEFAULT etc. */
    int style;
    /* whether the line can scroll */
    bool scroll;
};

/* default initializer, can be used for static initialitation also.
 * This initializer will result in single lines without style that don't scroll */
#define LINE_DESC_DEFINIT { .style = STYLE_DEFAULT, .height = -1, .line = 0, .nlines = 1, .scroll = false }

/**
 * Print a line at a given pixel postion, using decoration information from
 * line and content information from the format specifier. The format specifier
 * can include tags that depend on further parameters given to the function
 * (similar to the well-known printf()).
 *
 * Tags start with the $ sign. Below is a list of the currently supported tags:
 * $s - insert a column (1px wide) of empty space.
 * $S - insert a column (1 char wide) of empty space.
 * $i - insert an icon. put_line() expects a corresponding parameter of the
 *      type 'enum themable_icons'. If Icon_NOICON is passed, then empty
 *      space (icon-wide) will be inserted.
 * $I - insert an icon with padding. Works like $i but additionally
 *      adds a column (1px wide) of empty space on either side of the icon.
 * $t - insert text. put_line() expects a corresponding parameter of the type
 *      'const char *'
 * $$ - insert a '$' char, use it to escape $.
 *
 * $I, $s and $S support the following two forms:
 * $n[IsS] - inserts n columns (pixels/chars respectively) of empty space
 * $*[IsS] - inserts n columns (pixels/chars respectively) of empty space. put_line()
 *           expects a correspinding paramter of the type 'int' that specifies n.
 *
 * $t supports the following two forms:
 * $nt     - skips the first n pixels when displaying the string
 * $*t     - skips the first n pixels when displaying the string. put_line()
 *           expects a correspinding paramter of the type 'int' that specifies n.
 *
 * Inline text will be printed as is and can be freely intermixed with tags,
 * except when the line can scroll. Due to limitations of the scroll engine
 * only the last piece of text (whether inline or via $t) can scroll.
 *
 * x, y - pixel postion of the line.
 * line - holds information for the line decorations
 * fmt  - holds the text and optionally format tags
 * ...  - additional paramters for the format tags
 *
 */
void  put_line(struct screen *display,
               int x, int y, struct line_desc *line,
               const char *fmt, ...);


/**
 * Print a line at a given pixel postion, using decoration information from
 * line and content information from the format specifier. The format specifier
 * can include tags that depend on further parameters given to the function
 * (similar to the well-known vprintf()).
 *
 * For details, see put_line(). This function is equivalent, except for
 * accepting a va_list instead of a variable paramter list.
 */
void vput_line(struct screen *display,
               int x, int y, struct line_desc *line,
               const char *fmt, va_list ap);

#endif /* __LINE_H__*/
