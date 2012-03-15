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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "skin_parser.h"
#include "skin_debug.h"
#include "tag_table.h"

/* Global variables for debug output */
int debug_indent_level = 0;
extern int skin_line;
extern char* skin_start;
extern char* skin_buffer;

/* Global error variables */
int error_line;
int error_col;
const char *error_line_start;
char* error_message;


static inline struct skin_element*
get_child(OFFSETTYPE(struct skin_element**) children, int child)
{
    struct skin_element **kids = SKINOFFSETTOPTR(skin_buffer, children);
    return kids[child];
}

/* Debugging functions */
void skin_error(enum skin_errorcode error, const char* cursor)
{

    error_col = 0;

    while(cursor > skin_start && *cursor != '\n')
    {
        cursor--;
        error_col++;
    }
    error_line_start = cursor+1;

    error_line = skin_line;

    switch(error)
    {
    case MEMORY_LIMIT_EXCEEDED:
        error_line_start = NULL;
        printf("Error: Memory limit exceeded at Line %d\n", skin_line);
        error_message = "Memory limit exceeded";
        break;
    case NEWLINE_EXPECTED:
        error_message = "Newline expected";
        break;
    case ILLEGAL_TAG:
        error_message = "Illegal tag";
        break;
    case ARGLIST_EXPECTED:
        error_message = "Argument list expected";
        break;
    case TOO_MANY_ARGS:
        error_message =  "Too many arguments given";
        break;
    case DEFAULT_NOT_ALLOWED:
        error_message = "Argument can not be set to default";
        break;
    case UNEXPECTED_NEWLINE:
        error_message  = "Unexpected newline";
        break;
    case INSUFFICIENT_ARGS:
        error_message = "Not enough arguments";
        break;
    case INT_EXPECTED:
        error_message =  "Expected integer";
        break;
    case DECIMAL_EXPECTED:
        error_message =  "Expected decimal";
        break;
    case SEPARATOR_EXPECTED:
        error_message = "Expected argument separator";
        break;
    case CLOSE_EXPECTED:
        error_message = "Expected list close";
        break;
    case MULTILINE_EXPECTED:
        error_message = "Expected subline separator";
        break;
    };

}

int skin_error_line()
{
    return error_line;
}

int skin_error_col()
{
    return error_col;
}

char* skin_error_message()
{
    return error_message;
}

void skin_clear_errors()
{
    error_line = 0;
    error_col = 0;
    error_message = NULL;
}

#if !defined(ROCKBOX) || defined(__PCTOOL__)
void skin_debug_tree(struct skin_element* root)
{
    int i;
    char *text;

    struct skin_element* current = root;

    while(current)
    {
        skin_debug_indent();

        switch(current->type)
        {
        case UNKNOWN:
            printf("* Unknown element.. error *\n");
            break;

        case VIEWPORT:
            printf("{ Viewport \n");

            debug_indent_level++;
            skin_debug_tree(get_child(current->children, 0));
            debug_indent_level--;

            printf("}");
            break;

        case TEXT:
            text = SKINOFFSETTOPTR(skin_buffer, current->data);
            printf("* Plain text on line %d: \"%s\"\n", current->line, text);
            break;

        case COMMENT:
            printf("# Comment on line %d\n ", current->line);
            break;

        case TAG:
            if (current->params_count)
            {
                printf("( %%%s tag on line %d with %d arguments\n",
                       current->tag->name,
                       current->line, current->params_count);
                debug_indent_level++;
                skin_debug_params(current->params_count, SKINOFFSETTOPTR(skin_buffer, current->params));
                debug_indent_level--;
                skin_debug_indent();
                printf(")\n");
            }
            else
            {
                printf("[ %%%s tag on line %d ]\n",
                       current->tag->name, current->line);
            }

            break;

        case LINE_ALTERNATOR:
            printf("[ Alternator on line %d with %d sublines \n", current->line,
                   current->children_count);
            debug_indent_level++;
            for(i = 0; i < current->children_count; i++)
            {
                skin_debug_tree(get_child(current->children, i));
            }
            debug_indent_level--;

            skin_debug_indent();
            printf("]\n");
            break;

        case CONDITIONAL:
            printf("< Conditional tag %%?%s on line %d with %d enumerations \n",
                   current->tag->name, current->line, current->children_count);
            debug_indent_level++;

            for(i = 0; i < current->children_count; i++)
            {
                skin_debug_indent();
                printf("[ Enumeration %d\n", i);
                debug_indent_level++;
                skin_debug_tree(get_child(current->children, i));
                debug_indent_level--;
                skin_debug_indent();
                printf("]\n");
            }

            debug_indent_level--;
            skin_debug_indent();
            printf(">\n");


            break;

        case LINE:
            printf("[ Logical line on line %d\n", current->line);

            debug_indent_level++;
            if (current->children)
                skin_debug_tree(get_child(current->children, 0));
            debug_indent_level--;

            skin_debug_indent();
            printf("]\n");
            break;
        }

        current = SKINOFFSETTOPTR(skin_buffer, current->next);
    }

}

void skin_debug_params(int count, struct skin_tag_parameter params[])
{
    int i;
    for(i = 0; i < count; i++)
    {

        skin_debug_indent();
        switch(params[i].type)
        {
        case DEFAULT:
            printf("-");
            break;

        case STRING:
            printf("string: \"%s\"", SKINOFFSETTOPTR(skin_buffer, params[i].data.text));
            break;

        case INTEGER:
            printf("integer: %d", params[i].data.number);
            break;
            
        case DECIMAL:
            printf("decimal: %d.%d", params[i].data.number/10,
                              params[i].data.number%10);
            break;

        case CODE:
            printf("Skin Code: \n");
            debug_indent_level++;
            skin_debug_tree(SKINOFFSETTOPTR(skin_buffer, params[i].data.code));
            debug_indent_level--;
            skin_debug_indent();
            break;
        }

        printf("\n");

    }
}

void skin_debug_indent(void)
{
    int i;
    for(i = 0; i < debug_indent_level; i++)
        printf("    ");
}

#endif

#define MIN(a,b) ((a<b)?(a):(b))
void skin_error_format_message(void)
{
    int i;
    char text[128];
    if (!error_line_start)
        return;
    char* line_end = strchr(error_line_start, '\n');
    int len = MIN(line_end - error_line_start, 80);
    if (!line_end)
        len = strlen(error_line_start);
    printf("Error on line %d.\n", error_line);
    error_col--;
    if (error_col <= 10)
    {
        strncpy(text, error_line_start, len);
        text[len] = '\0';
    }
    else
    {
        int j;
        /* make it fit nicely.. "<start few chars>...<10 chars><error>" */
        strncpy(text, error_line_start, 6);
        i = 5;
        text[i++] = '.';
        text[i++] = '.';
        text[i++] = '.';
        for (j=error_col-10; error_line_start[j] && error_line_start[j] != '\n'; j++)
            text[i++] = error_line_start[j];
        text[i] = '\0';
        error_col = 18;
    }
    printf("%s\n", text);
    for (i=0; i<error_col; i++)
        text[i] = ' ';
    snprintf(&text[i],64, "^ \'%s\' Here", error_message);
    printf("%s\n", text);
}
