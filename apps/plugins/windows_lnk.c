/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2017 Sebastian Leonhardt
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

#include <setjmp.h>
#include "plugin.h"
#include "rbunicode.h"

/*
 *  Follow Windows shortcuts (*.lnk files) in Rockbox.
 *  If the destination is a file, it will be selected in the file browser,
 *  a directory will be entered.
 *  For now, only relative links are supported.
 */

static jmp_buf jmp;

/* a selection of link flags */
#define HAS_LINK_TARGET_ID_LIST 0x01
#define HAS_LINK_INFO           0x02
#define HAS_NAME                0x04
#define HAS_RELATIVE_PATH       0x08
#define HAS_WORKING_DIR         0x10
#define HAS_ARGUMENTS           0x20
#define HAS_ICON_LOCATION       0x40
#define IS_UNICODE              0x80
#define FORCE_NO_LINK_INFO     0x100
/* a selection of file attributes flags */
#define FILE_ATTRIBUTE_DIRECTORY    0x10
#define FILE_ATTRIBUTE_NORMAL       0x80


/**
 *  Read one byte from file
 *  \param  fd  file descriptor
 *  \return     data
 */
static int read_byte(int fd)
{
    unsigned char a;

    if (rb->read(fd, &a, 1) <= 0)
        longjmp(jmp, 1);    /* read error or unexpected end of file */
    return a;
}

/**
 *  Read 16-bit word from file, respecting windows endianness (little endian)
 *  \param  fd  file descriptor
 *  \return     data
 */
static int read_word(int fd)
{
    int a1,a2;

    a1=read_byte(fd);
    a2=read_byte(fd);
    return (a2<<8)+a1;
}

/**
 *  Read 32-bit word from file, respecting windows endianness (little endian)
 *  \param  fd  file descriptor
 *  \return     data
 */
static long read_lword(int fd)
{
    int a1,a2;

    a1=read_word(fd);
    a2=read_word(fd);
    return (a2<<16)+a1;
}


/**
 *  Open a *.lnk file and scan for a relative link target
 *  (currently only those are supported).
 *  \param  filename    the *.lnk file
 *  \param  link_target the extracted link destination
 *  \param  target_size available space for the extracted link (in bytes)
 *  \param  link_flags  the link flags are stored here
 *  \param  file_atts   file attributes are stored here
 *  \return             returns false if extraction failed.
 */
static bool extract_link_destination(char const *filename,
            volatile char *link_target_, const int target_size,
            uint32_t *link_flags, uint32_t *file_atts)
{
    /* this construct (copying volatile parameter) is used to get rid of gcc's
       "argument might be clobbered by 'longjmp'" warning */
    char *link_target = (char *) link_target_;

    int fd = rb->open(filename, O_RDONLY);
    if (fd < 0) {
        DEBUGF("Can't open link file\n");
        rb->splashf(HZ*3, "Can't open link file!");
        return false;
    }

    if (setjmp(jmp))
        goto error;     /* read error or unexpected EOF */

    /* Read ShellLinkHeader */
    long size = read_lword(fd);
    if (size!=0x4c) {       /* header size MUST be 76 bytes */
        DEBUGF("unexpected header size 0x%08lx (must be 0x0000004c)\n", size);
        goto error;
    }

    /* Skip LinkCLSID (class identifier) */
    rb->lseek(fd, 0x10, SEEK_CUR);
    /* We need the LinkFlags and File attribute (to see if target is a directory) */
    *link_flags = read_lword(fd);
    *file_atts = read_lword(fd);
    rb->lseek(fd, size, SEEK_SET);    /* Skip to end of header */

    /* For now we only support relative links, so we can exit right away
       if no relative link structure is present */
    if (!(*link_flags & HAS_RELATIVE_PATH)) {
        DEBUGF("Link doesn't have relative path information\n");
        goto error;
    }

    /* Read (skip) LinkTargetIDList structure if present */
    if (*link_flags & HAS_LINK_TARGET_ID_LIST) {
        int size=read_word(fd);
        rb->lseek(fd, size, SEEK_CUR);
    }

    /* Read (skip) LinkInfo structure if present */
    if (*link_flags & HAS_LINK_INFO) {
        int size = read_lword(fd);
        rb->lseek(fd, size-4, SEEK_CUR);
    }

    /* String Data section */

    /* Read (skip) NAME_STRING StringData structure if present */
    if (*link_flags & HAS_NAME) {
        int ccount = read_word(fd);
        if (*link_flags & IS_UNICODE)
            rb->lseek(fd, ccount*2, SEEK_CUR);
        else
            rb->lseek(fd, ccount, SEEK_CUR);
    }

    /* Read RELATIVE_PATH StringData structure if present */
    /* This is finally the data we are searching for! */
    if (*link_flags & HAS_RELATIVE_PATH) {
        int ccount = read_word(fd);
        if (*link_flags & IS_UNICODE) {
            unsigned char utf16[4], utf8[10];
            link_target[0] = '\0';
            for (int i=0; i<ccount; ++i) {
                utf16[0] = read_byte(fd);
                utf16[1] = read_byte(fd);
                /* check for surrogate pair and read the second char */
                if (utf16[1] >= 0xD8 && utf16[1] < 0xE0) {
                    utf16[2] = read_byte(fd);
                    utf16[3] = read_byte(fd);
                    ++i;
                }
                char *ptr = rb->utf16LEdecode(utf16, utf8, 1);
                *ptr = '\0';
                rb->strlcat(link_target, utf8, target_size);
            }
        }
        else { /* non-unicode */
            if (ccount >= target_size) {
                DEBUGF("ERROR: link target filename exceeds size!");
                goto error;
            }
            rb->read(fd, link_target, ccount);
            link_target[ccount] = '\0';
        }
    }

    /* convert from windows to unix subdir separators */
    for (int i=0; link_target[i] != '\0'; ++i) {
        if (link_target[i]=='\\')
            link_target[i] = '/';
    }

    rb->close(fd);
    return true;
error:
    rb->close(fd);
    DEBUGF("Error in extract_link_destination\n");
    rb->splashf(HZ*3, "Unsupported or erroneous link file format");
    return false;
}


