/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id
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

#undef NEW_DB_CODE

#ifdef NEW_DB_CODE
static char sbuf[1024];
static struct file_entry fe;
static int currentfeoffset, currentferecord;
#endif

int tagdb_fd = -1;
int tagdb_initialized = 0;
struct tagdb_header tagdbheader;

int tagdb_init(void)
{
    unsigned char* ptr = (char*)&tagdbheader.version;
#ifdef LITTLE_ENDIAN
    int i, *p;
#endif

    tagdb_fd = open(ROCKBOX_DIR "/rockbox.id3db", O_RDONLY);
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
#ifdef LITTLE_ENDIAN
    p=(int *)&tagdbheader;
    for(i=0;i<17;i++) {
        *p=BE32(*p);
        p++;
    }
#endif
    if ( (tagdbheader.version&0xFF) != TAGDB_VERSION)
    {
        splash(HZ,true,"Unsupported database version %d!", tagdbheader.version&0xFF);
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

#ifdef NEW_DB_CODE

/*** TagDatabase code ***/

void writetagdbheader() {
        lseek(tagdb_fd,0,SEEK_SET);
        write(tagdb_fd, &tagdbheader, 68);
}

void getfentrybyoffset(int offset) {
        lseek(tagdb_fd,offset,SEEK_SET);
        fread(tagdb_fd,sbuf,tagdbheader.filelen);
        fread(tagdb_fd,&fe+sizeof(char *),12);
        fe.name=sbuf;
        currentfeoffset=offset;
        currentferecord=(offset-tagdbheader.filestart)/FILEENTRY_SIZE;
}

#define getfentrybyrecord(_x_)  getfentrybyoffset(FILERECORD2OFFSET(_x_))

int getfentrybyfilename(char *fname) {
        int min=0;
        int max=tagdbheader.filecount;
        while(min<max) {
          int mid=(min+max)/2;
          int compare;
          getfentrybyrecord(mid);
          compare=strcasecmp(fname,fe.name));
          if(compare==0)
            return 1;
          else if(compare<0)
            max=mid;
          else
            min=mid+1;
        }
        return 0;
}

int getfentrybyhash(int hash) {
        int min=0;
        for(min=0;min<tagdbheader.filecount;min++) {
          getfentrybyrecord(min);
          if(hash==fe.hash)
             return 1;
        }
        return 0;
}

int deletefentry(char *fname) {
        if(!getfentrybyfilename(fname))
           return 0;
        int restrecord = currentferecord+1; 
        if(currentferecord==tagdbheader.filecount) /* file is last entry */
           ftruncate(tagdb_fd,lseek(tagdb_fd,0,SEEK_END)-FILEENTRY_SIZE);
        else 
           shiftdown(FILERECORD2OFFSET(currentferecord),FILERECORD2OFFSET(restrecord));
        tagdbheader.filecount--;
        update_fentryoffsets(restrecord,tagdbheader.filecount);
        writetagdbheader();
        return 1;
}

int update_fentryoffsets(int start, int end) {
        int i;
        for(int i=start;i<end;i++) {
                getfentrybyrecord(i);
                if(fe.songentry!=-1) {
                        int *p;
                        void *songentry=(void *)sbuf+tagdbheader.filelen+2;
                        lseek(tagdb_fd,fe.songentry,SEEK_SET);
                        read(tagdb_fd,songentry,SONGENTRY_SIZE);
                        p=(int *)(songentry+tagdbheader.songlen+8);
                        if(*p!=currentfeoffset) {
                           *p=currentfeoffset;
                          lseek(tagdb_fd,fe.songentry,SEEK_SET);
                          write(tagdb_fd,songentry,SONGENTRY_SIZE);
                        }
                }
                if(fe.rundbentry!=-1) {
                        splash(HZ*2, "o.o.. found a rundbentry? o.o; didn't update it, update the code o.o;");
                }
        }
}

int tagdb_shiftdown(int targetoffset, int startingoffset) {
        int amount;
        if(targetoffset>=startingoffset) {
                splash(HZ*2,"Woah. no beeping way. (tagdb_shiftdown)");
                return 0;
        }
        lseek(tagdb_fd,startingoffset,SEEK_SET);
        while(amount=read(tagdb_fd,sbuf,1024)) {
                int written;
                startingoffset+=amount;
                lseek(tagdb_fd,targetoffset,SEEK_SET);
                written=write(tagdb_fd,sbuf,amount);
                targetoffset+=written;
                if(amount!=written) {
                        splash(HZ*2,"Something went very wrong. expect database corruption. (tagdb_shiftdown)");
                        return 0;
                }
                lseek(tagdb_fd,startingoffset,SEEK_SET);
        }
        ftruncate(tagdb_fd,lseek(tagdb_fd,0,SEEK_END) - (startingoffset-targetoffset));
        return 1;
}

int tagdb_shiftup(int targetoffset, int startingoffset) {
        int amount,amount2;
        int readpos,writepos,filelen;
        int ok;
        if(targetoffset<=startingoffset) {
                splash(HZ*2,"Um. no. (tagdb_shiftup)");
        }
        filelen=lseek(tagdb_fd,0,SEEK_END);
        readpos=filelen;
        do {
          amount=readpos-startingoffset>1024 ? 1024 : readpos-startingoffset;
          readpos-=amount;
          writepos=readpos+(targetoffset-startingoffset);
          lseek(tagdb_fd,readpos,SEEK_SET);
          amount2=read(tagdb_fd,sbuf,amount);
          if(amount2!=amount) {
                  splash(HZ*2,"Something went very wrong. expect database corruption. (tagdb_shiftup)");
          }
          lseek(tagdb_fd,writepos,SEEK_SET);
          amount=write(tagdb_fd,sbuf,amount2);
          if(amount2!=amount) {
                  splash(HZ*2,"Something went very wrong. expect database corruption. (tagdb_shiftup)");
          }
        } while (amount>0);
        if(amount==0)
                return 1;
        else {
                splash(HZ*2,"Something went wrong, >.>;; (tagdb_shiftup)");
                return 0;
        }
}

/*** end TagDatabase code ***/

/*** RuntimeDatabase code ***/



/*** end RuntimeDatabase code ***/

#endif
