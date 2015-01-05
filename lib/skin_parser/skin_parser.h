/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2010 Robert Bieber
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

#ifndef GENERIC_PARSER_H
#define GENERIC_PARSER_H

#ifdef __cplusplus
extern "C"
{
#endif
#include <stdlib.h>
#include <stdbool.h>

#if defined(ROCKBOX) && !defined(__PCTOOL__)
/* Use this type and macro to convert a pointer from the
 * skin buffer to a useable pointer */
typedef long skinoffset_t;
#define SKINOFFSETTOPTR(base, offset) ((offset) < 0 ? NULL : ((void*)&base[offset]))
#define PTRTOSKINOFFSET(base, pointer) ((pointer) ? ((void*)pointer-(void*)base) : -1)
/* Use this macro when declaring a variable to self-document the code.
 * type is the actual type being pointed to (i.e OFFSETTYPE(char*) foo )
 * 
 * WARNING: Don't use the PTRTOSKINOFFSET() around a function call as it wont
 * do what you expect.
 */
#define OFFSETTYPE(type) skinoffset_t
#else
#define SKINOFFSETTOPTR(base, offset) offset
#define PTRTOSKINOFFSET(base, pointer) pointer
#define OFFSETTYPE(type) type
#endif

/********************************************************************
 ****** Data Structures *********************************************
 *******************************************************************/

/* Possible types of element in a WPS file */
enum skin_element_type
{
    UNKNOWN = -1,
    VIEWPORT,
    LINE_ALTERNATOR,
    LINE,
    CONDITIONAL,
    TAG,
    TEXT,
    COMMENT,
};

enum skin_errorcode
{
    MEMORY_LIMIT_EXCEEDED,
    NEWLINE_EXPECTED,
    ILLEGAL_TAG,
    ARGLIST_EXPECTED,
    TOO_MANY_ARGS,
    DEFAULT_NOT_ALLOWED,
    UNEXPECTED_NEWLINE,
    INSUFFICIENT_ARGS,
    INT_EXPECTED,
    DECIMAL_EXPECTED,
    SEPARATOR_EXPECTED,
    CLOSE_EXPECTED,
    MULTILINE_EXPECTED,
    GOT_CALLBACK_ERROR
};

/* Holds a tag parameter, either numeric or text */
struct skin_tag_parameter
{
    enum
    {
        INTEGER,
        DECIMAL, /* stored in data.number as (whole*10)+part */
        PERCENT, /* stored in data.number as (whole*10)+part */
        STRING,
        CODE,
        DEFAULT
    } type;

    union
    {
        int number;
        OFFSETTYPE(char*) text;
        OFFSETTYPE(struct skin_element*) code;
    } data;

    char type_code;
            
};

/* Defines an element of a SKIN file,
 *
 * This is allocated a lot, so it's optimized for size */
struct skin_element
{
    /* Link to the next element */
    OFFSETTYPE(struct skin_element*) next;
    /* Pointer to an array of children */
    OFFSETTYPE(struct skin_element**) children;
    /* Placeholder for element data
     * TEXT and COMMENT uses it for the text string
     * TAG, VIEWPORT, LINE, etc may use it for post parse extra storage
     */
    OFFSETTYPE(void*) data;

    /* The tag or conditional name */
    const struct tag_info *tag;

    /* Pointer to an array of parameters */
    OFFSETTYPE(struct skin_tag_parameter*) params;

    /* Number of elements in the children array */
    short children_count;
    /* The line on which it's defined in the source file */
    short line;
    /* Defines what type of element it is */
    enum skin_element_type type;
    /* Number of elements in the params array */
    char params_count;
};

enum skin_cb_returnvalue
{
    CALLBACK_ERROR = -666,
    FEATURE_NOT_AVAILABLE,
    CALLBACK_OK = 0,
    /* > 0 reserved for future use */
};
typedef int (*skin_callback)(struct skin_element* element, void* data);

/***********************************************************************
 ***** Functions *******************************************************
 **********************************************************************/

/* Parses a WPS document and returns a list of skin_element
   structures. */
#ifdef ROCKBOX
struct skin_element* skin_parse(const char* document, 
                                skin_callback callback, void* callback_data);
#else
struct skin_element* skin_parse(const char* document);
#endif
/* Memory management functions */
char* skin_alloc_string(int length);

void skin_free_tree(struct skin_element* root);


#ifdef __cplusplus
}
#endif

#endif /* GENERIC_PARSER_H */
