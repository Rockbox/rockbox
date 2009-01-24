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
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include "shortcuts.h"

#define SHORTCUTS_FILENAME "/shortcuts.link"

#define PATH_DISP_SEPARATOR "\t"
#define PATH_DISP_SEPARATOR_LEN 1 /* strlen(PATH_DISP_SEPARATOR) */
#define CONTROL_PREFIX "#"
#define CONTROL_PREFIX_LEN 1 /* strlen(CONTROL_PREFIX) */
#define NAME_VALUE_SEPARATOR "="
#define NAME_VALUE_SEPARATOR_LEN 1 /* strlen(NAME_VALUE_SEPARATOR) */

#define INSTR_DISPLAY_LAST_SEGMENTS "Display last path segments"

/* Memory (will be used for entries) */
void *memory_buf;
long memory_bufsize; /* Size of memory_buf in bytes */


/* The file we're processing */
sc_file_t sc_file;

bool parse_entry_content(char *line, sc_entry_t *entry, int last_segm);
char *last_segments(char *path, int nsegm);
bool is_control(char *line, sc_file_t *file);
bool starts_with(char *string, char *prefix);
bool parse_name_value(char *line, char *name, int namesize,
        char *value, int valuesize);
void write_entry_to_file(int fd, sc_entry_t *entry);
void write_int_instruction_to_file(int fd, char *instr, int value);


void allocate_memory(void **buf, size_t *bufsize)
{
    *buf = rb->plugin_get_buffer(bufsize);
    DEBUGF("Got %ld bytes of memory\n", *bufsize);
}


void init_sc_file(sc_file_t *file, void *buf, size_t bufsize)
{
    file->entries = (sc_entry_t*)buf;
    file->max_entries = bufsize / sizeof(sc_entry_t);
    DEBUGF("Buffer capacity: %d entries\n", file->max_entries);
    file->entry_cnt = 0;
    file->show_last_segments = -1;
}


bool load_sc_file(sc_file_t *file, char *filename, bool must_exist,
        void *entry_buf, size_t entry_bufsize)
{
    int fd = -1;
    bool ret_val = false; /* Assume bad case */
    int amountread = 0;
    char sc_content[2*MAX_PATH];
    sc_entry_t entry;

    /* We start to load a new file -> prepare it */
    init_sc_file(&sc_file, entry_buf, entry_bufsize);

    fd = rb->open(filename, O_RDONLY);
    if (fd < 0) {
        /* The file didn't exist on disk */
        if (!must_exist) {
            DEBUGF("Trying to create link file '%s'...\n", filename);
            fd = rb->creat(filename);
            if (fd < 0){
                /* For some reason we couldn't create the file,
                 * so return an error message and exit */
                rb->splashf(HZ*2, "Couldn't create the shortcuts file %s",
                           filename);
                goto end_of_proc;
            }
            /* File created, but there's nothing in it, so just exit */
            ret_val = true;
            goto end_of_proc;
        } else {
            rb->splashf(HZ, "Couldn't open %s", filename);
            goto end_of_proc;
        }
    }

    while ((amountread=rb->read_line(fd,sc_content, sizeof(sc_content)))) {
        if (is_control(sc_content, file)) {
            continue;
        }
        if (file->entry_cnt >= file->max_entries) {
            rb->splashf(HZ*2, "Too many entries in the file, max allowed: %d",
                       file->max_entries);
            goto end_of_proc;
        }
        if (!parse_entry_content(sc_content, &entry,file->show_last_segments)) {
            /* Could not parse the entry (too long path?) -> ignore */
            DEBUGF("Could not parse '%s' -> ignored\n", sc_content);
            continue;
        }
        DEBUGF("Parsed entry: path=%s, disp=%s\n", entry.path, entry.disp);
        append_entry(file, &entry);
    }

#ifdef SC_DEBUG
    print_file(file);
#endif
    
    ret_val = true; /* Everything went ok */

end_of_proc:
    if (fd >= 0) {
        rb->close(fd);
        fd = -1;
    }
    return ret_val;
}

#ifdef SC_DEBUG
void print_file(sc_file_t *file)
{
    DEBUGF("Number of entries : %d\n", file->entry_cnt);
    DEBUGF("Show Last Segments: %d\n", file->show_last_segments);
    int i;
    sc_entry_t *entry;
    for (i=0, entry=file->entries; i<file->entry_cnt; i++,entry++) {
        if (entry->explicit_disp) {
            DEBUGF("%2d. '%s', show as '%s'\n", i+1, entry->path, entry->disp);
        } else {
            DEBUGF("%2d. '%s' (%s)\n", i+1, entry->path, entry->disp);
        }
    }
}
#endif


bool append_entry(sc_file_t *file, sc_entry_t *entry)
{
    if (file->entry_cnt >= file->max_entries) {
        return false;
    }
    rb->memcpy(file->entries+file->entry_cnt, entry, sizeof(*entry));
    file->entry_cnt++;
    return true;
}


bool remove_entry(sc_file_t *file, int entry_idx)
{
    if ((entry_idx<0) || (entry_idx>=file->entry_cnt)) {
        return false;
    }
    sc_entry_t *start = file->entries + entry_idx;
    rb->memmove(start, start + 1,
            (file->entry_cnt-entry_idx-1) * sizeof(sc_entry_t));
    file->entry_cnt--;
    return true;
}


bool is_valid_index(sc_file_t *file, int entry_idx)
{
    return (entry_idx >= 0) && (entry_idx < file->entry_cnt);
}


