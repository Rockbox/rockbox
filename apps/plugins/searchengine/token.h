/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2005 by Michiel van der Kolk 
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
#define TOKEN_INVALID         -1
#define TOKEN_EOF              0 // EOF
#define TOKEN_NOT              1 // "not"
#define TOKEN_AND              2 // "and"
#define TOKEN_OR               3 // "or"
#define TOKEN_GT               4 // '>'
#define TOKEN_GTE              5 // '>='
#define TOKEN_LT               6 // '<'
#define TOKEN_LTE              7 // '<='
#define TOKEN_EQ               8 // '=='
#define TOKEN_NE               9 // '!='
#define TOKEN_CONTAINS         10 // "contains"
#define TOKEN_EQUALS           11 // "equals"
#define TOKEN_STARTSWITH       12
#define TOKEN_ENDSWITH         13
#define TOKEN_LPAREN           14 // '('
#define TOKEN_RPAREN           15 // ')'
#define TOKEN_NUM              16 // (0..9)+
#define TOKEN_NUMIDENTIFIER    17 // year, trackid, bpm, etc.
#define TOKEN_STRING           18 // (?)+
#define TOKEN_STRINGIDENTIFIER 19 // album, artist, title, genre ...
#define TOKEN_SHUFFLE          20
#define TOKEN_PLAYTIMELIMIT    21

#define INTVALUE_YEAR          1
#define INTVALUE_RATING        2
#define INTVALUE_PLAYCOUNT     3
#define INTVALUE_AUTORATING    4
#define INTVALUE_PLAYTIME      5
#define INTVALUE_TRACKNUM      6
#define INTVALUE_SAMPLERATE    7
#define INTVALUE_BITRATE       8
#define INTVALUE_TITLE        14
#define INTVALUE_ARTIST       15
#define INTVALUE_ALBUM        16
#define INTVALUE_GENRE        17
#define INTVALUE_FILENAME     18

#define SPELLING_LENGTH 100

struct token {
    char kind;
    char spelling[SPELLING_LENGTH + 3];
    long intvalue;
};

char *getstring(struct token *token);
int getvalue(struct token *token);
