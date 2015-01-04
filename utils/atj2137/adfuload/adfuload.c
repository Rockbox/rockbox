/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2013 by Marcin Bukat
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

#include <stdlib.h>
#include <libusb.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include "encrypt.h"

#define VERSION "v0.1"

#define RETRY_MAX 5
#define USB_TIMEOUT 512
#define VENDORID  0x10d6
#define PRODUCTID 0xff96

#define OUT_EP  0x01
#define IN_EP   0x82

#define CBW_SIGNATURE 0x43425355
#define CSW_SIGNATURE 0x53425355

struct CBWCB_t
{
    uint8_t  cbCode;
    uint8_t  cbLun;
    uint32_t LBA;
    uint32_t cbLen;
    uint8_t  reseved;
    uint8_t  control;
} __attribute__((__packed__));

struct CBW_t
{
    uint32_t dCBWSignature;
    uint32_t dCBWTag;
    uint32_t dCBWDataTransferLength;
    uint8_t  bmCBWFlags;
    uint8_t  bCBWLUN;
    uint8_t  bCBWCBLength;
    uint8_t  CBWCB[16];
} __attribute__((__packed__));

struct CSW_t
{
    uint32_t dCSWSignature;
    uint32_t dCSWTag;
    uint32_t dCSWDataResidue;
    uint8_t  bCSWStatus;
} __attribute__((__packed__));

static int send_msc_cmd(libusb_device_handle *hdev, void *cbwcb, uint32_t data_len, uint32_t *reftag)
{
    struct CBW_t cbw;
    int ret, repeat, transferred;

    repeat = 0;
    memset(&cbw, 0, sizeof(cbw));
    cbw.dCBWSignature = CBW_SIGNATURE;
    cbw.dCBWTag = 0;
    cbw.dCBWDataTransferLength = data_len;
    cbw.bmCBWFlags = 0;
    cbw.bCBWLUN = 0;
    cbw.bCBWCBLength = ((((char *)cbwcb)[0] == 0x10) ? 0 : 0x10);
    memcpy(cbw.CBWCB, cbwcb, sizeof(struct CBWCB_t));

    *reftag = cbw.dCBWTag;
    do
    {
        /* transfer command to the device */
        ret = libusb_bulk_transfer(hdev, OUT_EP, (unsigned char*)&cbw, 31, &transferred, USB_TIMEOUT);
        if (ret == LIBUSB_ERROR_PIPE)
        {
            libusb_clear_halt(hdev, OUT_EP);
        }
        repeat++;
    } while ((ret == LIBUSB_ERROR_PIPE) && (repeat < RETRY_MAX));

    if (ret != LIBUSB_SUCCESS)
    {
        printf("error: command transfer error\n");
        return -1;
    }

    return 0;
}

static int get_msc_csw(libusb_device_handle *hdev, uint32_t reftag)
{
    struct CSW_t csw;
    int ret, repeat, transferred;

    /* get CSW response from device */
    repeat = 0;
    do
    {
        ret = libusb_bulk_transfer(hdev, IN_EP, (unsigned char *)&csw, 13, &transferred, USB_TIMEOUT);
        if (ret == LIBUSB_ERROR_PIPE)
        {
            libusb_clear_halt(hdev, IN_EP);
        }
        repeat++;
    } while ((ret == LIBUSB_ERROR_PIPE) && (repeat < RETRY_MAX));

    if (ret != LIBUSB_SUCCESS)
    {
        printf("error reading CSW\n");
        return -3;
    }

    if (transferred != 13)
    {
        printf("error wrong size of CSW packet\n");
        return -4;
    }

    if (csw.dCSWSignature != CSW_SIGNATURE)
    {
        printf("error: wrong CSW signature.\n");
        return -5;
    }
    if (csw.dCSWTag != reftag)
    {
        printf("error: CSW dCSWTag mismatch. Is 0x%x, expected 0x%x\n", csw.dCSWTag, reftag);
        return -6;
    }

    if (csw.bCSWStatus)
    {
        /* In case of CSW indicating error dump the content of the packet */
        printf ("dCSWSignature: 0x%0x\n", csw.dCSWSignature);
        printf ("dCSWTag: 0x%0x\n", csw.dCSWTag);
        printf ("dCSWDataResidue: 0x%0x\n", csw.dCSWDataResidue);
        printf ("bCSWStatus: 0x%0x\n", csw.bCSWStatus);
    }

    return csw.bCSWStatus;
}

