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
#include "tv_bookmark.h"
#include "tv_menu.h"
#include "tv_settings.h"

/* settings helper functions */

static struct tv_preferences new_prefs;

/*                                 */
/* horizontal scroll settings menu */
/*                                 */

#ifdef HAVE_LCD_BITMAP
static bool tv_horizontal_scrollbar_setting(void)
{
    static const struct opt_items names[] = {
        {"No",  -1},
        {"Yes", -1},
    };

    return rb->set_option("Horizontal Scrollbar", &new_prefs.horizontal_scrollbar, INT,
                           names, 2, NULL);
}
#endif

static bool tv_horizontal_scroll_mode_setting(void)
{
    static const struct opt_items names[] = {
        {"Scroll by Screen", -1},
        {"Scroll by Column", -1},
    };

    return rb->set_option("Scroll Mode", &new_prefs.horizontal_scroll_mode, INT,
                          names, 2, NULL);
}

#ifdef HAVE_LCD_BITMAP
MENUITEM_FUNCTION(horizontal_scrollbar_item, 0, "Scrollbar",
                  tv_horizontal_scrollbar_setting,
                  NULL, NULL, Icon_NOICON);
#endif
MENUITEM_FUNCTION(horizontal_scroll_mode_item, 0, "Scroll Mode",
                  tv_horizontal_scroll_mode_setting, NULL, NULL, Icon_NOICON);

MAKE_MENU(horizontal_scroll_menu, "Horizontal", NULL, Icon_NOICON,
#ifdef HAVE_LCD_BITMAP
          &horizontal_scrollbar_item,
#endif
          &horizontal_scroll_mode_item);

/*                               */
/* vertical scroll settings menu */
/*                               */

#ifdef HAVE_LCD_BITMAP
static bool tv_vertical_scrollbar_setting(void)
{
    static const struct opt_items names[] = {
        {"No",  -1},
        {"Yes", -1},
    };

    return rb->set_option("Vertical Scrollbar", &new_prefs.vertical_scrollbar, INT,
                           names, 2, NULL);
}
#endif

static bool tv_vertical_scroll_mode_setting(void)
{
    static const struct opt_items names[] = {
        {"Scroll by Page", -1},
        {"Scroll by Line", -1},
    };

    return rb->set_option("Scroll Mode", &new_prefs.vertical_scroll_mode, INT,
                          names, 2, NULL);
}

static bool tv_page_mode_setting(void)
{
    static const struct opt_items names[] = {
        {"No",  -1},
        {"Yes", -1},
    };

    return rb->set_option("Overlap Pages", &new_prefs.page_mode, INT,
                           names, 2, NULL);
}

static bool tv_autoscroll_speed_setting(void)
{
    return rb->set_int("Auto-scroll Speed", "", UNIT_INT, 
                       &new_prefs.autoscroll_speed, NULL, 1, 1, 10, NULL);
}

static bool tv_narrow_mode_setting(void)
{
    static const struct opt_items names[] = {
        {"Previous/Next Page", -1},
        {"Top/Bottom Page",    -1},
    };

    return rb->set_option("Left/Right Key", &new_prefs.narrow_mode, INT,
                          names, 2, NULL);
}

#ifdef HAVE_LCD_BITMAP
MENUITEM_FUNCTION(vertical_scrollbar_item, 0, "Scrollbar",
                  tv_vertical_scrollbar_setting,
                  NULL, NULL, Icon_NOICON);
#endif
MENUITEM_FUNCTION(vertical_scroll_mode_item, 0, "Scroll Mode",
                  tv_vertical_scroll_mode_setting, NULL, NULL, Icon_NOICON);
MENUITEM_FUNCTION(page_mode_item, 0, "Overlap Pages", tv_page_mode_setting,
                  NULL, NULL, Icon_NOICON);
MENUITEM_FUNCTION(autoscroll_speed_item, 0, "Auto-Scroll Speed",
                  tv_autoscroll_speed_setting, NULL, NULL, Icon_NOICON);
MENUITEM_FUNCTION(narrow_mode_item, 0, "Left/Right Key (Narrow mode)",
                  tv_narrow_mode_setting, NULL, NULL, Icon_NOICON);

MAKE_MENU(vertical_scroll_menu, "Vertical", NULL, Icon_NOICON,
#ifdef HAVE_LCD_BITMAP
          &vertical_scrollbar_item,
#endif
          &vertical_scroll_mode_item, &page_mode_item, &autoscroll_speed_item,
          &narrow_mode_item);

/*                      */
/* scroll settings menu */
/*                      */

MAKE_MENU(scroll_menu, "Scroll Settings", NULL, Icon_NOICON,
          &horizontal_scroll_menu, &vertical_scroll_menu);

/*           */
/* main menu */
/*           */

static bool tv_encoding_setting(void)
{
    static struct opt_items names[NUM_CODEPAGES];
    int idx;

    for (idx = 0; idx < NUM_CODEPAGES; idx++)
    {
        names[idx].string = rb->get_codepage_name(idx);
        names[idx].voice_id = -1;
    }

    return rb->set_option("Encoding", &new_prefs.encoding, INT, names,
                          sizeof(names) / sizeof(names[0]), NULL);
}

static bool tv_word_wrap_setting(void)
{
    static const struct opt_items names[] = {
        {"On",               -1},
        {"Off (Chop Words)", -1},
    };

    return rb->set_option("Word Wrap", &new_prefs.word_mode, INT,
                          names, 2, NULL);
}

