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
    MULTILINE_EXPECTED
};

/* Holds a tag parameter, either numeric or text */
struct skin_tag_parameter
{
    enum
    {
        INTEGER,
        DECIMAL, /* stored in data.number as (whole*10)+part */
        STRING,
        CODE,
        DEFAULT
    } type;

    union
    {
        int number;
        char* text;
        struct skin_element* code;
    } data;

    char type_code;
            
};

/* Defines an element of a SKIN file */
struct skin_element
{
    /* Defines what type of element it is */
    enum skin_element_type type;

    /* The line on which it's defined in the source file */
    int line;

    /* Placeholder for element data
     * TEXT and COMMENT uses it for the text string
     * TAG, VIEWPORT, LINE, etc may use it for post parse extra storage
     */
    void* data;

    /* The tag or conditional name */
    const struct tag_info *tag;

    /* Pointer to and size of an array of parameters */
    int params_count;
    struct skin_tag_parameter* params;

    /* Pointer to and size of an array of children */
    int children_count;
    struct skin_element** children;

    /* Link to the next element */
    struct skin_element* next;
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
struct skin_element* skin_alloc_element(void);
struct skin_element** skin_alloc_children(int count);
struct skin_tag_parameter* skin_alloc_params(int count, bool use_shared_params);
char* skin_alloc_string(int length);

void skin_free_tree(struct skin_element* root);


#ifdef __cplusplus
}
#endif

#endif /* GENERIC_PARSER_H */
