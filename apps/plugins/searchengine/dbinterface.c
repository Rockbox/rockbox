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
#include "dbinterface.h"

#undef SONGENTRY_SIZE
#undef FILEENTRY_SIZE
#undef ALBUMENTRY_SIZE
#undef ARTISTENTRY_SIZE
#undef FILERECORD2OFFSET

#define SONGENTRY_SIZE    (rb->tagdbheader->songlen+12+rb->tagdbheader->genrelen+4)
#define FILEENTRY_SIZE    (rb->tagdbheader->filelen+12)
#define ALBUMENTRY_SIZE   (rb->tagdbheader->albumlen+4+rb->tagdbheader->songarraylen*4)
#define ARTISTENTRY_SIZE  (rb->tagdbheader->artistlen+rb->tagdbheader->albumarraylen*4)

#define FILERECORD2OFFSET(_x_) (rb->tagdbheader->filestart + _x_ * FILEENTRY_SIZE)

struct entry *currententry;

static struct entry *entryarray;

int database_init() {
    char *p;
    unsigned int i;
    // allocate room for all entries
    entryarray=(struct entry *)my_malloc(sizeof(struct entry)*rb->tagdbheader->filecount);
    p=(char *)entryarray;
    // zero all entries.
    for(i=0;i<sizeof(struct entry)*rb->tagdbheader->filecount;i++) 
	    *(p++)=0;
    if(*rb->tagdb_initialized!=1) {
        if(!rb->tagdb_init()) {
            // failed loading db
            return -1;
        }
    }
    return 0;
}

long readlong(int fd) {
	long num;
	rb->read(fd,&num,4);
#ifdef ROCKBOX_LITTLE_ENDIAN
	num=BE32(num);
#endif
	return num;
}

short readshort(int fd) {
        short num;
        rb->read(fd,&num,2);
#ifdef ROCKBOX_LITTLE_ENDIAN
        num=BE16(num);
#endif
        return num;
}


void loadentry(int filerecord) {
    if(entryarray[filerecord].loadedfiledata==0) {
        rb->lseek(*rb->tagdb_fd,FILERECORD2OFFSET(filerecord),SEEK_SET);
        entryarray[filerecord].filename=(char *)my_malloc(rb->tagdbheader->filelen);
	rb->read(*rb->tagdb_fd,entryarray[filerecord].filename,rb->tagdbheader->filelen);
	entryarray[filerecord].hash=readlong(*rb->tagdb_fd);
	entryarray[filerecord].songentry=readlong(*rb->tagdb_fd);
	entryarray[filerecord].rundbentry=readlong(*rb->tagdb_fd);
	entryarray[filerecord].loadedfiledata=1;
    }
    currententry=&entryarray[filerecord];
}

void loadsongdata() {
    if(currententry->loadedsongdata || 
        !currententry->loadedfiledata) 
        return;
    currententry->title=(char *)my_malloc(rb->tagdbheader->songlen);
    currententry->genre=(char *)my_malloc(rb->tagdbheader->genrelen);
    rb->lseek(*rb->tagdb_fd,currententry->songentry,SEEK_SET);
    rb->read(*rb->tagdb_fd,currententry->title,rb->tagdbheader->songlen);
    currententry->artistoffset=readlong(*rb->tagdb_fd);
    currententry->albumoffset=readlong(*rb->tagdb_fd);
    rb->lseek(*rb->tagdb_fd,4,SEEK_CUR);
    rb->read(*rb->tagdb_fd,currententry->genre,rb->tagdbheader->genrelen);
    currententry->bitrate=readshort(*rb->tagdb_fd);
    currententry->year=readshort(*rb->tagdb_fd);
    currententry->loadedsongdata=1;
}

void loadrundbdata() {
	// we don't do this yet.
    currententry->loadedrundbdata=1;
}

void loadartistname() {
   /* memory optimization possible, only malloc for an album name once, then
    * write that pointer to the entrys using it.
   */
   currententry->artistname=(char *)my_malloc(rb->tagdbheader->artistlen);
   rb->lseek(*rb->tagdb_fd,currententry->artistoffset,SEEK_SET);
   rb->read(*rb->tagdb_fd,currententry->artistname,rb->tagdbheader->artistlen);
   currententry->loadedartistname=1;
}

void loadalbumname() {
   /* see the note at loadartistname */
   currententry->albumname=(char *)my_malloc(rb->tagdbheader->albumlen);
   rb->lseek(*rb->tagdb_fd,currententry->albumoffset,SEEK_SET);
   rb->read(*rb->tagdb_fd,currententry->albumname,rb->tagdbheader->albumlen);
   currententry->loadedalbumname=1;
}

char *getfilename(int entry) {
   if(entryarray[entry].loadedfiledata==0)
	   return "error O.o;;;";
   else
	   return entryarray[entry].filename;
}
