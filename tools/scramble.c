/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 - 2007 by Björn Stenberg
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "iriver.h"
#include "gigabeat.h"
#include "gigabeats.h"
#include "mi4.h"
#include "telechips.h"
#include "creative.h"
#include "iaudio_bl_flash.h"

int iaudio_encode(char *iname, char *oname, char *idstring);
int ipod_encode(char *iname, char *oname, int fw_ver, bool fake_rsrc);

enum
{
    ARCHOS_PLAYER, /* and V1 recorder */
    ARCHOS_V2RECORDER,
    ARCHOS_FMRECORDER,
    ARCHOS_ONDIO_SP,
    ARCHOS_ONDIO_FM
};

static unsigned int size_limit[] =
{
    0x32000, /* ARCHOS_PLAYER */
    0x64000, /* ARCHOS_V2RECORDER */
    0x64000, /* ARCHOS_FMRECORDER */
    0x64000, /* ARCHOS_ONDIO_SP */
    0x64000  /* ARCHOS_ONDIO_FM */
};

void short2le(unsigned short val, unsigned char* addr)
{
    addr[0] = val & 0xFF;
    addr[1] = (val >> 8) & 0xff;
}

unsigned int le2int(unsigned char* buf)
{
   unsigned int res = (buf[3] << 24) | (buf[2] << 16) | (buf[1] << 8) | buf[0];

   return res;
}

void int2le(unsigned int val, unsigned char* addr)
{
    addr[0] = val & 0xFF;
    addr[1] = (val >> 8) & 0xff;
    addr[2] = (val >> 16) & 0xff;
    addr[3] = (val >> 24) & 0xff;
}

void int2be(unsigned int val, unsigned char* addr)
{
    addr[0] = (val >> 24) & 0xff;
    addr[1] = (val >> 16) & 0xff;
    addr[2] = (val >> 8) & 0xff;
    addr[3] = val & 0xFF;
}

void short2be(unsigned short val, unsigned char* addr)
{
    addr[0] = (val >> 8) & 0xff;
    addr[1] = val & 0xFF;
}

void usage(void)
{
    printf("usage: scramble [options] <input file> <output file> [xor string]\n");
    printf("options:\n"
           "\t-fm     Archos FM recorder format\n"
           "\t-v2     Archos V2 recorder format\n"
           "\t-ofm    Archos Ondio FM recorder format\n"
           "\t-osp    Archos Ondio SP format\n"
           "\t-neo    SSI Neo format\n"
           "\t-mm=X   Archos Multimedia format (X values: A=JBMM, B=AV1xx, C=AV3xx)\n"
           "\t-iriver iRiver format\n"
           "\t-iaudiox5 iAudio X5 format\n"
           "\t-iaudiox5v iAudio X5V format\n"
           "\t-iaudiom5 iAudio M5 format\n"
           "\t-iaudiom3 iAudio M3 format\n");
    printf("\t-ipod3g ipod firmware partition format (3rd Gen)\n"
           "\t-ipod4g ipod firmware partition format (4th Gen, Mini, Nano, Photo/Color)\n"
           "\t-ipod5g ipod firmware partition format (5th Gen - aka Video)\n"
           "\t-creative=X Creative firmware structure format\n"
           "\t            (X values: zvm, zvm60, zenvision\n"
           "\t                       zenv, zen\n");
    printf("\t-gigabeat Toshiba Gigabeat F/X format\n"
           "\t-gigabeats Toshiba Gigabeat S format\n"
           "\t-mi4v2  PortalPlayer .mi4 format (revision 010201)\n"
           "\t-mi4v3  PortalPlayer .mi4 format (revision 010301)\n"
           "\t-mi4r   Sandisk Rhapsody .mi4 format\n"
           "\t        All mi4 options take two optional arguments:\n");
    printf("\t        -model=XXXX   where XXXX is the model id string\n"
           "\t        -type=XXXX    where XXXX is a string indicating the \n"
           "\t                      type of binary, eg. RBOS, RBBL\n"
           "\t-tcc=X  Telechips generic firmware format (X values: sum, crc)\n"
           "\t-add=X  Rockbox generic \"add-up\" checksum format\n"
           "\t        (X values: h100, h120, h140, h300, ipco, nano, ipvd, mn2g\n"
           "\t                   ip3g, ip4g, mini, iax5, iam5, iam3, h10, h10_5gb,\n"
           "\t                   tpj2, c200, e200, giga, gigs, m100, m500, d2)\n");
    printf("\nNo option results in Archos standard player/recorder format.\n");

    exit(1);
}

