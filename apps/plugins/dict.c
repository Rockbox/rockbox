/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2005 Tomas Salfischberger
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
#include "lib/simple_viewer.h"



#define MIN_DESC_BUF_SIZE 0x400 /* arbitrary minimum size for description */

/* Some lenghts */
#define WORDLEN 32 /* has to be the same in rdf2binary.c */

/* Struct packing */
#ifdef __GNUC__
#define STRUCT_PACKED __attribute__((packed))
#else
#define STRUCT_PACKED
#pragma pack (push, 2)
#endif

/* The word struct :) */
struct stWord
{
    char word[WORDLEN];
    long offset;
} STRUCT_PACKED;

/* for endian problems */
#ifdef ROCKBOX_BIG_ENDIAN
#define reverse(x) x
#else
static long reverse (long N) {
    unsigned char B[4];
    B[0] = (N & 0x000000FF) >> 0;
    B[1] = (N & 0x0000FF00) >> 8;
    B[2] = (N & 0x00FF0000) >> 16;
    B[3] = (N & 0xFF000000) >> 24;
    return ((B[0] << 24) | (B[1] << 16) | (B[2] << 8) | (B[3] << 0));
}
#endif


/* data files */
#define DICT_INDEX PLUGIN_APPS_DIR "/dict.index"
#define DICT_DESC PLUGIN_APPS_DIR "/dict.desc"

/* the main plugin function */
enum plugin_status plugin_start(const void* parameter)
{
    char searchword[WORDLEN]; /* word to search for */
    char *description; /* pointer to description buffer */
    struct stWord word; /* the struct to read into */
    int fIndex, fData; /* files */
    int filesize, high, low, probe;
    char *buffer;
    size_t buffer_size;

    /* plugin stuff */
    (void)parameter;

    /* allocate buffer. */
    buffer = rb->plugin_get_buffer(&buffer_size);
    if (buffer == NULL || buffer_size < MIN_DESC_BUF_SIZE)
    {
        DEBUGF("Err: Failed to allocate buffer.\n");
        rb->splash(HZ*2, "Failed to allocate buffer.");
        return PLUGIN_ERROR;
    }

    description = buffer;

    /* "clear" input buffer */
    searchword[0] = '\0';

    /* get the word to search */
    if (rb->kbd_input(searchword, sizeof(searchword), NULL) < 0)
        return PLUGIN_OK; /* input cancelled */

    fIndex = rb->open(DICT_INDEX, O_RDONLY); /* index file */
    if (fIndex < 0)
    {
        DEBUGF("Err: Failed to open index file.\n");
        rb->splash(HZ*2, "Failed to open index.");
        return PLUGIN_ERROR;
    }

    filesize = rb->filesize(fIndex); /* get filesize */

    DEBUGF("Filesize: %d bytes = %d words \n", filesize,
           (filesize / (int)sizeof(struct stWord)));

    /* for the searching algorithm */
    high = filesize / sizeof( struct stWord );
    low = -1;

    while (high - low > 1)
    {
        probe = (high + low) / 2;

        /* Jump to word pointed by probe, and read it. */
        rb->lseek(fIndex, sizeof(struct stWord) * probe, SEEK_SET);
        rb->read(fIndex, &word, sizeof(struct stWord));

        /* jump according to the found word. */
        if (rb->strcasecmp(searchword, word.word) < 0)
        {
            high = probe;
        }
        else
        {
            low = probe;
        }
    }

    /* read in the word */
    rb->lseek(fIndex, sizeof(struct stWord) * low, SEEK_SET);
    rb->read(fIndex, &word, sizeof(struct stWord));
    rb->close(fIndex);

    /* Check if we found something */
    if (low == -1 || rb->strcasecmp(searchword, word.word) != 0)
    {
        DEBUGF("Not found.\n");
        rb->splash(HZ*2, "Not found.");
        return PLUGIN_OK;
    }

    DEBUGF("Found %s at offset %ld\n", word.word, reverse(word.offset));

    /* now open the description file */
    fData = rb->open(DICT_DESC, O_RDONLY);
    if (fData < 0)
    {
        DEBUGF("Err: Failed to open description file.\n");
        rb->splash(HZ*2, "Failed to open descriptions.");
        return PLUGIN_ERROR;
    }

    /* seek to the right offset */
    rb->lseek(fData, (off_t)reverse(word.offset), SEEK_SET);

    /* Read in the description */
    rb->read_line(fData, description, buffer_size);

    /* And print it to debug. */
    DEBUGF("Description: %s\n", description);

    rb->close(fData);

    /* display description. */
    view_text(searchword, description);

    return PLUGIN_OK;
}
