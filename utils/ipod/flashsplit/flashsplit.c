/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 Dave Chapman
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
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

static void int2be(unsigned int val, unsigned char* addr)
{
    addr[0] = (val >> 24) & 0xff;
    addr[1] = (val >> 16) & 0xff;
    addr[2] = (val >> 8) & 0xff;
    addr[3] = val & 0xFF;
}

static int le2int(unsigned char* buf)
{
   int32_t res = (buf[3] << 24) | (buf[2] << 16) | (buf[1] << 8) | buf[0];

   return res;
}

static char* get_modelname(int version)
{

    switch (version >> 8) {
        case 0x01:
            return "1g2g";
            break;
        case 0x02:
            return "ip3g";
            break;
        case 0x40:
            return "mini";
            break;
        case 0x50:
            return "ip4g";
            break;
        case 0x60:
            return "ipco";
            break;
        case 0x70:
            return "mn2g";
            break;
        case 0xc0:
            return "nano";
            break;
        case 0xb0:
            return "ipvd";
            break;
    }

    return NULL;
}

static int get_modelnum(int version)
{

    switch (version >> 8) {
        case 0x01:
            return 19; // "1g2g";
            break;
        case 0x02:
            return 7; // "ip3g";
            break;
        case 0x40:
            return 9; // "mini";
            break;
        case 0x50:
            return 8; // "ip4g";
            break;
        case 0x60:
            return 3; // "ipco";
            break;
        case 0x70:
            return 11; // "mn2g";
            break;
        case 0xc0:
            return 4;  // "nano";
            break;
        case 0xb0:
            return 5;  // "ipvd";
            break;
    }

    return -1;
}


static void dump_app(char* name, unsigned char* p, unsigned char* flash)
{
    char filename[64];
    unsigned char header[8];
    unsigned int i;
    unsigned int sum;
    int outfile;

    unsigned int unknown1;
    unsigned int offset;
    unsigned int length;
    unsigned int loadaddr;
    unsigned int unknown2;
    unsigned int checksum;
    unsigned int version;
    unsigned int unknown3;
    char* modelname;

    /* Extract variables from header */
    unknown1 = le2int(p+0x08);
    offset   = le2int(p+0x0c);
    length   = le2int(p+0x10);
    loadaddr = le2int(p+0x14);
    unknown2 = le2int(p+0x18);
    checksum = le2int(p+0x1c);
    version  = le2int(p+0x20);
    unknown3 = le2int(p+0x24);

    modelname = get_modelname(version);
    sum = 0; 

    for (i = offset; i < offset+length ; i++) { 
        sum += flash[i];
    }

    /* Display information: */
    printf("Image: %s\n",name);
    printf("unknown1: %08x\n",unknown1);
    printf("offset:   %08x\n",offset);
    printf("length:   %08x\n",length);
    printf("loadaddr: %08x\n",loadaddr);
    printf("unknown2: %08x\n",unknown2);
    printf("checksum: %08x  (flashsplit sum: %08x)\n",checksum,sum);
    printf("version:  %08x  (ipod model: %s)\n",version,modelname);
    printf("unknown3: %08x\n",unknown3);
    printf("\n");

    if (modelname == NULL) { 
        printf("Unknown version, not exporting to .ipod file\n");
        return;
    } else if (checksum != sum) {
        printf("Checksum mismatch, not exporting to .ipod file\n");
    } else {
        sum += get_modelnum(version);
        int2be(sum,header);
        memcpy(header+4,modelname,4);

        sprintf(filename,"%s.ipod",name);
        outfile = open(filename,O_CREAT|O_TRUNC|O_WRONLY,0666);
        if (outfile < 0) {
            fprintf(stderr,"[ERR]  Couldn't open file %s\n",filename);
            return;
        }

        write(outfile,header,8);
        write(outfile,flash+offset,length);
        close(outfile);
    }

    sprintf(filename,"%s.bin",name);
    outfile = open(filename,O_CREAT|O_TRUNC|O_WRONLY,0666);
    if (outfile < 0) {
        fprintf(stderr,"[ERR]  Couldn't open file %s\n",filename);
        return;
    }

    write(outfile,flash+offset,length);
    close(outfile);
}

int main(int argc,char* argv[])
{
  int fd;
  unsigned char buf[1024*1024];
  unsigned char* p;

  if (argc != 2) {
      fprintf(stderr,"Usage: flashsplit flash.bin\n");
      return 1;
  }

  fd=open(argv[1],O_RDONLY);
  if (fd < 0) {
      fprintf(stderr,"Can not open %s\n",argv[1]);
      return 1;
  }

  read(fd,buf,sizeof(buf));
  close(fd);


  p = buf + 0xffe00; /* Start of flash directory */

  while (le2int(p) != 0) {
      if (memcmp(p,"hslfksid",8)==0) {
          dump_app("diskmode",p,buf);
      } else if (memcmp(p,"hslfgaid",8)==0) {
          dump_app("diagmode",p,buf);
      } else if (memcmp(p,"hslfogol",8)==0) {
          dump_app("logo",p,buf);
      } else if (memcmp(p,"hslfnacs",8)==0) {
          dump_app("diskscan",p,buf);
      } else if (memcmp(p,"hslfscmv",8)==0) {
          dump_app("vmcs",p,buf);
      } else {
          printf("Unknown image type - %c%c%c%c%c%c%c%c\n",p[3],p[2],p[1],p[0],p[7],p[6],p[5],p[4]);
      }
      p += 0x28;
  }

  return 0;
}