int main (int argc, char** argv)
{
    unsigned long length,i,slen=0;
    unsigned char *inbuf,*outbuf;
    unsigned short crc=0;
    unsigned long chksum=0; /* 32 bit checksum */
    unsigned char header[24];
    char *iname = argv[1];
    char *oname = argv[2];
    char *xorstring=NULL;
    int headerlen = 6;
    FILE* file;
    int version=0;
    unsigned long modelnum;
    char modelname[5];
    int model_id;
    enum { none, scramble, xor, tcc_sum, tcc_crc, add } method = scramble;

    model_id = ARCHOS_PLAYER;
    
    if (argc < 3) {
        usage();
    }

    if(!strcmp(argv[1], "-fm")) {
        headerlen = 24;
        iname = argv[2];
        oname = argv[3];
        version = 4;
        model_id = ARCHOS_FMRECORDER;
    }
    
    else if(!strcmp(argv[1], "-v2")) {
        headerlen = 24;
        iname = argv[2];
        oname = argv[3];
        version = 2;
        model_id = ARCHOS_V2RECORDER;
    }

    else if(!strcmp(argv[1], "-ofm")) {
        headerlen = 24;
        iname = argv[2];
        oname = argv[3];
        version = 8;
        model_id = ARCHOS_ONDIO_FM;
    }

    else if(!strcmp(argv[1], "-osp")) {
        headerlen = 24;
        iname = argv[2];
        oname = argv[3];
        version = 16;
        model_id = ARCHOS_ONDIO_SP;
    }

    else if(!strcmp(argv[1], "-neo")) {
        headerlen = 17;
        iname = argv[2];
        oname = argv[3];
        method = none;
    }
    else if(!strncmp(argv[1], "-mm=", 4)) {
        headerlen = 16;
        iname = argv[2];
        oname = argv[3];
        method = xor;
        version = argv[1][4];
        if (argc > 4)
            xorstring = argv[4];
        else {
            printf("Multimedia needs an xor string\n");
            return -1;
        }
    }
    else if(!strncmp(argv[1], "-tcc=", 4)) {
        headerlen = 0;
        iname = argv[2];
        oname = argv[3];

        if(!strcmp(&argv[1][5], "sum"))
            method = tcc_sum;
        else if(!strcmp(&argv[1][5], "crc"))
            method = tcc_crc;
        else {
            fprintf(stderr, "unsupported TCC method: %s\n", &argv[1][5]);
            return 2;
        }
    }
    else if(!strncmp(argv[1], "-add=", 5)) {
        iname = argv[2];
        oname = argv[3];
        method = add;

        if(!strcmp(&argv[1][5], "h120"))
            modelnum = 0;
        else if(!strcmp(&argv[1][5], "h140"))
            modelnum = 0; /* the same as the h120 */
        else if(!strcmp(&argv[1][5], "h100"))
            modelnum = 1;
        else if(!strcmp(&argv[1][5], "h300"))
            modelnum = 2;
        else if(!strcmp(&argv[1][5], "ipco"))
            modelnum = 3;
        else if(!strcmp(&argv[1][5], "nano"))
            modelnum = 4;
        else if(!strcmp(&argv[1][5], "ipvd"))
            modelnum = 5;
        else if(!strcmp(&argv[1][5], "fp7x"))
            modelnum = 6;
        else if(!strcmp(&argv[1][5], "ip3g"))
            modelnum = 7;
        else if(!strcmp(&argv[1][5], "ip4g"))
            modelnum = 8;
        else if(!strcmp(&argv[1][5], "mini"))
            modelnum = 9;
        else if(!strcmp(&argv[1][5], "iax5"))
            modelnum = 10;
        else if(!strcmp(&argv[1][5], "mn2g"))
            modelnum = 11;
        else if(!strcmp(&argv[1][5], "h10"))
            modelnum = 13;
        else if(!strcmp(&argv[1][5], "h10_5gb"))
            modelnum = 14;
        else if(!strcmp(&argv[1][5], "tpj2"))
            modelnum = 15;
        else if(!strcmp(&argv[1][5], "e200"))
            modelnum = 16;
        else if(!strcmp(&argv[1][5], "iam5"))
            modelnum = 17;
        else if(!strcmp(&argv[1][5], "giga"))
            modelnum = 18;
        else if(!strcmp(&argv[1][5], "1g2g"))
            modelnum = 19;
        else if(!strcmp(&argv[1][5], "c200"))
            modelnum = 20;
        else if(!strcmp(&argv[1][5], "gigs"))
            modelnum = 21;
        else if(!strcmp(&argv[1][5], "m500"))
            modelnum = 22;
        else if(!strcmp(&argv[1][5], "m100"))
            modelnum = 23;
        else if(!strcmp(&argv[1][5], "d2"))
            modelnum = 24;
        else if(!strcmp(&argv[1][5], "iam3"))
            modelnum = 25;
        else {
            fprintf(stderr, "unsupported model: %s\n", &argv[1][5]);
            return 2;
        }
        /* we store a 4-letter model name too, for humans */
        strcpy(modelname, &argv[1][5]);
        chksum = modelnum; /* start checksum calcs with this */
    }

    else if(!strcmp(argv[1], "-iriver")) {
        /* iRiver code dealt with in the iriver.c code */
        iname = argv[2];
        oname = argv[3];
        iriver_encode(iname, oname, FALSE);
        return 0;
    }
    else if(!strcmp(argv[1], "-gigabeat")) {
        /* iRiver code dealt with in the iriver.c code */
        iname = argv[2];
        oname = argv[3];
        gigabeat_code(iname, oname);
        return 0;
    }
    else if(!strcmp(argv[1], "-gigabeats")) {
        iname = argv[2];
        oname = argv[3];
        gigabeat_s_code(iname, oname);
        return 0;
    }
    else if(!strcmp(argv[1], "-iaudiox5")) {
        iname = argv[2];
        oname = argv[3];
        return iaudio_encode(iname, oname, "COWON_X5_FW");
    }
    else if(!strcmp(argv[1], "-iaudiox5v")) {
        iname = argv[2];
        oname = argv[3];
        return iaudio_encode(iname, oname, "COWON_X5V_FW");
    }
    else if(!strcmp(argv[1], "-iaudiom5")) {
        iname = argv[2];
        oname = argv[3];
        return iaudio_encode(iname, oname, "COWON_M5_FW");
    }
    else if(!strcmp(argv[1], "-iaudiom3")) {
        iname = argv[2];
        oname = argv[3];
        return iaudio_encode(iname, oname, "COWON_M3_FW");
    }
    else if(!strcmp(argv[1], "-ipod3g")) {
        iname = argv[2];
        oname = argv[3];
        return ipod_encode(iname, oname, 2, false); /* Firmware image v2 */
    }
    else if(!strcmp(argv[1], "-ipod4g")) {
        iname = argv[2];
        oname = argv[3];
        return ipod_encode(iname, oname, 3, false); /* Firmware image v3 */
    }
    else if(!strcmp(argv[1], "-ipod5g")) {
        iname = argv[2];
        oname = argv[3];
        return ipod_encode(iname, oname, 3, true);  /* Firmware image v3 */
    }
    else if(!strncmp(argv[1], "-creative=", 10)) {
        iname = argv[2];
        oname = argv[3];
        if(!strcmp(&argv[1][10], "zvm"))
            return zvm_encode(iname, oname, ZENVISIONM);
        else if(!strcmp(&argv[1][10], "zvm60"))
            return zvm_encode(iname, oname, ZENVISIONM60);
        else if(!strcmp(&argv[1][10], "zenvision"))
            return zvm_encode(iname, oname, ZENVISION);
        else if(!strcmp(&argv[1][10], "zenv"))
            return zvm_encode(iname, oname, ZENV);
        else if(!strcmp(&argv[1][10], "zen"))
            return zvm_encode(iname, oname, ZEN);
        else {
            fprintf(stderr, "unsupported Creative device: %s\n", &argv[1][10]);
            return 2;
        }
    }
    else if(!strncmp(argv[1], "-mi4", 4)) {
        int mi4magic;
        char model[4] = "";
        char type[4] = "";
        
        if(!strcmp(&argv[1][4], "v2")) {
            mi4magic = MI4_MAGIC_DEFAULT;
            version = 0x00010201;
        }
        else if(!strcmp(&argv[1][4], "v3")) {
            mi4magic = MI4_MAGIC_DEFAULT;
            version = 0x00010301;
        }
        else if(!strcmp(&argv[1][4], "r")) {
            mi4magic = MI4_MAGIC_R;
            version = 0x00010301;
        }
        else {
            printf( "Invalid mi4 version: %s\n", &argv[1][4]);
            return -1;
        }

        iname = argv[2];
        oname = argv[3];
        
        if(!strncmp(argv[2], "-model=", 7)) {
            iname = argv[3];
            oname = argv[4];
            strncpy(model, &argv[2][7], 4);
            
            if(!strncmp(argv[3], "-type=", 6)) {
                iname = argv[4];
                oname = argv[5];
                strncpy(type, &argv[3][6], 4);
            }
        }

        return mi4_encode(iname, oname, version, mi4magic, model, type);
    }
    
    /* open file */
    file = fopen(iname,"rb");
    if (!file) {
       perror(iname);
       return -1;
    }
    fseek(file,0,SEEK_END);
    length = ftell(file);
    length = (length + 3) & ~3; /* Round up to nearest 4 byte boundary */
    
    if ((method == scramble) &&
        ((length + headerlen) >= size_limit[model_id])) {
        printf("error: firmware image is %ld bytes while max size is %u!\n",
               length + headerlen,
               size_limit[model_id]);
        fclose(file);
        return -1;
    }

    fseek(file,0,SEEK_SET); 
    inbuf = malloc(length);
    if (method == xor)
        outbuf = malloc(length*2);
    else if(method == add)
        outbuf = malloc(length + 8);
    else
        outbuf = malloc(length);
    if ( !inbuf || !outbuf ) {
       printf("out of memory!\n");
       return -1;
    }
    if(length> 4) {
        /* zero-fill the last 4 bytes to make sure there's no rubbish there
           when we write the size-aligned file later */
        memset(outbuf+length-4, 0, 4);
    }

    /* read file */
    i=fread(inbuf,1,length,file);
    if ( !i ) {
       perror(iname);
       return -1;
    }
    fclose(file);

    switch (method)
    {
        case add:
            for (i = 0; i < length; i++) {
                /* add 8 unsigned bits but keep a 32 bit sum */
                chksum += inbuf[i];
            }
            break;
        case scramble:
            slen = length/4;
            for (i = 0; i < length; i++) {
                unsigned long addr = (i >> 2) + ((i % 4) * slen);
                unsigned char data = inbuf[i];
                data = ~((data << 1) | ((data >> 7) & 1)); /* poor man's ROL */
                outbuf[addr] = data;
            }
            break;

        case xor:
            /* "compress" */
            slen = 0;
            for (i=0; i<length; i++) {
                if (!(i&7))
                    outbuf[slen++] = 0xff; /* all data is uncompressed */
                outbuf[slen++] = inbuf[i];
            }
            break;
        case none:
        default:
            /* dummy case just to silence picky compilers */
            break;
    }

    if((method == none) || (method == scramble) || (method == xor)) {
        /* calculate checksum */
        for (i=0;i<length;i++)
            crc += inbuf[i];
    }

    memset(header, 0, sizeof header);
    switch (method)
    {
        case add:
        {
            int2be(chksum, header); /* checksum, big-endian */
            memcpy(&header[4], modelname, 4); /* 4 bytes model name */
            memcpy(outbuf, inbuf, length); /* the input buffer to output*/
            headerlen = 8;
        }
        break;

        case tcc_sum:
            memcpy(outbuf, inbuf, length); /* the input buffer to output*/
            telechips_encode_sum(outbuf, length);
            break;

        case tcc_crc:
            memcpy(outbuf, inbuf, length); /* the input buffer to output*/
            telechips_encode_crc(outbuf, length);
            break;

        case scramble:
            if (headerlen == 6) {
                int2be(length, header);
                header[4] = (crc >> 8) & 0xff;
                header[5] = crc & 0xff;
            }
            else {
                header[0] =
                    header[1] =
                    header[2] =
                    header[3] = 0xff; /* ??? */
                
                header[6] = (crc >> 8) & 0xff;
                header[7] = crc & 0xff;
                
                header[11] = version;
                
                header[15] = headerlen; /* really? */
                
                int2be(length, &header[20]);
            }
            break;

        case xor:
        {
            int xorlen = strlen(xorstring);

            /* xor data */
            for (i=0; i<slen; i++)
                outbuf[i] ^= xorstring[i & (xorlen-1)];
            
            /* calculate checksum */
            for (i=0; i<slen; i++)
                crc += outbuf[i];

            header[0] = header[2] = 'Z';
            header[1] = header[3] = version;
            int2le(length, &header[4]);
            int2le(slen, &header[8]);
            int2le(crc, &header[12]);
            length = slen;
            break;
        }

#define MY_FIRMWARE_TYPE "Rockbox"
#define MY_HEADER_VERSION 1
        default:
            strncpy((char *)header, MY_FIRMWARE_TYPE,9);
            header[9]='\0'; /*shouldn't have to, but to be SURE */
            header[10]=MY_HEADER_VERSION&0xFF;
            header[11]=(crc>>8)&0xFF;
            header[12]=crc&0xFF;
            int2be(sizeof(header), &header[12]);
            break;
    }

    /* write file */
    file = fopen(oname,"wb");
    if ( !file ) {
       perror(oname);
       return -1;
    }
    if (headerlen > 0) {
       if ( !fwrite(header,headerlen,1,file) ) {
          perror(oname);
          return -1;
       }
    }
    if ( !fwrite(outbuf,length,1,file) ) {
       perror(oname);
       return -1;
    }
    fclose(file);
    
    free(inbuf);
    free(outbuf);
    
    return 0;
}

