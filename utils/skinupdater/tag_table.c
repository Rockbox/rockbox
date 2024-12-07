/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id: tag_table.c 26297 2010-05-26 03:53:06Z jdgordon $
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
    { "ac", "" },
    { "al", "" },
    { "aL", "" },
    { "ar", "" },
    { "aR", "" },
    { "ax", "" },
    
    { "bl" , "*fIIII" },
    { "bv", "" },
    { "bt", "" },
    { "bs", "" },
    { "bc", "" },
    { "bp", "" },
    { "bu", "" },
    
    
    { "cc", "" },
    { "cd", "" },
    { "ce", "" },
    { "cf", "" },
    { "cH", "" },
    { "ck", "" },
    { "cI", "" },
    { "cl", "" },
    { "cm", "" },
    { "cM", "" },
    { "cS", "" },
    { "cy", "" },
    { "cY", "" },
    { "cP", "" },
    { "cp", "" },
    { "ca", "" },
    { "cb", "" },
    { "cu", "" },
    { "cw", "" },
        
    { "fb", "" },
    { "fc", "" },
    { "ff", "" },
    { "fk", "" },
    { "fm", "" },
    { "fn", "" },
    { "fp", "" },
    { "fs", "" },
    { "fv", "" },
    { "d"  , "I" },
    
    { "Fb", "" },
    { "Fc", "" },
    { "Ff", "" },
    { "Fk", "" },
    { "Fm", "" },
    { "Fn", "" },
    { "Fp", "" },
    { "Fs", "" },
    { "Fv", "" },
    { "D"  , "I" },
    
    
    { "ia", "" },
    { "ic", "" },
    { "id", "" },
    { "iA", "" },
    { "iG", "" },
    { "ig", "" },
    { "ik", "" },
    { "in", "" },
    { "it", "" },
    { "iv", "" },
    { "iy", "" },
    { "iC", "" },
    
    { "Ia", "" },
    { "Ic", "" },
    { "Id", "" },
    { "IA", "" },
    { "IG", "" },
    { "Ig", "" },
    { "Ik", "" },
    { "In", "" },
    { "It", "" },
    { "Iv", "" },
    { "Iy", "" },
    { "IC", "" },
    
    { "Sp", "" },
    { "Ss", "" },
    
    { "lh", "" },
    
    { "mh", "" },
    { "mr", "" },
    { "mm", "" },
    { "mp", "" },
    { "mv", "|I" },
    
    { "pm", "" },
    { "pf", "" },
    { "pb" , "*fIIII" },
    { "pv" , "*fIIII" },
    
    { "px", "" },
    { "pc", "" },
    { "pr", "" },
    { "pt", "" },
    { "pS" , "|I"},
    { "pE" , "|I"},
    { "pp", "" },
    { "pe", "" },
    { "pn", "" },
    { "ps", "" },
    
    { "rp", "" },
    { "rr", "" },
    { "ra", "" },
    
    { "rg", "" },
    { "xf", "" },
    
    { "tp", "" },
    { "tt", "" },
    { "tm", "" },
    { "ts", "" },
    { "ta", "" },
    { "tb", "" },
    { "tf", "" },
    { "Ti", "" },
    { "Tn", "" },
    { "Tf", "" },
    { "Tc", "" },
    { "tx", "" },
    { "ty", "" },
    { "tz", "" },
    
    { "s", "" },
    { "t"  , "I" },
    
    { "we", "" },
    { "wd", "" },
    { "wi", "" },
    
    { "xl", "SFIIi" },
    { "xd", "S" },
    { "x", "SFII" },
    
    { "Fl" , "IF"},
    { "Cl" , "IISS"},
    { "C" , "important"},
    
    { "Vd" , "S"},
    { "VI" , "S"},
    
    { "Vp" , "ICC"},
    { "Lt" , ""},
    { "Li" , ""},
    
    { "Vl" , "SIIiii|ii"},
    { "Vi" , "sIIiii|ii"},
    { "V"  , "IIiii|ii"},
    
    { "X"  , "f"},
    
    { "St" , "S"},
    { "Sx" , "S"},
    { "Sr" , ""},
    
    { "Tl" , "|I"},
    { "cs", "" },
    { "T"  , "IIiiI"},
    
    { "Rp"   , ""},
    { "Rr"   , ""},
    { "Rf"   , ""},
    { "Re"   , ""},
    { "Rb"   , ""},
    { "Rm"   , ""},
    { "Rs"   , ""},
    { "Rn"   , ""},
    { "Rh"   , ""},
    { "s",""},
    
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