static void adfu_upload(libusb_device_handle *hdev, unsigned char *buf, int size, uint32_t address)
{
    uint8_t cmdbuf[16];
    uint32_t reftag;
    int transferred;

    memset(cmdbuf, 0, sizeof(cmdbuf));

    fprintf(stderr,"[info]: loading binary @ 0x%08x size %d bytes\n", address, size);

    cmdbuf[0] = 0x05; /* transfer */

    cmdbuf[1] = address         & 0xff; /* addr LSB */
    cmdbuf[2] = (address >> 8)  & 0xff;
    cmdbuf[3] = (address >> 16) & 0xff;
    cmdbuf[4] = (address >> 24) & 0xff; /* addr MSB */

    cmdbuf[5] = size         & 0xff; /* len LSB */
    cmdbuf[6] = (size >> 8)  & 0xff;
    cmdbuf[7] = (size >> 16) & 0xff;
    cmdbuf[8] = (size >> 24) & 0xff; /* len MSB */

    send_msc_cmd(hdev, (void *)cmdbuf, size, &reftag);

    libusb_bulk_transfer(hdev, OUT_EP, buf, size, &transferred, USB_TIMEOUT);
    fprintf(stderr, "[info]: actual xfered data: %d bytes\n", transferred);

    /* get CSW from device*/
    fprintf(stderr, "[info]: CSW status: %d\n", get_msc_csw(hdev, reftag));
}

#if 0
/* unused, I'll leve it here for reference */
static void adfu_download(libusb_device_handle *hdev, unsigned char *buf, int size, uint32_t address)
{
    uint8_t cmdbuf[16];
    int transferred, ret, repeat;
    uint32_t reftag;

    fprintf(stderr, "[info]: downloading %d bytes from 0x%08x\n", size, address); 
    memset(cmdbuf, 0, sizeof(cmdbuf));

    cmdbuf[0] = 0x05;

    cmdbuf[1] = address         & 0xff; /* addr LSB */
    cmdbuf[2] = (address >> 8)  & 0xff;
    cmdbuf[3] = (address >> 16) & 0xff;
    cmdbuf[4] = (address >> 24) & 0xff; /* addr MSB */

    cmdbuf[5] = size         & 0xff; /* len LSB */
    cmdbuf[6] = (size >> 8)  & 0xff;
    cmdbuf[7] = (size >> 16) & 0xff;
    cmdbuf[8] = (size >> 24) & 0xff; /* len MSB */

    cmdbuf[9] = 0x80; /* send data from device */

    send_msc_cmd(hdev, (void *)cmdbuf, size, &reftag);

    memset(buf, 0, sizeof(buf));

    /* get data from device */
    repeat = 0;
    do
    {
        ret = libusb_bulk_transfer(hdev, IN_EP, buf, size, &transferred, USB_TIMEOUT);
        if (ret == LIBUSB_ERROR_PIPE)
        {
            libusb_clear_halt(hdev, IN_EP);
        }
        repeat++;
    } while ((ret == LIBUSB_ERROR_PIPE) && (repeat < RETRY_MAX));

    fprintf(stderr, "[info]: actual xfered data: %d bytes\n", transferred);

    /* get CSW from device*/
    fprintf(stderr, "[info]: CSW status: %d\n", get_msc_csw(hdev, reftag));
}
#endif

