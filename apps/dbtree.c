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
#include "lang.h"

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

static int db_play_folder(struct tree_context* c);

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

int db_load(struct tree_context* c)
{
    int i, offset, rc;
    int dcachesize = global_settings.max_files_in_dir * sizeof(struct entry);
    int max_items, itemcount, stringlen;
    unsigned int* nptr = (void*) c->name_buffer;
    unsigned int* dptr = c->dircache;
    unsigned int* safeplace = NULL;
    int safeplacelen = 0;

    int table = c->currtable;
    int extra = c->currextra;

    char* end_of_nbuf = c->name_buffer + c->name_buffer_size;

    if (!initialized) {
        DEBUGF("ID3 database is not initialized.\n");
        c->filesindir = 0;
        return 0;
    }
    
    c->dentry_size = 2;
    c->dirfull = false;

    DEBUGF("db_load(%d, %x, %d)\n", table, extra, c->firstpos);

    if (!table) {
        table = root;
        c->currtable = table;
    }
    
    switch (table) {
        case root: {
            static const int tables[] = {allartists, allalbums, allsongs};
            char* nbuf = (char*)nptr;
            char* labels[] = { str(LANG_ID3DB_ARTISTS),
                               str(LANG_ID3DB_ALBUMS),
                               str(LANG_ID3DB_SONGS)};

            for (i=0; i < 3; i++) {
                strcpy(nbuf, labels[i]);
                dptr[0] = (unsigned int)nbuf;
                dptr[1] = tables[i];
                nbuf += strlen(nbuf) + 1;
                dptr += 2;
            }
            c->dirlength = c->filesindir = i;
            return i;
        }
            
        case allsongs:
            offset = songstart + c->firstpos * (songlen + 12);
            itemcount = songcount;
            stringlen = songlen;
            c->dentry_size = 3;
            break;

        case allalbums:
            offset = albumstart +
                c->firstpos * (albumlen + 4 + songarraylen * 4);
            itemcount = albumcount;
            stringlen = albumlen;
            break;

        case allartists:
            offset = artiststart +
                c->firstpos * (artistlen + albumarraylen * 4);
            itemcount = artistcount;
            stringlen = artistlen;
            break;

        case albums4artist:
            /* 'extra' is offset to the artist */
            safeplacelen = albumarraylen * 4;
            safeplace = (void*)(end_of_nbuf - safeplacelen);
            lseek(fd, extra + artistlen, SEEK_SET);
            rc = read(fd, safeplace, safeplacelen);
            if (rc < safeplacelen)
                return -1;

#ifdef LITTLE_ENDIAN
            for (i=0; i<albumarraylen; i++)
                safeplace[i] = BE32(safeplace[i]);
#endif
            offset = safeplace[0];
            itemcount = albumarraylen;
            stringlen = albumlen;
            break;

        case songs4album:
            /* 'extra' is offset to the album */
            safeplacelen = songarraylen * 4;
            safeplace = (void*)(end_of_nbuf - safeplacelen);
            lseek(fd, extra + albumlen + 4, SEEK_SET);
            rc = read(fd, safeplace, safeplacelen);
            if (rc < safeplacelen)
                return -1;

#ifdef LITTLE_ENDIAN
            for (i=0; i<songarraylen; i++)
                safeplace[i] = BE32(safeplace[i]);
#endif
            offset = safeplace[0];
            itemcount = songarraylen;
            stringlen = songlen;
            c->dentry_size = 3;
            break;

        default:
            DEBUGF("Unsupported table %d\n", table);
            return -1;
    }
    max_items = dcachesize / (c->dentry_size * sizeof(int));
    end_of_nbuf -= safeplacelen;

    c->dirlength = itemcount;
    itemcount -= c->firstpos;

    if (!safeplace) {
        //DEBUGF("Seeking to %x\n", offset);
        lseek(fd, offset, SEEK_SET);
    }

    /* name_buffer (nptr) contains only names, null terminated.
       the first word of dcache (dptr) is a pointer to the name,
       the rest is table specific. see below. */

    if (itemcount > max_items)
        c->dirfull = true;
    
    if (max_items > itemcount) {
        max_items = itemcount;
    }

    for ( i=0; i < max_items; i++ ) {
        int rc, skip=0;
        int intbuf[4];

        if (safeplace) {
            if (!safeplace[i]) {
                c->dirlength = i;
                break;
            }
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
            case allsongs:
            case songs4album:
                /* save offset of this song */
                dptr[1] = offset;

                rc = read(fd, intbuf, 12);
                if (rc < 12) {
                    DEBUGF("%d read(%d) returned %d\n", i, 12, rc);
                    return -1;
                }
                /* save offset of filename */
                dptr[2] = BE32(intbuf[2]);
                break;

            case allalbums:
            case albums4artist:
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

        if (skip)
            lseek(fd, skip, SEEK_CUR);
        
        /* next name is stored immediately after this */
        nptr = (void*)nptr + strlen((char*)nptr) + 1;
        if ((void*)nptr + stringlen > (void*)end_of_nbuf) {
            c->dirfull = true;
            break;
        }
        dptr = (void*)dptr + c->dentry_size * sizeof(int);

        if (!safeplace)
            offset += stringlen + skip;
    }

    c->filesindir = i;

    return i;
}

int db_enter(struct tree_context* c)
{
    int rc = 0;
    int newextra = ((int*)c->dircache)[(c->dircursor + c->dirstart)*2 + 1];

    c->dirpos[c->dirlevel] = c->dirstart;
    c->cursorpos[c->dirlevel] = c->dircursor;
    c->table_history[c->dirlevel] = c->currtable;
    c->extra_history[c->dirlevel] = c->currextra;
    c->pos_history[c->dirlevel] = c->firstpos;
    c->dirlevel++;
    
    switch (c->currtable) {
        case root:
            c->currtable = newextra;
            c->currextra = newextra;
            break;
            
        case allartists:
            c->currtable = albums4artist;
            c->currextra = newextra;
            break;

        case allalbums:
        case albums4artist:
            c->currtable = songs4album;
            c->currextra = newextra;
            break;

        case allsongs:
        case songs4album:
            c->dirlevel--;
            if (db_play_folder(c) >= 0)
                rc = 3;
            break;
            
        default:
            break;
    }

    c->dirstart = c->dircursor = c->firstpos = 0;

    return rc;
}

void db_exit(struct tree_context* c)
{
    c->dirlevel--;
    c->dirstart = c->dirpos[c->dirlevel];
    c->dircursor = c->cursorpos[c->dirlevel];
    c->currtable = c->table_history[c->dirlevel];
    c->currextra = c->extra_history[c->dirlevel];
    c->firstpos  = c->pos_history[c->dirlevel];
}

static int db_play_folder(struct tree_context* c)
{
    char buf[MAX_PATH];
    int rc, i;
    int filenum = c->dircursor + c->dirstart;

    if (playlist_create(NULL, NULL) < 0) {
        DEBUGF("Failed creating playlist\n");
        return -1;
    }

    /* TODO: add support for very long tables */
    
    for (i=0; i < c->filesindir; i++) {
        int pathoffset = ((int*)c->dircache)[i * c->dentry_size + 2];
        lseek(fd, pathoffset, SEEK_SET);
        rc = read(fd, buf, sizeof(buf));
        if (rc < songlen) {
            DEBUGF("short path read(%d) = %d\n", sizeof(buf), rc);
            return -2;
        }

        playlist_insert_track(NULL, buf, PLAYLIST_INSERT, false);
    }

    if (global_settings.playlist_shuffle)
        filenum = playlist_shuffle(current_tick, filenum);
    if (!global_settings.play_selected)
        filenum = 0;

    playlist_start(filenum,0);

    return 0;
}

#ifdef HAVE_LCD_BITMAP
const char* db_get_icon(struct tree_context* c)
{
    int icon;

    switch (c->currtable)
    {
        case allsongs:
        case songs4album:
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
    int icon;
    switch (c->currtable)
    {
        case allsongs:
        case songs4album:
            icon = File;
            break;

        default:
            icon = Folder;
            break;
    }
    return icon;
}
#endif