int iaudio_encode(char *iname, char *oname, char *idstring)
{
    size_t len;
    int length;
    FILE *file;
    unsigned char *outbuf;
    int i;
    unsigned char sum = 0;
    
    file = fopen(iname, "rb");
    if (!file) {
       perror(iname);
       return -1;
    }
    fseek(file,0,SEEK_END);
    length = ftell(file);
    
    fseek(file,0,SEEK_SET); 
    outbuf = malloc(length+0x1030);

    if ( !outbuf ) {
       printf("out of memory!\n");
       return -1;
    }

    len = fread(outbuf+0x1030, 1, length, file);
    if(len < (size_t) length) {
        perror(iname);
        return -2;
    }
    
    memset(outbuf, 0, 0x1030);
    strcpy((char *)outbuf, idstring);
    memcpy(outbuf+0x20, iaudio_bl_flash, 
           BMPWIDTH_iaudio_bl_flash * (BMPHEIGHT_iaudio_bl_flash/8) * 2);
    short2be(BMPWIDTH_iaudio_bl_flash, &outbuf[0x10]);
    short2be((BMPHEIGHT_iaudio_bl_flash/8), &outbuf[0x12]);
    outbuf[0x19] = 2;

    for(i = 0; i < length;i++)
        sum += outbuf[0x1030 + i];

    int2be(length, &outbuf[0x1024]);
    outbuf[0x102b] = sum;

    fclose(file);

    file = fopen(oname, "wb");
    if (!file) {
       perror(oname);
       return -3;
    }
    
    len = fwrite(outbuf, 1, length+0x1030, file);
    if(len < (size_t)length) {
        perror(oname);
        return -4;
    }

    fclose(file);
    return 0;
}


