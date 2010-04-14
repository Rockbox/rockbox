/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 Gilles Roux
 *               2003 Garrett Derner
 *               2010 Yoshihisa Uchida
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
#include "plugin.h"
#include "lib/playback_control.h"


/* settings helper functions */

static bool encoding_setting(void)
{
    static struct opt_items names[NUM_CODEPAGES];
    int idx;
    bool res;
    enum codepages oldenc = prefs.encoding;

    for (idx = 0; idx < NUM_CODEPAGES; idx++)
    {
        names[idx].string = rb->get_codepage_name(idx);
        names[idx].voice_id = -1;
    }

    res = rb->set_option("Encoding", &prefs.encoding, INT, names,
                          sizeof(names) / sizeof(names[0]), NULL);

    /* When prefs.encoding changes into UTF-8 or changes from UTF-8,
     * filesize (file_size) might change.
     * In addition, if prefs.encoding is UTF-8, then BOM does not read.
     */
    if (oldenc != prefs.encoding && (oldenc == UTF_8 || prefs.encoding == UTF_8))
    {
        check_bom();
        get_filesize();
        fill_buffer(file_pos, buffer, buffer_size);
    }

    return res;
}

static bool word_wrap_setting(void)
{
    static const struct opt_items names[] = {
        {"On",               -1},
        {"Off (Chop Words)", -1},
    };

    return rb->set_option("Word Wrap", &prefs.word_mode, INT,
                          names, 2, NULL);
}

static bool line_mode_setting(void)
{
    static const struct opt_items names[] = {
        {"Normal",       -1},
        {"Join Lines",   -1},
        {"Expand Lines", -1},
#ifdef HAVE_LCD_BITMAP
        {"Reflow Lines", -1},
#endif
    };

    return rb->set_option("Line Mode", &prefs.line_mode, INT, names,
                          sizeof(names) / sizeof(names[0]), NULL);
}

static bool view_mode_setting(void)
{
    static const struct opt_items names[] = {
        {"No (Narrow)", -1},
        {"Yes",         -1},
    };
    bool ret;
    ret = rb->set_option("Wide View", &prefs.view_mode, INT,
                           names , 2, NULL);
    if (prefs.view_mode == NARROW)
        col = 0;
    calc_max_width();
    return ret;
}

static bool scroll_mode_setting(void)
{
    static const struct opt_items names[] = {
        {"Scroll by Page", -1},
        {"Scroll by Line", -1},
    };

    return rb->set_option("Scroll Mode", &prefs.scroll_mode, INT,
                          names, 2, NULL);
}

#ifdef HAVE_LCD_BITMAP
static bool page_mode_setting(void)
{
    static const struct opt_items names[] = {
        {"No",  -1},
        {"Yes", -1},
    };

    return rb->set_option("Overlap Pages", &prefs.page_mode, INT,
                           names, 2, NULL);
}

static bool scrollbar_setting(void)
{
    static const struct opt_items names[] = {
        {"Off", -1},
        {"On",  -1}
    };

    return rb->set_option("Show Scrollbar", &prefs.scrollbar_mode, INT,
                           names, 2, NULL);
}

static bool header_setting(void)
{
    int len = (rb->global_settings->statusbar == STATUSBAR_TOP)? 4 : 2;
    struct opt_items names[len];

    names[0].string   = "None";
    names[0].voice_id = -1;
    names[1].string   = "File path";
    names[1].voice_id = -1;

    if (rb->global_settings->statusbar == STATUSBAR_TOP)
    {
        names[2].string   = "Status bar";
        names[2].voice_id = -1;
        names[3].string   = "Both";
        names[3].voice_id = -1;
    }

    return rb->set_option("Show Header", &prefs.header_mode, INT,
                         names, len, NULL);
}

static bool footer_setting(void)
{
    int len = (rb->global_settings->statusbar == STATUSBAR_BOTTOM)? 4 : 2;
    struct opt_items names[len];

    names[0].string   = "None";
    names[0].voice_id = -1;
    names[1].string   = "Page Num";
    names[1].voice_id = -1;

    if (rb->global_settings->statusbar == STATUSBAR_BOTTOM)
    {
        names[2].string   = "Status bar";
        names[2].voice_id = -1;
        names[3].string   = "Both";
        names[3].voice_id = -1;
    }

    return rb->set_option("Show Footer", &prefs.footer_mode, INT,
                           names, len, NULL);
}

static int font_comp(const void *a, const void *b)
{
    struct opt_items *pa;
    struct opt_items *pb;

    pa = (struct opt_items *)a;
    pb = (struct opt_items *)b;

    return rb->strcmp(pa->string, pb->string);
}