static bool tv_line_mode_setting(void)
{
    static const struct opt_items names[] = {
        {"Normal",       -1},
        {"Join Lines",   -1},
        {"Expand Lines", -1},
        {"Reflow Lines", -1},
    };

    return rb->set_option("Line Mode", &new_prefs.line_mode, INT, names,
                          sizeof(names) / sizeof(names[0]), NULL);
}

static bool tv_windows_setting(void)
{
    return rb->set_int("Screens Per Page", "", UNIT_INT, 
                       &new_prefs.windows, NULL, 1, 1, 5, NULL);
}

static bool tv_alignment_setting(void)
{
    static const struct opt_items names[] = {
        {"Left", -1},
        {"Right", -1},
    };

    return rb->set_option("Alignment", &new_prefs.alignment, INT,
                           names , 2, NULL);
}

#ifdef HAVE_LCD_BITMAP
static bool tv_header_setting(void)
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

    return rb->set_option("Show Header", &new_prefs.header_mode, INT,
                         names, len, NULL);
}

static bool tv_footer_setting(void)
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

    return rb->set_option("Show Footer", &new_prefs.footer_mode, INT,
                           names, len, NULL);
}

static bool tv_font_setting(void)
{
    int count = 0;
    int i = 0;
    int new_font = 0;
    int old_font;
    bool res;
    unsigned char font_path[MAX_PATH];

    struct tree_context *tree;
    struct tree_context backup;
    struct entry *dc;
    int dirfilter = SHOW_FONT;

    tree = rb->tree_get_context();
    backup = *tree;
    dc = tree->dircache;
    rb->strlcat(backup.currdir, "/", MAX_PATH);
    rb->strlcat(backup.currdir, dc[tree->selected_item].name, MAX_PATH);
    tree->dirfilter = &dirfilter;
    rb->snprintf(font_path, MAX_PATH, "%s/", FONT_DIR);
    rb->set_current_file(font_path);
    count = tree->filesindir;

    struct opt_items names[count];

    for (i = 0; i < count; i++)
    {
        char *p = rb->strrchr(dc[i].name, '.');
        if (p) *p = 0;
        if (!rb->strcmp(dc[i].name, new_prefs.font_name))
            new_font = i;

        names[i].string = dc[i].name;
        names[i].voice_id = -1;
    }

    old_font = new_font;

    res = rb->set_option("Select Font", &new_font, INT,
                         names, count, NULL);

    if (new_font != old_font)
    {
        rb->memset(new_prefs.font_name, 0, MAX_PATH);
        rb->strlcpy(new_prefs.font_name, names[new_font].string, MAX_PATH);
    }

    *tree = backup;
    rb->set_current_file(backup.currdir);
    return res;
}
#endif

MENUITEM_FUNCTION(encoding_item, 0, "Encoding", tv_encoding_setting,
                  NULL, NULL, Icon_NOICON);
MENUITEM_FUNCTION(word_wrap_item, 0, "Word Wrap", tv_word_wrap_setting,
                  NULL, NULL, Icon_NOICON);
MENUITEM_FUNCTION(line_mode_item, 0, "Line Mode", tv_line_mode_setting,
                  NULL, NULL, Icon_NOICON);
MENUITEM_FUNCTION(windows_item, 0, "Screens Per Page", tv_windows_setting,
                  NULL, NULL, Icon_NOICON);
MENUITEM_FUNCTION(alignment_item, 0, "Alignment", tv_alignment_setting,
                  NULL, NULL, Icon_NOICON);
#ifdef HAVE_LCD_BITMAP
MENUITEM_FUNCTION(header_item, 0, "Show Header", tv_header_setting,
                  NULL, NULL, Icon_NOICON);
MENUITEM_FUNCTION(footer_item, 0, "Show Footer", tv_footer_setting,
                  NULL, NULL, Icon_NOICON);
MENUITEM_FUNCTION(font_item, 0, "Font", tv_font_setting,
                  NULL, NULL, Icon_NOICON);
#endif

MAKE_MENU(option_menu, "Viewer Options", NULL, Icon_NOICON,
            &encoding_item, &word_wrap_item, &line_mode_item, &windows_item,
            &alignment_item,
#ifdef HAVE_LCD_BITMAP
            &header_item, &footer_item, &font_item,
#endif
            &scroll_menu);

static enum tv_menu_result tv_options_menu(void)
{
    bool result = TV_MENU_RESULT_EXIT_MENU;

    if (rb->do_menu(&option_menu, NULL, NULL, false) == MENU_ATTACHED_USB)
        result = TV_MENU_RESULT_ATTACHED_USB;

    return result;
}

enum tv_menu_result tv_display_menu(void)
{
    int result = TV_MENU_RESULT_EXIT_MENU;

    MENUITEM_STRINGLIST(menu, "Viewer Menu", NULL,
                        "Return", "Viewer Options",
                        "Show Playback Menu", "Select Bookmark",
                        "Global Settings", "Quit");

    switch (rb->do_menu(&menu, NULL, NULL, false))
    {
        case 0: /* return */
            break;
        case 1: /* change settings */
            tv_copy_preferences(&new_prefs);
            result = tv_options_menu();
            tv_set_preferences(&new_prefs);
            break;
        case 2: /* playback control */
            playback_control(NULL);
            break;
        case 3: /* select bookmark */
            tv_select_bookmark();
            break;
        case 4: /* change global settings */
            if (!tv_load_global_settings(&new_prefs))
                tv_set_default_preferences(&new_prefs);

            result = tv_options_menu();
            tv_save_global_settings(&new_prefs);
            break;
        case 5: /* quit */
            result = TV_MENU_RESULT_EXIT_PLUGIN;
            break;
    }
    return result;
}
