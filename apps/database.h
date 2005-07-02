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
#ifndef DATABASE_H
#define DATABASE_H

#ifdef ROCKBOX_LITTLE_ENDIAN
#define BE32(_x_) (((_x_ & 0xff000000) >> 24) | \
                                   ((_x_ & 0x00ff0000) >> 8) | \
                                   ((_x_ & 0x0000ff00) << 8) | \
                                   ((_x_ & 0x000000ff) << 24))
#define BE16(_x_) ( ((_x_&0xFF00) >> 8)|((_x_&0xFF)<<8))
#else
#define BE32(_x_) _x_
#define BE16(_x_) _x_
#endif

#define SONGENTRY_SIZE    (tagdbheader.songlen+12+tagdbheader.genrelen+12)
#define FILEENTRY_SIZE    (tagdbheader.filelen+12)
#define ALBUMENTRY_SIZE   (tagdbheader.albumlen+4+tagdbheader.songarraylen*4)
#define ARTISTENTRY_SIZE  (tagdbheader.artistlen+tagdbheader.albumarraylen*4)

#define FILERECORD2OFFSET(_x_) (tagdbheader.filestart + _x_ * FILEENTRY_SIZE)

extern int tagdb_initialized;

struct tagdb_header {
        int version;
        int artiststart;
        int albumstart;
        int songstart;
        int filestart;
        int artistcount;
        int albumcount;
        int songcount;
        int filecount;
        int artistlen;
        int albumlen;
        int songlen;
        int genrelen;
        int filelen;
        int songarraylen;
        int albumarraylen;
        int rundbdirty;
};

struct file_entry {
        char *name;
        int hash;
        int songentry;
        int rundbentry;
};

extern struct tagdb_header tagdbheader;
extern int tagdb_fd;

int tagdb_init(void);
void tagdb_shutdown(void);

#define TAGDB_VERSION 3

extern int rundb_fd, rundb_initialized;
extern struct rundb_header rundbheader;

struct rundb_header {
        int version;
        int entrycount;
};

struct rundb_entry {
        int fileentry;
        int hash;
        short rating;
        short voladjust;
        int playcount;
        int lastplayed;
};

extern struct rundb_header rundbheader;

#define RUNDB_VERSION 1

void tagdb_shutdown(void);
void addrundbentry(void);
void loadruntimeinfo(char *filename);
void increaseplaycount(void);
void setrating(int rating);
int rundb_init(void);
void rundb_shutdown(void);
#endif