/* Create an ipod firmware partition image 

   fw_ver = 2 for 3rd Gen ipods, 3 for all later ipods including 5g.

   This function doesn't yet handle the Broadcom resource image for the 5g,
   so the resulting images won't be usable.

   This has also only been tested on an ipod Photo 
*/

int ipod_encode(char *iname, char *oname, int fw_ver, bool fake_rsrc)
{
    static const char *apple_stop_sign = "{{~~  /-----\\   "\
                                         "{{~~ /       \\  "\
                                         "{{~~|         | "\
                                         "{{~~| S T O P | "\
                                         "{{~~|         | "\
                                         "{{~~ \\       /  "\
                                         "{{~~  \\-----/   "\
                                         "Copyright(C) 200"\
                                         "1 Apple Computer"\
                                         ", Inc.----------"\
                                         "----------------"\
                                         "----------------"\
                                         "----------------"\
                                         "----------------"\
                                         "----------------"\
                                         "---------------";
    size_t len;
    int length;
    int rsrclength;
    int rsrcoffset;
    FILE *file;
    unsigned int sum = 0;
    unsigned int rsrcsum = 0;
    unsigned char *outbuf;
    int bufsize;
    int i;

    file = fopen(iname, "rb");
    if (!file) {
       perror(iname);
       return -1;
    }
    fseek(file,0,SEEK_END);
    length = ftell(file);
    
    fseek(file,0,SEEK_SET);

    bufsize=(length+0x4600);
    if (fake_rsrc) {
        bufsize = (bufsize + 0x400) & ~0x200;
    }

    outbuf = malloc(bufsize);

    if ( !outbuf ) {
       printf("out of memory!\n");
       return -1;
    }

    len = fread(outbuf+0x4600, 1, length, file);
    if(len < (size_t)length) {
        perror(iname);
        return -2;
    }
    fclose(file);

    /* Calculate checksum for later use in header */    
    for(i = 0x4600; i < 0x4600+length;i++)
        sum += outbuf[i];

    /* Clear the header area to zero */
    memset(outbuf, 0, 0x4600);

    /* APPLE STOP SIGN */
    strcpy((char *)outbuf, apple_stop_sign);

    /* VOLUME HEADER */
    memcpy(&outbuf[0x100],"]ih[",4);   /* Magic */
    int2le(0x4000,   &outbuf[0x104]);  /* Firmware offset relative to 0x200 */
    short2le(0x10c,  &outbuf[0x108]);  /* Location of extended header */
    short2le(fw_ver, &outbuf[0x10a]);

    /* Firmware Directory - "osos" entry */
    memcpy(&outbuf[0x4200],"!ATAsoso",8);  /* dev and type */
    int2le(0,          &outbuf[0x4208]);   /* id */
    int2le(0x4400,     &outbuf[0x420c]);   /* devOffset */
    int2le(length,     &outbuf[0x4210]);   /* Length of firmware */
    int2le(0x10000000, &outbuf[0x4214]);   /* Addr */
    int2le(0,          &outbuf[0x4218]);   /* Entry Offset */
    int2le(sum,        &outbuf[0x421c]);   /* Checksum */
    int2le(0x00006012, &outbuf[0x4220]);   /* vers - 0x6012 is a guess */
    int2le(0xffffffff, &outbuf[0x4224]);   /* LoadAddr - for flash images */

    /* "rsrc" entry (if applicable) */
    if (fake_rsrc) {
        rsrcoffset=(length+0x4600+0x200) & ~0x200;
        rsrclength=0x200;
        rsrcsum=0;

        memcpy(&outbuf[0x4228],"!ATAcrsr",8); /* dev and type */
        int2le(0,          &outbuf[0x4230]);  /* id */
        int2le(rsrcoffset, &outbuf[0x4234]);  /* devOffset */
        int2le(rsrclength, &outbuf[0x4238]);  /* Length of firmware */
        int2le(0x10000000, &outbuf[0x423c]);  /* Addr */
        int2le(0,          &outbuf[0x4240]);  /* Entry Offset */
        int2le(rsrcsum,    &outbuf[0x4244]);  /* Checksum */
        int2le(0x0000b000, &outbuf[0x4248]);  /* vers */
        int2le(0xffffffff, &outbuf[0x424c]);  /* LoadAddr - for flash images */
    }

    file = fopen(oname, "wb");
    if (!file) {
       perror(oname);
       return -3;
    }
    
    len = fwrite(outbuf, 1, length+0x4600, file);
    if(len < (size_t)length) {
        perror(oname);
        return -4;
    }

    fclose(file);

    return 0;
}

