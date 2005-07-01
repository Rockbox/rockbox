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
#include "wps.h"
#include "filetypes.h"
#include "applimits.h"
#include "icons.h"
#include "lang.h"
#include "keyboard.h"
#include "database.h"
#include "autoconf.h"
#include "playback.h"
#include "logf.h"

/* internal functions */
void writetagdbheader(void);
void writefentry(void);
void getfentrybyoffset(int offset);
void update_fentryoffsets(int start, int end);
void writerundbheader(void);
void getrundbentrybyoffset(int offset);
void writerundbentry(void);
int getfentrybyfilename(char *fname);
int getfentrybyhash(int hash);
int deletefentry(char *fname);
int tagdb_shiftdown(int targetoffset, int startingoffset, int bytes);
int tagdb_shiftup(int targetoffset, int startingoffset, int bytes);
                    
static char sbuf[1024];
static struct file_entry fe;
static int currentfeoffset, currentferecord;

int tagdb_fd = -1;
int tagdb_initialized = 0;
struct tagdb_header tagdbheader;

int tagdb_init(void)
{
    unsigned char* ptr = (char*)&tagdbheader.version;
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
        splash(HZ,true,"Not a rockbox ID3 database!");
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
        splash(HZ,true,"Unsupported database version %d!",
               tagdbheader.version&0xFF);
        return -1;
    }

    if (tagdbheader.songstart > tagdbheader.filestart ||
        tagdbheader.albumstart > tagdbheader.songstart ||
        tagdbheader.artiststart > tagdbheader.albumstart)
    {
        splash(HZ,true,"Corrupt ID3 database!");
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

void writefentry(void)
{
    lseek(tagdb_fd,currentfeoffset,SEEK_SET);
    write(tagdb_fd,sbuf,tagdbheader.filelen);
    write(tagdb_fd,&fe.hash,12);
    fsync(tagdb_fd);
}

void getfentrybyoffset(int offset)
{
    memset(&fe,0,sizeof(struct file_entry));
    lseek(tagdb_fd,offset,SEEK_SET);
    read(tagdb_fd,sbuf,tagdbheader.filelen);
    read(tagdb_fd,&fe.hash,12);
    fe.name=sbuf;
    currentfeoffset=offset;
    currentferecord=(offset-tagdbheader.filestart)/FILEENTRY_SIZE;
}

#define getfentrybyrecord(_x_)  getfentrybyoffset(FILERECORD2OFFSET(_x_))

int getfentrybyfilename(char *fname)
{
    int min=0;
    int max=tagdbheader.filecount;
    while(min<max) {
        int mid=(min+max)/2;
        int compare;
        getfentrybyrecord(mid);
        compare=strcasecmp(fname,fe.name);
        if(compare==0)
            return 1;
        else if(compare<0)
            max=mid;
        else
            min=mid+1;
    }
    return 0;
}

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
              splash(HZ*2,true, "o.o.. found a rundbentry? o.o; didn't update "
                     "it, update the code o.o;");
         }
    }
}

int tagdb_shiftdown(int targetoffset, int startingoffset, int bytes)
{
    int amount;
    if(targetoffset>=startingoffset) {
        splash(HZ*2,true,"Woah. no beeping way. (tagdb_shiftdown)");
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
            splash(HZ*2,true,"Something went very wrong. expect database "
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
        splash(HZ*2,true,"Um. no. (tagdb_shiftup)");
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
             splash(HZ*2,true,"Something went very wrong. expect database "
                    "corruption. (tagdb_shiftup)");
             return 0;
        }
        lseek(tagdb_fd,writepos,SEEK_SET);
        amount=write(tagdb_fd,sbuf,amount2);
        if(amount2!=amount) {
            splash(HZ*2,true,"Something went very wrong. expect database "
                   "corruption. (tagdb_shiftup)");
            return 0;
        }
        bytes-=amount;
    } while (amount>0);
    if(bytes==0)
        return 1;
    else {
        splash(HZ*2,true,"Something went wrong, >.>;; (tagdb_shiftup)");
        return 0;
    }
}

/*** end TagDatabase code ***/

int rundb_fd = -1;
int rundb_initialized = 0;
struct rundb_header rundbheader;

static int valid_file, currentreoffset,rundbsize;
static struct rundb_entry rundbentry;

/*** RuntimeDatabase code ***/

