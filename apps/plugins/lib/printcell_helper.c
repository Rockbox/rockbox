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

#define COLUMN_ENDLEN 3
#define TITLE_FLAG 0xFF
#define SELECTED_FLAG 0x1

#if LCD_DEPTH == 1
#define BAR_WIDTH (1)
#else
#define BAR_WIDTH (COLUMN_ENDLEN)
#endif

struct printcell_info_t {
    struct gui_synclist *gui_list; /* list to display */
    int  offw[NB_SCREENS];         /* padding between column boundries and text */
    int  iconw[NB_SCREENS];        /* width of an icon */
    int  selcol_offw[NB_SCREENS];  /* offset width calculated for selected item */
    int  totalcolw[NB_SCREENS];    /* total width of all columns */
    int  firstcolxw[NB_SCREENS];   /* first column x + width, save recalculating */
    int  ncols;                    /* number of columns */
    int  selcol;                   /* selected column (-1 to ncols-1) */
    uint32_t hidecol_flags;        /*bits  0-31 set bit to 1 to hide a column (1<<col#) */
    uint16_t colw[NB_SCREENS][PRINTCELL_MAX_COLUMNS]; /* width of title text 
                                    or MIN(or user defined width / screen width) */
    char title[PRINTCELL_MAXLINELEN]; /* title buffer */
    char titlesep;  /* character that separates title column items (ex '$') */
    char colsep;    /* character that separates text column items (ex ',') */
    bool separator; /* draw grid */
};

static struct printcell_info_t printcell;

