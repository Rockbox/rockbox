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

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>

#include "skin_buffer.h"
#include "skin_parser.h"
#include "skin_debug.h"
#include "tag_table.h"
#include "symbols.h"
#include "skin_scan.h"

/* Global variables for the parser */
int skin_line = 0;
char* skin_start = 0;
int viewport_line = 0;

#ifdef ROCKBOX
static skin_callback callback = NULL;
static void* callback_data;
#endif

/* Auxiliary parsing functions (not visible at global scope) */
static struct skin_element* skin_parse_viewport(const char** document);
static struct skin_element* skin_parse_line(const char** document);
static struct skin_element* skin_parse_line_optional(const char** document,
                                                     int conditional);
static struct skin_element* skin_parse_sublines(const char** document);
static struct skin_element* skin_parse_sublines_optional(const char** document,
                                                         int conditional);

static int skin_parse_tag(struct skin_element* element, const char** document);
static int skin_parse_text(struct skin_element* element, const char** document,
                           int conditional);
static int skin_parse_conditional(struct skin_element* element,
                                  const char** document);
static int skin_parse_comment(struct skin_element* element, const char** document);
static struct skin_element* skin_parse_code_as_arg(const char** document);


static void skip_whitespace(const char** document)
{
    while(**document == ' ' || **document == '\t')
        (*document)++;
}

