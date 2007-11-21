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


bool append_entry_to_file(sc_file_t *file, char *path, bool is_dir)
{
    sc_entry_t entry;

    unsigned int required_len = rb->strlen(path);
    if (is_dir) {
        required_len += PATH_SEPARATOR_LEN; /* Add 1 for the trailing / */
    }
    if (required_len >= sizeof(entry.path)) {
        /* no attempt to print it: it will also be so too long for the splash */
        rb->splash(HZ*2, "Can't append shortcut, it's too long");
        return false;
    }
    entry.explicit_disp = false;
    rb->strcpy(entry.path, path);
    if (is_dir) {
        rb->strcat(entry.path, PATH_SEPARATOR);
    }
    if (!append_entry(file, &entry)) {
        rb->splash(HZ*2, "Too many entries!");
        return false;
    }
    return true;
}


enum plugin_status plugin_start(struct plugin_api* api, void* void_parameter)
{
    rb = api;
    bool found;
    bool its_a_dir;
    
    /* This is a viewer, so a parameter must have been specified */
    if (void_parameter == NULL) {
        rb->splash(HZ*2, "No parameter specified!");
        return PLUGIN_ERROR;
    }
    char *parameter = (char*)void_parameter;
    DEBUGF("Trying to append '%s' to the default link file '%s'...\n",
            parameter, SHORTCUTS_FILENAME);

    allocate_memory(&memory_buf, &memory_bufsize);

    /* Determine if it's a file or a directory. First check
     * if it's a dir and then file (not vice versa) since
     * open() can also open a dir */
    found = true;
    if (rb->dir_exists(parameter)) {
        its_a_dir = true;
    } else if (rb->file_exists(parameter)) {
        its_a_dir = false;
    } else {
        found = false;
    }
    /* now we know if it's a file or a directory
     * (or something went wrong) */

    if (!found) {
        /* Something's gone properly pear shaped -
         * we couldn't even find the entry */
        rb->splash(HZ*2, "File/Dir not found: %s", parameter);
        return PLUGIN_ERROR;
    }

    DEBUGF("'%s' is a %s\n", parameter, (its_a_dir ? "dir" : "file"));

    if (!load_sc_file(&sc_file, SHORTCUTS_FILENAME, false,
                memory_buf, memory_bufsize)) {
        DEBUGF("Couldn't load '%s'\n", SHORTCUTS_FILENAME);
        return PLUGIN_ERROR;
    }
    
    if (!append_entry_to_file(&sc_file, parameter, its_a_dir)) {
        DEBUGF("Couldn't append entry (too many entries?)\n");
        return PLUGIN_ERROR;
    }
    
    if (!dump_sc_file(&sc_file, SHORTCUTS_FILENAME)) {
        DEBUGF("Couldn't write shortcuts to '%s'\n", SHORTCUTS_FILENAME);
        return PLUGIN_ERROR;
    }

    return PLUGIN_OK;
}
