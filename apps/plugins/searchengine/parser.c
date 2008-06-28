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
#include "searchengine.h"
#include "token.h"
#include "dbinterface.h"
#include "parser.h"

struct token *currentToken, curtoken;
unsigned char *filter[20],*nofilter=0;
int currentlevel=0;
int syntaxerror;
int parse_fd;
char errormsg[250];

unsigned char *parse(int fd) {
    unsigned char *ret=0;
    int i;
    syntaxerror=0;
    parse_fd=fd;
    currentlevel=0;
    if(nofilter==0) {
        nofilter=my_malloc(sizeof(unsigned char)*rb->tagdbheader->filecount);
        rb->memset(nofilter,1,rb->tagdbheader->filecount);
    }
    for(i=0;i<20;i++)
        filter[i]=nofilter;
    database_init();
    parser_acceptIt();
    currentToken=&curtoken;
    PUTS("parse");
    ret=parseMExpr();
    if(syntaxerror) {
        PUTS("Syntaxerror");
        rb->splash(HZ*3,errormsg);
    }
    parser_accept(TOKEN_EOF);
    return ret;
}

void parser_acceptIt(void) {
    if(syntaxerror) return;
    rb->read(parse_fd,&curtoken,sizeof(struct token));
}

int parser_accept(unsigned char kind) {
    if(currentToken->kind!=kind) {
        syntaxerror=1;
        rb->snprintf(errormsg,250,"'%d' found where '%d' expected\n",currentToken->kind,kind);
        return 0;
    }
    else {
        parser_acceptIt();
        return 1;
    }
}

unsigned char *parseCompareNum() {
    struct token number1,number2;
    unsigned char *ret;
    int i,n1=-1,n2=-1;
    int op;
    if(syntaxerror) return 0;
    PUTS("parseCompareNum");
    if(currentToken->kind==TOKEN_NUM ||
        currentToken->kind==TOKEN_NUMIDENTIFIER) {
        rb->memcpy(&number1,currentToken,sizeof(struct token));
        parser_acceptIt();
    }
    else {
        syntaxerror=1;
        rb->snprintf(errormsg,250,"'%d' found where NUM/NUMID expected\n",currentToken->kind);
        return 0;
    }
    if(currentToken->kind>=TOKEN_GT && currentToken->kind <= TOKEN_NE) {
        op=currentToken->kind;
        parser_acceptIt();
    }
    else {
        syntaxerror=1;
        rb->snprintf(errormsg,250,"'%d' found where NUMOP expected\n",currentToken->kind);
        return 0;
    }
    if(currentToken->kind==TOKEN_NUM ||
         currentToken->kind==TOKEN_NUMIDENTIFIER) {
        rb->memcpy(&number2,currentToken,sizeof(struct token));
        parser_acceptIt();
    }
    else {
        syntaxerror=1;
        rb->snprintf(errormsg,250,"'%d' found where NUM/NUMID expected\n",currentToken->kind);
        return 0;
    }
    ret=my_malloc(sizeof(unsigned char)*rb->tagdbheader->filecount);
    if(number1.kind==TOKEN_NUM)
        n1=getvalue(&number1);
    if(number2.kind==TOKEN_NUM)
        n2=getvalue(&number2);
    for(i=0;i<rb->tagdbheader->filecount;i++) 
        if(filter[currentlevel][i]) {
            loadentry(i);
            if(number1.kind==TOKEN_NUMIDENTIFIER)
                n1=getvalue(&number1);
            if(number2.kind==TOKEN_NUMIDENTIFIER)
                n2=getvalue(&number2);
            switch(op) {
                case TOKEN_GT:
                    ret[i]=n1 > n2;
                    break;
                case TOKEN_GTE:
                    ret[i]=n1 >= n2;
                    break;
                case TOKEN_LT:
                    ret[i]=n1 < n2;
                    break;
                case TOKEN_LTE:
                    ret[i]=n1 <= n2;
                    break;
                case TOKEN_EQ:
                    ret[i]=n1 == n2;
                    break;
                case TOKEN_NE:
                    ret[i]=n1 != n2;
                    break;
            }
        }
    return ret;
}

