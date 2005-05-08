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
#include "databox.h"
#include "edittoken.h"

char *tokentypetostring(int tokentype) {
    switch(tokentype) {
        case TOKEN_EOF: return "<END>";
        case TOKEN_NOT: return "not";
        case TOKEN_AND: return "and";
        case TOKEN_OR: return "or";
        case TOKEN_GT: return ">";
        case TOKEN_GTE: return ">=";
        case TOKEN_LT: return "<";
        case TOKEN_LTE: return "<=";
        case TOKEN_EQ: return "==";
        case TOKEN_NE: return "!=";
        case TOKEN_LPAREN: return "(";
        case TOKEN_RPAREN: return ")";
        case TOKEN_CONTAINS: return "contains";
        case TOKEN_EQUALS: return "equals";
        case TOKEN_NUM: return "<number>";
        case TOKEN_NUMIDENTIFIER: return "<numberproperty>";
        case TOKEN_STRING: return "<string>";
        case TOKEN_STRINGIDENTIFIER: return "<stringproperty>";
        case TOKEN_INVALID: return "<INVALID>";
        case TOKEN_EDIT: return tokentostring(&editing.old_token);
        case TOKEN_YEAR: return "year";
        case TOKEN_RATING: return "rating";
        case TOKEN_PLAYCOUNT: return "playcount";
        case TOKEN_TITLE: return "title";
        case TOKEN_ARTIST: return "artist";
        case TOKEN_ALBUM: return "album";
        case TOKEN_GENRE: return "genre";
        case TOKEN_FILENAME: return "filename";
    }
    return "tokentypeerror";
}

char *numidtostring(int numid) {
    switch(numid) {
        case INTVALUE_YEAR: return "<year>";
        case INTVALUE_RATING: return "<rating>";
        case INTVALUE_PLAYCOUNT: return "<playcount>";
    }
    return "numiderror";
}

char *stridtostring(int strid) {
    switch(strid) {
        case INTVALUE_TITLE: return "<title>";
        case INTVALUE_ARTIST: return "<artist>";
        case INTVALUE_ALBUM: return "<album>";
        case INTVALUE_GENRE: return "<genre>";
        case INTVALUE_FILENAME: return "<filename>";
    }
    return "striderror";
}

char bufbla[40];

void buildtoken(int tokentype,struct token *token) {
    // TODO
    char buf[200];
    rb->memset(token,0,sizeof(struct token));
    rb->memset(buf,0,200);
    token->kind=tokentype;
    token->intvalue=0;
    switch(token->kind) {
        case TOKEN_STRING:
            do {
                rb->splash(HZ*2,true,"Enter String.");
            } while(rb->kbd_input(token->spelling, 254));
            break;
        case TOKEN_YEAR:
            token->kind=TOKEN_NUMIDENTIFIER;
            token->intvalue=INTVALUE_YEAR;
            break;
        case TOKEN_RATING:
            token->kind=TOKEN_NUMIDENTIFIER;
            token->intvalue=INTVALUE_RATING;
            break;
        case TOKEN_PLAYCOUNT:
            token->kind=TOKEN_NUMIDENTIFIER;
            token->intvalue=INTVALUE_PLAYCOUNT;
            break;
        case TOKEN_TITLE:
            token->kind=TOKEN_STRINGIDENTIFIER;
            token->intvalue=INTVALUE_TITLE;
            break;
        case TOKEN_ARTIST:
            token->kind=TOKEN_STRINGIDENTIFIER;
            token->intvalue=INTVALUE_ARTIST;
            break;
        case TOKEN_ALBUM:
            token->kind=TOKEN_STRINGIDENTIFIER;
            token->intvalue=INTVALUE_ALBUM;
            break;
        case TOKEN_GENRE:
            token->kind=TOKEN_STRINGIDENTIFIER;
            token->intvalue=INTVALUE_GENRE;
            break;
        case TOKEN_FILENAME:
            token->kind=TOKEN_STRINGIDENTIFIER;
            token->intvalue=INTVALUE_TITLE;
            break;
        case TOKEN_NUM:
            do {
                rb->splash(HZ*2,true,"Enter Number.");
            } while(rb->kbd_input(buf, 199));
            token->intvalue=rb->atoi(buf);
            break;
        case TOKEN_EDIT:
            rb->memcpy(token,&editing.old_token,sizeof(struct token));
            break;
    }
}

void removetoken(struct token *token,int index) {
     struct token *currenttoken;
     currenttoken=&token[index];
     do {
         rb->memcpy(currenttoken,&token[++index],sizeof(struct token));
         currenttoken=&token[index];
     } while (currenttoken->kind!=TOKEN_EOF);
}  

void inserttoken(struct token *token,int index,int tokentype) {
     struct token *currenttoken;
     int max,i;
     currenttoken=&token[0];
     for(i=1;currenttoken->kind!=TOKEN_EOF;i++)
             currenttoken=&token[i];
     max=i;
     for(i=max;i>=index;i--) {
         rb->memcpy(&token[i+1],&token[i],sizeof(struct token));
     }
     buildtoken(tokentype,&token[index]);
}


char *tokentostring(struct token *token) {
    switch(token->kind) {
        case TOKEN_INVALID:
        case TOKEN_EOF:
        case TOKEN_NOT:
        case TOKEN_AND:
        case TOKEN_OR:
        case TOKEN_GT:
        case TOKEN_GTE:
        case TOKEN_LT:
        case TOKEN_LTE:
        case TOKEN_EQ:
        case TOKEN_NE:
        case TOKEN_LPAREN:
        case TOKEN_RPAREN: 
        case TOKEN_CONTAINS:
        case TOKEN_EQUALS:
            return tokentypetostring(token->kind);
        case TOKEN_NUM: rb->snprintf(bufbla,40,"%d",token->intvalue);
            return bufbla;
        case TOKEN_NUMIDENTIFIER: 
            return numidtostring(token->intvalue);
        case TOKEN_STRING: return token->spelling;
        case TOKEN_STRINGIDENTIFIER:
            return stridtostring(token->intvalue);
        case TOKEN_EDIT: return tokentostring(&editing.old_token);
        default: return "unknown token";
    }
}