static bool font_setting(void)
{
    int count = 0;
    DIR *dir;
    struct dirent *entry;
    int i = 0;
    int len;
    int new_font = 0;
    int old_font;
    bool res;
    int size = 0;

    dir = rb->opendir(FONT_DIR);
    if (!dir)
    {
        rb->splash(HZ/2, "font dir does not access.");
        return false;
    }

    while (1)
    {
        entry = rb->readdir(dir);

        if (entry == NULL)
            break;

        len = rb->strlen(entry->d_name);
        if (len < 4 || rb->strcmp(entry->d_name + len-4, ".fnt"))
            continue;
        size += len-3;
        count++;
    }
    rb->closedir(dir);

    struct opt_items names[count];
    unsigned char font_names[size];
    unsigned char *p = font_names;

    dir = rb->opendir(FONT_DIR);
    if (!dir)
    {
        rb->splash(HZ/2, "font dir does not access.");
        return false;
    }

    while (1)
    {
        entry = rb->readdir(dir);

        if (entry == NULL)
            break;

        len = rb->strlen(entry->d_name);
        if (len < 4 || rb->strcmp(entry->d_name + len-4, ".fnt"))
            continue;

        rb->snprintf(p, len-3, "%s", entry->d_name);
        names[i].string = p;
        names[i].voice_id = -1;
        p += len-3;
        i++;
        if (i >= count)
            break;
    }
    rb->closedir(dir);

    rb->qsort(names, count, sizeof(struct opt_items), font_comp);

    for (i = 0; i < count; i++)
    {
        if (!rb->strcmp(names[i].string, prefs.font))
        {
            new_font = i;
            break;
        }
    }
    old_font = new_font;

    res = rb->set_option("Select Font", &new_font, INT,
                         names, count, NULL);

    if (new_font != old_font)
    {
        rb->memset(prefs.font, 0, MAX_PATH);
        rb->snprintf(prefs.font, MAX_PATH, "%s", names[new_font].string);
        change_font(prefs.font);
    }

    return res;
}
#endif

static bool autoscroll_speed_setting(void)
{
    return rb->set_int("Auto-scroll Speed", "", UNIT_INT, 
                       &prefs.autoscroll_speed, NULL, 1, 1, 10, NULL);
}

MENUITEM_FUNCTION(encoding_item, 0, "Encoding", encoding_setting,
                  NULL, NULL, Icon_NOICON);
MENUITEM_FUNCTION(word_wrap_item, 0, "Word Wrap", word_wrap_setting,
                  NULL, NULL, Icon_NOICON);
MENUITEM_FUNCTION(line_mode_item, 0, "Line Mode", line_mode_setting,
                  NULL, NULL, Icon_NOICON);
MENUITEM_FUNCTION(view_mode_item, 0, "Wide View", view_mode_setting,
                  NULL, NULL, Icon_NOICON);
#ifdef HAVE_LCD_BITMAP
MENUITEM_FUNCTION(scrollbar_item, 0, "Show Scrollbar", scrollbar_setting,
                  NULL, NULL, Icon_NOICON);
MENUITEM_FUNCTION(page_mode_item, 0, "Overlap Pages", page_mode_setting,
                  NULL, NULL, Icon_NOICON);
MENUITEM_FUNCTION(header_item, 0, "Show Header", header_setting,
                  NULL, NULL, Icon_NOICON);
MENUITEM_FUNCTION(footer_item, 0, "Show Footer", footer_setting,
                  NULL, NULL, Icon_NOICON);
MENUITEM_FUNCTION(font_item, 0, "Font", font_setting,
                  NULL, NULL, Icon_NOICON);
#endif
MENUITEM_FUNCTION(scroll_mode_item, 0, "Scroll Mode", scroll_mode_setting,
                  NULL, NULL, Icon_NOICON);
MENUITEM_FUNCTION(autoscroll_speed_item, 0, "Auto-Scroll Speed",
                  autoscroll_speed_setting, NULL, NULL, Icon_NOICON);
MAKE_MENU(option_menu, "Viewer Options", NULL, Icon_NOICON,
            &encoding_item, &word_wrap_item, &line_mode_item, &view_mode_item,
#ifdef HAVE_LCD_BITMAP
            &scrollbar_item, &page_mode_item, &header_item, &footer_item, &font_item,
#endif
            &scroll_mode_item, &autoscroll_speed_item);

static bool viewer_options_menu(bool is_global)
{
    bool result;
    struct preferences tmp_prefs;

    rb->memcpy(&tmp_prefs, &prefs, sizeof(struct preferences));

    result = (rb->do_menu(&option_menu, NULL, NULL, false) == MENU_ATTACHED_USB);

    if (!is_global && rb->memcmp(&tmp_prefs, &prefs, sizeof(struct preferences)))
    {
        /* Show-scrollbar mode for current view-width mode */
#ifdef HAVE_LCD_BITMAP
        init_need_scrollbar();
        init_header_and_footer();
#endif
        calc_page();
    }
    return result;
}

void viewer_menu(void)
{
    int result;

    MENUITEM_STRINGLIST(menu, "Viewer Menu", NULL,
                        "Return", "Viewer Options",
                        "Show Playback Menu", "Select Bookmark",
                        "Global Settings", "Quit");

    result = rb->do_menu(&menu, NULL, NULL, false);
    switch (result)
    {
        case 0: /* return */
            break;
        case 1: /* change settings */
            done = viewer_options_menu(false);
            break;
        case 2: /* playback control */
            playback_control(NULL);
            break;
        case 3: /* select bookmark */
            viewer_select_bookmark(viewer_add_last_read_bookmark());
            viewer_remove_last_read_bookmark();
            fill_buffer(file_pos, buffer, buffer_size);
            if (prefs.scroll_mode == PAGE && cline > 1)
                viewer_scroll_to_top_line();
            break;
        case 4: /* change global settings */
            {
                struct preferences orig_prefs;

                rb->memcpy(&orig_prefs, &prefs, sizeof(struct preferences));
                if (!viewer_load_global_settings())
                    viewer_default_preferences();
                done = viewer_options_menu(true);
                viewer_save_global_settings();
                rb->memcpy(&prefs, &orig_prefs, sizeof(struct preferences));
            }
            break;
        case 5: /* quit */
            viewer_exit(NULL);
            done = true;
            break;
    }
    viewer_draw(col);
}