bool parse_entry_content(char *line, sc_entry_t *entry, int last_segm)
{
    char *sep;
    char *path, *disp;
    unsigned int path_len, disp_len;
    bool expl;
    
    sep = rb->strcasestr(line, PATH_DISP_SEPARATOR);
    expl = (sep != NULL);
    if (expl) {
        /* Explicit name for the entry is specified -> use it */
        path = line;
        path_len = sep - line;
        disp = sep + PATH_DISP_SEPARATOR_LEN;
        disp_len = rb->strlen(disp);
    } else {
        /* No special name to display */
        path = line;
        path_len = rb->strlen(line);
        if (last_segm <= 0) {
            disp = path;
        } else {
            disp = last_segments(line, last_segm);
        }
        disp_len = rb->strlen(disp);
    }
    
    if (path_len >= sizeof(entry->path) || disp_len >= sizeof(entry->disp)) {
        DEBUGF("Bad entry: pathlen=%d, displen=%d\n", path_len, disp_len);
        return false;
    }
    rb->strncpy(entry->path, path, path_len);
    entry->path[path_len] = '\0';
    rb->strcpy(entry->disp, disp); /* Safe since we've checked the length */
    entry->explicit_disp = expl;
    return true;
}


char *last_segments(char *path, int nsegm)
{
    char *p = rb->strrchr(path, PATH_SEPARATOR[0]); /* Hack */
    int seg_cnt;
    if (p == NULL)
        return path; /* No separator??? */
    seg_cnt = 0;
    while ((p > path) && (seg_cnt < nsegm)) {
        p--;
        if (!starts_with(p, PATH_SEPARATOR)) {
            continue;
        }
        seg_cnt++;
        if (seg_cnt == nsegm && p > path) {
            p++; /* Eat the '/' to show that something has been truncated */
        }
    }
    return p;
}


bool is_control(char *line, sc_file_t *file)
{
    char name[MAX_PATH], value[MAX_PATH];
    if (!starts_with(line, CONTROL_PREFIX)) {
        return false;
    }
    line += CONTROL_PREFIX_LEN;
    
    if (!parse_name_value(line, name, sizeof(name),
            value, sizeof(value))) {
        DEBUGF("Bad processing instruction: '%s'\n", line);
        return true;
    }
    
    /* Process control instruction */
    if (rb->strcasestr(name, INSTR_DISPLAY_LAST_SEGMENTS)) {
        file->show_last_segments = rb->atoi(value);
        DEBUGF("Set show last segms to %d\n", file->show_last_segments);
    } else {
        /* Unknown instruction -> ignore */
        DEBUGF("Unknown processing instruction: '%s'\n", name);
    }
    
    return true;
}


bool starts_with(char *string, char *prefix)
{
    unsigned int pfx_len = rb->strlen(prefix);
    if (rb->strlen(string) < pfx_len)
        return false;
    return (rb->strncmp(string, prefix, pfx_len) == 0);
}


bool parse_name_value(char *line, char *name, int namesize,
        char *value, int valuesize)
{
    char *sep;
    int name_len, val_len;
    name[0] = value[0] = '\0';
    
    sep = rb->strcasestr(line, NAME_VALUE_SEPARATOR);
    if (sep == NULL) {
        /* No separator char -> weird instruction */
        return false;
    }
    name_len = sep - line;
    if (name_len >= namesize) {
        /* Too long name */
        return false;
    }
    rb->strncpy(name, line, name_len);
    name[name_len] = '\0';
    
    val_len = rb->strlen(line) - name_len - NAME_VALUE_SEPARATOR_LEN;
    if (val_len >= valuesize) {
        /* Too long value */
        return false;
    }
    rb->strncpy(value, sep+NAME_VALUE_SEPARATOR_LEN, val_len+1);
    return true;
}


bool dump_sc_file(sc_file_t *file, char *filename)
{
    DEBUGF("Dumping shortcuts to the file '%s'\n", filename);
    int fd;

    /* ideally, we should just write a new
    * entry to the file, but I'm going to
    * be lazy, and just re-write the whole
    * thing. */
    fd = rb->open(filename, O_WRONLY|O_TRUNC);
    if (fd < 0) {
        rb->splashf(HZ*2, "Could not open shortcuts file %s for writing",
                filename);
        return false;
    }
    
    /*
     * Write instructions
     */
    /* Always dump the 'display last segms' settings (even it it's
     * not active) so that it can be easily changed without having
     * to remember the setting name */
    write_int_instruction_to_file(fd,
            INSTR_DISPLAY_LAST_SEGMENTS, file->show_last_segments);
    
    int i;
    sc_entry_t *entry;
    for (i=0, entry=file->entries; i<file->entry_cnt; i++,entry++) {
        write_entry_to_file(fd, entry);
    }

    rb->close(fd);
    DEBUGF("Dumped %d entries to the file '%s'\n", file->entry_cnt, filename);

    return true;
}


void write_int_instruction_to_file(int fd, char *instr, int value)
{
    rb->fdprintf(fd, "%s%s%s%d\n", CONTROL_PREFIX, instr,
                 NAME_VALUE_SEPARATOR, value);
}


void write_entry_to_file(int fd, sc_entry_t *entry)
{
    rb->fdprintf(fd, "%s", entry->path);
    if (entry->explicit_disp) {
        rb->fdprintf(fd, "%s%s", PATH_DISP_SEPARATOR, entry->disp);
    }
    rb->fdprintf(fd, "\n");
}
