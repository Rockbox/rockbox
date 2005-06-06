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
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#ifndef EDITTOKEN_H
#define EDITTOKEN_H

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

// pseudotokens..
#define TOKEN_YEAR      118
#define TOKEN_RATING    119
#define TOKEN_PLAYCOUNT 120
#define TOKEN_TITLE     121
#define TOKEN_ARTIST    122
#define TOKEN_ALBUM     123
#define TOKEN_GENRE     124
#define TOKEN_FILENAME  125
#define TOKEN_EDIT      126
#define TOKEN_AUTORATING 127
#define TOKEN_PLAYTIME      128
#define TOKEN_TRACKNUM      129
#define TOKEN_SAMPLERATE    130
#define TOKEN_BITRATE       131

#define ACCEPT_EOF          0x1
#define ACCEPT_BOOLOP       0x2
#define ACCEPT_NUMOP        0x4
#define ACCEPT_STROP        0x8
#define ACCEPT_LPAREN       0x10
#define ACCEPT_RPAREN       0x20
#define ACCEPT_NUMARG       0x40
#define ACCEPT_STRARG       0x80
#define ACCEPT_NOT          0x100
#define ACCEPT_DELETE       0x200

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

struct token {
    char kind;
    char spelling[255];
    long intvalue;
};
char *tokentypetostring(int tokentype);
char *tokentostring(struct token *token);
void buildtoken(int tokentype,struct token *token);
#endif