void rundb_track_changed(struct track_info *ti)
{
    increaseplaycount();
    logf("rundb new track: %s", ti->id3.path);
    loadruntimeinfo(ti->id3.path);
}

int rundb_init(void)
{
#if CONFIG_HWCODEC != MASNONE
    return -1;
#else
    unsigned char* ptr = (char*)&rundbheader.version;
#ifdef ROCKBOX_LITTLE_ENDIAN
    int i, *p;
#endif
    if(!tagdb_initialized) /* forget it.*/
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
        splash(HZ,true,"Not a rockbox runtime database!");
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
        splash(HZ,true,"Unsupported runtime database version %d!",
               rundbheader.version&0xFF);
        return -1;
    }

    rundb_initialized = 1;
    audio_set_track_changed_event(&rundb_track_changed);
    memset(&rundbentry,0,sizeof(struct rundb_entry));
    rundbsize=lseek(rundb_fd,0,SEEK_END);
    return 0;
#endif
}

void writerundbheader(void)
{
  lseek(rundb_fd,0,SEEK_SET);
  write(rundb_fd, &rundbheader, 8);
  fsync(rundb_fd);
}

#define getrundbentrybyrecord(_x_)  getrundbentrybyoffset(8+_x_*20)

void getrundbentrybyoffset(int offset) {
    lseek(rundb_fd,offset,SEEK_SET);
    read(rundb_fd,&rundbentry,20);
    currentreoffset=offset;
#ifdef ROCKBOX_LITTLE_ENDIAN
    rundbentry.fileentry=BE32(rundbentry.fileentry);
    rundbentry.hash=BE32(rundbentry.hash);
    rundbentry.rating=BE16(rundbentry.rating);
    rundbentry.voladjust=BE16(rundbentry.voladjust);
    rundbentry.playcount=BE32(rundbentry.playcount);
    rundbentry.lastplayed=BE32(rundbentry.lastplayed);
#endif
}

int getrundbentrybyhash(int hash)
{
    int min=0;
    for(min=0;min<rundbheader.entrycount;min++) {
        getrundbentrybyrecord(min);
        if(hash==rundbentry.hash)
             return 1;
    }
    memset(&rundbentry,0,sizeof(struct rundb_entry));
    return 0;
}

void writerundbentry(void)
{
    if(rundbentry.hash==0) /* 0 = invalid rundb info. */
       return;
    lseek(rundb_fd,currentreoffset,SEEK_SET);
    write(rundb_fd,&rundbentry,20);
    fsync(rundb_fd);
}

void loadruntimeinfo(char *filename)
{
    memset(&rundbentry,0,sizeof(struct rundb_entry));
    valid_file=0;
    if(!getfentrybyfilename(filename)) 
        return; /* file is not in tagdatabase, could not load. */
    valid_file=1;
    if(fe.rundbentry!=-1&&fe.rundbentry<rundbsize) {
        logf("load rundbentry: 0x%x",fe.rundbentry);
        getrundbentrybyoffset(fe.rundbentry);
        if(fe.hash!=rundbentry.hash) {
            logf("Rundb: Hash mismatch. trying to repair entry.",
                 fe.hash,rundbentry.hash);
            addrundbentry();
        }
    }
    else  /* add new rundb entry. */
        addrundbentry();
}

void addrundbentry()
{
    /* first search for an entry with an equal hash. */
    if(getrundbentrybyhash(fe.hash)) {
        logf("Found existing rundb entry: 0x%x",currentreoffset);
        fe.rundbentry=currentreoffset;
        writefentry();
        return;
    }
    rundbheader.entrycount++;
    writerundbheader();
    fe.rundbentry=currentreoffset=lseek(rundb_fd,0,SEEK_END);
    logf("Add rundb entry: 0x%x hash: 0x%x",fe.rundbentry,fe.hash);
    rundbentry.hash=fe.hash;
    rundbentry.fileentry=currentfeoffset;
    writefentry();
    writerundbentry();
    rundbsize=lseek(rundb_fd,0,SEEK_END);
}

void increaseplaycount(void)
{
    if(rundbentry.hash==0) /* 0 = invalid rundb info. */
       return;
    rundbentry.playcount++;
    writerundbentry();
}

void setrating(int rating)
{
    if(rundbentry.hash==0) /* 0 = invalid rundb info. */
       return;
    rundbentry.rating=rating;
    writerundbentry();
}

/*** end RuntimeDatabase code ***/
