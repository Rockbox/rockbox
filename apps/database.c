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
#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include "file.h"
#include "screens.h"
#include "kernel.h"
#include "tree.h"
#include "lcd.h"
#include "font.h"
#include "settings.h"
#include "icons.h"
#include "status.h"
#include "debug.h"
#include "button.h"
#include "menu.h"
#include "main_menu.h"
#include "mpeg.h"
#include "misc.h"
#include "ata.h"
#include "filetypes.h"
#include "applimits.h"
#include "icons.h"
#include "lang.h"
#include "keyboard.h"
#include "database.h"
#include "autoconf.h"
#include "splash.h"

#if CONFIG_CODEC == SWCODEC
#include "playback.h"
#else
#include "mpeg.h"
#endif

#include "logf.h"

/* internal functions */
void writetagdbheader(void);
void writefentry(struct mp3entry *id);
void getfentrybyoffset(struct mp3entry *id,int offset);
void update_fentryoffsets(int start, int end);
void writerundbheader(void);
void getrundbentrybyoffset(struct mp3entry *id,int offset);
void writerundbentry(struct mp3entry *id);
int getfentrybyfilename(struct mp3entry *id);
void clearfileentryinfo(struct mp3entry *id);
void clearruntimeinfo(struct mp3entry *id);
int findrundbentry(struct mp3entry *id);

int getfentrybyhash(int hash);
int deletefentry(char *fname);
int tagdb_shiftdown(int targetoffset, int startingoffset, int bytes);
int tagdb_shiftup(int targetoffset, int startingoffset, int bytes);
                    
static char sbuf[MAX_PATH];

int tagdb_fd = -1;
int tagdb_initialized = 0;
struct tagdb_header tagdbheader;

int tagdb_init(void)
{
    unsigned char* ptr = (unsigned char*)&tagdbheader.version;
#ifdef ROCKBOX_LITTLE_ENDIAN
    int i, *p;
#endif

    tagdb_fd = open(ROCKBOX_DIR "/rockbox.tagdb", O_RDWR);
    if (tagdb_fd < 0) {
        DEBUGF("Failed opening database\n");
        return -1;
    }
    read(tagdb_fd, &tagdbheader, 68);

    if (ptr[0] != 'R' ||
        ptr[1] != 'D' ||
        ptr[2] != 'B')
    {
        gui_syncsplash(HZ, true,
                       (unsigned char *)"Not a rockbox ID3 database!");
        return -1;
    }
#ifdef ROCKBOX_LITTLE_ENDIAN
    p=(int *)&tagdbheader;
    for(i=0;i<17;i++) {
        *p=BE32(*p);
        p++;
    }
#endif
    if ( (tagdbheader.version&0xFF) != TAGDB_VERSION)
    {
        gui_syncsplash(HZ, true,
                       (unsigned char *)"Unsupported database version %d!",
                       tagdbheader.version&0xFF);
        return -1;
    }

    if (tagdbheader.songstart > tagdbheader.filestart ||
        tagdbheader.albumstart > tagdbheader.songstart ||
        tagdbheader.artiststart > tagdbheader.albumstart)
    {
        gui_syncsplash(HZ, true, (unsigned char *)"Corrupt ID3 database!");
        return -1;
    }

    tagdb_initialized = 1;
    return 0;
}

void tagdb_shutdown(void)
{
    if (tagdb_fd >= 0)
        close(tagdb_fd);
    tagdb_initialized = 0;
}

/* NOTE: All these functions below are yet untested. */

/*** TagDatabase code ***/

void writetagdbheader(void)
{
    lseek(tagdb_fd,0,SEEK_SET);
    write(tagdb_fd, &tagdbheader, 68);
    fsync(tagdb_fd);
}

void writefentry(struct mp3entry *id)
{   long temp;
    if(!id->fileentryoffset)
        return;
    lseek(tagdb_fd,id->fileentryoffset,SEEK_SET);
    write(tagdb_fd,id->path,tagdbheader.filelen);
    temp=id->filehash;
    write(tagdb_fd,&temp,4);
    temp=id->songentryoffset;
    write(tagdb_fd,&temp,4);
    temp=id->rundbentryoffset;
    write(tagdb_fd,&temp,4);
}

