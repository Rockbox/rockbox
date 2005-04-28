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
#include <stdlib.h>
#include <stdio.h>
#include "token.h"

struct token token;
char buf[500];
long num;
FILE *fp;
main() {
	int done=0;
	printf("Output filename? ");
	fflush(stdout);
	fgets(buf,254,stdin);
	buf[strlen(buf)-1]=0;
	fp=fopen(buf,"w");
	if(fp<0) {
		printf("Error opening outputfile.\n");
		return -1;
	}
	do {
		printf("EOF=0 NOT=1 AND=2 OR=3 GT=4 GTE=5 LT=6 LTE=7 EQ=8 NE=9\n");
		printf("(strings:) CONTAINS=10 EQUALS=11\n");
		printf("'('=12 ')'=13\n");
		printf("(arguments:) NUMBER=14 NUMBERFIELD=15 STRING=16 STRINGFIELD=17\n");
		printf("Token kind? ");
		fflush(stdout);
		fgets(buf,254,stdin);
		token.kind=strtol(buf,0,10);
		memset(&token.spelling,0,256);
		if(token.kind==TOKEN_STRING) {
	  	  printf("Token spelling? ");
		  fflush(stdout);
		  fgets(token.spelling,254,stdin);
		}
                if(token.kind==TOKEN_STRINGIDENTIFIER)
                        printf("TITLE=4 ARTIST=5 ALBUM=6 GENRE=7 FILENAME=8\n");
                else if(token.kind==TOKEN_NUMIDENTIFIER)
                        printf("YEAR=1 RATING=2 PLAYCOUNT=3\n");
		token.intvalue=0;
		if(token.kind==TOKEN_STRINGIDENTIFIER ||
			token.kind==TOKEN_NUMIDENTIFIER ||
			token.kind==TOKEN_NUM) {	
			printf("Token intvalue? ");
			fflush(stdout);
			fgets(buf,254,stdin);
			token.intvalue=strtol(buf,0,10);
		}
		fwrite(&token,sizeof(struct token),1,fp);
		done=token.kind==0;
	} while(!done);
	fclose(fp);
	printf("Successfully wrote tokenfile\n");
}
