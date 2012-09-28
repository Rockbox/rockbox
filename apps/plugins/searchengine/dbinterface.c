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
#include "dbinterface.h"

#undef SONGENTRY_SIZE
#undef FILEENTRY_SIZE
#undef ALBUMENTRY_SIZE
#undef ARTISTENTRY_SIZE
#undef FILERECORD2OFFSET

#define SONGENTRY_SIZE    (tagdbheader->songlen+12+tagdbheader->genrelen+12)
#define FILEENTRY_SIZE    (tagdbheader->filelen+12)
#define ALBUMENTRY_SIZE   (tagdbheader->albumlen+4+tagdbheader->songarraylen*4)
#define ARTISTENTRY_SIZE  (tagdbheader->artistlen+tagdbheader->albumarraylen*4)
#define RUNDBENTRY_SIZE   20

#define FILERECORD2OFFSET(_x_) (tagdbheader->filestart + _x_ * FILEENTRY_SIZE)

struct dbentry *currententry;
struct dbglobals dbglobal;
static struct dbentry *entryarray;

int database_init() {
    char *p;
    unsigned int i;
    // allocate room for all entries
    entryarray=(struct dbentry *)my_malloc(sizeof(struct dbentry)*tagdbheader->filecount);
    p=(char *)entryarray;
    // zero all entries.
    for(i=0;i<sizeof(struct dbentry)*tagdbheader->filecount;i++) 
        *(p++)=0;
    if(!*tagdb_initialized) {
        if(!tagdb_init()) {
            // failed loading db
            return -1;
        }
    }
    dbglobal.playcountmin=0;
    dbglobal.playcountmax=0;
    dbglobal.gotplaycountlimits=0;
    return 0;
}

long readlong(int fd) {
    long num;
    read(fd,&num,4);
#ifdef ROCKBOX_LITTLE_ENDIAN
    num=BE32(num);
#endif
    return num;
}

short readshort(int fd) {
    short num;
    read(fd,&num,2);
#ifdef ROCKBOX_LITTLE_ENDIAN
    num=BE16(num);
#endif
    return num;
}


void loadentry(int filerecord) {
    if(entryarray[filerecord].loadedfiledata==0) {
        lseek(*tagdb_fd,FILERECORD2OFFSET(filerecord),SEEK_SET);
        entryarray[filerecord].filename=(char *)my_malloc(tagdbheader->filelen);
        read(*tagdb_fd,entryarray[filerecord].filename,tagdbheader->filelen);
        entryarray[filerecord].hash=readlong(*tagdb_fd);
        entryarray[filerecord].songentry=readlong(*tagdb_fd);
        entryarray[filerecord].rundbentry=readlong(*tagdb_fd);
        entryarray[filerecord].loadedfiledata=1;
    }
    currententry=&entryarray[filerecord];
    dbglobal.currententryindex=filerecord;
}

void loadsongdata() {
    if(currententry->loadedsongdata) 
        return;
    currententry->title=(char *)my_malloc(tagdbheader->songlen);
    currententry->genre=(char *)my_malloc(tagdbheader->genrelen);
    lseek(*tagdb_fd,currententry->songentry,SEEK_SET);
    read(*tagdb_fd,currententry->title,tagdbheader->songlen);
    currententry->artistoffset=readlong(*tagdb_fd);
    currententry->albumoffset=readlong(*tagdb_fd);
    lseek(*tagdb_fd,4,SEEK_CUR);
    read(*tagdb_fd,currententry->genre,tagdbheader->genrelen);
    currententry->bitrate=readshort(*tagdb_fd);
    currententry->year=readshort(*tagdb_fd);
    currententry->playtime=readlong(*tagdb_fd);
    currententry->track=readshort(*tagdb_fd);
    currententry->samplerate=readshort(*tagdb_fd);
    currententry->loadedsongdata=1;
}

void loadrundbdata() {
    currententry->loadedrundbdata=1;
    if(!*rundb_initialized) 
        return;
    if(currententry->rundbentry==-1)
        return;
    lseek(*rundb_fd,currententry->rundbentry,SEEK_SET);
    currententry->rundbfe=readlong(*rundb_fd);
    currententry->rundbhash=readlong(*rundb_fd);
    currententry->rating=readshort(*rundb_fd);
    currententry->voladj=readshort(*rundb_fd);
    currententry->playcount=readlong(*rundb_fd);
    currententry->lastplayed=readlong(*rundb_fd);
}

void loadartistname() {
   /* memory optimization possible, only malloc for an album name once, then
    * write that pointer to the entrys using it.
   */
   if(currententry->loadedartistname)
       return;
   loadsongdata();
   currententry->artistname=(char *)my_malloc(tagdbheader->artistlen);
   lseek(*tagdb_fd,currententry->artistoffset,SEEK_SET);
   read(*tagdb_fd,currententry->artistname,tagdbheader->artistlen);
   currententry->loadedartistname=1;
}

void loadalbumname() {
   /* see the note at loadartistname */
   if(currententry->loadedalbumname)
       return;      
   loadsongdata();
   currententry->albumname=(char *)my_malloc(tagdbheader->albumlen);
   lseek(*tagdb_fd,currententry->albumoffset,SEEK_SET);
   read(*tagdb_fd,currententry->albumname,tagdbheader->albumlen);
   currententry->loadedalbumname=1;
}

char *getfilename(int entry) {
   if(entryarray[entry].loadedfiledata==0)
       return "error O.o;;;";
   else
       return entryarray[entry].filename;
}
