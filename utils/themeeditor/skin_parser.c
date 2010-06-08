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
#include <string.h>
#include <ctype.h>

#include "skin_parser.h"
#include "skin_debug.h"
#include "tag_table.h"
#include "symbols.h"
#include "skin_scan.h"

/* Declaration of parse tree buffer */
char skin_parse_tree[SKIN_MAX_MEMORY];
int skin_current_block = 0;

/* Global variables for the parser */
int skin_line = 0;

/* Auxiliary parsing functions (not visible at global scope) */
struct skin_element* skin_parse_viewport(char** document);
struct skin_element* skin_parse_line(char** document);
struct skin_element* skin_parse_line_optional(char** document, int conditional);
struct skin_element* skin_parse_sublines(char** document);
struct skin_element* skin_parse_sublines_optional(char** document,
                                                  int conditional);

int skin_parse_tag(struct skin_element* element, char** document);
int skin_parse_text(struct skin_element* element, char** document,
                    int conditional);
int skin_parse_conditional(struct skin_element* element, char** document);
int skin_parse_comment(struct skin_element* element, char** document);
struct skin_element* skin_parse_code_as_arg(char** document);

struct skin_element* skin_parse(const char* document)
{

    struct skin_element* root = NULL;
    struct skin_element* last = NULL;

    struct skin_element** to_write = 0;

    char* cursor = (char*)document; /*Keeps track of location in the document*/

    skin_line = 1;

    skin_clear_errors();

    while(*cursor != '\0')
    {

        if(!root)
            to_write = &root;
        else
            to_write = &(last->next);


        *to_write = skin_parse_viewport(&cursor);
        last = *to_write;
        if(!last)
        {
            skin_free_tree(root); /* Clearing any memory already used */
            return NULL;
        }

        /* Making sure last is at the end */
        while(last->next)
            last = last->next;

    }

    return root;

}

struct skin_element* skin_parse_viewport(char** document)
{

    struct skin_element* root = NULL;
    struct skin_element* last = NULL;
    struct skin_element* retval = NULL;

    retval = skin_alloc_element();
    retval->type = VIEWPORT;
    retval->children_count = 1;
    retval->line = skin_line;

    struct skin_element** to_write = 0;

    char* cursor = *document; /* Keeps track of location in the document */
    char* bookmark; /* Used when we need to look ahead */

    int sublines = 0; /* Flag for parsing sublines */

    /* Parsing out the viewport tag if there is one */
    if(check_viewport(cursor))
    {
        retval->children_count = 2;
        retval->children = skin_alloc_children(2);
        retval->children[0] = skin_alloc_element();
        skin_parse_tag(retval->children[0], &cursor);
        if(*cursor == '\n')
        {
            cursor++;
            skin_line++;
        }
    }
    else
    {
        retval->children_count = 1;
        retval->children = skin_alloc_children(1);
    }


    while(*cursor != '\0' && !(check_viewport(cursor) && cursor != *document))
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
                /* A ';' directly after a '%' doesn't count */
                cursor ++;

                if(*cursor == '\0')
                    break;

                cursor++;
            }
            else if(*cursor == COMMENTSYM)
            {
                skip_comment(&cursor);
            }
            else if(*cursor == ARGLISTOPENSYM)
            {
                skip_arglist(&cursor);
            }
            else if(*cursor == ENUMLISTOPENSYM)
            {
                skip_arglist(&cursor);
            }
            else
            {
                /* Advancing the cursor as normal */
                cursor++;
            }
        }
        cursor = bookmark;

        if(!root)
            to_write = &root;
        else
            to_write = &(last->next);

        if(sublines)
        {
            *to_write = skin_parse_sublines(&cursor);
            last = *to_write;
            if(!last)
                return NULL;
        }
        else
        {

            *to_write = skin_parse_line(&cursor);
            last = *to_write;
            if(!last)
                return NULL;

        }

        /* Making sure last is at the end */
        while(last->next)
            last = last->next;

        if(*cursor == '\n')
        {
            cursor++;
            skin_line++;
        }
    }

    *document = cursor;

    retval->children[retval->children_count - 1] = root;
    return retval;

}

/* Auxiliary Parsing Functions */

struct skin_element* skin_parse_line(char**document)
{

    return skin_parse_line_optional(document, 0);

}


/*
 * If conditional is set to true, then this will break upon encountering
 * SEPERATESYM.  This should only be used when parsing a line inside a
 * conditional, otherwise just use the wrapper function skin_parse_line()
 */
struct skin_element* skin_parse_line_optional(char** document, int conditional)
{
    char* cursor = *document;

    struct skin_element* root = NULL;
    struct skin_element* current = NULL;
    struct skin_element* retval = NULL;