#ifdef ROCKBOX
struct skin_element* skin_parse(const char* document, 
                                skin_callback cb, void* cb_data)
{
    callback = cb;
    callback_data = cb_data;
#else
struct skin_element* skin_parse(const char* document)
{
#endif
    struct skin_element* root = NULL;
    struct skin_element* last = NULL;

    const char* cursor = document; /*Keeps track of location in the document*/
    
    skin_line = 1;
    skin_start = (char*)document;
    viewport_line = 0;

    skin_clear_errors();

    while(*cursor != '\0')
    {
        struct skin_element* tree = skin_parse_viewport(&cursor);
        if(!root)
        {
            root = tree;
            last = root;
        }
        else
        {
            last->next = skin_buffer_to_offset(tree);
            last = tree;
        }

        if(!last)
        {
            skin_free_tree(root); /* Clearing any memory already used */
            return NULL;
        }

        /* Making sure last is at the end */
        while(IS_VALID_OFFSET(last->next))
            last = skin_buffer_from_offset(last->next);

    }
    return root;

}

static struct skin_element* skin_parse_viewport(const char** document)
{
    struct skin_element* root = NULL;
    struct skin_element* last = NULL;
    struct skin_element* retval = NULL;

    retval = skin_alloc_element();
    if (!retval)
        return NULL;
    retval->type = VIEWPORT;
    retval->children_count = 1;
    retval->line = skin_line;
    viewport_line = skin_line;

    OFFSETTYPE(struct skin_element*)* children;

    const char* cursor = *document; /* Keeps track of location in the document */
    const char* bookmark; /* Used when we need to look ahead */

    int sublines = 0; /* Flag for parsing sublines */

    /* Parsing out the viewport tag if there is one */
    if(check_viewport(cursor))
    {
        if (!skin_parse_tag(retval, &cursor))
            return NULL;
        if(*cursor == '\n')
        {
            cursor++;
            skin_line++;
        }
    }
#ifdef ROCKBOX
    else if (callback)
    {
        if (callback(retval, callback_data) == CALLBACK_ERROR)
            return NULL;
    }
#endif

    if (check_viewport(cursor))
    {
        retval->children_count = 0;
        *document = cursor;
        return retval;
    }
    retval->children_count = 1;
    children = skin_alloc_children(1);
    if (!children)
        return NULL;
    do
    {

        /* First, we check to see if this line will contain sublines */
        bookmark = cursor;
        sublines = 0;
        while(*cursor != '\n' && *cursor != '\0'
              && !(check_viewport(cursor) && cursor != *document))
        {
            if(*cursor == MULTILINESYM)
            {
                sublines = 1;
                break;
            }
            else if(*cursor == TAGSYM)
            {
                skip_tag(&cursor);
            }
            else if(*cursor == COMMENTSYM)
            {
                skip_comment(&cursor);
            }
            else
            {
                /* Advancing the cursor as normal */
                cursor++;
            }
        }
        cursor = bookmark;

        if(sublines)
        {
            struct skin_element* out = skin_parse_sublines(&cursor);
            if (!root)
            {
                root = out;
                last = root;
            }
            else
            {
                last->next = skin_buffer_to_offset(out);
                last = out;
            }
            if(!last)
                return NULL;
        }
        else
        {
#ifdef ROCKBOX
            /* strip all leading comments */
            while(*cursor == '#')
            {
                skip_comment(&cursor);
                skin_line++;
                
            }
            if (check_viewport(cursor))
                break;
#endif

            struct skin_element* out = skin_parse_line(&cursor);
            if (!root)
            {
                root = out;
                last = root;
            }
            else
            {
                last->next = skin_buffer_to_offset(out);
                last = out;
            }
            if(!last)
                return NULL;

        }
        /* Making sure last is at the end */
        while(IS_VALID_OFFSET(last->next))
            last = skin_buffer_from_offset(last->next);

        if(*cursor == '\n')
        {
            cursor++;
            skin_line++;
        }
#ifdef ROCKBOX
        /* strip all comments */
        while(*cursor == '#')
        {
            skip_comment(&cursor);
            skin_line++;
            
        }
        if (check_viewport(cursor))
            break;
#endif

    }
    while(*cursor != '\0' && !(check_viewport(cursor) && cursor != *document));

    *document = cursor;

    children[0] = skin_buffer_to_offset(root);
    retval->children = skin_buffer_to_offset(children);
    return retval;
}

/* Auxiliary Parsing Functions */

static struct skin_element* skin_parse_line(const char**document)
{
    return skin_parse_line_optional(document, 0);
}

/*
 * If conditional is set to true, then this will break upon encountering
 * SEPARATESYM.  This should only be used when parsing a line inside a
 * conditional, otherwise just use the wrapper function skin_parse_line()
 */
static struct skin_element* skin_parse_line_optional(const char** document,
                                                     int conditional)
{
    const char* cursor = *document;

    struct skin_element* root = NULL;
    struct skin_element* current = NULL;
    struct skin_element* retval = NULL;
    OFFSETTYPE(struct skin_element*)* children = NULL;

    /* A wrapper for the line */
    retval = skin_alloc_element();
    if (!retval)
        return NULL;
    retval->type = LINE;
    retval->line = skin_line;
    while (*cursor == '\t')
        cursor++;

    if(*cursor != '\0' && *cursor != '\n' && *cursor != MULTILINESYM
       && !(conditional && (*cursor == ARGLISTSEPARATESYM
                            || *cursor == ARGLISTCLOSESYM
                            || *cursor == ENUMLISTSEPARATESYM
                            || *cursor == ENUMLISTCLOSESYM)))
    {
        retval->children_count = 1;
    }
    else
    {
        retval->children_count = 0;
    }

    if(retval->children_count > 0)
    {
        children = skin_alloc_children(1);
        if (!children)
            return NULL;
    }

#ifdef ROCKBOX
    if (callback)
    {
        switch (callback(retval, callback_data))
        {
            case CALLBACK_ERROR:
                return NULL;
            default:
                break;
        }
    }
#endif

    while(*cursor != '\n' && *cursor != '\0' && *cursor != MULTILINESYM
          && !((*cursor == ARGLISTSEPARATESYM
                || *cursor == ARGLISTCLOSESYM
                || *cursor == ENUMLISTSEPARATESYM
                || *cursor == ENUMLISTCLOSESYM)
               && conditional)
          && !(check_viewport(cursor) && cursor != *document))
    {
        /* Allocating memory if necessary */
        if(root)
        {
            struct skin_element *next = skin_alloc_element();
            if (!next)
                return NULL;
            current->next = skin_buffer_to_offset(next);
            current = next;
        }
        else
        {
            current = skin_alloc_element();
            if (!current)
                return NULL;
            root = current;
        }

        /* Parsing the current element */
        if(*cursor == TAGSYM && cursor[1] == CONDITIONSYM)
        {
            if(!skin_parse_conditional(current, &cursor))
                return NULL;
        }
        else if(*cursor == TAGSYM && !find_escape_character(cursor[1]))
        {
            if(!skin_parse_tag(current, &cursor))
                return NULL;
        }
        else if(*cursor == COMMENTSYM)
        {
            if(!skin_parse_comment(current, &cursor))
                return NULL;
        }
        else
        {
            if(!skin_parse_text(current, &cursor, conditional))
                return NULL;
        }
    }

    /* Moving up the calling function's pointer */
    *document = cursor;
    
    if(root)
    {
        children[0] = skin_buffer_to_offset(root);
        retval->children = skin_buffer_to_offset(children);
    }
    return retval;
}

static struct skin_element* skin_parse_sublines(const char** document)
{
    return skin_parse_sublines_optional(document, 0);
}

static struct skin_element* skin_parse_sublines_optional(const char** document,
                                                  int conditional)
{
    struct skin_element* retval;
    OFFSETTYPE(struct skin_element*)* children;
    const char* cursor = *document;
    int sublines = 1;
    int i;

    retval = skin_alloc_element();
    if (!retval)
        return NULL;
    retval->type = LINE_ALTERNATOR;
    retval->next = skin_buffer_to_offset(NULL);
    retval->line = skin_line;
    while (*cursor == '\t')
        cursor++;

    /* First we count the sublines */
    while(*cursor != '\0' && *cursor != '\n'
          && !((*cursor == ARGLISTSEPARATESYM
                || *cursor == ARGLISTCLOSESYM
                || *cursor == ENUMLISTSEPARATESYM
                || *cursor == ENUMLISTCLOSESYM)
               && conditional)
          && !(check_viewport(cursor) && cursor != *document))
    {
        if(*cursor == COMMENTSYM)
        {
            skip_comment(&cursor);
        }
        else if(*cursor == TAGSYM)
        {
            skip_tag(&cursor);
        }
        else if(*cursor == MULTILINESYM)
        {
            sublines++;
            cursor++;
        }
        else
        {
            cursor++;
        }
    }

    /* ...and then we parse them */
    retval->children_count = sublines;
    children = skin_alloc_children(sublines);
    if (!children)
        return NULL;

    cursor = *document;
    for(i = 0; i < sublines; i++)
    {
        children[i] = skin_buffer_to_offset(skin_parse_line_optional(&cursor, conditional));
        if (children[i] < 0)
            return NULL;
        skip_whitespace(&cursor);

        if(*cursor != MULTILINESYM && i != sublines - 1)
        {
            skin_error(MULTILINE_EXPECTED, cursor);
            return NULL;
        }
        else if(i != sublines - 1)
        {
            cursor++;
        }
    }

#ifdef ROCKBOX
    if (callback)
    {
        if (callback(retval, callback_data) == CALLBACK_ERROR)
            return NULL;
    }
#endif
    *document = cursor;
    retval->children = skin_buffer_to_offset(children);

    return retval;
}

static int skin_parse_tag(struct skin_element* element, const char** document)
{
    const char* cursor = *document + 1;
    const char* bookmark;
    char *open_square_bracket = NULL;

    char tag_name[MAX_TAG_LENGTH];
    char* tag_args;
    const struct tag_info *tag;
    struct skin_tag_parameter* params = NULL;

    int num_args = 1;
    int i;
    int qmark = 0; /* Flag for the all-or-none option */

    int optional = 0;

    /* Checking the tag name */
    for (i=0; cursor[i] && i<MAX_TAG_LENGTH; i++)
        tag_name[i] = cursor[i];

    /* First we check the two characters after the '%', then a single char */
    tag = NULL;
    i = MAX_TAG_LENGTH;
    while (!tag && i > 1)
    {
        tag_name[i-1] = '\0';
        tag = find_tag(tag_name);
        i--;
    }

    if(!tag)
    {
        skin_error(ILLEGAL_TAG, cursor);
        return 0;
    }
    cursor += i;

    /* Copying basic tag info */
    if(element->type != CONDITIONAL && element->type != VIEWPORT)
        element->type = TAG;
    element->tag = tag;
    tag_args = tag->params;
    element->line = skin_line;

    /* Checking for the * flag */
    if(tag_args[0] == '?')
    {
        qmark = 1;
        tag_args++;
    }

    /* If this tag has no arguments, we can bail out now */
    if(strlen(tag_args) == 0
       || (tag_args[0] == '|' && *cursor != ARGLISTOPENSYM)
       || (qmark && *cursor != ARGLISTOPENSYM))
    {
        
#ifdef ROCKBOX
        if (callback)
        {
            if (callback(element, callback_data) == CALLBACK_ERROR)
                return 0;
        }
#endif
        *document = cursor;
        return 1;
    }

    /* Checking the number of arguments and allocating args */
    if(*cursor != ARGLISTOPENSYM && tag_args[0] != '|')
    {
        skin_error(ARGLIST_EXPECTED, cursor);
        return 0;
    }
    else
    {
        cursor++;
    }

    bookmark = cursor;
    while(*cursor != '\n' && *cursor != '\0' && *cursor != ARGLISTCLOSESYM)
    {
        /* Skipping over escaped characters */
        if(*cursor == TAGSYM)
        {
            skip_tag(&cursor);
        }
        else if(*cursor == COMMENTSYM)
        {
            skip_comment(&cursor);
        }
        else if(*cursor == ARGLISTSEPARATESYM)
        {
            num_args++;
            cursor++;
        }
        else
        {
            cursor++;
        }
    }

    cursor = bookmark; /* Restoring the cursor */
    element->params_count = num_args;
    params = skin_alloc_params(num_args);
    if (!params)
        return 0;

    /* Now we have to actually parse each argument */
    for(i = 0; i < num_args; i++)
    {
        char type_code;
        /* Making sure we haven't run out of arguments */
        if(*tag_args == '\0')
        {
            skin_error(TOO_MANY_ARGS, cursor);
            return 0;
        }

        /* Checking for the optional bar */
        if(*tag_args == '|')
        {
            optional = 1;
            tag_args++;
        }

        /* Scanning the arguments */
        skip_whitespace(&cursor);

        /* Checking for comments */
        if(*cursor == COMMENTSYM)
            skip_comment(&cursor);
            
        if (*tag_args == '[')
        {
            /* we need to guess which type of param it is. 
             * guess using this priority:
             * default > decimal/integer > single tag/code > string
             */
            int j=0;
            bool canbedefault = false;
            bool haspercent = false, number = true, hasdecimal = false;
            char temp_params[8];
            open_square_bracket = tag_args;
            tag_args++;
            while (*tag_args != ']')
            {
                if (*tag_args >= 'a' && *tag_args <= 'z')
                    canbedefault = true;
                temp_params[j++] = tolower(*tag_args++);
            }
            temp_params[j] = '\0';
            j = 0;
            while (cursor[j] && cursor[j] != ',' && cursor[j] != ')')
            {
                haspercent = haspercent || (cursor[j] == '%');
                hasdecimal = hasdecimal || (cursor[j] == '.');
                number = number && (isdigit(cursor[j]) || 
                                    (cursor[j] == '.') ||
                                    (cursor[j] == '-'));
                j++;
            }
            type_code = '?';
            if (canbedefault && *cursor == DEFAULTSYM && !isdigit(cursor[1]))
            {
                type_code = 'i';
            }
            else if (number && hasdecimal && strchr(temp_params, 'd'))
            {
                type_code = 'd';
            }
            else if (number && 
                     (strchr(temp_params, 'i') || strchr(temp_params, 'd')))
            {
                type_code = strchr(temp_params, 'i') ? 'i' : 'd';
            }
            else if (haspercent && 
                    (strchr(temp_params, 't') || strchr(temp_params, 'c')))
            {
                type_code = strchr(temp_params, 't') ? 't' : 'c';
            }
            else if (strchr(temp_params, 's'))
            {
                type_code = 's';
            }
            if (type_code == '?')
            {
                skin_error(INSUFFICIENT_ARGS, cursor);
                return 0;
            }   
        }
        else
            type_code = *tag_args;
        /* Storing the type code */
        params[i].type_code = type_code;

        /* Checking a nullable argument for null. */
        if(*cursor == DEFAULTSYM && !isdigit(cursor[1]))
        {
            if(islower(type_code))
            {
                params[i].type = DEFAULT;
                cursor++;
            }
            else
            {
                skin_error(DEFAULT_NOT_ALLOWED, cursor);
                return 0;
            }
        }
        else if(tolower(type_code) == 'i')
        {
            /* Scanning an int argument */
            if(!isdigit(*cursor) && *cursor != '-')
            {
                skin_error(INT_EXPECTED, cursor);
                return 0;
            }

            params[i].type = INTEGER;
            params[i].data.number = scan_int(&cursor);
        }
        else if(tolower(type_code) == 'd')
        {
            int val = 0;
            bool have_point = false;
            bool have_tenth = false;
            while ( isdigit(*cursor) || *cursor == '.' )
            {
                if (*cursor != '.')
                {
                    val *= 10;
                    val += *cursor - '0';
                    if (have_point)
                    {
                        have_tenth = true;
                        cursor++;
                        break;
                    }
                }
                else
                    have_point = true;
                cursor++;
            }
            if (have_tenth == false)
                val *= 10;
            params[i].type = DECIMAL;
            params[i].data.number = val;
        }
        else if(tolower(type_code) == 's' || tolower(type_code) == 'f')
        {
            /* Scanning a string argument */
            params[i].type = STRING;
            params[i].data.text = skin_buffer_to_offset(scan_string(&cursor));

        }
        else if(tolower(type_code) == 'c')
        {
            /* Recursively parsing a code argument */
            params[i].type = CODE;
            params[i].data.code = skin_buffer_to_offset(skin_parse_code_as_arg(&cursor));
            if(params[i].data.code < 0)
                return 0;
        }
        else if (tolower(type_code) == 't')
        {
            struct skin_element* child = skin_alloc_element();
            child->type = TAG;
            if (!skin_parse_tag(child, &cursor))
                return 0;
            child->next = skin_buffer_to_offset(NULL);
            params[i].type = CODE;
            params[i].data.code = skin_buffer_to_offset(child);
        }
            

        skip_whitespace(&cursor);

        if(*cursor != ARGLISTSEPARATESYM && i < num_args - 1)
        {
            skin_error(SEPARATOR_EXPECTED, cursor);
            return 0;
        }
        else if(*cursor != ARGLISTCLOSESYM && i == num_args - 1)
        {
            skin_error(CLOSE_EXPECTED, cursor);
            return 0;
        }
        else
        {
            cursor++;
        }

        if (*(tag_args + 1) == '*')
        {
            if (i+1 == num_args)
                tag_args += 2;
            else if (open_square_bracket)
            {
                tag_args = open_square_bracket;
                open_square_bracket = NULL;
            }
        }
        else
            tag_args++;

        /* Checking for the optional bar */
        if(*tag_args == '|')
        {
            optional = 1;
            tag_args++;
        }
    }
    element->params = skin_buffer_to_offset(params);

    /* Checking for a premature end */
    if(*tag_args != '\0' && !optional)
    {
        skin_error(INSUFFICIENT_ARGS, cursor);
        return 0;
    }
#ifdef ROCKBOX
    if (callback)
    {
        if (callback(element, callback_data) == CALLBACK_ERROR)
            return 0;
    }
#endif
    *document = cursor;

    return 1;
}

/*
 * If the conditional flag is set true, then parsing text will stop at an
 * ARGLISTSEPARATESYM.  Only set that flag when parsing within a conditional
 */
static int skin_parse_text(struct skin_element* element, const char** document,
                           int conditional)
{
    const char* cursor = *document;
    int length = 0;
    int dest;
    char *text = NULL;

    /* First figure out how much text we're copying */
    while(*cursor != '\0' && *cursor != '\n' && *cursor != MULTILINESYM
          && *cursor != COMMENTSYM
          && !((*cursor == ARGLISTSEPARATESYM
                || *cursor == ARGLISTCLOSESYM
                || *cursor == ENUMLISTSEPARATESYM
                || *cursor == ENUMLISTCLOSESYM)
               && conditional))
    {
        /* Dealing with possibility of escaped characters */
        if(*cursor == TAGSYM)
        {
            if(find_escape_character(cursor[1]))
                cursor++;
            else
                break;
        }

        length++;
        cursor++;
    }

    cursor = *document;

    /* Copying the text into the element struct */
    element->type = TEXT;
    element->line = skin_line;
    element->next = skin_buffer_to_offset(NULL);
    text = skin_alloc_string(length);
    element->data = skin_buffer_to_offset(text);
    if (element->data < 0)
        return 0;
    
    for(dest = 0; dest < length; dest++)
    {
        /* Advancing cursor if we've encountered an escaped character */
        if(*cursor == TAGSYM)
            cursor++;

        text[dest] = *cursor;
        cursor++;
    }
    text[length] = '\0';
    
#ifdef ROCKBOX
    if (callback)
    {
        if (callback(element, callback_data) == CALLBACK_ERROR)
            return 0;
    }
#endif

    *document = cursor;

    return 1;
}

static int skin_parse_conditional(struct skin_element* element, const char** document)
{
    const char* cursor = *document + 1; /* Starting past the "%" */
    const char* bookmark;
    int children = 1;
    int i;
    
#ifdef ROCKBOX
    bool feature_available = true;
    const char *false_branch = NULL;
    const char *conditional_end = NULL;
#endif
    OFFSETTYPE(struct skin_element*)* children_array = NULL;

    /* Some conditional tags allow for target feature checking,
     * so to handle that call the callback as usual with type == TAG
     * then call it a second time with type == CONDITIONAL and check the return
     * value */
    element->type = TAG;
    element->line = skin_line;

    /* Parsing the tag first */
    if(!skin_parse_tag(element, &cursor))
        return 0;

    element->type = CONDITIONAL;
#ifdef ROCKBOX
    if (callback)
    {
        switch (callback(element, callback_data))
        {
            case FEATURE_NOT_AVAILABLE:
                feature_available = false;
                break;
            case CALLBACK_ERROR:
                return 0;
            default:
                break;
        }
    }
#endif
    
    /* Counting the children */
    if(*(cursor++) != ENUMLISTOPENSYM)
    {
        skin_error(ARGLIST_EXPECTED, cursor);
        return 0;
    }
    bookmark = cursor;
    while(*cursor != ENUMLISTCLOSESYM && *cursor != '\0')
    {
        if(*cursor == COMMENTSYM)
        {
            skip_comment(&cursor);
        }
        else if(*cursor == TAGSYM)
        {
            skip_tag(&cursor);
        }
        else if(*cursor == ENUMLISTSEPARATESYM)
        {
            children++;
            cursor++;
            if (*cursor == '\n')
                cursor++;
#ifdef ROCKBOX
            if (false_branch == NULL && !feature_available)
            {
                false_branch = cursor;
                children--;
            }
#endif
        }
        else
        {
            cursor++;
        }
    }
#ifdef ROCKBOX
    if (*cursor == ENUMLISTCLOSESYM && 
        false_branch == NULL && !feature_available)
    {
        false_branch = cursor+1;
        children--;
    }
    if (element->tag->flags&FEATURE_TAG)
    {
        if (feature_available && children > 1)
            children--;
    }
    conditional_end = cursor;
    /* if we are skipping the true branch fix that up */
    cursor = false_branch ? false_branch : bookmark;
#else
    cursor = bookmark;
#endif
    /* Parsing the children */
    
    /* Feature tags could end up having 0 children which breaks
     * the render in dangerous ways. Minor hack, but insert an empty
     * child.  (e.g %?xx<foo> when xx isnt available ) */
    
    if (children == 0)
    {
        const char* emptyline= "";
        children = 1;
        children_array = skin_alloc_children(children);
        if (!children_array)
            return 0;
        element->children_count = children;
        children_array[0] = skin_buffer_to_offset(skin_parse_code_as_arg(&emptyline));
    }
    else
    {    
        children_array = skin_alloc_children(children);
        if (!children_array)
            return 0;
        element->children_count = children;

        for(i = 0; i < children; i++)
        {
            if (*cursor == '\n')
            {
                skin_line++;
                cursor++;
            }
            children_array[i] = skin_buffer_to_offset(skin_parse_code_as_arg(&cursor));
            if (children_array[i] < 0)
                return 0;
            skip_whitespace(&cursor);
#ifdef ROCKBOX
            if ((element->tag->flags&FEATURE_TAG) && feature_available)
                cursor = conditional_end;
#endif

            if(i < children - 1 && *cursor != ENUMLISTSEPARATESYM)
            {
                skin_error(SEPARATOR_EXPECTED, cursor);
                return 0;
            }
            else if(i == children - 1 && *cursor != ENUMLISTCLOSESYM)
            {
                skin_error(CLOSE_EXPECTED, cursor);
                return 0;
            }
            else
            {
                cursor++;
            }
        }
    }
    *document = cursor;
    element->children = skin_buffer_to_offset(children_array);

    return 1;
}

static int skin_parse_comment(struct skin_element* element, const char** document)
{
    const char* cursor = *document;
#ifndef ROCKBOX
    char* text = NULL;
#endif
    int length;
    /*
     * Finding the index of the ending newline or null-terminator
     * The length of the string of interest doesn't include the leading #, the
     * length we need to reserve is the same as the index of the last character
     */
    for(length = 0; cursor[length] != '\n' && cursor[length] != '\0'; length++);

    element->type = COMMENT;
    element->line = skin_line;
#ifdef ROCKBOX 
    element->data = INVALID_OFFSET;
#else    
    element->data = text = skin_alloc_string(length);
    if (!element->data)
        return 0;
    /* We copy from one char past cursor to leave out the # */
    memcpy((void*)text, (void*)(cursor + 1),
           sizeof(char) * (length-1));
    text[length - 1] = '\0';
#endif
    if(cursor[length] == '\n')
        skin_line++;

    *document += (length); /* Move cursor up past # and all text */
    if(**document == '\n')
        (*document)++;

    return 1;
}

static struct skin_element* skin_parse_code_as_arg(const char** document)
{
    int sublines = 0;
    const char* cursor = *document;

    /* Checking for sublines */
    while(*cursor != '\n' && *cursor != '\0'
          && *cursor != ENUMLISTSEPARATESYM && *cursor != ARGLISTSEPARATESYM
          && *cursor != ENUMLISTCLOSESYM && *cursor != ARGLISTCLOSESYM)
    {
        if(*cursor == MULTILINESYM)
        {
            sublines = 1;
            break;
        }
        else if(*cursor == TAGSYM)
        {
            skip_tag(&cursor);
        }
        else
        {
            /* Advancing the cursor as normal */
            cursor++;
        }
    }

    if(sublines)
        return skin_parse_sublines_optional(document, 1);
    else
        return skin_parse_line_optional(document, 1);
}

/* Memory management */
struct skin_element* skin_alloc_element()
{
    struct skin_element* retval =  (struct skin_element*)
                                   skin_buffer_alloc(sizeof(struct skin_element));
    if (!retval)
        return NULL;
    retval->type = UNKNOWN;
    retval->next = skin_buffer_to_offset(NULL);
    retval->params = skin_buffer_to_offset(NULL);
    retval->tag = NULL;
    retval->params_count = 0;
    retval->children_count = 0;
    retval->data = INVALID_OFFSET;

    return retval;

}
/* On a ROCKBOX build we try to save space as much as possible
 * so if we can, use a shared param pool which should be more then large
 * enough for any tag. params should be used straight away by the callback
 * so this is safe.
 */
struct skin_tag_parameter* skin_alloc_params(int count)
{
    size_t size = sizeof(struct skin_tag_parameter) * count;
    return (struct skin_tag_parameter*)skin_buffer_alloc(size);

}

char* skin_alloc_string(int length)
{
    return (char*)skin_buffer_alloc(sizeof(char) * (length + 1));
}

OFFSETTYPE(struct skin_element*)* skin_alloc_children(int count)
{
    return (OFFSETTYPE(struct skin_element*)*)
            skin_buffer_alloc(sizeof(struct skin_element*) * count);
}

void skin_free_tree(struct skin_element* root)
{
#ifndef ROCKBOX
    int i;

    /* First make the recursive call */
    if(!root)
        return;
    skin_free_tree(root->next);

    /* Free any text */
    if(root->type == TEXT || root->type == COMMENT)
        free(root->data);

    /* Then recursively free any children, before freeing their pointers */
    for(i = 0; i < root->children_count; i++)
        skin_free_tree(root->children[i]);
    if(root->children_count > 0)
        free(root->children);

    /* Free any parameters, making sure to deallocate strings */
    for(i = 0; i < root->params_count; i++)
        if(root->params[i].type == STRING)
            free(root->params[i].data.text);
    if(root->params_count > 0)
        free(root->params);

    /* Finally, delete root's memory */
    free(root);
#else
    (void)root;
#endif
}
