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
                    rb->splash(HZ*2,true,"unknown stringid intvalue");
                    return 0;
            }
            break;
        default:
            // report error
             rb->splash(HZ*2,true,"unknown token...");
            return 0;
    }
}

int getvalue(struct token *token) {
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
                default:
                    rb->splash(HZ*2,true,"unknown numid intvalue");
                    // report error.
                    return 0;
            }
        default:
            rb->splash(HZ*2,true,"unknown token...");
            return 0;
    }
}