    /* A wrapper for the line */
    retval = skin_alloc_element();
    retval->type = LINE;
    retval->line = skin_line;
    retval->children_count = 1;
    retval->children = skin_alloc_children(1);

    while(*cursor != '\n' && *cursor != '\0' && *cursor != MULTILINESYM
          && !((*cursor == ARGLISTSEPERATESYM
                || *cursor == ARGLISTCLOSESYM
                || *cursor == ENUMLISTSEPERATESYM
                || *cursor == ENUMLISTCLOSESYM)
               && conditional)
          && !(check_viewport(cursor) && cursor != *document))
    {
        /* Allocating memory if necessary */
        if(root)
        {
            current->next = skin_alloc_element();
            current = current->next;
        }
        else
        {
            current = skin_alloc_element();
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

    retval->children[0] = root;
    return retval;
}

struct skin_element* skin_parse_sublines(char** document)
{
    return skin_parse_sublines_optional(document, 0);
}

struct skin_element* skin_parse_sublines_optional(char** document,
                                                  int conditional)
{
    struct skin_element* retval;
    char* cursor = *document;
    int sublines = 1;
    int i;

    retval = skin_alloc_element();
    retval->type = SUBLINES;
    retval->next = NULL;
    retval->line = skin_line;

    /* First we count the sublines */
    while(*cursor != '\0' && *cursor != '\n'
          && !((*cursor == ARGLISTSEPERATESYM
                || *cursor == ARGLISTCLOSESYM
                || *cursor == ENUMLISTSEPERATESYM
                || *cursor == ENUMLISTCLOSESYM)
               && conditional)
          && !(check_viewport(cursor) && cursor != *document))
    {
        if(*cursor == COMMENTSYM)
        {
            skip_comment(&cursor);
        }
        else if(*cursor == ENUMLISTOPENSYM)
        {
            skip_enumlist(&cursor);
        }
        else if(*cursor == ARGLISTOPENSYM)
        {
            skip_arglist(&cursor);
        }
        else if(*cursor == TAGSYM)
        {
            cursor++;
            if(*cursor == '\0' || *cursor == '\n')
                break;
            cursor++;
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
    retval->children = skin_alloc_children(sublines);

    cursor = *document;
    for(i = 0; i < sublines; i++)
    {
        retval->children[i] = skin_parse_line_optional(&cursor, conditional);
        skip_whitespace(&cursor);

        if(*cursor != MULTILINESYM && i != sublines - 1)
        {
            skin_error(MULTILINE_EXPECTED);
            return NULL;
        }
        else if(i != sublines - 1)
        {
            cursor++;
        }
    }

    *document = cursor;

    return retval;
}

int skin_parse_tag(struct skin_element* element, char** document)
{

    char* cursor = *document + 1;
    char* bookmark;

    char tag_name[3];
    char* tag_args;
    struct tag_info *tag;

    int num_args = 1;
    int i;
    int star = 0; /* Flag for the all-or-none option */
    int req_args; /* To mark when we enter optional arguments */

    int optional = 0;

    /* Checking the tag name */
    tag_name[0] = cursor[0];
    tag_name[1] = cursor[1];
    tag_name[2] = '\0';

    /* First we check the two characters after the '%', then a single char */
    tag = find_tag(tag_name);

    if(!tag)
    {
        tag_name[1] = '\0';
        tag = find_tag(tag_name);
        cursor++;
    }
    else
    {
        cursor += 2;
    }

    if(!tag)
    {
        skin_error(ILLEGAL_TAG);
        return 0;
    }

    /* Copying basic tag info */
    element->type = TAG;
    element->tag = tag;
    tag_args = tag->params;
    element->line = skin_line;

    /* Checking for the * flag */
    if(tag_args[0] == '*')
    {
        star = 1;
        tag_args++;
    }

    /* If this tag has no arguments, we can bail out now */
    if(strlen(tag_args) == 0
       || (tag_args[0] == '|' && *cursor != ARGLISTOPENSYM)
       || (star && *cursor != ARGLISTOPENSYM))
    {
        *document = cursor;
        return 1;
    }

    /* Checking the number of arguments and allocating args */
    if(*cursor != ARGLISTOPENSYM && tag_args[0] != '|')
    {
        skin_error(ARGLIST_EXPECTED);
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
            cursor++;
            if(*cursor == '\0')
                break;
            cursor++;
        }
        else if(*cursor == COMMENTSYM)
        {
            skip_comment(&cursor);
        }
        else if(*cursor == ARGLISTOPENSYM)
        {
            skip_arglist(&cursor);
        }
        else if(*cursor == ARGLISTSEPERATESYM)
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
    element->params = skin_alloc_params(num_args);

    /* Now we have to actually parse each argument */
    for(i = 0; i < num_args; i++)
    {
        /* Making sure we haven't run out of arguments */
        if(*tag_args == '\0')
        {
            skin_error(TOO_MANY_ARGS);
            return 0;
        }

        /* Checking for the optional bar */
        if(*tag_args == '|')
        {
            optional = 1;
            req_args = i;
            tag_args++;
        }

        /* Scanning the arguments */
        skip_whitespace(&cursor);


        /* Checking for comments */
        if(*cursor == COMMENTSYM)
            skip_comment(&cursor);

        /* Storing the type code */
        element->params[i].type_code = *tag_args;

        /* Checking a nullable argument for null */
        if(*cursor == DEFAULTSYM && !isdigit(cursor[1]))
        {
            if(islower(*tag_args))
            {
                element->params[i].type = DEFAULT;
                cursor++;
            }
            else
            {
                skin_error(DEFAULT_NOT_ALLOWED);
                return 0;
            }
        }
        else if(tolower(*tag_args) == 'i')
        {
            /* Scanning an int argument */
            if(!isdigit(*cursor) && *cursor != '-')
            {
                skin_error(INT_EXPECTED);
                return 0;
            }

            element->params[i].type = NUMERIC;
            element->params[i].data.numeric = scan_int(&cursor);
        }
        else if(tolower(*tag_args) == 's' || tolower(*tag_args) == 'f')
        {
            /* Scanning a string argument */
            element->params[i].type = STRING;
            element->params[i].data.text = scan_string(&cursor);

        }
        else if(tolower(*tag_args) == 'c')
        {
            /* Recursively parsing a code argument */
            element->params[i].type = CODE;
            element->params[i].data.code = skin_parse_code_as_arg(&cursor);
            if(!element->params[i].data.code)
                return 0;
        }

        skip_whitespace(&cursor);

        if(*cursor != ARGLISTSEPERATESYM && i < num_args - 1)
        {
            skin_error(SEPERATOR_EXPECTED);
            return 0;
        }
        else if(*cursor != ARGLISTCLOSESYM && i == num_args - 1)
        {
            skin_error(CLOSE_EXPECTED);
            return 0;
        }
        else
        {
            cursor++;
        }

        tag_args++;

        /* Checking for the optional bar */
        if(*tag_args == '|')
        {
            optional = 1;
            req_args = i + 1;
            tag_args++;
        }

    }

    /* Checking for a premature end */
    if(*tag_args != '\0' && !optional)
    {
        skin_error(INSUFFICIENT_ARGS);
        return 0;
    }

    *document = cursor;

    return 1;
}

/*
 * If the conditional flag is set true, then parsing text will stop at an
 * ARGLISTSEPERATESYM.  Only set that flag when parsing within a conditional
 */
int skin_parse_text(struct skin_element* element, char** document,
                    int conditional)
{
    char* cursor = *document;

    int length = 0;

    int dest;

    /* First figure out how much text we're copying */
    while(*cursor != '\0' && *cursor != '\n' && *cursor != MULTILINESYM
          && *cursor != COMMENTSYM
          && !((*cursor == ARGLISTSEPERATESYM
                || *cursor == ARGLISTCLOSESYM
                || *cursor == ENUMLISTSEPERATESYM
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
    element->next = NULL;
    element->text = skin_alloc_string(length);
    
    for(dest = 0; dest < length; dest++)
    {
        /* Advancing cursor if we've encountered an escaped character */
        if(*cursor == TAGSYM)
            cursor++;

        element->text[dest] = *cursor;
        cursor++;
    }
    element->text[length] = '\0';

    *document = cursor;

    return 1;
}

int skin_parse_conditional(struct skin_element* element, char** document)
{

    char* cursor = *document + 1; /* Starting past the "%" */
    char* bookmark;
    struct skin_element* tag = skin_alloc_element(); /* The tag to evaluate */
    int children = 1;
    int i;

    element->type = CONDITIONAL;
    element->line = skin_line;

    /* Parsing the tag first */
    if(!skin_parse_tag(tag, &cursor))
        return 0;

    /* Counting the children */
    if(*(cursor++) != ENUMLISTOPENSYM)
    {
        skin_error(ARGLIST_EXPECTED);
        return 0;
    }
    bookmark = cursor;
    while(*cursor != ENUMLISTCLOSESYM && *cursor != '\n' && *cursor != '\0')
    {
        if(*cursor == COMMENTSYM)
        {
            skip_comment(&cursor);
        }
        else if(*cursor == ENUMLISTOPENSYM)
        {
            skip_enumlist(&cursor);
        }
        else if(*cursor == TAGSYM)
        {
            cursor++;
            if(*cursor == '\0' || *cursor == '\n')
                break;
            cursor++;
        }
        else if(*cursor == ENUMLISTSEPERATESYM)
        {
            children++;
            cursor++;
        }
        else
        {
            cursor++;
        }
    }
    cursor = bookmark;

    /* Parsing the children */
    element->children_count = children + 1; /* Make sure to include the tag */
    element->children = skin_alloc_children(children + 1);
    element->children[0] = tag;

    for(i = 1; i < children + 1; i++)
    {
        element->children[i] = skin_parse_code_as_arg(&cursor);
        skip_whitespace(&cursor);

        if(i < children && *cursor != ENUMLISTSEPERATESYM)
        {
            skin_error(SEPERATOR_EXPECTED);
            return 0;
        }
        else if(i == children && *cursor != ENUMLISTCLOSESYM)
        {
            skin_error(CLOSE_EXPECTED);
            return 0;
        }
        else
        {
            cursor++;
        }
    }

    *document = cursor;

    return 1;
}

int skin_parse_comment(struct skin_element* element, char** document)
{
    char* cursor = *document;

    int length;
    /*
     * Finding the index of the ending newline or null-terminator
     * The length of the string of interest doesn't include the leading #, the
     * length we need to reserve is the same as the index of the last character
     */
    for(length = 0; cursor[length] != '\n' && cursor[length] != '\0'; length++);

    element->type = COMMENT;
    element->line = skin_line;
    element->text = skin_alloc_string(length);
    /* We copy from one char past cursor to leave out the # */
    memcpy((void*)(element->text), (void*)(cursor + 1),
           sizeof(char) * (length-1));
    element->text[length - 1] = '\0';

    if(cursor[length] == '\n')
        skin_line++;

    *document += (length); /* Move cursor up past # and all text */
    if(**document == '\n')
        (*document)++;

    return 1;
}

struct skin_element* skin_parse_code_as_arg(char** document)
{

    int sublines = 0;
    char* cursor = *document;

    /* Checking for sublines */
    while(*cursor != '\n' && *cursor != '\0')
    {
        if(*cursor == MULTILINESYM)
        {
            sublines = 1;
            break;
        }
        else if(*cursor == TAGSYM)
        {
            /* A ';' directly after a '%' doesn't count */
            cursor ++;

            if(*cursor == '\0')
                break;

            cursor++;
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

#if 0

    char* retval = &skin_parse_tree[skin_current_block * 4];

    int delta = sizeof(struct skin_element) / (sizeof(char) * 4);

    /* If one block is partially filled, make sure to advance to the 
     * next one for the next allocation
     */
    if(sizeof(struct skin_element) % (sizeof(char) * 4) != 0)
        delta++;

    skin_current_block += delta;

    return (struct skin_element*)retval;

#endif

    struct skin_element* retval =  (struct skin_element*)
                                   malloc(sizeof(struct skin_element));
    retval->next = NULL;
    retval->params_count = 0;
    retval->children_count = 0;

    return retval;

}

struct skin_tag_parameter* skin_alloc_params(int count)
{
#if 0

    char* retval = &skin_parse_tree[skin_current_block * 4];

    int delta = sizeof(struct skin_tag_parameter) / (sizeof(char) * 4);
    delta *= count;

    /* Correcting uneven alignment */
    if(count * sizeof(struct skin_tag_parameter) % (sizeof(char) * 4) != 0)
        delta++;

    skin_current_block += delta;

    return (struct skin_tag_parameter*) retval;

#endif

    return (struct skin_tag_parameter*)malloc(sizeof(struct skin_tag_parameter) 
                                              * count);

}

char* skin_alloc_string(int length)
{

#if 0
    char* retval = &skin_parse_tree[skin_current_block * 4];
    
    /* Checking alignment */
    length++; /* Accounting for the null terminator */
    int delta = length / 4;
    if(length % 4 != 0)
        delta++;

    skin_current_block += delta;

    if(skin_current_block >= SKIN_MAX_MEMORY / 4)
        skin_error(MEMORY_LIMIT_EXCEEDED);

    return retval;

#endif

    return (char*)malloc(sizeof(char) * (length + 1));

}

struct skin_element** skin_alloc_children(int count)
{
    return (struct skin_element**) malloc(sizeof(struct skin_element*) * count);
}

void skin_free_tree(struct skin_element* root)
{
    int i;

    /* First make the recursive call */
    if(!root)
        return;
    skin_free_tree(root->next);

    /* Free any text */
    if(root->type == TEXT)
        free(root->text);

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
}
