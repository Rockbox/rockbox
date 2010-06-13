/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id: tag_table.c 26346 2010-05-28 02:30:27Z jdgordon $
 *
 * Copyright (C) 2010 Jonathan Gordon
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
#include <stdbool.h>
#include <string.h>
#include <ctype.h>

#include "skin_parser.h"
#include "tag_table.h"
#include "skin_structs.h"

int handle_tree(struct skin *skin, struct skin_element* tree, struct line* line);
void skin_render(struct skin_element* root);

int main(int argc, char* argv[])
{
    char buffer[10*1024], temp[512];
    FILE *in;
    int filearg = 1, i=0;
    if( (argc < 2) ||
        strcmp(argv[1],"-h") == 0 ||
        strcmp(argv[1],"--help") == 0 )
    {
        printf("Usage: %s infile \n", argv[0]);
        return 0;
    }
    
    while ((argc > filearg) && argv[filearg][0] == '-')
    {
        i=1;
        while (argv[filearg][i])
        {
            switch(argv[filearg][i])
            {
            }
            i++;
        }
        filearg++;
    }
    if (argc == filearg)
    {
        printf("Missing input filename\n");
        return 1;
    }
    
    in = fopen(argv[filearg], "r");
    if (!in)
        return 1;
    while (fgets(temp, 512, in))
        strcat(buffer, temp);
    fclose(in);
    filearg++;
    
    struct skin_element* tree = skin_parse(buffer);
    struct skin skin;
    handle_tree(&skin, tree, NULL);
    skin_render(tree);
    
    skin_free_tree(tree);
    return 0;
}
