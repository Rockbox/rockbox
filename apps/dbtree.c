/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2005 by Björn Stenberg
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
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
#include "dbtree.h"
#include "icons.h"

#ifdef LITTLE_ENDIAN
#include <netinet/in.h>
#define BE32(_x_) htonl(_x_)
#else
#define BE32(_x_) _x_
#endif

#define ID3DB_VERSION 1

static int fd;

static int
    songstart, albumstart, artiststart,
    songcount, albumcount, artistcount,
    songlen, songarraylen,
    albumlen, albumarraylen,
    artistlen, initialized = 0;

int db_init(void)
{
    unsigned int version;
    unsigned int buf[12];
    unsigned char* ptr = (char*)buf;
    
    fd = open(ROCKBOX_DIR "/rockbox.id3db", O_RDONLY);
    if (fd < 0) {
        DEBUGF("Failed opening database\n");
        return -1;
    }
    read(fd, buf, 48);

    if (ptr[0] != 'R' ||
        ptr[1] != 'D' ||
        ptr[2] != 'B')
    {
        DEBUGF("File is not a rockbox id3 database, aborting\n");
        return -1;
    }
    
    version = BE32(buf[0]) & 0xff;
    if (version != ID3DB_VERSION)
    {
        DEBUGF("Unsupported database version %d, aborting.\n");
        return -1;
    }
    DEBUGF("Version: RDB%d\n", version);

    songstart = BE32(buf[1]);
    songcount = BE32(buf[2]);
    songlen   = BE32(buf[3]);
    DEBUGF("Number of songs: %d\n", songcount);
    DEBUGF("Songstart: %x\n", songstart);
    DEBUGF("Songlen: %d\n", songlen);

    albumstart = BE32(buf[4]);
    albumcount = BE32(buf[5]);
    albumlen   = BE32(buf[6]);
    songarraylen = BE32(buf[7]);
    DEBUGF("Number of albums: %d\n", albumcount);
    DEBUGF("Albumstart: %x\n", albumstart);
    DEBUGF("Albumlen: %d\n", albumlen);

    artiststart = BE32(buf[8]);
    artistcount = BE32(buf[9]);
    artistlen   = BE32(buf[10]);
    albumarraylen = BE32(buf[11]);
    DEBUGF("Number of artists: %d\n", artistcount);
    DEBUGF("Artiststart: %x\n", artiststart);
    DEBUGF("Artistlen: %d\n", artistlen);

    if (songstart > albumstart ||
        albumstart > artiststart)
    {
        DEBUGF("Corrupt id3db database, aborting.\n");
        return -1;
    }

    initialized = 1;
    return 0;
}