static void adfu_execute(libusb_device_handle *hdev, uint32_t address)
{
    uint8_t cmdbuf[16];
    uint32_t reftag;

    fprintf(stderr, "[info]: jumping to address 0x%08x\n", address);
    memset(cmdbuf, 0, sizeof(cmdbuf));

    cmdbuf[0] = 0x10; /* execute */

    cmdbuf[1] = address         & 0xff; /* addr LSB */
    cmdbuf[2] = (address >> 8)  & 0xff;
    cmdbuf[3] = (address >> 16) & 0xff;
    cmdbuf[4] = (address >> 24) & 0xff; /* addr MSB */

    send_msc_cmd(hdev, (void *)cmdbuf, 0, &reftag);

    /* get CSW from device*/
    fprintf(stderr, "[info]: CSW status: %d\n", get_msc_csw(hdev, reftag));
}

static void usage(char *name)
{
    printf("usage: (sudo) %s [-u vid:pid] [-e] -s1 stage1.bin -s2 stage2.bin\n", name);
    printf("stage1.bin          Binary of the stage1 (ADEC_N63.BIN for example)\n");
    printf("stage2.bin          Binary of the custom user code\n");
    printf("\n");
    printf("options:\n");
    printf("-u|--usb vid:pid    Override device PID and PID (default 0x%04x:0x%04x)\n",
           VENDORID, PRODUCTID);
    printf("-e                  Encode stage1 binary as needed by brom adfu mode\n");
}

