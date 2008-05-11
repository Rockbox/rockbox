/*

amsinfo - a tool for examining AMS firmware files

Copyright (C) Dave Chapman 2007

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110, USA

*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>


/* Win32 compatibility */
#ifndef O_BINARY
#define O_BINARY 0
#endif


#define PAD_TO_BOUNDARY(x) ((x) + 0x1ff) & ~0x1ff;


static off_t filesize(int fd) {
    struct stat buf;

    if (fstat(fd,&buf) < 0) {
        perror("[ERR]  Checking filesize of input file");
        return -1;
    } else {
        return(buf.st_size);
    }
}

static uint32_t get_uint32le(unsigned char* p)
{
    return p[0] | (p[1] << 8) | (p[2] << 16) | (p[3] << 24);
}

static uint16_t get_uint16le(unsigned char* p)
{
    return p[0] | (p[1] << 8);
}

static int calc_checksum(unsigned char* buf, int n)
{
    int sum = 0;
    int i;

    for (i=0;i<n;i+=4)
        sum += get_uint32le(buf + 0x400 + i);

    return sum;
}


static void dump_header(unsigned char* buf, int i)
{
    printf("0x%08x:\n",i);
    printf("  HEADER: 0x%08x\n",i);;
    printf("    FirmwareHeaderIndex:     0x%08x\n",get_uint32le(&buf[i]));
    printf("    FirmwareChecksum:        0x%08x\n",get_uint32le(&buf[i+0x04]));
    printf("    CodeBlockSizeMultiplier: 0x%08x\n",get_uint32le(&buf[i+0x08]));
    printf("    FirmwareSize:            0x%08x\n",get_uint32le(&buf[i+0x0c]));
    printf("    Unknown1:                0x%08x\n",get_uint32le(&buf[i+0x10]));
    printf("    ModelID:                 0x%04x\n",get_uint16le(&buf[i+0x14]));
    printf("    Unknown2:                0x%04x\n",get_uint16le(&buf[i+0x16]));
}

static int dump_lib(unsigned char* buf, int i)
{
    int export_count;
    int size;
    int unknown1;
    int baseaddr, endaddr;

    baseaddr = get_uint32le(&buf[i+0x04]);
    endaddr = get_uint32le(&buf[i+0x08]);
    size = get_uint32le(&buf[i+0x0c]);
    unknown1 = get_uint32le(&buf[i+0x10]);
    export_count = get_uint32le(&buf[i+0x14]);

    printf("0x%08x: \"%s\"  0x%08x  0x%08x  0x%08x  0x%08x  0x%08x\n",i, buf + i + get_uint32le(&buf[i]),baseaddr,endaddr,size,unknown1,export_count);

#if 0
    if (export_count > 1) { 
      for (j=0;j<export_count;j++) {
        printf("    Exports[%02d]:   0x%08x\n",j,get_uint32le(&buf[i+0x18+4*j]));
      }
    }
#endif
    return PAD_TO_BOUNDARY(size);
}

int main(int argc, char* argv[])
{
    int fd;
    off_t len;
    int n;
    unsigned char* buf;
    int firmware_size;
    int i;

    if (argc != 2) {
         fprintf(stderr,"USAGE: amsinfo firmware.bin\n");
         return 1;
    }

    fd = open(argv[1],O_RDONLY|O_BINARY);

    if ((len = filesize(fd)) < 0)
        return 1;

    if ((buf = malloc(len)) == NULL) {
        fprintf(stderr,"[ERR]  Could not allocate buffer for input file (%d bytes)\n",(int)len);
        return 1;
    }

    n = read(fd, buf, len);

    if (n != len) {
        fprintf(stderr,"[ERR] Could not read file\n");
        return 1;
    }

    close(fd);

    /* Now we dump the firmware structure */

    dump_header(buf,0);      /* First copy of header block */
//    dump_header(buf,0x200);  /* Second copy of header block */

    firmware_size = get_uint32le(&buf[0x0c]);

    printf("Calculated firmware checksum: 0x%08x\n",calc_checksum(buf,firmware_size));

    /* Round size up to next multiple of 0x200 */

    firmware_size = PAD_TO_BOUNDARY(firmware_size);

    i = firmware_size + 0x400;

    printf("LIBRARY BLOCKS:\n");
    printf("Offset      Name           BaseAddr    EndAddr     BlockSize   Unknown1    EntryCount\n");

    while (get_uint32le(&buf[i]) != 0xffffffff)
    {
        i += dump_lib(buf,i);

        while (get_uint32le(&buf[i]) == 0xefbeadde)
           i+=4;
    }

    printf("0x%08x: PADDING BLOCK\n",i);
 
    return 0;

}
