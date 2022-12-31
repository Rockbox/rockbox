/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2021 by William Wilgus
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
/* spreadsheet cells for rockbox lists */
#include "plugin.h"
#include "lib/printcell_helper.h"

#ifndef PRINTCELL_MAX_COLUMNS
#define PRINTCELL_MAX_COLUMNS 16
#endif

#define COLUMN_ENDLEN 3
#define TITLE_FLAG 0xFF
#define SELECTED_FLAG 0x1

#if LCD_DEPTH == 1
#define BAR_WIDTH (1)
#else
#define BAR_WIDTH (COLUMN_ENDLEN)
#endif

struct printcell_info_t {
    int  offw[NB_SCREENS];
    int  iconw[NB_SCREENS];
    int  selcol_offw[NB_SCREENS];
    int  totalcolw[NB_SCREENS];
    int  firstcolxw[NB_SCREENS];
    uint16_t colw[NB_SCREENS][PRINTCELL_MAX_COLUMNS];
    int  ncols;
    int  selcol;
    int  selcol_index;
    char title[PRINTCELL_MAXLINELEN];
    bool separator;
};
static struct printcell_info_t printcell;

static void parse_dsptext(int ncols, const char *dsp_text, char* buffer, uint16_t *sidx)
{
    /*Internal function loads sidx with split offsets indexing
      the buffer of null terminated strings, splits on '$'
      _assumptions_:
      dsp_text[len - 1] = \0,
      buffer[PRINTCELL_MAXLINELEN],
      sidx[PRINTCELL_MAX_COLUMNS]
    */
    int i = 0;
    int j = 0;
    int ch = '$'; /* first column $ is optional */
    if (*dsp_text == '$')
        dsp_text++;
    /* add null to the start of the text buffer */
    buffer[j++] = '\0';
    do
    {
        if (ch == '$')
        {
            sidx[i] = j;
            if (*dsp_text == '$') /* $$ escaped user must want to display $*/
                buffer[j++] = *dsp_text++;
            while (*dsp_text != '\0' && *dsp_text != '$' && j < PRINTCELL_MAXLINELEN - 1)
                buffer[j++] = *dsp_text++;
            buffer[j++] = '\0';
            i++;
            if (i >= ncols || j >= (PRINTCELL_MAXLINELEN - 1))
                break;
        }
        ch = *dsp_text++;
    } while (ch != '\0');
    while (i < ncols)
        sidx[i++] = 0; /* point to  null */
}

static void draw_selector(struct screen *display, struct line_desc *linedes,
                          int selected_flag, int selected_col,
                          int separator_height, int x, int y, int w, int h)
{
    /* Internal function draws the currently selected items row & column styling */
    if (!(separator_height > 0 || (selected_flag & SELECTED_FLAG)))
        return;
    y--;
    h++;
    int linestyle = linedes->style & _STYLE_DECO_MASK;
    bool invert = (selected_flag == SELECTED_FLAG && linestyle >= STYLE_COLORBAR);
    if (invert || (linestyle & STYLE_INVERT) == STYLE_INVERT)
    {
        display->set_drawmode(DRMODE_SOLID | DRMODE_INVERSEVID);
    }

    if (selected_col == printcell.selcol)
    {
        if (selected_flag & SELECTED_FLAG)
        {
            /* expand left and right bars to show selected column */
            display->fillrect(x, y, BAR_WIDTH, h);
            display->fillrect(x + w - BAR_WIDTH + 1, y, BAR_WIDTH, h);
            display->set_drawmode(DRMODE_FG);
        }
        else
        {
            /* only draw left and right bars */
            display->drawrect(x + 1, y, 1, h);
            display->drawrect(x + w - 1, y, 1, h);
            return;
        }
    }
    else if (printcell.selcol < 0)
    {
        if (selected_flag == SELECTED_FLAG)
        {
            if (selected_col > 0)
                x--;
            w++;
        }
    }
    /* draw whole rect outline */
    display->drawrect(x + 1, y, w - 1, h);
}

static inline void set_cell_width(struct viewport *vp, int max_w, int new_w)
{
    /* Internal function sets cell width if less than the max width */
    if (new_w > max_w)
        vp->width = max_w;
    else
        vp->width = new_w;
    vp->width -= COLUMN_ENDLEN;
}

