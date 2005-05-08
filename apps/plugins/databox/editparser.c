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
#include "editparser.h"

struct token *currenttoken,*lasttoken,*tokenstream;
int currentindex;
int lparencount,acceptedmask;
int tokensleft;
int invalid;
int invalid_mode;

void check_accepted(struct token *tstream, int count) {
    parse_stream(tstream,count+1,INVALID_EXPERT);
}

void parse_stream(struct token *tstream, int count, int inv_mode) {
    invalid_mode=inv_mode;
    acceptedmask=0;
    lparencount=0;
    tokensleft=count;
    currentindex=0;
    invalid=0;
    tokenstream=tstream;
    currenttoken=&tokenstream[currentindex];
    parseMExpr();
}

int check_tokenstream(struct token *tstream, int inv_mode) {
    int inval=0;
    int i;
    parse_stream(tstream,-1,inv_mode);
    inval=invalid;
    while( (inv_mode==INVALID_STRIP||inv_mode==INVALID_MARK) && invalid)
        parse_stream(tstream,-1,inv_mode);
    i=0;
    while(tstream[i].kind!=TOKEN_EOF)
        if(tstream[i++].kind==TOKEN_INVALID) {
          inval=1;
          break;
        }
    return inval==0;
}


void parse_accept_rparen(void) {
    if(!tokensleft) return;
    if(lparencount) {
        acceptedmask|=ACCEPT_RPAREN;
    }
}

void parse_accept(int bitmask) {
    if(!tokensleft) return;
    acceptedmask|=bitmask;
    if(lparencount) {
        acceptedmask&=~ACCEPT_EOF;
    }
}

void parse_checktoken() {
    int ok=0;
    if(!tokensleft) return;
    lasttoken=currenttoken;
    switch(lasttoken->kind) {
         case TOKEN_EOF:
              ok=acceptedmask&ACCEPT_EOF;
              break;
         case TOKEN_NOT:
              ok=acceptedmask&ACCEPT_NOT;
              break;
         case TOKEN_AND:
         case TOKEN_OR:
              ok=acceptedmask&ACCEPT_BOOLOP;
              break;
         case TOKEN_GT:
         case TOKEN_GTE:
         case TOKEN_LT:
         case TOKEN_LTE:
         case TOKEN_NE:
         case TOKEN_EQ:
              ok=acceptedmask&ACCEPT_NUMOP;
              break;
         case TOKEN_EQUALS:
         case TOKEN_CONTAINS:
              ok=acceptedmask&ACCEPT_STROP;
              break;
         case TOKEN_STRING:
         case TOKEN_STRINGIDENTIFIER:
              ok=acceptedmask&ACCEPT_STRARG;
              break;
         case TOKEN_NUM:
         case TOKEN_NUMIDENTIFIER:
              ok=acceptedmask&ACCEPT_NUMARG;
              break;
         case TOKEN_LPAREN:
              ok=acceptedmask&ACCEPT_LPAREN;
              if(ok) lparencount++;
              break;
         case TOKEN_RPAREN:
              ok=acceptedmask&ACCEPT_RPAREN;
              if(ok) lparencount--;
              break;
         case TOKEN_INVALID:
              if(invalid_mode!=INVALID_STRIP)
                  ok=1;
              break;
    }
    tokensleft--;
    if(lasttoken->kind==TOKEN_EOF)
            tokensleft=0;
    if(!ok&&tokensleft) { 
        // delete token
        int i=currentindex;
        //printf("Syntax error. accepted: 0x%x index:%d token: %d %s\n",acceptedmask,currentindex,currenttoken->kind,tokentostring(currenttoken));
        switch (invalid_mode) {
            case INVALID_STRIP:        
                do {
                   rb->memcpy(currenttoken,&tokenstream[++i],sizeof(struct token));;
                   currenttoken=&tokenstream[i];
                } while (currenttoken->kind!=TOKEN_EOF);
                currenttoken=&tokenstream[currentindex];
                break;
            case INVALID_MARK:
                currenttoken->kind=TOKEN_INVALID;
                break;
        }
        tokensleft=0;
        invalid=1;
    }
    if(tokensleft) {
        currenttoken=&tokenstream[++currentindex];
        acceptedmask=0;
    }
}

void parseCompareNum() {
    parse_accept(ACCEPT_NUMOP);
    parse_checktoken();
    parse_accept(ACCEPT_NUMARG);
    parse_checktoken();
}

void parseCompareString() {
    parse_accept(ACCEPT_STROP);
    parse_checktoken();
    parse_accept(ACCEPT_STRARG);
    parse_checktoken();
}

void parseExpr() {
    if(!tokensleft) return;
    parse_accept(ACCEPT_NOT|ACCEPT_LPAREN|ACCEPT_NUMARG|ACCEPT_STRARG);
    parse_checktoken();
    switch(lasttoken->kind) {
        case TOKEN_NOT:
            parseExpr();
            break;
        case TOKEN_LPAREN:
            parseMExpr();
            break;
        case TOKEN_NUM:
        case TOKEN_NUMIDENTIFIER:
            parseCompareNum();
            break;
        case TOKEN_STRING:
        case TOKEN_STRINGIDENTIFIER:
            parseCompareString();
            break;
    }
}

void parseMExpr() {
    parseExpr();
    parse_accept_rparen();
    parse_accept(ACCEPT_BOOLOP|ACCEPT_EOF);
    parse_checktoken();
    while(lasttoken->kind==TOKEN_OR || lasttoken->kind == TOKEN_AND) {
        parseExpr();
        parse_accept_rparen();
        parse_accept(ACCEPT_BOOLOP|ACCEPT_EOF);
        parse_checktoken();
        if(!tokensleft)
                return;
    }
}