unsigned char *parseCompareString() {
    struct token string1,string2;
    unsigned char *ret;
    char *s1=NULL,*s2=NULL;
    int i,i2;
    int op;
    if(syntaxerror) return 0;
    PUTS("parseCompareString");
    if(currentToken->kind==TOKEN_STRING ||
          currentToken->kind==TOKEN_STRINGIDENTIFIER) {
        rb->memcpy(&string1,currentToken,sizeof(struct token));
        parser_acceptIt();
    }
    else {
        syntaxerror=1;
        rb->snprintf(errormsg,250,"'%d' found where STRING/STRINGID expected\n",currentToken->kind);
        return 0;
    }
    op=currentToken->kind;
    if(op>=TOKEN_CONTAINS&&op<=TOKEN_ENDSWITH) {
        parser_acceptIt();
    } else {
        syntaxerror=1;
        rb->snprintf(errormsg,250,"'%d' found where STROP expected\n",op);
        return 0;
    }
    if(currentToken->kind==TOKEN_STRING ||
          currentToken->kind==TOKEN_STRINGIDENTIFIER) {
        rb->memcpy(&string2,currentToken,sizeof(struct token));
        parser_acceptIt();
    }
    else {
        syntaxerror=1;
        rb->snprintf(errormsg,250,"'%d' found where STRING/STRINGID expected\n",currentToken->kind);
        return 0;
    }
    ret=my_malloc(sizeof(unsigned char)*rb->tagdbheader->filecount);
    if(string1.kind==TOKEN_STRING)
        s1=getstring(&string1);
    if(string2.kind==TOKEN_STRING)
        s2=getstring(&string2);
    for(i=0;i<rb->tagdbheader->filecount;i++) 
        if(filter[currentlevel][i]) {
            loadentry(i);
            if(string1.kind==TOKEN_STRINGIDENTIFIER)
                s1=getstring(&string1);
            if(string2.kind==TOKEN_STRINGIDENTIFIER)
                s2=getstring(&string2);
            switch(op) {
                case TOKEN_CONTAINS:
                    ret[i]=rb->strcasestr(s1,s2)!=0;
                    break;
                case TOKEN_EQUALS:
                    ret[i]=rb->strcasecmp(s1,s2)==0;
                    break;
                case TOKEN_STARTSWITH:
                    ret[i]=rb->strncasecmp(s1,s2,rb->strlen(s2))==0;
                    break;
                case TOKEN_ENDSWITH:
                    i2=rb->strlen(s2);
                    ret[i]=rb->strncasecmp(s1+rb->strlen(s1)-i2,s2,i2)==0;
                    break;
            }
        }
    return ret;
}

unsigned char *parseExpr() {
    unsigned char *ret;
    int i;
    if(syntaxerror) return 0;
    PUTS("parseExpr");       
    switch(currentToken->kind) {
        case TOKEN_NOT:
            parser_accept(TOKEN_NOT);
            PUTS("parseNot");
            ret = parseExpr();
            if(ret==NULL) return 0;
            for(i=0;i<rb->tagdbheader->filecount;i++)
                if(filter[currentlevel][i])
                    ret[i]=!ret[i];
            break;
        case TOKEN_LPAREN:
            parser_accept(TOKEN_LPAREN);
            currentlevel++;
            ret = parseMExpr();
            currentlevel--;
            if(ret==NULL) return 0;
            parser_accept(TOKEN_RPAREN);
            break;
        case TOKEN_NUM:
        case TOKEN_NUMIDENTIFIER:
            ret = parseCompareNum();
            if(ret==NULL) return 0;
            break;
        case TOKEN_STRING:
        case TOKEN_STRINGIDENTIFIER:
            ret = parseCompareString();
            if(ret==NULL) return 0;
            break;
        default:
            // error, unexpected symbol
            syntaxerror=1;
            rb->snprintf(errormsg,250,"unexpected '%d' found at parseExpr\n",currentToken->kind);
            ret=0;
            break;
    }
    return ret;
}

unsigned char *parseMExpr() {
    unsigned char *ret,*ret2;
    int i;
    if(syntaxerror) return 0;
    PUTS("parseMExpr");       
    ret=parseLExpr();
    while(currentToken->kind==TOKEN_OR) {
        parser_accept(TOKEN_OR);
        PUTS("parseOr");
        ret2 = parseLExpr();
        if(ret2==NULL) return 0;
        for(i=0;i<rb->tagdbheader->filecount;i++)
            if(filter[currentlevel][i]) // this should always be true
                ret[i]=ret[i] || ret2[i];
            else
                rb->splash(HZ*2,"An or is having a filter, bad.");
    }
    return ret;
}

unsigned char *parseLExpr() {
    unsigned char *ret,*ret2;
    int i;
    if(syntaxerror) return 0;
    PUTS("parseLExpr");
    filter[currentlevel]=nofilter;
    ret=parseExpr();
    filter[currentlevel]=ret;
    while(currentToken->kind==TOKEN_AND) {
        parser_accept(TOKEN_AND);
        PUTS("parseAnd");
        ret2 = parseExpr();
        if(ret2==NULL) return 0;
        for(i=0;i<rb->tagdbheader->filecount;i++)
            ret[i]=ret[i] && ret2[i];
    }
    filter[currentlevel]=nofilter;
    return ret;
}
