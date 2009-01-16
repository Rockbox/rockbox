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

PLUGIN_HEADER

/* screen info */
static int display_columns, display_lines;

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

/* A funtion to get width and height etc (from viewer.c) */
void init_screen(void)
{
#ifdef HAVE_LCD_BITMAP
    int w,h;

    rb->lcd_getstringsize("o", &w, &h);
    display_lines = LCD_HEIGHT / h;
    display_columns = LCD_WIDTH / w;
#else

    display_lines = 2;
    display_columns = 11;
#endif
}

/* global vars for pl_malloc() */
void *bufptr;
ssize_t bufleft;

/* simple function to "allocate" memory in pluginbuffer. */
void *pl_malloc(ssize_t size)
{
    void *ptr;
    ptr = bufptr;

    if (bufleft < size)
    {
        return NULL;
    }
    else
    {
        bufptr += size;
        bufleft -= size;
        return ptr;
    }
}

/* init function for pl_malloc() */
void pl_malloc_init(void)
{
    bufptr = rb->plugin_get_buffer((size_t *)&bufleft);
}

/* for endian problems */
#ifdef ROCKBOX_BIG_ENDIAN
#define reverse(x) x
#else
long reverse (long N) {
    unsigned char B[4];
    B[0] = (N & 0x000000FF) >> 0;
    B[1] = (N & 0x0000FF00) >> 8;
    B[2] = (N & 0x00FF0000) >> 16;
    B[3] = (N & 0xFF000000) >> 24;
    return ((B[0] << 24) | (B[1] << 16) | (B[2] << 8) | (B[3] << 0));
}
#endif

/* Button definitions */
#if CONFIG_KEYPAD == PLAYER_PAD
#define LP_QUIT BUTTON_STOP
#elif (CONFIG_KEYPAD == IPOD_4G_PAD) || \
      (CONFIG_KEYPAD == IPOD_3G_PAD) || \
      (CONFIG_KEYPAD == IPOD_1G2G_PAD)
#define LP_QUIT BUTTON_MENU
#elif CONFIG_KEYPAD == IRIVER_IFP7XX_PAD
#define LP_QUIT BUTTON_PLAY
#elif CONFIG_KEYPAD == IAUDIO_X5M5_PAD
#define LP_QUIT BUTTON_POWER
#elif CONFIG_KEYPAD == GIGABEAT_PAD
#define LP_QUIT BUTTON_POWER
#elif (CONFIG_KEYPAD == SANSA_E200_PAD) || \
      (CONFIG_KEYPAD == SANSA_C200_PAD) || \
      (CONFIG_KEYPAD == SANSA_CLIP_PAD) || \
      (CONFIG_KEYPAD == SANSA_M200_PAD)
#define LP_QUIT BUTTON_POWER
#elif (CONFIG_KEYPAD == SANSA_FUZE_PAD)
/* TODO: #define LP_QUIT BUTTOM_HOME */
#define LP_QUIT BUTTON_POWER
#elif CONFIG_KEYPAD == IRIVER_H10_PAD
#define LP_QUIT BUTTON_POWER
#elif CONFIG_KEYPAD == MROBE500_PAD
#define LP_QUIT BUTTON_POWER
#elif CONFIG_KEYPAD == MROBE100_PAD
#define LP_QUIT BUTTON_POWER
#elif CONFIG_KEYPAD == GIGABEAT_S_PAD
#define LP_QUIT BUTTON_BACK
#elif CONFIG_KEYPAD == IAUDIO_M3_PAD
#define LP_QUIT BUTTON_RC_REC
#elif CONFIG_KEYPAD == COWOND2_PAD
#define LP_QUIT BUTTON_POWER
#elif CONFIG_KEYPAD == IAUDIO67_PAD
#define LP_QUIT BUTTON_POWER
#elif CONFIG_KEYPAD == CREATIVEZVM_PAD
#define LP_QUIT BUTTON_BACK
#else
#define LP_QUIT BUTTON_OFF
#endif

