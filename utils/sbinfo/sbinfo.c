/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2010 Bertrik Sikken
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

/*
 * .sb file parser and chunk extractor
 *
 * Based on amsinfo, which is
 * Copyright © 2008 Rafaël Carré <rafael.carre@gmail.com>
 */

#define _ISOC99_SOURCE /* snprintf() */
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#if 1 /* ANSI colors */

#	define color(a) printf("%s",a)
char OFF[] 		= { 0x1b, 0x5b, 0x31, 0x3b, '0', '0', 0x6d, '\0' };

char GREY[] 	= { 0x1b, 0x5b, 0x31, 0x3b, '3', '0', 0x6d, '\0' };
char RED[] 		= { 0x1b, 0x5b, 0x31, 0x3b, '3', '1', 0x6d, '\0' };
char GREEN[] 	= { 0x1b, 0x5b, 0x31, 0x3b, '3', '2', 0x6d, '\0' };
char YELLOW[] 	= { 0x1b, 0x5b, 0x31, 0x3b, '3', '3', 0x6d, '\0' };
char BLUE[] 	= { 0x1b, 0x5b, 0x31, 0x3b, '3', '4', 0x6d, '\0' };

#else
	/* disable colors */
#	define color(a)
#endif

#define bug(...) do { fprintf(stderr,"ERROR: "__VA_ARGS__); exit(1); } while(0)
#define bugp(a) do { perror("ERROR: "a); exit(1); } while(0)

/* byte swapping */
#define get32le(a) ((uint32_t) \
    ( buf[a+3] << 24 | buf[a+2] << 16 | buf[a+1] << 8 | buf[a] ))
#define get16le(a) ((uint16_t)( buf[a+1] << 8 | buf[a] ))

/* all blocks are sized as a multiple of 0x1ff */
#define PAD_TO_BOUNDARY(x) (((x) + 0x1ff) & ~0x1ff)

/* If you find a firmware that breaks the known format ^^ */
#define assert(a) do { if(!(a)) { fprintf(stderr,"Assertion \"%s\" failed in %s() line %d!\n\nPlease send us your firmware!\n",#a,__func__,__LINE__); exit(1); } } while(0)

/* globals */

size_t sz;	/* file size */
uint8_t *buf; /* file content */


/* 1st block description */
uint32_t idx,checksum,bs_multiplier,firmware_sz;
uint32_t unknown_4_1; uint16_t unknown_1, unknown_2;
uint32_t unknown_4_2,unknown_4_3;

static void *xmalloc(size_t s) /* malloc helper */
{
	void * r = malloc(s);
	if(!r) bugp("malloc");
	return r;
}

static char getchr(int offset)
{
    char c;
    c = buf[offset];
    return isprint(c) ? c : '_';
}

static void getstrle(char string[], int offset)
{
    int i;
    for (i = 0; i < 4; i++) {
        string[i] = getchr(offset + 3 - i);
    }
    string[4] = 0;
}

static void getstrbe(char string[], int offset)
{
    int i;
    for (i = 0; i < 4; i++) {
        string[i] = getchr(offset + i);
    }
    string[4] = 0;
}

static void printhex(int offset, int len)
{
    int i;
    
    for (i = 0; i < len; i++) {
        printf("%02X ", buf[offset + i]);
    }
    printf("\n");
}

/* verify the firmware header */
static void check(unsigned long filesize)
{
    /* check STMP marker */
    char stmp[5];
    getstrbe(stmp, 0x14);
    assert(strcmp(stmp, "STMP") == 0);
    color(GREEN);

    /* get total size */
    unsigned long totalsize = 16 * get32le(0x1C);
    color(GREEN);
    assert(filesize == totalsize);
}

static void extract(unsigned long filesize)
{
    /* Basic header info */
    color(BLUE);
    printf("Basic info:\n");
    color(GREEN);
    printf("\tHeader SHA-1: ");
    printhex(0, 20);
    printf("\tFlags: ");
    printhex(0x18, 4);
    printf("\tTotal file size : %ld\n", filesize);
    
    /* Sizes and offsets */
    color(BLUE);
    printf("Sizes and offsets:\n");
    color(GREEN);
    int num_enc = get16le(0x28);
    printf("\t# of encryption info = %d\n", num_enc);
    int num_chunks = get16le(0x2E);
    printf("\t# of chunk headers = %d\n", num_chunks);

    /* Versions */
    color(BLUE);
    printf("Versions\n");
    color(GREEN);

    printf("\tRandom 1: ");
    printhex(0x32, 6);
    printf("\tRandom 2: ");
    printhex(0x5A, 6);
    
    uint64_t micros_l = get32le(0x38);
    uint64_t micros_h = get32le(0x3c);
    uint64_t micros = ((uint64_t)micros_h << 32) | micros_l;
    time_t seconds = (micros / (uint64_t)1000000L);
    seconds += 946684800; /* 2000/1/1 0:00:00 */
    struct tm *time = gmtime(&seconds);
    color(GREEN);
    printf("\tCreation date/time = %s", asctime(time));

    int p_maj = get32le(0x40);
    int p_min = get32le(0x44);
    int p_sub = get32le(0x48);
    int c_maj = get32le(0x4C);
    int c_min = get32le(0x50);
    int c_sub = get32le(0x54);
    color(GREEN);
    printf("\tProduct version   = %X.%X.%X\n", p_maj, p_min, p_sub);
    printf("\tComponent version = %X.%X.%X\n", c_maj, c_min, c_sub);

    /* chunks */
    color(BLUE);
    printf("Chunks\n");

    int i;
    for (i = 0; i < num_chunks; i++) {
        uint32_t ofs = 0x60 + (i * 16);
    
        char name[5];
        getstrle(name, ofs + 0);
        int pos = 16 * get32le(ofs + 4);
        int size = 16 * get32le(ofs + 8);
        int flags = get32le(ofs + 12);
    
        color(GREEN);
        printf("\tChunk '%s'\n", name);
        printf("\t\tpos   = %8x - %8x\n", pos, pos+size);
        printf("\t\tlen   = %8x\n", size);
        printf("\t\tflags = %8x\n", flags);
        
        /* save it */
        char filename[16];
        strcpy(filename, name);
        strcat(filename, ".bin");
        FILE *file = fopen(filename, "wb");
        if (file != NULL) {
            fwrite(buf + pos, size, 1, file);
            fclose(file);
        }
    }
    
    /* final signature */
    color(BLUE);
    printf("Final signature:\n\t");
    color(GREEN);
    printhex(filesize - 32, 16);
    printf("\t");
    printhex(filesize - 16, 16);
}

int main(int argc, const char **argv)
{
	int fd;
	struct stat st;
	if(argc != 2)
		bug("Usage: %s <firmware>\n",*argv);

	if( (fd = open(argv[1],O_RDONLY)) == -1 )
		bugp("opening firmware failed");

	if(fstat(fd,&st) == -1)
		bugp("firmware stat() failed");
	sz = st.st_size;

	buf=xmalloc(sz);
	if(read(fd,buf,sz)!=(ssize_t)sz) /* load the whole file into memory */
		bugp("reading firmware");

	close(fd);

	check(st.st_size);	/* verify header and checksums */
	extract(st.st_size);	/* split in blocks */
	
	color(OFF);

	free(buf);
	return 0;
}