int main (int argc, char **argv)
{
    uint16_t pid = PRODUCTID;
    uint16_t vid = VENDORID;

    libusb_device_handle *hdev;
    int ret = 0;
    int i = 1;
    uint8_t *buf;
    uint32_t filesize, filesize_round;
    char *s1_filename = NULL, *s2_filename = NULL;
    bool encode = false;
    FILE *fp = NULL;

    while (i < argc)
    {
        if (strcmp(argv[i],"-e") == 0)
        {
            encode = true;
            i++;
        }
        else if (strcmp(argv[i],"-s1") == 0)
        {
            i++;
            if (i == argc)
            {
                usage(argv[0]);
                return -1;
            }
            s1_filename = argv[i];
            i++;
        }
        else if (strcmp(argv[i],"-s2") == 0)
        {
            i++;
            if (i == argc)
            {
                usage(argv[0]);
                return -2;
            }
            s2_filename = argv[i];
            i++;
        }
        else if ((strcmp(argv[i],"-u")==0) || (strcmp(argv[i],"--usb")==0))
        {
            if (i + 1 == argc)
            {
                fprintf(stderr,"Missing argument for USB IDs\n");
                return -1;
            }
            i++;
            char *svid = argv[i];
            char *spid = strchr(svid, ':');
            if(svid == NULL)
            {
                fprintf(stderr,"Invalid argument for USB IDs (missing ':')\n");
                return -2;
            }
            char *end;
            vid = strtoul(svid, &end, 0);
            if(*end != ':')
            {
                fprintf(stderr,"Invalid argument for USB VID\n");
                return -3;
            }
            pid = strtoul(spid + 1, &end, 0);
            if(*end)
            {
                fprintf(stderr,"Invalid argument for USB PID\n");
                return -4;
            }
            i++;
        }
        else
        {
            usage(argv[0]);
            return -3;
        }
    }

    if (!s1_filename)
    {
        usage(argv[0]);
        return -3;
    }

    /* print banner */
    fprintf(stderr,"adfuload " VERSION "\n");
    fprintf(stderr,"(C) Marcin Bukat 2013\n");
    fprintf(stderr,"This is free software; see the source for copying conditions.  There is NO\n");
    fprintf(stderr,"warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n\n");

    /* initialize libusb */
    libusb_init(NULL);

    hdev = libusb_open_device_with_vid_pid(NULL, vid, pid);
    if (hdev == NULL)
    {
        fprintf(stderr, "[error]: can't open device with VID:PID = 0x%0x:0x%0x\n", vid, pid);
        libusb_exit(NULL);
        return -4;
    }

    ret = libusb_kernel_driver_active(hdev, 0);

    if (ret < 0)
    {
        fprintf(stderr, "[error]: checking kernel driver active\n");
        ret = -5;
        goto end;
    }
    else
    {
        if (ret)
            libusb_detach_kernel_driver(hdev, 0);
    }

    ret = libusb_set_configuration(hdev, 1);
    if (ret < 0)
    {
        fprintf(stderr, "[error]: could not select configuration (1)\n");
        ret = -6;
        goto end;
    }

    ret = libusb_claim_interface(hdev, 0);
    if (ret < 0)
    {
            fprintf(stderr, "[error]: could not claim interface #0\n");
            ret = -7;
            goto end;
    }

    ret = libusb_set_interface_alt_setting(hdev, 0, 0);
    if ( ret != LIBUSB_SUCCESS)
    {
        fprintf(stderr, "[error]: could not set alt setting for interface #0\n");
        ret = -8;
        goto end;
    }

    fp = fopen(s1_filename, "rb");

    if (fp == NULL)
    {
        fprintf(stderr, "[error]: Could not open file \"%s\"\n", s1_filename);
        ret = -10;
        goto end;
    }

    fseek(fp, 0, SEEK_END);
    filesize = (uint32_t)ftell(fp);
    fseek(fp, 0, SEEK_SET);

    filesize_round = ( (filesize + 511) / 512 ) * 512;

    buf = malloc(filesize_round);

    if (buf == NULL)
    {
        fprintf(stderr, "[error]: Cannot allocate %d bytes of memory\n", filesize_round);
        ret = -11;
        goto end_fclose;
    }

    if (fread(buf, 1, filesize, fp) != filesize)
    {
        fprintf(stderr, "[error]: can't read file: %s\n", s1_filename);
        ret = -12;
        goto end_free;
    }

    fclose(fp);


    if (encode)
    {
        fprintf(stderr, "[info]: encrypting binary\n");
        ret = encrypt(buf, filesize_round);

        if (ret)
        {
            fprintf(stderr, "[error]: can't encrypt binary\n");
            ret = -13;
            goto end_free;
        }
    }

    adfu_upload(hdev, buf, filesize_round, 0xb4040000);
    adfu_execute(hdev, 0xb4040000);
    /* Now ADEC_N63.BIN should be operational */

    if (s2_filename)
    {
        fprintf(stderr, "[info]: file %s\n", s2_filename);

        /* upload custom binary and run it */
        fp = fopen(s2_filename, "rb");

        if (fp == NULL)
        {
            fprintf(stderr, "[error]: could not open file \"%s\"\n", s2_filename);
            ret = -20;
            goto end;
        }

        fseek(fp, 0, SEEK_END);
        filesize = (uint32_t)ftell(fp);
        fseek(fp, 0, SEEK_SET);

        buf = realloc(buf, filesize);

        if (buf == NULL)
        {
            fprintf(stderr, "[error]: can't allocate %d bytes of memory\n", filesize);
            ret = -21;
            goto end_fclose;
        }

        if (fread(buf, 1, filesize, fp) != filesize)
        {
            fprintf(stderr, "[error]: can't read file: %s\n", s2_filename);
            ret = -22;
            goto end_free;
        }

        fprintf(stderr, "[info]: file %s\n", s2_filename);
        /* upload binary to the begining of DRAM */
        adfu_upload(hdev, buf, filesize, 0xa0000000);
        adfu_execute(hdev, 0xa0000000);
    }
    else
        fp = NULL;

end_free:
    if (buf) {free(buf);}
end_fclose:
    if (fp) {fclose(fp);}
end:
    libusb_close(hdev);
    libusb_exit(NULL);

    return ret;
}