void getfentrybyoffset(struct mp3entry *id,int offset)
{   
    clearfileentryinfo(id);
    lseek(tagdb_fd,offset,SEEK_SET);
    read(tagdb_fd,sbuf,tagdbheader.filelen);
    id->fileentryoffset=offset;
    read(tagdb_fd,&id->filehash,4);
    read(tagdb_fd,&id->songentryoffset,4);
    read(tagdb_fd,&id->rundbentryoffset,4);
}

#define getfentrybyrecord(_y_,_x_)  getfentrybyoffset(_y_,FILERECORD2OFFSET(_x_))

int getfentrybyfilename(struct mp3entry *id)
{
    int min=0;
    int max=tagdbheader.filecount;
    while(min<max) {
        int mid=(min+max)/2;
        int compare;
        getfentrybyrecord(id,mid);
        compare=strcasecmp(id->path,sbuf);
        if(compare==0)
            return 1;
        else if(compare<0)
            max=mid;
        else
            min=mid+1;
    }
    clearfileentryinfo(id);
    return 0;
}

#if 0
int getfentrybyhash(int hash)
{
    int min=0;
    for(min=0;min<tagdbheader.filecount;min++) {
        getfentrybyrecord(min);
        if(hash==fe.hash)
             return 1;
    }
    return 0;
}

int deletefentry(char *fname)
{
    if(!getfentrybyfilename(fname))
        return 0;
    int restrecord = currentferecord+1; 
    if(currentferecord!=tagdbheader.filecount) /* file is not last entry */
         tagdb_shiftdown(FILERECORD2OFFSET(currentferecord),
                         FILERECORD2OFFSET(restrecord),
                         (tagdbheader.filecount-restrecord)*FILEENTRY_SIZE);
    ftruncate(tagdb_fd,lseek(tagdb_fd,0,SEEK_END)-FILEENTRY_SIZE);
    tagdbheader.filecount--;
    update_fentryoffsets(restrecord,tagdbheader.filecount);
    writetagdbheader();
    return 1;
}

void update_fentryoffsets(int start, int end)
{
    int i;
    for(i=start;i<end;i++) {
        getfentrybyrecord(i);
        if(fe.songentry!=-1) {
             int p;
             lseek(tagdb_fd,fe.songentry+tagdbheader.songlen+8,SEEK_SET);
             read(tagdb_fd,&p,sizeof(int));
             if(p!=currentfeoffset) {
                  p=currentfeoffset;
                  lseek(tagdb_fd,fe.songentry+tagdbheader.songlen+8,SEEK_SET);
                  write(tagdb_fd,&p,sizeof(int));
             }
         }
         if(fe.rundbentry!=-1) {
              gui_syncsplash(HZ*2,true, "o.o.. found a rundbentry? o.o; didn't update "
                     "it, update the code o.o;");
         }
    }
}

int tagdb_shiftdown(int targetoffset, int startingoffset, int bytes)
{
    int amount;
    if(targetoffset>=startingoffset) {
        gui_syncsplash(HZ*2,true,"Woah. no beeping way. (tagdb_shiftdown)");
        return 0;
    }
    lseek(tagdb_fd,startingoffset,SEEK_SET);
    while((amount=read(tagdb_fd,sbuf,(bytes > 1024) ? 1024 : bytes))) {
        int written;
        startingoffset+=amount;
        lseek(tagdb_fd,targetoffset,SEEK_SET);
        written=write(tagdb_fd,sbuf,amount);
        targetoffset+=written;
        if(amount!=written) {
            gui_syncsplash(HZ*2,true,"Something went very wrong. expect database "
                   "corruption. (tagdb_shiftdown)");
            return 0;
        }
        lseek(tagdb_fd,startingoffset,SEEK_SET);
        bytes-=amount;
    }
    return 1;
}