/**
 * strip rightmost part of file/pathname to next '/', i.e. remove filename
 * or last subdirectory. Leaves a trailing '/' character.
 * \param   pathname    full path or filename
*/
static void strip_rightmost_part(char *pathname)
{
    for (int i = rb->strlen(pathname)-2; i >= 0; --i) {
        if (pathname[i] == '/') {
            pathname[i+1] = '\0'; /* cut off */
            return;
        }
    }
    pathname[0] = '\0';
}


/**
 * Combine link file's absolute path with relative link target to form
 * (absolute) link destination
 * \param   abs_path    full shortcut filename (including path)
 * \param   rel_path    the extracted relative link target
 * \param   max_len     maximum lengt of combined filename
 */
static void assemble_link_dest(char *const abs_path, char const *rel_path,
                                const size_t max_len)
{
    strip_rightmost_part(abs_path); /* cut off link filename */

    for (;;) {
        if (rb->strncmp(rel_path, "../", 3)==0) {
            rel_path += 3;
            strip_rightmost_part(abs_path);
            continue;
        }
        else if (rb->strncmp(rel_path, "./", 2)==0) {
            rel_path += 2;
            continue;
        }
        else
            break;
    }

    if (*rel_path=='/')
        ++rel_path;      /* avoid double '/' chars when concatenating */
    rb->strlcat(abs_path, rel_path, max_len);
}


/**
 *  Select the chosen file in the file browser. A directory (filename ending
 *  with '/') will be entered.
 *  \param  link_target link target to be selected in the browser
 *  \return             returns false if the target doesn't exist
 */
static bool goto_entry(char *link_target)
{
    DEBUGF("Trying to go to '%s'...\n", link_target);
    if (!rb->file_exists(link_target))
        return false;

    /* Set the browsers dirfilter to the global setting.
     * This is required in case the plugin was launched
     * from the plugins browser, in which case the
     * dirfilter is set to only display .rock files */
    rb->set_dirfilter(rb->global_settings->dirfilter);

    /* Change directory to the entry selected by the user */
    rb->set_current_file(link_target);
    return true;
}


enum plugin_status plugin_start(const void* void_parameter)
{
    char *link_filename;
    char extracted_link[MAX_PATH];
    char link_target[MAX_PATH];
    uint32_t lnk_flags;
    uint32_t file_atts;

    /* This is a viewer, so a parameter must have been specified */
    if (void_parameter == NULL) {
        rb->splash(HZ*3, "No *.lnk file selected");
        return PLUGIN_OK;
    }

    link_filename = (char*)void_parameter;
    DEBUGF("Shortcut filename: \"%s\"\n", link_filename);

    if (!extract_link_destination(link_filename, extracted_link,
                           sizeof(extracted_link), &lnk_flags, &file_atts)) {
        return PLUGIN_OK;
    }
    DEBUGF("Shortcut destination: \"%s\"\n", extracted_link);

    rb->strcpy(link_target, link_filename);
    assemble_link_dest(link_target, extracted_link, sizeof(link_target));
    DEBUGF("Link absolute path: \"%s\"\n", link_target);

    /* if target is a directory, add '/' to the dir name,
       so that the directory gets entered instead of just highlighted */
    if (file_atts & FILE_ATTRIBUTE_DIRECTORY)
        if (link_target[rb->strlen(link_target)-1] != '/')
            rb->strlcat(link_target, "/", sizeof(link_target));

    if (!goto_entry(link_target)) {
        char *what;
        if (file_atts & FILE_ATTRIBUTE_DIRECTORY)
            what = "directory";
        else
            what = "file";
        rb->splashf(HZ*3, "Can't find %s %s", what, link_target);
        DEBUGF("Can't find %s %s", what, link_target);
        return PLUGIN_OK;
    }

    return PLUGIN_OK;
}
