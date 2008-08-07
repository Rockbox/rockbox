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
#include <dirent.h>

#define VERSION "0.1"

static unsigned char* int2le(unsigned int val)
{
    static unsigned char addr[4];
    addr[0] = val & 0xff;
    addr[1] = (val >> 8) & 0xff;
    addr[2] = (val >> 16) & 0xff;
    addr[3] = (val >> 24) & 0xff;
    return addr;
}

static unsigned int le2int(unsigned char* buf)
{
   unsigned int res = (buf[3] << 24) | (buf[2] << 16) | (buf[1] << 8) | buf[0];

   return res;
}

unsigned int _filesize(FILE* fd)
{
    unsigned int tmp, oldpos;
    oldpos = ftell(fd);
    fseek(fd, 0, SEEK_END);
    tmp = ftell(fd);
    fseek(fd, oldpos, SEEK_SET);
    return tmp;
}

static void print_usage(void)
{
#ifdef _WIN32
    fprintf(stderr, "Usage: hxfreplace.exe [IN_FW] [OUT_FW] [BIN_FILE]\n\n");
    fprintf(stderr, "Example: hxfreplace.exe VX747.HXF out.hxf ccpmp.bin\n\n");
#else
    fprintf(stderr, "Usage: HXFreplace [IN_FW] [OUT_FW] [BIN_FILE]\n\n");
    fprintf(stderr, "Example: HXFreplace VX747.HXF out.hxf ccpmp.bin\n\n");
#endif
}

static int checksum(FILE *file)
{
    int oldpos = ftell(file);
    int ret=0, i, filesize = _filesize(file)-0x40;
    unsigned char *buf;
    
    buf = (unsigned char*)malloc(filesize);
    
    if(buf == NULL)
    {
        fseek(file, oldpos, SEEK_SET);
        fprintf(stderr, "[ERR] Error while allocating memory\n");
        return 0;
    }
    
    fseek(file, 0x40, SEEK_SET);
    if(fread(buf, filesize, 1, file) != 1)
    {
        free(buf);
        fseek(file, oldpos, SEEK_SET);
        fprintf(stderr, "[ERR] Error while reading from file\n");
        return 0;
    }
    
    fprintf(stderr, "[INFO] Computing checksum...");
    
    for(i = 0; i < filesize; i+=4)
        ret += le2int(&buf[i]);
    
    free(buf);
    fseek(file, oldpos, SEEK_SET);
    
    fprintf(stderr, " Done!\n");
    return ret;
}

int main(int argc, char *argv[])
{
    FILE *infile, *outfile, *fw;
    
    fprintf(stderr, "HXFreplace v" VERSION " - (C) 2008 Maurus Cuelenaere\n");
    fprintf(stderr, "This is free software; see the source for copying conditions.  There is NO\n");
    fprintf(stderr, "warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n\n");
    
    if(argc != 4)
    {
        print_usage();
        return 1;
    }

    if((infile = fopen(argv[1], "rb")) == NULL)
    {
        fprintf(stderr, "[ERR]  Cannot open %s\n", argv[1]);
        return 2;
    }
    
    if(fseek(infile, 0x40, SEEK_SET) != 0)
    {
        fprintf(stderr, "[ERR]  Cannot seek to 0x40\n");
        fclose(infile);
        return 3;
    }
    
    fprintf(stderr, "[INFO] Searching for ccpmp.bin...\n");
    
    int found = -1;
    int filenamesize;
    char *filename;
    unsigned char tmp[4];
    
#define READ(x, len) if(fread(x, len, 1, infile) != 1) \
                     { \
                        fprintf(stderr, "[ERR]  Cannot read from %s\n", argv[1]); \
                        fclose(infile); \
                        return 4; \
                     }
    while(found < 0)
    {
        READ(&tmp[0], 4);
        filenamesize = le2int(tmp);
        filename = (char*)malloc(filenamesize);
        READ(filename, filenamesize);
        if(strcmp(filename, "ccpmp.bin") == 0)
            found = ftell(infile);
        else
        {
            READ(&tmp[0], 4);
            fseek(infile, le2int(tmp), SEEK_CUR);
        }
        free(filename);
    }
    
    fprintf(stderr, "[INFO] Found ccpmp.bin at 0x%x\n", found);
    
    if((outfile = fopen(argv[2], "wb+")) == NULL)
    {
        fclose(infile);
        fprintf(stderr, "[ERR]  Cannot open %s\n", argv[2]);
        return 5;
    }
    
#define WRITE(x, len) if(fwrite(x, len, 1, outfile) != 1) \
                      { \
                            fprintf(stderr, "[ERR]  Cannot write to %s\n", argv[2]); \
                            fclose(outfile); \
                            if(fw != NULL) \
                                fclose(fw); \
                            return 5; \
                       }
                            
    unsigned char* buffer;
    
    buffer = (unsigned char*)malloc(found);
    fseek(infile, 0, SEEK_SET);
    READ(buffer, found);
    WRITE(buffer, found);
    free(buffer);
    
    if((fw = fopen(argv[3], "rb")) == NULL)
    {
        fclose(infile);
        fclose(outfile);
        fprintf(stderr, "[ERR]  Cannot open %s\n", argv[3]);
    }
    
    int fw_filesize = _filesize(fw);
    
#define READ2(x, len) if(fread(x, len, 1, fw) != 1) \
                      { \
                         fprintf(stderr, "[ERR]  Cannot read from %s\n", argv[3]); \
                         fclose(infile); \
                         fclose(outfile); \
                         return 6; \
                      }
    buffer = (unsigned char*)malloc(fw_filesize);
    READ2(buffer, fw_filesize);
    fputc(0x20, outfile); /* Padding */
    WRITE(int2le(fw_filesize), 4);
    WRITE(buffer, fw_filesize);
    free(buffer);
    fclose(fw);
    fw = NULL;
    
    fseek(infile, found+1, SEEK_SET);
    READ(&tmp, 4);
    if(fseek(infile, le2int(&tmp[0]), SEEK_CUR) != 0)
    {
        fprintf(stderr, "[INFO] Cannot seek into %s\n", argv[1]);
        fclose(infile);
        fclose(outfile);
        return 7;
    }
    found = ftell(infile);
    
    int other_size = _filesize(infile) - found;
    buffer = (unsigned char*)malloc(other_size);
    READ(buffer, other_size);
    WRITE(buffer, other_size);
    free(buffer);
    fclose(infile);
    
    fflush(outfile);
    fseek(outfile, 0x14, SEEK_SET);
    WRITE(int2le(_filesize(outfile)), 4);
    WRITE(int2le(checksum(outfile)), 4);
    fclose(outfile);
    
    fprintf(stderr, "[INFO] Done!\n");
    
    return 0;
}
