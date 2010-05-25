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

#include "tag_table.h"

#include <string.h>

/* The tag definition table */
struct tag_info legal_tags[] = 
{
    { "bl" , "*|fIIII" },
    { "pb" , "*|fIIII" },
    { "pv" , "*|fIIII" },
    { "d"  , "I" },
    { "D"  , "I" },
    { "t"  , "I" },
    { "mv" , "|I"},
    { "pS" , "|I"},
    { "pE" , "|I"},
    { "Tl" , "|I"},
    { "X"  , "F"},
    { "Fl" , "IF"},
    { "Cl" , "II|II"},
    { "V"  , "*IIiii|ii"},
    { "Vl" , "*SIIiii|ii"},
    { "Vi" , "*sIIiii|ii"},
    { "Vd" , "S"},
    { "VI" , "S"},
    { "Vp" , "ICC"},
    { "St" , "S"},
    { "Sx" , "S"},
    { "T"  , "IIiiI"},
    { ""   , ""} /* Keep this here to mark the end of the table */
};

/* A table of legal escapable characters */
char legal_escape_characters[] = "%(,);#<|>";

/*
 * Just does a straight search through the tag table to find one by
 * the given name
 */
char* find_tag(char* name)
{
    
    struct tag_info* current = legal_tags;
    
    /* 
     * Continue searching so long as we have a non-empty name string
     * and the name of the current element doesn't match the name
     * we're searching for
     */
    
    while(strcmp(current->name, name) && current->name[0] != '\0')
        current++;

    if(current->name[0] == '\0')
        return NULL;
    else
        return current->params;

}

/* Searches through the legal escape characters string */
int find_escape_character(char lookup)
{
    char* current = legal_escape_characters;
    while(*current != lookup && *current != '\0')
        current++;

    if(*current == lookup)
        return 1;
    else
        return 0;
}
