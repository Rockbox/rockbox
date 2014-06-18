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

/* Possible line decoration styles. Specify one of
 * STYLE_NONE, _DEFAULT, _INVERT, _COLORBAR or _GRADIENT, and optionally
 * or with STYLE_COLORED specifying line_desc.text_color */
enum line_styles {
    /* Just print the text. Do not clear or draw line decorations */
    STYLE_NONE       = 0x00,
    /* Line background filled with the bg color (or backdrop if present) */ 
    STYLE_DEFAULT    = 0x01,
    /* Like STYLE_DFEAULT except that text and background color will be swapped */
    STYLE_INVERT     = 0x02,
    /* Line background filled with line color line_desc.line_color */
    STYLE_COLORBAR   = 0x04,
    /* Line background filled with gradient, colors taken from
     * line_desc.line_color and line_desc.line_end_color */
    STYLE_GRADIENT   = 0x08,
    /* Modifier for the text color, which will be taken from line_desc.text_color */
    STYLE_COLORED    = 0x10,
    /* These are used internally */
    _STYLE_DECO_MASK = 0x0f,
    _STYLE_MODE_MASK = 0x7F,
};

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
    unsigned text_color;
    /* line color if STYLE_COLORBAR or STYLE_GRADIENT is specified, in native
     * lcd format (convert with LCD_RGBPACK() if necessary) */
    unsigned line_color, line_end_color;
    /* line decorations, see STYLE_DEFAULT etc. */
    enum line_styles style;
    /* whether the line can scroll */
    bool scroll;
    /* height of the line separator (in pixels). 0 to disable drawing
     * of the separator */
    int8_t separator_height;
};

/* default initializer, can be used for static initialitation also.
 * This initializer will result in single lines without style that don't scroll */
#define LINE_DESC_DEFINIT { .style = STYLE_DEFAULT, .height = -1, .separator_height = 0, .line = 0, .nlines = 1, .scroll = false }

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
 * Inline text will be printed as is (be sure to escape '$') and can be freely
 * intermixed with tags. Inline text will be truncated after MAX_PATH+31 bytes.
 * If you have a longer inline string use a separate buffer and pass that via $t,
 * which does not suffer from this truncation.
 * 
 * Text can appear anywhere, before or after (or both) tags. However, when the
 * line can scroll, only the last piece of text (whether inline or via $t) can
 * scroll. This is due to a scroll_engine limitation.
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
