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
#include "searchengine.h"
#include "token.h"
#include "dbinterface.h"

char *getstring(struct token *token) {
    char buf[200];
    switch(token->kind) {
        case TOKEN_STRING:
            return token->spelling;
        case TOKEN_STRINGIDENTIFIER:
            switch(token->intvalue) {
                case INTVALUE_TITLE:
                    loadsongdata();
                    return currententry->title;
                case INTVALUE_ARTIST:
                    loadartistname();
                    return currententry->artistname;
                case INTVALUE_ALBUM:
                    loadalbumname();
                    return currententry->albumname;
                case INTVALUE_GENRE:
                    loadsongdata();
                    return currententry->genre;
                case INTVALUE_FILENAME:
                    return currententry->filename;
                default:
                    rb->snprintf(buf,199,"unknown stringid intvalue %d",token->intvalue);
                    rb->splash(HZ*2,true,buf);
                    return "";
            }
            break;
        default:
            // report error
            rb->snprintf(buf,199,"unknown token %d in getstring..",token->kind); 
            rb->splash(HZ*2,true,buf);
            return "";
    }
}

int getvalue(struct token *token) {
    char buf[200];
    int index,i;
    switch(token->kind) {
        case TOKEN_NUM:
            return token->intvalue;
        case TOKEN_NUMIDENTIFIER:
            switch(token->intvalue) {
                case INTVALUE_YEAR:
                    loadsongdata();
                    return currententry->year;
                case INTVALUE_RATING:
                    loadrundbdata();
                    return currententry->rating;
                case INTVALUE_PLAYCOUNT:
                    loadrundbdata();
                    return currententry->playcount;
                case INTVALUE_AUTORATING:
                    if(!dbglobal.gotplaycountlimits) {
                        index=dbglobal.currententryindex;
                        dbglobal.playcountmax=0;
                        dbglobal.playcountmin=0xFFFFFFFF;
                        for(i=0;i<rb->tagdbheader->filecount;i++) {
                            loadentry(i);
                            loadrundbdata();
                            if(currententry->playcount>dbglobal.playcountmax)
                                dbglobal.playcountmax=currententry->playcount;
                            if(currententry->playcount<dbglobal.playcountmin)
                                dbglobal.playcountmin=currententry->playcount;
                        }
                        dbglobal.gotplaycountlimits=1;
                        loadentry(index);
                    }
                    loadrundbdata();
                    return (currententry->playcount-dbglobal.playcountmin)*10/(dbglobal.playcountmax-dbglobal.playcountmin);
                default:
                    rb->snprintf(buf,199,"unknown numid intvalue %d",token->intvalue);
                    rb->splash(HZ*2,true,buf);
                    // report error.
                    return 0;
            }
        default:
            rb->snprintf(buf,199,"unknown token %d in getvalue..",token->kind);
            rb->splash(HZ*2,true,buf);
            return 0;
    }
}
