/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2008 by Maurus Cuelenaere
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
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <stdbool.h>

#define VERSION "0.2"

struct header{
	char main_header[20];
	unsigned int size;
	unsigned int checksum;
	unsigned int unknown;
	char other_header[32];
};

static char* basepath(char* path)
{
    static char tmp[255];
    char *ptr, *ptr2, *ptr3;
    ptr = path;
    ptr2 = (char*)tmp;
#ifdef _WIN32
    ptr3 = strrchr(path, 0x5C);
#else
    ptr3 = strrchr(path, 0x2F);
#endif
    while((int)ptr < (int)ptr3)
    {
        *ptr2 = *ptr;
        ptr++;
        ptr2++;
    }
#ifdef _WIN32
    *ptr2 = 0x5C;
#else
    *ptr2 = 0x2F;
#endif
    ptr2++;
    *ptr2 = 0;
    return (char*)tmp;
}

#ifndef _WIN32
static void replace(char* str)
{
    char *ptr = str;
    while(*ptr != 0)
    {
        if(*ptr == 0x5C) /* \ */
            *ptr = 0x2F; /* / */
        ptr++;
    }
}
#endif

static unsigned int le2int(unsigned char* buf)
{
   unsigned int res = (buf[3] << 24) | (buf[2] << 16) | (buf[1] << 8) | buf[0];

   return res;
}

#ifdef _WIN32
 #define PATH_SEPARATOR '\\'
#else
 #define PATH_SEPARATOR '/'
#endif

static unsigned int __mkdir(const char *path)
{
    char opath[256];
    char *p;
    size_t len;

    strncpy(opath, path, sizeof(opath));
    len = strlen(opath);
    if(opath[len - 1] == PATH_SEPARATOR)
        opath[len - 1] = '\0';
    for(p = opath; *p; p++)
        if(*p == PATH_SEPARATOR)
        {
            *p = '\0';
            if(access(opath, F_OK))
#ifdef _WIN32
                mkdir(opath);
#else
                mkdir(opath, S_IRWXU);
#endif
            *p = PATH_SEPARATOR;
        }
    if(access(opath, F_OK))    
#ifdef _WIN32
        return mkdir(opath);
#else
        return mkdir(opath, S_IRWXU);
#endif
    else
        return -1;
}

#if 0
static bool dir_exists(const char *dir)
{
  struct stat buf;
  memset(&buf, 0, sizeof(struct stat));
  printf("start: %s\n", dir);
  char *dir_cpy = (char*)malloc(strlen(dir));
  strcpy(dir_cpy, dir);
  printf("%s\n", dir_cpy);
  int tmp = (int)dir_cpy;
  while(*dir_cpy != 0)
  {
    dir_cpy++;
    if(*dir_cpy == PATH_SEPARATOR && *(dir_cpy+1) == 0)
        *dir_cpy = 0;
  }
  printf("while_done\n");
  dir_cpy = (char*)tmp;
  printf("statting %s...\n", dir_cpy);
  tmp = stat(dir_cpy, &buf);
  printf("chk_dir(%s) = %d\n", dir_cpy, tmp);
  free(dir_cpy);
  return tmp == 0;
}
#endif

static bool file_exists(const char *file)
{
  struct stat buf;
  return stat(file, &buf) == 0;
}


static int split_hxf(const unsigned char* infile, unsigned int size, const char* outpath)
{
    FILE *outfile;
    char *filename;
    unsigned int filenamesize, filesize;
    while(size > 0)
    {
        filenamesize = le2int((unsigned char*)infile);
        infile += 4;
        size -= 4;
        if(size > 0)
        {
            filename = (char*)calloc(1, filenamesize+1+strlen(outpath));
            memcpy(filename, outpath, strlen(outpath));
            memcpy(&filename[strlen(outpath)], infile, filenamesize);
#ifndef _WIN32
            replace(filename);
#endif
            infile += filenamesize + 1; /* + padding */
            size -= filenamesize + 1;
            
            filesize = le2int((unsigned char*)infile);
            infile += 4;
            size -= 4;
#if 0
            if(!dir_exists(basepath(filename)))
#endif
            {
                printf("[INFO] %s\n", basepath(filename));
                if(__mkdir(basepath(filename)) != 0)
                {
#if 0
                    fprintf(stderr, "[ERR]  Error creating directory %s\n", basepath(filename));
                    return -3;
#endif
                }
            }
            
            if(!file_exists(filename))
            {
                printf("[INFO] %s: %d bytes\n", filename, filesize);
            	if((outfile = fopen(filename, "wb")) == NULL)
                {
                    fprintf(stderr, "[ERR]  Error opening file %s\n", filename);
                    return -1;
                }
                if(filesize>0)
                {
                    if(fwrite(infile, filesize, 1, outfile) != 1)
                    {
                        fclose(outfile);
                        fprintf(stderr, "[ERR]  Error writing to file %s\n", filename);
                        return -2;
                    }
                }
                fclose(outfile);
            }
            
            infile += filesize;
            size -= filesize;
        }
    }
    return 0;
}

