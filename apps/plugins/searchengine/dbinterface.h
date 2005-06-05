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
struct dbglobals {
    int playcountmin;
    int playcountmax;
    int gotplaycountlimits;
    int currententryindex;
};

struct entry {
    int loadedfiledata,
        loadedsongdata,
        loadedrundbdata,
        loadedalbumname,
        loadedartistname;
    char *filename;
    long hash;
    long songentry;
    long rundbentry;
    short year;
    short bitrate;
    long rating;
    long playcount;
    char *title;
    char *genre;
    long artistoffset;
    long albumoffset;
    char *artistname;
    char *albumname;
    long playtime;
    short track;
    short samplerate;
};

extern struct entry *currententry;
extern struct entry *entryarray;
extern struct dbglobals dbglobal;

int database_init(void);
void loadentry(int filerecord);
void loadsongdata(void);
void loadrundbdata(void);
void loadartistname(void);
void loadalbumname(void);
char *getfilename(int entry);
