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
#include "token.h"
#include "dbinterface.h"

#define REQUIRESONGDATA() if(!currententry->loadedsongdata) loadsongdata();
#define REQUIRERUNDBDATA() if(!currententry->loadedrundbdata) loadrundbdata();
#define REQUIREALBUMNAME() if(!currententry->loadedalbumname) { REQUIRESONGDATA(); loadalbumname(); }
#define REQUIREARTISTNAME() if(!currententry->loadedartistname) { REQUIRESONGDATA(); loadartistname(); }

char *getstring(struct token *token) {
	switch(token->kind) {
		case TOKEN_STRING:
			return token->spelling;
		case TOKEN_STRINGIDENTIFIER:
			switch(token->intvalue) {
				case INTVALUE_TITLE:
					REQUIRESONGDATA();
					return currententry->title;
				case INTVALUE_ARTIST:
					REQUIREARTISTNAME();
					return currententry->artistname;
				case INTVALUE_ALBUM:
					REQUIREALBUMNAME();
					return currententry->albumname;
				case INTVALUE_GENRE:
					REQUIRESONGDATA();
					return currententry->genre;
				case INTVALUE_FILENAME:
					return currententry->filename;
				default:
					return 0;
			}
			break;
		default:
			// report error
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
					REQUIRESONGDATA();
					return currententry->year;
				case INTVALUE_RATING:
					REQUIRERUNDBDATA();
					return currententry->rating;
				case INTVALUE_PLAYCOUNT:
					REQUIRERUNDBDATA();
					return currententry->playcount;
				default:
					// report error.
					return 0;
			}
		default:
			return 0;
	}
}