static void print_usage(void)
{
#ifdef _WIN32
    fprintf(stderr, "Usage: hxfsplit.exe [FW] [OUTPUT_DIR]\n\n");
    fprintf(stderr, "Example: hxfsplit.exe VX747.HXF VX747_extracted\\\n\n");
#else
    fprintf(stderr, "Usage: HXFsplit [FW] [OUTPUT_DIR]\n\n");
    fprintf(stderr, "Example: HXFsplit VX747.HXF VX747_extracted/\n\n");
#endif
}

int main(int argc, char *argv[])
{
    FILE *infile;
    struct header hdr;
    unsigned char *inbuffer;
    
    fprintf(stderr, "HXFsplit v" VERSION " - (C) 2008 Maurus Cuelenaere\n");
    fprintf(stderr, "This is free software; see the source for copying conditions.  There is NO\n");
    fprintf(stderr, "warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n\n");
    
    if(argc != 3)
    {
        print_usage();
        return 1;
    }

#ifdef _WIN32
    if(strcmp((char*)(argv[2]+strlen(argv[2])-1), "\\") != 0)
    {
		fprintf(stderr, "[ERR]  Output path must end with a \\\n");
#else
    if(strcmp((char*)(argv[2]+strlen(argv[2])-1), "/") != 0)
    {
		fprintf(stderr, "[ERR]  Output path must end with a /\n");
#endif
		return 2;
    }

	if((infile = fopen(argv[1], "rb")) == NULL)
    {
		fprintf(stderr, "[ERR]  Cannot open %s\n", argv[1]);
		return 3;
	}
    
    if((inbuffer = (unsigned char*)malloc(sizeof(struct header))) == NULL)
    {
        fclose(infile);
        fprintf(stderr, "[ERR]  Error allocating %d bytes buffer\n", sizeof(struct header));
        return 4;
    }

	if(fread(inbuffer, sizeof(struct header), 1, infile) != 1)
    {
        fclose(infile);
		fprintf(stderr, "Cannot read header of %s\n", argv[1]);
		return 5;
	}
    
    memcpy(hdr.main_header, inbuffer, 20);
    hdr.size = le2int(&inbuffer[20]);
    hdr.checksum = le2int(&inbuffer[24]);
    hdr.unknown = le2int(&inbuffer[28]);
    memcpy(hdr.other_header, &inbuffer[32], 32);
    free(inbuffer);
    
    if(strcmp(hdr.other_header, "Chinachip PMP firmware V1.0") != 0)
    {
        fclose(infile);
		fprintf(stderr, "[ERR]  Header doesn't match\n");
		return 6;
    }
    
    if((inbuffer = (unsigned char*)malloc(hdr.size)) == NULL)
    {
        fclose(infile);
        fprintf(stderr, "[ERR]  Error allocating %d bytes buffer\n", hdr.size);
        return 7;
    }
    
    fseek(infile, sizeof(struct header), SEEK_SET);
    
	if(fread(inbuffer, hdr.size-sizeof(struct header), 1, infile) != 1)
    {
        fclose(infile);
        free(inbuffer);
		fprintf(stderr, "[ERR]  Cannot read file in buffer\n");
		return 8;
	}
    
    fclose(infile);
    
    split_hxf(inbuffer, hdr.size-sizeof(struct header), argv[2]);
    
    free(inbuffer);
    
    return 0;
}