static inline int printcells(struct screen *display, char* buffer,
                             uint16_t *sidx, struct line_desc *linedes,
                             struct viewport *vp, int vp_w, int separator,
                             int x, int y, int offw, int selected_flag, bool scroll)
{
    /* Internal function prints remaining cells */
    int text_offset = offw + offw;
    int ncols = printcell.ncols;
    int screen = display->screen_type;
    int height = linedes->height;
    int selsep = (selected_flag == 0) ? 0: separator;
    uint16_t *screencolwidth = printcell.colw[screen];

    for(int i = 1; i < ncols; i++)
    {
        int ny = y;
        int nw = screencolwidth[i] + text_offset;
        int nx = x + nw;
        char *buftext;
        if (nx > 0 && x < vp_w)
        {
            set_cell_width(vp, vp_w, nx);

            if (i == printcell.selcol)
            {
                linedes->scroll = (selected_flag > 0);
                linedes->separator_height = selsep;
            }
            else
            {
                linedes->scroll = scroll;
                linedes->separator_height = separator;
            }
            buftext = &buffer[sidx[i]];
            display->put_line(x + offw, ny, linedes, "$t", buftext);
            vp->width += COLUMN_ENDLEN + 1;
            draw_selector(display, linedes, selected_flag, i, separator, x, ny, nw, height);
        }
        x = nx;
    }
    return x;
}

static inline int calcvisible(int screen, int vp_w, int text_offset, int sbwidth)
{
    /* Internal function determine how many of the previous colums can be shown */
    uint16_t *screencolwidth = printcell.colw[screen];
    int screenicnwidth = printcell.iconw[screen];
    int offset = 0;
    int selcellw = 0;
    if (printcell.selcol >= 0)
        selcellw = screencolwidth[printcell.selcol] + text_offset;
    int maxw = vp_w - (sbwidth + selcellw + 1);

    for (int i = printcell.selcol - 1; i >= 0; i--)
    {
        int cw = screencolwidth[i] + text_offset;
        if (i == 0)
            cw += screenicnwidth;
        if (offset > 0 || cw > maxw)
            offset += cw; /* can not display this cell -- everything left goes here too */
        else
            maxw -= cw; /* can display this cell subtract from the max width */
    }
    return offset;
}