/* data files */
#define DICT_INDEX ROCKBOX_DIR "/rocks/apps/dict.index"
#define DICT_DESC ROCKBOX_DIR "/rocks/apps/dict.desc"

/* the main plugin function */
enum plugin_status plugin_start(const void* parameter)
{
    char searchword[WORDLEN]; /* word to search for */
    char *description; /* pointer to description buffer */
    char *output; /* pointer to output buffer */
    char *ptr, *space;
    struct stWord word; /* the struct to read into */
    int fIndex, fData; /* files */
    int filesize, high, low, probe;
    int lines, len, outputted, next;

    /* plugin stuff */
    (void)parameter;

    /* get screen info */
    init_screen();

    /* get pl_malloc() buffer ready. */
    pl_malloc_init();

    /* init description buffer (size is because we don't have scrolling)*/
    description = (char *)pl_malloc(display_columns * display_lines);
    if (description == NULL)
    {
        DEBUGF("Err: failed to allocate description buffer.");
        return PLUGIN_ERROR;
    }

    /* init output buffer */
    output = (char *)pl_malloc(display_columns);
    if (output == NULL)
    {
        DEBUGF("Err: failed to allocate output buffer.");
        return PLUGIN_ERROR;
    }

    /* "clear" input buffer */
    searchword[0] = '\0';

    rb->kbd_input(searchword, sizeof(searchword)); /* get the word to search */

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

    /* Check if we found something */
    if (low == -1 || rb->strcasecmp(searchword, word.word) != 0)
    {
        DEBUGF("Not found.\n");
        rb->splash(HZ*2, "Not found.");
        rb->close(fIndex);
        return PLUGIN_OK;
    }

    DEBUGF("Found %s at offset %ld\n", word.word, reverse(word.offset));

    /* now open the description file */
    fData = rb->open(DICT_DESC, O_RDONLY);
    if (fData < 0)
    {
        DEBUGF("Err: Failed to open description file.\n");
        rb->splash(HZ*2, "Failed to open descriptions.");
        rb->close(fIndex);
        return PLUGIN_ERROR;
    }

    /* seek to the right offset */
    rb->lseek(fData, (off_t)reverse(word.offset), SEEK_SET);

    /* Read in the description */
    rb->read_line(fData, description, display_columns * display_lines);

    /* And print it to debug. */
    DEBUGF("Description: %s\n", description);

    /* get pointer to first char */
    ptr = description;

    lines = 0;
    outputted = 0;
    len = rb->strlen(description);

    /* clear screen */
    rb->lcd_clear_display();

    /* for large screens display the searched word. */
    if(display_lines > 4)
    {
        rb->lcd_puts(0, lines, searchword);
        lines++;
    }

    /* TODO: Scroll, or just stop when there are to much lines. */
    while (1)
    {
        /* copy one lcd line */
        rb->strncpy(output, ptr, display_columns);
        output[display_columns] = '\0';

        /* typecast to kill a warning... */
        if((int)rb->strlen(ptr) < display_columns)
        {
            rb->lcd_puts(0, lines, output);
            lines++;
            break;
        }


        /* get the last spacechar */
        space = rb->strrchr(output, ' ');

        if (space != NULL)
        {
            *space = '\0';
            next = (space - (char*)output) + 1;
        }
        else
        {
            next = display_columns;
        }

        /* put the line on screen */
        rb->lcd_puts(0, lines, output);

        /* get output count */
        outputted += rb->strlen(output);

        if (outputted < len)
        {
            /* set pointer to the next part */
            ptr += next;
            lines++;
        }
        else
        {
            break;
        }
    }
    rb->lcd_update();

    /* wait for keypress */
    while(rb->button_get(true) != LP_QUIT)
    {
        /* do nothing */
        /* maybe define some keys for navigation here someday. */
    }

    rb->close(fIndex);
    rb->close(fData);
    return PLUGIN_OK;
}