int tagdb_shiftup(int targetoffset, int startingoffset, int bytes)
{
    int amount,amount2;
    int readpos,writepos,filelen;
    if(targetoffset<=startingoffset) {
        gui_syncsplash(HZ*2,true,"Um. no. (tagdb_shiftup)");
        return 0;
    }
    filelen=lseek(tagdb_fd,0,SEEK_END);
    readpos=startingoffset+bytes;
    do {
        amount=bytes>1024 ? 1024 : bytes;
        readpos-=amount;
        writepos=readpos+targetoffset-startingoffset;
        lseek(tagdb_fd,readpos,SEEK_SET);
        amount2=read(tagdb_fd,sbuf,amount);
        if(amount2!=amount) {
             gui_syncsplash(HZ*2,true,"Something went very wrong. expect database "
                    "corruption. (tagdb_shiftup)");
             return 0;
        }
        lseek(tagdb_fd,writepos,SEEK_SET);
        amount=write(tagdb_fd,sbuf,amount2);
        if(amount2!=amount) {
            gui_syncsplash(HZ*2,true,"Something went very wrong. expect database "
                   "corruption. (tagdb_shiftup)");
            return 0;
        }
        bytes-=amount;
    } while (amount>0);
    if(bytes==0)
        return 1;
    else {
        gui_syncsplash(HZ*2,true,"Something went wrong, >.>;; (tagdb_shiftup)");
        return 0;
    }
}
#endif

/*** end TagDatabase code ***/

int rundb_fd = -1;
int rundb_initialized = 0;
struct rundb_header rundbheader;

static long rundbsize;

/*** RuntimeDatabase code ***/

void rundb_unbuffer_track(struct mp3entry *id, bool last_track) {
    writeruntimeinfo(id);
    if(last_track) {
        fsync(rundb_fd);
        fsync(tagdb_fd);
    }
}

void rundb_track_change(struct mp3entry *id) {
    id->playcount++;
}

void rundb_buffer_track(struct mp3entry *id, bool last_track) {
    loadruntimeinfo(id);
    if(last_track) {
        fsync(rundb_fd);
        fsync(tagdb_fd);
    }
}

int rundb_init(void)
{
    unsigned char* ptr = (unsigned char*)&rundbheader.version;
#ifdef ROCKBOX_LITTLE_ENDIAN
    int i, *p;
#endif
    if(!tagdb_initialized) /* forget it.*/
        return -1;
        
    if(!global_settings.runtimedb) /* user doesn't care */
        return -1;
    
    rundb_fd = open(ROCKBOX_DIR "/rockbox.rundb", O_CREAT|O_RDWR);
    if (rundb_fd < 0) {
        DEBUGF("Failed opening database\n");
        return -1;
    }
    if(read(rundb_fd, &rundbheader, 8)!=8) {
        ptr[0]=ptr[1]='R';
        ptr[2]='D';
        ptr[3]=0x1;
        rundbheader.entrycount=0;
        writerundbheader();
    }

    if (ptr[0] != 'R' ||
        ptr[1] != 'R' ||
        ptr[2] != 'D')
    {
        gui_syncsplash(HZ, true,
                       (unsigned char *)"Not a rockbox runtime database!");
        return -1;
    }
#ifdef ROCKBOX_LITTLE_ENDIAN
    p=(int *)&rundbheader;
    for(i=0;i<2;i++) {
        *p=BE32(*p);
        p++;
    }
#endif
    if ( (rundbheader.version&0xFF) != RUNDB_VERSION)
    {
        gui_syncsplash(HZ, true, (unsigned char *)
                       "Unsupported runtime database version %d!",
                       rundbheader.version&0xFF);
        return -1;
    }

    rundb_initialized = 1;
    audio_set_track_buffer_event(&rundb_buffer_track);
    audio_set_track_changed_event(&rundb_track_change);
    audio_set_track_unbuffer_event(&rundb_unbuffer_track);
    logf("rundb inited.");

    rundbsize=lseek(rundb_fd,0,SEEK_END);
    return 0;
}

void rundb_shutdown(void)
{
    if (rundb_fd >= 0)
        close(rundb_fd);
    rundb_initialized = 0;
    audio_set_track_buffer_event(NULL);
    audio_set_track_unbuffer_event(NULL);
    audio_set_track_changed_event(NULL);
}

void writerundbheader(void)
{
  lseek(rundb_fd,0,SEEK_SET);
  write(rundb_fd, &rundbheader, 8);
}

#define getrundbentrybyrecord(_y_,_x_)  getrundbentrybyoffset(_y_,8+_x_*20)