static void printcell_listdraw_fn(struct list_putlineinfo_t *list_info)
{
/* Internal function callback from the list, draws items as they are requested */
#define ICON_PADDING 1
#define ICON_PADDING_S "1"
    struct screen *display = list_info->display;
    int screen = display->screen_type;
    int col_offset_width = printcell.offw[screen];
    int text_offset = col_offset_width + col_offset_width;

    static char printcell_buffer[PRINTCELL_MAXLINELEN];
    static uint16_t sidx[PRINTCELL_MAX_COLUMNS]; /*indexes zero terminated strings in buffer*/

    struct gui_synclist *list = list_info->list;
    int offset_pos = list_info->list->offset_position[screen];
    int x = list_info->x - offset_pos;
    int y = list_info->y;
    int line_indent = list_info->item_indent;
    int item_offset = list_info->item_offset;
    int icon = list_info->icon;
    int icon_w = list_info->icon_width;
    bool is_title = list_info->is_title;
    bool is_selected = list_info->is_selected;
    bool show_cursor = list_info->show_cursor;
    bool have_icons = list_info->have_icons;
    struct line_desc *linedes = list_info->linedes;
    const char *dsp_text = list_info->dsp_text;
    struct viewport *vp = list_info->vp;
    int line = list_info->line;

    int selected_flag = ((is_selected || is_title) ?
                         (is_title ? TITLE_FLAG : SELECTED_FLAG) : 0);
    bool scroll_items = ((selected_flag == TITLE_FLAG) ||
                  (printcell.selcol < 0 && selected_flag > 0));

    /* save for restore */
    int vp_w = vp->width;
    int saved_separator_height = linedes->separator_height;
    bool saved_scroll = linedes->scroll;

    linedes->separator_height = 0;
    int separator = saved_separator_height;

    if (printcell.separator || (selected_flag & SELECTED_FLAG))
        separator = 1;

    int nx = x;
    int nw, colxw;

    printcell_buffer[0] = '\0';
    parse_dsptext(printcell.ncols, dsp_text, printcell_buffer, sidx);
    char *buftext = &printcell_buffer[sidx[0]];
    uint16_t *screencolwidth = printcell.colw[screen];

    if (is_title)
    {
        int sbwidth = 0;
        if (rb->global_settings->scrollbar == SCROLLBAR_LEFT)
            sbwidth = rb->global_settings->scrollbar_width;

        printcell.iconw[screen] = have_icons ? ICON_PADDING + icon_w : 0;

        if (printcell.selcol_offw[screen] == 0 && printcell.selcol > 0)
            printcell.selcol_offw[screen] = calcvisible(screen, vp_w, text_offset, sbwidth);

        nx -= printcell.selcol_offw[screen];

        nw = screencolwidth[0] + printcell.iconw[screen] + text_offset;
        nw += sbwidth;

        colxw = nx + nw;
        printcell.firstcolxw[screen] = colxw; /* save position of first column for subsequent items */

        if (colxw > 0)
        {
            set_cell_width(vp, vp_w, colxw);
            linedes->separator_height = separator;

            if (have_icons)
            {
                display->put_line(nx + (COLUMN_ENDLEN/2), y, linedes,
                                  "$"ICON_PADDING_S"I$t", icon, buftext);
            }
            else
            {
                display->put_line(nx + col_offset_width, y, linedes, "$t", buftext);
            }
        }
    }
    else
    {
        int cursor = Icon_NOICON;
        nx -= printcell.selcol_offw[screen];

        if (selected_flag & SELECTED_FLAG)
        {
            if (printcell.selcol >= 0)
                printcell.selcol_index = sidx[printcell.selcol]; /* save the item offset*/
            else
                printcell.selcol_index = -1;

            cursor = Icon_Cursor;
            /* limit length of selection if columns don't reach end */
            int maxw = nx + printcell.totalcolw[screen] + printcell.iconw[screen];
            maxw += text_offset * printcell.ncols;
            if (vp_w > maxw)
                vp->width = maxw;
            /* display a blank line first to draw selector across all cells */
            display->put_line(x, y, linedes, "$t", "");
        }

        //nw = screencolwidth[0] + printcell.iconw[screen] + text_offset;
        colxw = printcell.firstcolxw[screen] - vp->x; /* match title spacing */
        nw = colxw - nx;
        if (colxw > 0)
        {
            set_cell_width(vp, vp_w, colxw);
            if (printcell.selcol == 0 && selected_flag == 0)
                linedes->separator_height = 0;
            else
            {
                linedes->scroll = printcell.selcol == 0 || scroll_items;
                linedes->separator_height = separator;
            }
            if (show_cursor && have_icons)
            {
            /* the list can have both, one of or neither of cursor and item icons,
             * if both don't apply icon padding twice between the icons */
                display->put_line(nx, y,
                        linedes, "$*s$"ICON_PADDING_S"I$i$"ICON_PADDING_S"s$*t",
                        line_indent, cursor, icon, item_offset, buftext);
            }
            else if (show_cursor || have_icons)
            {
                display->put_line(nx, y, linedes, "$*s$"ICON_PADDING_S"I$*t", line_indent,
                        show_cursor ? cursor:icon, item_offset, buftext);
            }
            else
            {
                display->put_line(nx + col_offset_width, y, linedes,
                                  "$*s$*t", line_indent, item_offset, buftext);
            }
        }
    }

    if (colxw > 0) /* draw selector for first column (title or items) */
    {
        vp->width += COLUMN_ENDLEN + 1;
        draw_selector(display, linedes, selected_flag, 0,
                      separator, nx, y, nw, linedes->height);
    }
    nx += nw;
    /* display remaining cells */
    printcells(display, printcell_buffer, sidx, linedes, vp, vp_w, separator,
               nx, y, col_offset_width, selected_flag, scroll_items);

    /* draw a line at the bottom of the list */
    if (separator > 0 && line == list->nb_items - 1)
        display->hline(x, LCD_WIDTH, y + linedes->height - 1);

    /* restore settings */
    linedes->scroll = saved_scroll;
    linedes->separator_height = saved_separator_height;
    display->set_drawmode(DRMODE_FG);
    vp->width = vp_w;
}

