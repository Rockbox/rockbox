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

#define INTVALUE_YEAR        1
#define INTVALUE_RATING        2
#define INTVALUE_PLAYCOUNT    3
#define INTVALUE_TITLE        4
#define INTVALUE_ARTIST        5
#define INTVALUE_ALBUM        6
#define INTVALUE_GENRE        7
#define INTVALUE_FILENAME    8

/* static char *spelling[] = { "not", "and", "or",">",">=","<", "<=","==","!=",
            "contains","(",")" }; */

struct token {
      unsigned char kind;
      char spelling[256];
      long intvalue;
};

char *getstring(struct token *token);
int getvalue(struct token *token);
