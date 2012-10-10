/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id:$
 *
 * Copyright (C) 2012 by Andrew Ryabinin
 *
 * major portion of code is taken from firmware/test/fat/ata-sim.c
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "debug.h"
#include "dir.h"

#define BLOCK_SIZE 512

static FILE* file;

extern char *img_filename;

void mutex_init(struct mutex* l) {}
void mutex_lock(struct mutex* l) {}
void mutex_unlock(struct mutex* l) {}

void panicf( char *fmt, ...) {
    va_list ap;
    va_start( ap, fmt );
    fprintf(stderr,"***PANIC*** ");
    vfprintf(stderr, fmt, ap );
    va_end( ap );
    exit(1);
}

void debugf(const char *fmt, ...) {
    va_list ap;
    va_start( ap, fmt );
    fprintf(stderr,"DEBUGF: ");
    vfprintf( stderr, fmt, ap );
    va_end( ap );
}

void ldebugf(const char* file, int line, const char *fmt, ...) {
    va_list ap;
    va_start( ap, fmt );
    fprintf( stderr, "%s:%d ", file, line );
    vfprintf( stderr, fmt, ap );
    va_end( ap );
}

int storage_read_sectors(unsigned long start, int count, void* buf)
{
    if ( count > 1 )
        DEBUGF("[Reading %d blocks: 0x%lx to 0x%lx]\n",
               count, start, start+count-1); 
    else
        DEBUGF("[Reading block 0x%lx]\n", start); 

    if(fseek(file,start*BLOCK_SIZE,SEEK_SET)) {
        perror("fseek");
        return -1;
    }
    if(!fread(buf,BLOCK_SIZE,count,file)) {
        DEBUGF("ata_write_sectors(0x%lx, 0x%x, %p)\n", start, count, buf );
        perror("fread");
        panicf("Disk error\n");
    }
    return 0;
}

int storage_write_sectors(unsigned long start, int count, void* buf)
{
    if ( count > 1 )
        DEBUGF("[Writing %d blocks: 0x%lx to 0x%lx]\n",
               count, start, start+count-1);
    else
        DEBUGF("[Writing block 0x%lx]\n", start);

    if (start == 0)
        panicf("Writing on sector 0!\n");

    if(fseek(file,start*BLOCK_SIZE,SEEK_SET)) {
        perror("fseek");
        return -1;
    }
    if(!fwrite(buf,BLOCK_SIZE,count,file)) {
        DEBUGF("ata_write_sectors(0x%lx, 0x%x, %p)\n", start, count, buf );
        perror("fwrite");
        panicf("Disk error\n");
    }
    return 0;
}

int ata_init(void)
{
    file=fopen(img_filename,"rb+");
    if(!file) {
        fprintf(stderr, "read_disk() - Could not find \"%s\"\n",img_filename);
        return -1;
    }
    return 0;
}

void ata_exit(void)
{
    fclose(file);
}
