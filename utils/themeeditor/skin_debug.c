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

/* Debugging functions */
void skin_error(enum skin_errorcode error)
{

    fprintf(stderr, "Error on line %d: ", skin_line);

    switch(error)
    {
    case MEMORY_LIMIT_EXCEEDED:
        fprintf(stderr, "Memory limit exceeded\n");
        break;
    case NEWLINE_EXPECTED:
        fprintf(stderr, "Newline expected\n");
        break;
    case ILLEGAL_TAG:
        fprintf(stderr, "Illegal tag\n");
        break;
    case ARGLIST_EXPECTED:
        fprintf(stderr, "Argument list expected\n");
        break;
    case TOO_MANY_ARGS:
        fprintf(stderr, "Too many arguments given\n");
        break;
    case DEFAULT_NOT_ALLOWED:
        fprintf(stderr, "Argument can not be set to default\n");
        break;
    case UNEXPECTED_NEWLINE:
        fprintf(stderr, "Unexpected newline\n");
        break;
    case INSUFFICIENT_ARGS:
        fprintf(stderr, "Not enough arguments\n");
        break;
    case INT_EXPECTED:
        fprintf(stderr, "Expected integer\n");
        break;
    case SEPERATOR_EXPECTED:
        fprintf(stderr, "Expected argument seperator\n");
        break;
    case CLOSE_EXPECTED:
        fprintf(stderr, "Expected list close\n");
        break;
    case MULTILINE_EXPECTED:
        fprintf(stderr, "Expected subline seperator\n");
        break;
    };

}

void skin_debug_tree(struct skin_element* root)
{
    int i;

    struct skin_element* current = root;

    while(current)
    {
        skin_debug_indent();

        switch(current->type)
        {

        case VIEWPORT:
            printf("[ Viewport \n");

            debug_indent_level++;
            skin_debug_tree(current->children[0]);
            debug_indent_level--;

            printf("]");
            break;

        case TEXT:
            printf("[ Plain text on line %d : %s ]\n", current->line,
                   current->text);
            break;

        case COMMENT:
            printf("[ Comment on line %d: ", current->line);
            for(i = 0; i < (int)strlen(current->text); i++)
            {
                if(current->text[i] == '\n')
                    printf("\\n");
                else
                    printf("%c", current->text[i]);
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

        case SUBLINES:
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

        case NUMERIC:
            printf("[%d]", params[i].data.numeric);
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
