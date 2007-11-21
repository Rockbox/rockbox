/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 Bryan Childs
 * Copyright (c) 2007 Alexander Levin
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include "shortcuts.h"

PLUGIN_HEADER

enum sc_list_action_type
{
    SCLA_NONE,
    SCLA_SELECT,
    SCLA_DELETE,
    SCLA_USB,
};


static char *link_filename;
static bool user_file;
static int gselected_item;
static bool usb_connected = false;

enum sc_list_action_type draw_sc_list(struct gui_synclist gui_sc);

/* Will be passed sc_file* as data */
char* build_sc_list(int selected_item, void *data, char *buffer);

/* Returns true iff we should leave the main loop */
bool list_sc(bool is_editable);

bool goto_entry(char *file_or_dir);
bool ends_with(char *str, char *suffix);


enum sc_list_action_type draw_sc_list(struct gui_synclist gui_sc)
{
    int button;

    rb->gui_synclist_draw(&gui_sc);

    while (true) {
        /* draw the statusbar, should be done often */
        rb->gui_syncstatusbar_draw(rb->statusbars, true);
        /* user input */
        button = rb->get_action(CONTEXT_LIST, TIMEOUT_BLOCK);
        if (rb->gui_synclist_do_button(&gui_sc, &button,
                                            LIST_WRAP_UNLESS_HELD)) {
            /* automatic handling of user input.
            * _UNLESS_HELD can be _ON or _OFF also
            * selection changed, so redraw */
            continue;
        }
        switch (button) { /* process the user input */
            case ACTION_STD_OK:
                gselected_item = rb->gui_synclist_get_sel_pos(&gui_sc);
                return SCLA_SELECT;
            case ACTION_STD_MENU:
                /* Only allow delete entries in the default file
                 * since entries can be appended (with a plugin)
                 * to the default file only. The behaviour is thus
                 * symmetric in this respect. */
                if (!user_file) {
                    gselected_item = rb->gui_synclist_get_sel_pos(&gui_sc);
                    return SCLA_DELETE;
                }
                break;
            case ACTION_STD_CANCEL:
                return SCLA_NONE;
            default:
                if (rb->default_event_handler(button) == SYS_USB_CONNECTED) {
                    return SCLA_USB;
                }
        }
    }
}


char* build_sc_list(int selected_item, void *data, char *buffer)
{
    char text_buffer[MAX_PATH];
    sc_file_t *file = (sc_file_t*)data;
    
    if (!is_valid_index(file, selected_item)) {
        return NULL;
    }
    sc_entry_t *entry = file->entries + selected_item;
    rb->snprintf(text_buffer, sizeof(text_buffer), "%s", entry->disp);
    rb->strcpy(buffer, text_buffer);
    return buffer;
}


bool list_sc(bool is_editable)
{
    int selected_item = 0;
    char selected_dir[MAX_PATH];
    enum sc_list_action_type action = SCLA_NONE;
    struct gui_synclist gui_sc;

    rb->memset(selected_dir, 0, sizeof(selected_dir));

    /* Setup the GUI list object, draw it to the screen,
     * and then handle the user input to it */
    rb->gui_synclist_init(&gui_sc, &build_sc_list, &sc_file, false, 1);
    rb->gui_synclist_set_title(&gui_sc,
        (is_editable?"Shortcuts (editable)":"Shortcuts (sealed)"), NOICON);
    rb->gui_synclist_set_nb_items(&gui_sc, sc_file.entry_cnt);
    rb->gui_synclist_limit_scroll(&gui_sc, false);
    rb->gui_synclist_select_item(&gui_sc, 0);

    /* Draw the prepared widget to the LCD now */
    action = draw_sc_list(gui_sc);
    if (action == SCLA_USB) {
        usb_connected = true;
        return true;
    }

    /* which item do we action? */
    selected_item = gselected_item;

    if (!is_valid_index(&sc_file, selected_item)) {
        /* This should never happen */
        rb->splash(HZ*2, "Bad entry selected!");
        return true;
    }

    /* perform the following actions if the user "selected"
     * the item in the list (i.e. they want to go there
     * in the filebrowser tree */
    switch(action) {
        case SCLA_SELECT:
            return goto_entry(sc_file.entries[selected_item].path);
        case SCLA_DELETE:
            rb->splash(HZ, "Deleting %s", sc_file.entries[selected_item].disp);
            remove_entry(&sc_file, selected_item);
            dump_sc_file(&sc_file, link_filename);
            return (sc_file.entry_cnt == 0);
        default:
            return true;
    }
}


bool goto_entry(char *file_or_dir)
{
    DEBUGF("Trying to go to '%s'...\n", file_or_dir);
    
    bool is_dir = ends_with(file_or_dir, PATH_SEPARATOR);
    bool exists;
    char *what;
    if (is_dir) {
        what = "Directory";
        exists = rb->dir_exists(file_or_dir);
    } else {
        what = "File";
        exists = rb->file_exists(file_or_dir);
    }

    if (!exists) {
        rb->splash(HZ*2, "%s %s no longer exists on disk", what, file_or_dir);
        return false;
    }
    /* Set the browsers dirfilter to the global setting
     * This is required in case the plugin was launched
     * from the plugins browser, in which case the
     * dirfilter is set to only display .rock files */
    rb->set_dirfilter(rb->global_settings->dirfilter);

    /* Change directory to the entry selected by the user */
    rb->set_current_file(file_or_dir);
    return true;
}


bool ends_with(char *string, char *suffix)
{
    unsigned int str_len = rb->strlen(string);
    unsigned int sfx_len = rb->strlen(suffix);
    if (str_len < sfx_len)
        return false;
    return (rb->strncmp(string + str_len - sfx_len, suffix, sfx_len) == 0);
}


enum plugin_status plugin_start(struct plugin_api* api, void* void_parameter)
{
    rb = api;
    bool leave_loop;
    
    /* This is a viewer, so a parameter must have been specified */
    if (void_parameter == NULL) {
        rb->splash(HZ*2, "No parameter specified!");
        return PLUGIN_ERROR;
    }
    link_filename = (char*)void_parameter;
    user_file = (rb->strcmp(link_filename, SHORTCUTS_FILENAME) != 0);

    allocate_memory(&memory_buf, &memory_bufsize);

    if (!load_sc_file(&sc_file, link_filename, true,
            memory_buf, memory_bufsize)) {
        DEBUGF("Could not load %s\n", link_filename);
        return PLUGIN_ERROR;
    }
    if (sc_file.entry_cnt==0) {
        rb->splash(HZ*2, "No shortcuts in the file!");
        return PLUGIN_OK;
    } else if ((sc_file.entry_cnt==1) && user_file) {
        /* if there's only one entry in the user .link file,
         * go straight to it without displaying the menu
         * thus allowing 'quick links' */
        goto_entry(sc_file.entries[0].path);
        return PLUGIN_OK;
    }
    
    do {
        /* Display a menu to choose between the entries */
        leave_loop = list_sc(!user_file);
    } while (!leave_loop);

    return usb_connected ? PLUGIN_USB_CONNECTED : PLUGIN_OK;
}
