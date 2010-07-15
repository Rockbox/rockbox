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

/* Global error variables */
int error_line;
char* error_message;

/* Debugging functions */
void skin_error(enum skin_errorcode error)
{

    error_line = skin_line;

    switch(error)
    {
    case MEMORY_LIMIT_EXCEEDED:
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
    case SEPERATOR_EXPECTED:
        error_message = "Expected argument seperator";
        break;
    case CLOSE_EXPECTED:
        error_message = "Expected list close";
        break;
    case MULTILINE_EXPECTED:
        error_message = "Expected subline seperator";
        break;
    };

}

int skin_error_line()
{
    return error_line;
}

char* skin_error_message()
{
    return error_message;
}

void skin_clear_errors()
{
    error_line = 0;
    error_message = NULL;
}

#ifndef ROCKBOX
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
            printf("[ Unknown element.. error\n]");
            break;

        case VIEWPORT:
            printf("[ Viewport \n");

            debug_indent_level++;
            skin_debug_tree(current->children[0]);
            debug_indent_level--;

            printf("]");
            break;

        case TEXT:
            text = current->data;
            printf("[ Plain text on line %d : %s ]\n", current->line, text);
            break;

        case COMMENT:
            text = current->data;
            printf("[ Comment on line %d: ", current->line);
            for(i = 0; i < (int)strlen(text); i++)
            {
                if(text[i] == '\n')
                    printf("\\n");
                else
                    printf("%c", text[i]);
            }
            printf(" ]\n");
            break;

        case TAG:
            printf("[ %s tag on line %d with %d arguments\n",
                   current->tag->name,
                   current->line, current->params_count);
            debug_indent_level++;
            skin_debug_params(current->params_count, current->params);
            debug_indent_level--;
            skin_debug_indent();
            printf("]\n");

            break;

        case LINE_ALTERNATOR:
            printf("[ Alternator on line %d with %d sublines \n", current->line,
                   current->children_count);
            debug_indent_level++;
            for(i = 0; i < current->children_count; i++)
            {
                skin_debug_tree(current->children[i]);
            }
            debug_indent_level--;

            skin_debug_indent();
            printf("]\n");
            break;

        case CONDITIONAL:
            printf("[ Conditional tag on line %d with %d enumerations \n",
                   current->line, current->children_count - 1);
            debug_indent_level++;

            skin_debug_indent();
            printf("[ Condition tag \n");
            debug_indent_level++;
            skin_debug_tree(current->children[0]);
            debug_indent_level--;
            skin_debug_indent();
            printf("]\n");

            for(i = 1; i < current->children_count; i++)
            {
                skin_debug_indent();
                printf("[ Enumeration %d\n", i - 1);
                debug_indent_level++;
                skin_debug_tree(current->children[i]);
                debug_indent_level--;
                skin_debug_indent();
                printf("]\n");
            }

            debug_indent_level--;
            skin_debug_indent();
            printf("]\n");


            break;

        case LINE:
            printf("[ Logical line on line %d\n", current->line);

            debug_indent_level++;
            skin_debug_tree(current->children[0]);
            debug_indent_level--;

            skin_debug_indent();
            printf("]\n");
            break;
        }

        current = current->next;
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
            printf("[-]");
            break;

        case STRING:
            printf("[%s]", params[i].data.text);
            break;

        case INTEGER:
            printf("[%d]", params[i].data.number);
            break;
            
        case DECIMAL:
            printf("[%d.%d]", params[i].data.number/10,
                              params[i].data.number%10);
            break;

        case CODE:
            printf("[ WPS Code: \n");
            debug_indent_level++;
            skin_debug_tree(params[i].data.code);
            debug_indent_level--;
            skin_debug_indent();
            printf("]");
            break;
        }

        printf("\n");

    }
}

void skin_debug_indent()
{
    int i;
    for(i = 0; i < debug_indent_level; i++)
        printf("    ");
}
#endif
