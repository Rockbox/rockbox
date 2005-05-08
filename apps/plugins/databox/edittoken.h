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

#define TOKEN_INVALID    -1
#define TOKEN_EOF    0 // EOF
#define TOKEN_NOT       1 // "not"
#define TOKEN_AND       2 // "and"
#define TOKEN_OR        3 // "or"
#define TOKEN_GT        4 // '>'
#define TOKEN_GTE       5 // '>='
#define TOKEN_LT        6 // '<'
#define TOKEN_LTE       7 // '<='
#define TOKEN_EQ        8 // '=='
#define TOKEN_NE        9 // '!='
#define TOKEN_CONTAINS  10 // "contains"
#define TOKEN_EQUALS    11 // "equals"
#define TOKEN_LPAREN    12 // '('
#define TOKEN_RPAREN    13 // ')'
#define TOKEN_NUM       14 // (0..9)+
#define TOKEN_NUMIDENTIFIER 15 // year, trackid, bpm, etc.
#define TOKEN_STRING    16 // (?)+
#define TOKEN_STRINGIDENTIFIER 17 // album, artist, title, genre ...
#define TOKEN_YEAR  18
#define TOKEN_RATING    19
#define TOKEN_PLAYCOUNT 20
#define TOKEN_TITLE     21
#define TOKEN_ARTIST    22
#define TOKEN_ALBUM     23
#define TOKEN_GENRE     24
#define TOKEN_FILENAME  25
#define TOKEN_EDIT      26

#define ACCEPT_EOF    0x1
#define ACCEPT_BOOLOP    0x2
#define ACCEPT_NUMOP    0x4
#define ACCEPT_STROP    0x8
#define ACCEPT_LPAREN    0x10
#define ACCEPT_RPAREN    0x20
#define ACCEPT_NUMARG    0x40
#define ACCEPT_STRARG    0x80
#define ACCEPT_NOT    0x100
#define ACCEPT_DELETE    0x200

#define INTVALUE_YEAR        1
#define INTVALUE_RATING        2
#define INTVALUE_PLAYCOUNT    3
#define INTVALUE_TITLE        4
#define INTVALUE_ARTIST        5
#define INTVALUE_ALBUM        6
#define INTVALUE_GENRE        7
#define INTVALUE_FILENAME    8

struct token {
    char kind;
    char spelling[255];
    long intvalue;
};
char *tokentypetostring(int tokentype);
char *tokentostring(struct token *token);
void buildtoken(int tokentype,struct token *token);
#endif