int db_load(struct tree_context* c, bool* dir_buffer_full)
{
    int i, offset, len, rc;
    int dcachesize = global_settings.max_files_in_dir * sizeof(struct entry);
    int max_items, itemcount, stringlen;
    unsigned int* nptr = (void*) c->name_buffer;
    unsigned int* dptr = c->dircache;
    unsigned int* safeplace = NULL;

    int table = c->currtable;
    int extra = c->currextra;

    if (!initialized) {
        DEBUGF("ID3 database is not initialized.\n");
        c->filesindir = 0;
        return 0;
    }
    
    c->dentry_size = 2 * sizeof(int);
    
    DEBUGF("db_load(%d, %x)\n", table, extra);

    if (!table) {
        table = allartists;
        c->currtable = table;
    }
    
    switch (table) {
        case allsongs:
            offset = songstart;
            itemcount = songcount;
            stringlen = songlen;
            break;

        case allalbums:
            offset = albumstart;
            itemcount = albumcount;
            stringlen = albumlen;
            break;

        case allartists:
            offset = artiststart;
            itemcount = artistcount;
            stringlen = artistlen;
            break;

        case albums:
            /* 'extra' is offset to the artist */
            len = albumarraylen * 4;
            safeplace = (void*)(c->name_buffer + c->name_buffer_size - len);
            //DEBUGF("Seeking to %x\n", extra + artistlen);
            lseek(fd, extra + artistlen, SEEK_SET);
            rc = read(fd, safeplace, len);
            if (rc < len)
                return -1;

#ifdef LITTLE_ENDIAN
            for (i=0; i<albumarraylen; i++)
                safeplace[i] = BE32(safeplace[i]);
#endif

            offset = safeplace[0];
            itemcount = albumarraylen;
            stringlen = albumlen;
            break;

        case songs:
            /* 'extra' is offset to the album */
            len = songarraylen * 4;
            safeplace = (void*)(c->name_buffer + c->name_buffer_size - len);
            //DEBUGF("Seeking to %x\n", extra + albumlen + 4);
            lseek(fd, extra + albumlen + 4, SEEK_SET);
            rc = read(fd, safeplace, len);
            if (rc < len)
                return -1;

#ifdef LITTLE_ENDIAN
            for (i=0; i<songarraylen; i++)
                safeplace[i] = BE32(safeplace[i]);
#endif
            offset = safeplace[0];
            itemcount = songarraylen;
            stringlen = songlen;
            break;

        default:
            DEBUGF("Unsupported table %d\n", table);
            return -1;
    }
    max_items = dcachesize / c->dentry_size;

    if (!safeplace) {
        //DEBUGF("Seeking to %x\n", offset);
        lseek(fd, offset, SEEK_SET);
    }

    /* name_buffer (nptr) contains only names, null terminated.
       the first word of dcache (dptr) is a pointer to the name,
       the rest is table specific. see below. */

    if (itemcount > max_items)
        if (dir_buffer_full)
            *dir_buffer_full = true;
    
    if (max_items > itemcount) {
        max_items = itemcount;
    }

    for ( i=0; i < max_items; i++ ) {
        int rc, skip=0;

        if (safeplace) {
            if (!safeplace[i])
                break;
            //DEBUGF("Seeking to %x\n", safeplace[i]);
            lseek(fd, safeplace[i], SEEK_SET);
            offset = safeplace[i];
        }

        /* read name */
        rc = read(fd, nptr, stringlen);
        if (rc < stringlen)
        {
            DEBUGF("%d read(%d) returned %d\n", i, stringlen, rc);
            return -1;
        }

        /* store name pointer in dir cache */
        dptr[0] = (unsigned int)nptr;

        switch (table) {
            case songs:
            case allsongs:
                /* save offset of this song */
                skip = 12;
                dptr[1] = offset;
                break;

            case allalbums:
            case albums:
                /* save offset of this album */
                skip = songarraylen * 4 + 4;
                dptr[1] = offset;
                break;

            case allartists:
                /* save offset of this artist */
                skip = albumarraylen * 4;
                dptr[1] = offset;
                break;
        }

        //DEBUGF("%x: %s\n", dptr[1], dptr[0]);

        if (skip)
            lseek(fd, skip, SEEK_CUR);
        
        /* next name is stored immediately after this */
        nptr = (void*)nptr + strlen((char*)nptr) + 1;
        if ((void*)nptr > (void*)c->name_buffer + c->name_buffer_size) {
            DEBUGF("Name buffer overflow (%d)\n",i);
            break;
        }
        dptr = (void*)dptr + c->dentry_size;

        if (!safeplace)
            offset += stringlen + skip;
    }

    c->filesindir = i;

    return i;
}

void db_enter(struct tree_context* c)
{
    switch (c->currtable) {
        case allartists:
        case albums:
            c->dirpos[c->dirlevel] = c->dirstart;
            c->cursorpos[c->dirlevel] = c->dircursor;
            c->table_history[c->dirlevel] = c->currtable;
            c->extra_history[c->dirlevel] = c->currextra;
            c->dirlevel++;
            break;

        default:
            break;
    }

    switch (c->currtable) {
        case allartists:
            c->currtable = albums;
            c->currextra = ((int*)c->dircache)[(c->dircursor + c->dirstart)*2 + 1];
            break;

        case albums:
            c->currtable = songs;
            c->currextra = ((int*)c->dircache)[(c->dircursor + c->dirstart)*2 + 1];
            break;

        case songs:
            splash(HZ,true,"No playing implemented yet");
#if 0
            /* find filenames, build playlist, play */
            playlist_create(NULL,NULL);
#endif       
            break;
            
        default:
            break;
    }

    c->dirstart = c->dircursor = 0;
}

void db_exit(struct tree_context* c)
{
    c->dirlevel--;
    c->dirstart = c->dirpos[c->dirlevel];
    c->dircursor = c->cursorpos[c->dirlevel];
    c->currtable = c->table_history[c->dirlevel];
    c->currextra = c->extra_history[c->dirlevel];
}

#ifdef HAVE_LCD_BITMAP
const char* db_get_icon(struct tree_context* c)
{
    int icon;

    switch (c->currtable)
    {
        case allsongs:
        case songs:
            icon = File;
            break;

        default:
            icon = Folder;
            break;
    }

    return bitmap_icons_6x8[icon];
}
#else
int   db_get_icon(struct tree_context* c)
{
    (void)c;
    return Folder;
}
#endif