static void parse_dsptext(char splitchr, int ncols, const char *dsp_text,
                          char* buffer, size_t bufsz, uint16_t *sidx)
{
    /*Internal function loads sidx with split offsets indexing
      the buffer of null terminated strings, splits on 'splitchr'
      _assumptions_:
      dsp_text[len - 1] = \0,
      sidx[PRINTCELL_MAX_COLUMNS]
    */
    int i = 0;
    size_t j = 0;
    int ch = splitchr; /* first column $ is optional */
    if (*dsp_text == splitchr)
        dsp_text++;
    /* add null to the start of the text buffer */
    buffer[j++] = '\0';
    do
    {
        if (ch == splitchr)
        {
            sidx[i] = j; /* save start index and copy next column to the buffer */
            while (*dsp_text != '\0' && *dsp_text != splitchr && j < (bufsz - 1))
            {
                buffer[j++] = *dsp_text++;
            }
            buffer[j++] = '\0';

            i++;
            if (i >= ncols || j >= (bufsz - 1))
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
                             int x, int y, int offw, int selected_flag, int last_col,
                             bool scroll, bool is_title)
{
    /* Internal function prints remaining cells */
    int text_offset = offw + offw;
    int screen = display->screen_type;
    int height = linedes->height;
    int selsep = (selected_flag == 0) ? 0: separator;
    uint16_t *screencolwidth = printcell.colw[screen];

    for(int i = 1; i <= last_col; i++)
    {
        int ny = y;
        int nw = screencolwidth[i] + text_offset;
        int offx = 0;

        if (i == last_col || x + nw >= vp_w - offw + 1)
        {  /* not enough space for next column use up excess */
            if (nw < (vp_w - x))
            {
                if (is_title)
                    offx = ((vp_w - x) - nw) / 2;
                nw = vp_w - x;
            }
        }

        if (!PRINTCELL_COLUMN_IS_VISIBLE(printcell.hidecol_flags, i))
            nw = 0;

        int nx = x + nw;
        char *buftext;

        if (nx > 0 && nw > offw && x < vp_w)
        {
            set_cell_width(vp, vp_w, nx);

            if (i == printcell.selcol)
            {
                linedes->scroll = (selected_flag > 0);
                linedes->separator_height = selsep;
            }
            else
            {
                if (vp_w < x + text_offset)
                {
                    scroll = false;
                }
                linedes->scroll = scroll;
                linedes->separator_height = separator;
            }
            buftext = &buffer[sidx[i]];
            display->put_line(x + offw + offx, ny, linedes, "$t", buftext);
            vp->width += COLUMN_ENDLEN + 1;
            if (vp->width > vp_w)
                vp->width = vp_w;

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

        if (!PRINTCELL_COLUMN_IS_VISIBLE(printcell.hidecol_flags, i))
            cw = 0;

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
    int last_col = printcell.ncols - 1;
    int hidden_w = 0;
    int nw, colxw;
    char *buftext;
    printcell_buffer[0] = '\0';

    uint16_t *screencolwidth = printcell.colw[screen];
    if (printcell.hidecol_flags > 0)
    {
        for (int i = 0; i < printcell.ncols; i++)
        {
            if (PRINTCELL_COLUMN_IS_VISIBLE(printcell.hidecol_flags, i))
                last_col = i;
            else
            hidden_w += (screencolwidth[i] + text_offset);
        }
    }

    if (is_title)
    {
        parse_dsptext(printcell.titlesep, printcell.ncols, dsp_text,
                      printcell_buffer, sizeof(printcell_buffer), sidx);

        buftext = &printcell_buffer[sidx[0]]; /* set to first column text */
        int sbwidth = 0;
        if (rb->global_settings->scrollbar == SCROLLBAR_LEFT)
            sbwidth = rb->global_settings->scrollbar_width;

        printcell.iconw[screen] = have_icons ? ICON_PADDING + icon_w : 0;

        if (printcell.selcol_offw[screen] == 0 && printcell.selcol > 0)
            printcell.selcol_offw[screen] = calcvisible(screen, vp_w, text_offset, sbwidth);

        nx -= printcell.selcol_offw[screen];

        nw = screencolwidth[0] + printcell.iconw[screen] + text_offset;

        if (!PRINTCELL_COLUMN_IS_VISIBLE(printcell.hidecol_flags, 0))
            nw = printcell.iconw[screen] - 1;
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
        parse_dsptext(printcell.colsep, printcell.ncols, dsp_text,
                      printcell_buffer, sizeof(printcell_buffer), sidx);

        buftext = &printcell_buffer[sidx[0]]; /* set to first column text */
        int cursor = Icon_NOICON;
        nx -= printcell.selcol_offw[screen];

        if (selected_flag & SELECTED_FLAG)
        {
            cursor = Icon_Cursor;
            /* limit length of selection if columns don't reach end */
            int maxw = nx + printcell.totalcolw[screen] + printcell.iconw[screen];
            maxw += text_offset * printcell.ncols;
            maxw -= hidden_w;

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
        if (vp->width > vp_w)
            vp->width = vp_w;
        draw_selector(display, linedes, selected_flag, 0,
                      separator, nx, y, nw, linedes->height);
    }
    nx += nw;
    /* display remaining cells */
    printcells(display, printcell_buffer, sidx, linedes, vp, vp_w, separator,
               nx, y, col_offset_width, selected_flag, last_col, scroll_items, is_title);

    /* draw a line at the bottom of the list */
    if (separator > 0 && line == list->nb_items - 1)
        display->hline(x, LCD_WIDTH, y + linedes->height - 1);

    /* restore settings */
    linedes->scroll = saved_scroll;
    linedes->separator_height = saved_separator_height;
    display->set_drawmode(DRMODE_FG);
    vp->width = vp_w;
}

void printcell_enable(bool enable)
{
    if (!printcell.gui_list)
        return;
    struct gui_synclist *gui_list = printcell.gui_list;
#ifdef HAVE_LCD_COLOR
    static int list_sep_color = INT_MIN;
    if (enable)
    {
        if (list_sep_color == INT_MIN)
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

int printcell_increment_column(int increment, bool wrap)
{
    if (!printcell.gui_list)
        return -1;
    struct gui_synclist *gui_list = printcell.gui_list;
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

        rb->gui_synclist_draw(gui_list);
        rb->gui_synclist_speak_item(gui_list);
    }
    return item;
}

int printcell_get_column_selected(void)
{
    if (!printcell.gui_list)
        return -1;
    return printcell.selcol;
}

uint32_t printcell_get_column_visibility(int col)
{
    if (col >= 0)
        return (PRINTCELL_COLUMN_IS_VISIBLE(printcell.hidecol_flags, col) ? 0:1);
    else /* return flag of all columns */
        return printcell.hidecol_flags;
}

void printcell_set_column_visible(int col, bool visible)
{
    /* visible columns have 0 for the column bit hidden columns have the bit set */
    if (col >= 0)
    {
        if (visible)
            printcell.hidecol_flags &= ~(PRINTCELL_COLUMN_FLAG(col));
        else
            printcell.hidecol_flags |= PRINTCELL_COLUMN_FLAG(col);
    }
    else
    {
        if (visible) /* set to everything visible */
            printcell.hidecol_flags = 0;
        else /* set to everything hidden */
            printcell.hidecol_flags = ((uint32_t)-1);
    }
}

int printcell_set_columns(struct gui_synclist *gui_list,
                          struct printcell_settings * pcs,
                          char * title, enum themable_icons icon)
{

    if (title == NULL)
        title = "$PRINTCELL NOT SETUP";

    uint16_t sidx[PRINTCELL_MAX_COLUMNS]; /* starting position of column in title string */
    int width, height, user_minwidth;
    int i = 0;
    size_t j = 0;
    rb->memset(&printcell, 0, sizeof(struct printcell_info_t));

    if (pcs == NULL) /* DEFAULTS */
    {
#if LCD_DEPTH > 1
        /* If line sep is set to automatic then outline cells */
        bool sep = (rb->global_settings->list_separator_height < 0);
#else
        bool sep = (rb->global_settings->cursor_style == 0);
#endif
        printcell.separator = sep;
        printcell.titlesep  = '$';
        printcell.colsep    = '$';
        printcell.hidecol_flags = 0;
    }
    else
    {
        printcell.separator = pcs->cell_separator;
        printcell.titlesep  = pcs->title_delimeter;
        printcell.colsep    = pcs->text_delimeter;
        printcell.hidecol_flags = pcs->hidecol_flags;
    }
    printcell.gui_list = gui_list;


    int ch = printcell.titlesep; /* first column $ is optional */

    FOR_NB_SCREENS(n)
    {
        rb->screens[n]->getstringsize("W", &width, &height);
        printcell.offw[n] = width; /* set column text offset */
    }

    if (*title == printcell.titlesep)
        title++;
    do
    {
        if (ch == printcell.titlesep)
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
                if (*title != printcell.titlesep) /* user forgot titlesep or wants to display '*' */
                {
                    title = dspst;
                    user_minwidth = 0;
                }
                else
                    title++;
            }

            sidx[i] = j;

            while (*title != '\0'
                   && *title != printcell.titlesep
                   && j < PRINTCELL_MAXLINELEN - 1)
            {
                printcell.title[j++] = *title++;
            }

            FOR_NB_SCREENS(n)
            {
                rb->screens[n]->getstringsize(&printcell.title[sidx[i]],
                                              &width, &height);

                if (width < user_minwidth)
                    width = user_minwidth;

                if (width > LCD_WIDTH)
                    width = LCD_WIDTH;

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

    rb->gui_synclist_set_title(gui_list, printcell.title, icon);
    return printcell.ncols;
}

char *printcell_get_title_text(int selcol, char *buf, size_t bufsz)
{
    /* note offsets are calculated everytime this function is called
     * shouldn't be used in hot code paths */
    int index = 0;
    buf[0] = '\0';
    if (selcol < 0) /* return entire string incld col formatting '$'*/
        return printcell.title;

    if (selcol < printcell.ncols)
    {
        uint16_t sidx[PRINTCELL_MAX_COLUMNS]; /*indexes zero terminated strings in buffer*/
        parse_dsptext(printcell.titlesep, selcol + 1, printcell.title, buf, bufsz, sidx);
        index = sidx[selcol];
    }
    return &buf[index];
}

char *printcell_get_column_text(int selcol, char *buf, size_t bufsz)
{
    int index = 0;
    char *bpos;
    struct gui_synclist *gui_list = printcell.gui_list;

    if (gui_list && gui_list->callback_draw_item == printcell_listdraw_fn)
    {
        int col = selcol;
        int item = gui_list->selected_item;
        void *data = gui_list->data;

        if (col < printcell.ncols
            && gui_list->callback_get_item_name(item, data, buf, bufsz) == buf)
        {
            bpos = buf;
            if (col < 0) /* return entire string incld col formatting '$'*/
            {
                return bpos;
            }
            bpos++; /* Skip sep/NULL */

            while(bpos < &buf[bufsz - 1])
            {
                if (*bpos == printcell.colsep || *bpos == '\0')
                {
                    if (col-- == 0)
                        goto success;
                    index = bpos - buf + 1; /* Skip sep/NULL */
                }
                bpos++;
            }
        }
    }
/*failure*/
        bpos = buf;
        index = 0;
success:
        *bpos = '\0';
        return &buf[index];
}