void getrundbentrybyoffset(struct mp3entry *id,int offset) {
    lseek(rundb_fd,offset+4,SEEK_SET); // skip fileentry offset
    id->rundbentryoffset=offset;
    read(rundb_fd,&id->rundbhash,4);
    read(rundb_fd,&id->rating,2);
    read(rundb_fd,&id->voladjust,2);
    read(rundb_fd,&id->playcount,4);
    read(rundb_fd,&id->lastplayed,4);
#ifdef ROCKBOX_LITTLE_ENDIAN
    id->rundbhash=BE32(id->rundbhash);
    id->rating=BE16(id->rating);
    id->voladjust=BE16(id->voladjust);
    id->playcount=BE32(id->playcount);
    id->lastplayed=BE32(id->lastplayed);
#endif
}

int getrundbentrybyhash(struct mp3entry *id)
{
    int min=0;
    for(min=0;min<rundbheader.entrycount;min++) {
        getrundbentrybyrecord(id,min);
        if(id->filehash==id->rundbhash)
             return 1;
    }
    clearruntimeinfo(id);
    return 0;
}

void writerundbentry(struct mp3entry *id)
{
    if(id->rundbhash==0) /* 0 = invalid rundb info. */
       return;
    lseek(rundb_fd,id->rundbentryoffset,SEEK_SET);
    write(rundb_fd,&id->fileentryoffset,4);
    write(rundb_fd,&id->rundbhash,4);
    write(rundb_fd,&id->rating,2);
    write(rundb_fd,&id->voladjust,2);
    write(rundb_fd,&id->playcount,4);
    write(rundb_fd,&id->lastplayed,4);
}

void writeruntimeinfo(struct mp3entry *id) {
    logf("rundb write");
    if(!id->rundbhash) 
        addrundbentry(id);
    else
        writerundbentry(id);
}

void clearfileentryinfo(struct mp3entry *id) {
    id->fileentryoffset=0;
    id->filehash=0;
    id->songentryoffset=0;
    id->rundbentryoffset=0;
}

void clearruntimeinfo(struct mp3entry *id) {
    id->rundbhash=0;
    id->rating=0;
    id->voladjust=0;
    id->playcount=0;
    id->lastplayed=0;
}

void loadruntimeinfo(struct mp3entry *id)
{
    logf("rundb load");
    clearruntimeinfo(id);
    clearfileentryinfo(id);
    if(!getfentrybyfilename(id)) {
        logf("tagdb fail: %s",id->path);
        return; /* file is not in tagdatabase, could not load. */
    }
    if(id->rundbentryoffset!=-1 && id->rundbentryoffset<rundbsize) {
        logf("load rundbentry: 0x%x", id->rundbentryoffset);
        getrundbentrybyoffset(id, id->rundbentryoffset);
        if(id->filehash != id->rundbhash) {
            logf("Rundb: Hash mismatch. trying to repair entry.",
                 id->filehash, id->rundbhash);
            findrundbentry(id);
        }
    }
    else 
#ifdef ROCKBOX_HAS_LOGF
        if(!findrundbentry(id))
            logf("rundb:no entry and not found.");
#else
        findrundbentry(id);
#endif
}

int findrundbentry(struct mp3entry *id) {
    if(getrundbentrybyhash(id)) {
        logf("Found existing rundb entry: 0x%x",id->rundbentryoffset);
        writefentry(id);
        return 1;
    }
    clearruntimeinfo(id);
    return 0;
}

void addrundbentry(struct mp3entry *id)
{
    /* first search for an entry with an equal hash. */
/*    if(findrundbentry(id)) 
           return; disabled cause this SHOULD have been checked at the buffer event.. */
    rundbheader.entrycount++;
    writerundbheader();
    id->rundbentryoffset=lseek(rundb_fd,0,SEEK_END);
    logf("Add rundb entry: 0x%x hash: 0x%x",id->rundbentryoffset,id->filehash);
    id->rundbhash=id->filehash;
    writefentry(id);
    writerundbentry(id);
    rundbsize=lseek(rundb_fd,0,SEEK_END);
}

/*** end RuntimeDatabase code ***/