void printcell_enable(struct gui_synclist *gui_list, bool enable, bool separator)
{
    printcell.separator = separator;
#ifdef HAVE_LCD_COLOR
    static int list_sep_color = INT_MIN;
    if (enable)
    {
        list_sep_color = rb->global_settings->list_separator_color;
        rb->global_settings->list_separator_color = rb->global_settings->fg_color;
        gui_list->callback_draw_item = printcell_listdraw_fn;
    }
    else
    {
        gui_list->callback_draw_item = NULL;
        if (list_sep_color != INT_MIN)
            rb->global_settings->list_separator_color = list_sep_color;
        list_sep_color = INT_MIN;
    }
#else
    if (enable)
        gui_list->callback_draw_item = printcell_listdraw_fn;
    else
        gui_list->callback_draw_item = NULL;
#endif

}

int printcell_increment_column(struct gui_synclist *gui_list, int increment, bool wrap)
{
    int item = printcell.selcol + increment;
    int imin = -1;
    int imax = printcell.ncols - 1;
    if(wrap)
    {
        imin =  imax;
        imax = -1;
    }

    if (item < -1)
        item = imin;
    else if (item >= printcell.ncols)
        item = imax;

    if (item != printcell.selcol)
    {
        FOR_NB_SCREENS(n) /* offset needs recalculated */
            printcell.selcol_offw[n] = 0;
        printcell.selcol = item;
        printcell.selcol_index = 0;
        rb->gui_synclist_draw(gui_list);
    }
    return item;
}

int printcell_set_columns(struct gui_synclist *gui_list,
                           char * title, enum themable_icons icon)
{
    if (title == NULL)
        title = "$PRINTCELL NOT SETUP";
    uint16_t sidx[PRINTCELL_MAX_COLUMNS]; /* starting position of column in title string */
    int width, height, user_minwidth;
    int i = 0;
    int j = 0;
    int ch = '$'; /* first column $ is optional */

    rb->memset(&printcell, 0, sizeof(struct printcell_info_t));

    FOR_NB_SCREENS(n)
    {
        rb->screens[n]->getstringsize("W", &width, &height);
        printcell.offw[n] = width; /* set column text offset */
    }

    if (*title == '$')
        title++;
    do
    {
        if (ch == '$')
        {
            printcell.title[j++] = ch;
            user_minwidth = 0;
            if (*title == '*')/* user wants a minimum size for this column */
            {
                char *dspst = title++; /* store starting position in case this is wrong */
                while(isdigit(*title))
                {
                    user_minwidth = 10*user_minwidth + *title - '0';
                    title++;
                }
                if (*title != '$') /* user forgot $ or wants to display '*' */
                {
                    title = dspst;
                    user_minwidth = 0;
                }
                else
                    title++;
            }

            sidx[i] = j;
            if (*title == '$') /* $$ escaped user must want to display $*/
                printcell.title[j++] = *title++;

            while (*title != '\0' && *title != '$' && j < PRINTCELL_MAXLINELEN - 1)
                printcell.title[j++] = *title++;

            FOR_NB_SCREENS(n)
            {
                rb->screens[n]->getstringsize(&printcell.title[sidx[i]], &width, &height);
                if (width < user_minwidth)
                    width = user_minwidth;
                printcell.colw[n][i] = width;
                printcell.totalcolw[n] += width;
            }
            if (++i >= PRINTCELL_MAX_COLUMNS - 1)
                break;
        }
        ch = *title++;
    } while (ch != '\0');
    printcell.ncols = i;
    printcell.title[j] = '\0';
    printcell.selcol = -1;
    printcell.selcol_index = 0;

    rb->gui_synclist_set_title(gui_list, printcell.title, icon);
    return printcell.ncols;
}

char *printcell_get_selected_column_text(struct gui_synclist *gui_list, char *buf, size_t bufsz)
{
    int selected = gui_list->selected_item;
    int index = printcell.selcol_index - 1;

    if (index < 0)
        index = 0;
    char *bpos;

    if (gui_list->callback_get_item_name(selected, gui_list->data, buf, bufsz) == buf)
    {
        bpos = &buf[index];
        if (printcell.selcol < 0) /* return entire string incld col formatting '$'*/
            return bpos;
        while(bpos < &buf[bufsz - 1])
        {
            if (*bpos == '$' || *bpos == '\0')
                goto success;
            bpos++;
        }
    }
/*failure*/
    bpos = buf;
    index = 0;
success:
    *bpos = '\0';
    return &buf[index];
}
