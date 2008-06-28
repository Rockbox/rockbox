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
 * based on tcctool.c by Dave Chapman
 *
 * USB code based on ifp-line - http://ifp-driver.sourceforge.net
 *
 * ifp-line is (C) Pavel Kriz, Jun Yamishiro and Joe Roback and
 * licensed under the GPL (v2)
 *
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
#include <inttypes.h>
#include <usb.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include "jz4740.h"
#include <stdbool.h>
#include <unistd.h>

#define VERSION "0.3"

#define MAX_FIRMWARESIZE   (64*1024*1024)   /* Arbitrary limit (for safety) */

/* For win32 compatibility: */
#ifndef O_BINARY
#define O_BINARY 0
#endif

/* USB IDs for USB Boot Mode */
#define VID		 0x601A
#define PID		 0x4740

#define EP_BULK_TO       0x01
#define TOUT             5000

enum USB_JZ4740_REQUEST
{
    VR_GET_CPU_INFO = 0,
    VR_SET_DATA_ADDRESS,
    VR_SET_DATA_LENGTH,
    VR_FLUSH_CACHES,
    VR_PROGRAM_START1,
    VR_PROGRAM_START2,
    VR_NOR_OPS,
    VR_NAND_OPS,
    VR_SDRAM_OPS,
    VR_CONFIGURATION
};

enum NOR_OPS_TYPE
{
    NOR_INIT = 0,
    NOR_QUERY,
    NOR_WRITE,
    NOR_ERASE_CHIP,
    NOR_ERASE_SECTOR
};

enum NOR_FLASH_TYPE
{
    NOR_AM29 = 0,
    NOR_SST28,
    NOR_SST39x16,
    NOR_SST39x8
};

enum NAND_OPS_TYPE
{
    NAND_QUERY = 0,
    NAND_INIT,
    NAND_MARK_BAD,
    NAND_READ_OOB,
    NAND_READ_RAW,
    NAND_ERASE,
    NAND_READ,
    NAND_PROGRAM,
    NAND_READ_TO_RAM
};

enum SDRAM_OPS_TYPE
{
    SDRAM_LOAD,

};

enum DATA_STRUCTURE_OB
{
    DS_flash_info ,
    DS_hand
};

enum OPTION
{
    OOB_ECC,
    OOB_NO_ECC,
    NO_OOB,
};

int filesize(FILE* fd)
{
    int tmp;
    fseek(fd, 0, SEEK_END);
    tmp = ftell(fd);
    fseek(fd, 0, SEEK_SET);
    return tmp;
}

#define SEND_COMMAND(cmd, arg) err = usb_control_msg(dh, USB_ENDPOINT_OUT | USB_TYPE_VENDOR, cmd, arg>>16, arg&0xFFFF, NULL, 0, TOUT);\
                               if (err < 0) \
                               { \
                                   fprintf(stderr,"\n[ERR]  Error sending control message (%d, %s)\n", err, usb_strerror()); \
                                   return -1; \
                               }

#define GET_CPU_INFO(s)        err = usb_control_msg(dh, USB_ENDPOINT_IN | USB_TYPE_VENDOR, VR_GET_CPU_INFO, 0, 0, s, 8, TOUT); \
                               if (err < 0) \
                               { \
                                   fprintf(stderr,"\n[ERR]  Error sending control message (%d, %s)\n", err, usb_strerror()); \
                                   return -1; \
                               }

#define SEND_DATA(ptr, size)   err = usb_bulk_write(dh, USB_ENDPOINT_OUT | EP_BULK_TO, ptr, size, TOUT); \
                               if (err != size)  \
                               { \
                                   fprintf(stderr,"\n[ERR]  Error writing data\n"); \
                                   fprintf(stderr,"[ERR]  Bulk write error (%d, %s)\n", err, strerror(-err)); \
                                   return -1; \
                               }

#define GET_DATA(ptr, size)    err = usb_bulk_read(dh, USB_ENDPOINT_IN | EP_BULK_TO, ptr, size, TOUT); \
                               if (err != size)  \
                               { \
                                   fprintf(stderr,"\n[ERR]  Error writing data\n"); \
                                   fprintf(stderr,"[ERR]  Bulk write error (%d, %s)\n", err, strerror(-err)); \
                                   return -1; \
                               }

int upload_app(usb_dev_handle* dh, int address, unsigned char* p, int len, bool stage2)
{
    int err;
    char buf[8];
    unsigned char* tmp_buf;

    fprintf(stderr, "[INFO] GET_CPU_INFO: ");
    GET_CPU_INFO(buf);
    buf[8] = 0;
    fprintf(stderr, "%s\n", buf);
#if 0
    fprintf(stderr, "[INFO] Flushing cache...");
    SEND_COMMAND(VR_FLUSH_CACHES, 0);
    fprintf(stderr, " Done!\n");
#endif

    fprintf(stderr, "[INFO] SET_DATA_ADDRESS to 0x%x...", address);
    SEND_COMMAND(VR_SET_DATA_ADDRESS, address);
    fprintf(stderr, " Done!\n");

    fprintf(stderr, "[INFO] Sending data...");
    /* Must not split the file in several packages! */
    SEND_DATA(p, len);
    fprintf(stderr, " Done!\n");
    
    fprintf(stderr, "[INFO] Verifying data...");
    SEND_COMMAND(VR_SET_DATA_ADDRESS, address);
    SEND_COMMAND(VR_SET_DATA_LENGTH, len);
    tmp_buf = malloc(len);
    if (tmp_buf == NULL)
    {
        fprintf(stderr, "\n[ERR]  Could not allocate memory.\n");
        return -1;
    }
    GET_DATA(tmp_buf, len);
    if (memcmp(tmp_buf, p, len) != 0)
        fprintf(stderr, "\n[WARN] Sent data isn't the same as received data...\n");
    else
        fprintf(stderr, " Done!\n");
    free(tmp_buf);

    fprintf(stderr, "[INFO] Booting device [STAGE%d]...", (stage2 ? 2 : 1));
    SEND_COMMAND((stage2 ? VR_PROGRAM_START2 : VR_PROGRAM_START1), (address+(stage2 ? 8 : 0)) );
    fprintf(stderr, " Done!\n");
    
    return 0;
}

int read_data(usb_dev_handle* dh, int address, unsigned char *p, int len)
{
    int err;
    char buf[8];

    fprintf(stderr, "[INFO] GET_CPU_INFO: ");
    GET_CPU_INFO(buf);
    buf[8] = 0;
    fprintf(stderr, "%s\n", buf);

    fprintf(stderr, "[INFO] Reading data...");
    SEND_COMMAND(VR_SET_DATA_ADDRESS, address);
    SEND_COMMAND(VR_SET_DATA_LENGTH, len);
    GET_DATA(p, len);
    fprintf(stderr, " Done!\n");
    return 0;
}

unsigned int read_reg(usb_dev_handle* dh, int address, int size)
{
    int err;
    unsigned char buf[4];
    
    SEND_COMMAND(VR_SET_DATA_ADDRESS, address);
    SEND_COMMAND(VR_SET_DATA_LENGTH, size);
    GET_DATA(buf, size);

    if(size == 1)
        return buf[0];
    else if(size == 2)
        return (buf[1] << 8) | buf[0];
    else if(size == 4)
        return (buf[3] << 24) | (buf[2] << 16) | (buf[1] << 8) | buf[0];
    else
        return 0;
}

int set_reg(usb_dev_handle* dh, int address, unsigned int val, int size)
{
    int err, i;
    unsigned char buf[4];
    
    buf[0] = val & 0xff;
    if(i > 1)
    {
        buf[1] = (val >> 8) & 0xff;
        if(i > 2)
        {
            buf[2] = (val >> 16) & 0xff;
            buf[3] = (val >> 24) & 0xff;
        }
    }
    
    SEND_COMMAND(VR_SET_DATA_ADDRESS, address);
    SEND_DATA(buf, size);

    return 0;
}
#define or_reg(dh, adr, val, size) set_reg(dh, adr, (read_reg(dh, adr, size) | (val)), size);
#define and_reg(dh, adr, val, size) set_reg(dh, adr, (read_reg(dh, adr, size) & (val)), size);
#define bc_reg(dh, adr, val, size) set_reg(dh, adr, (read_reg(dh, adr, size) & ~(val)), size);
#define xor_reg(dh, adr, val, size) set_reg(dh, adr, (read_reg(dh, adr, size) ^ (val)), size);

#define TEST(m, size) fprintf(stderr, "%s -> %x\n", #m, read_reg(dh, m, size));
int test_device(usb_dev_handle* dh)
{
    TEST(INTC_ISR, 4);
    TEST(INTC_IMR, 4);
    TEST(INTC_IMSR, 4);
    TEST(INTC_IMCR, 4);
    TEST(INTC_IPR, 4);
    
    fprintf(stderr, "\n");
    TEST(RTC_RCR, 4);
    TEST(RTC_RSR, 4);
    TEST(RTC_RSAR, 4);
    TEST(RTC_RGR, 4);
    TEST(RTC_HCR, 4);
    TEST(RTC_RCR, 4);
    TEST(RTC_HWFCR, 4);
    TEST(RTC_HRCR, 4);
    TEST(RTC_HWCR, 4);
    TEST(RTC_HWSR, 4);
    
    fprintf(stderr, "\n");
    TEST(GPIO_PXPIN(0), 4);
    TEST(GPIO_PXPIN(1), 4);
    TEST(GPIO_PXPIN(2), 4);
    TEST(GPIO_PXPIN(3), 4);
    
    fprintf(stderr, "\n");
    TEST(CPM_CLKGR, 4);
    
    fprintf(stderr, "\n");
    //or_reg(dh, SADC_ENA, SADC_ENA_TSEN, 1);
    TEST(SADC_ENA, 1);
    TEST(SADC_CTRL, 1);
    TEST(SADC_TSDAT, 4);
    TEST(SADC_BATDAT, 2);
    TEST(SADC_STATE, 1);
    
    fprintf(stderr, "\n");
    
    TEST(SLCD_CFG, 4);
    TEST(SLCD_CTRL, 1);
    TEST(SLCD_STATE, 1);
    
    return 0;
}

#define VOL_DOWN (1 << 27)
#define VOL_UP (1 << 0)
#define MENU (1 << 1)
#define HOLD (1 << 16)
#define OFF (1 << 29)
#define MASK (VOL_DOWN|VOL_UP|MENU|HOLD|OFF)
#define TS_MASK (SADC_STATE_PEND|SADC_STATE_PENU|SADC_STATE_TSRDY)
int probe_device(usb_dev_handle* dh)
{
    int tmp;
    
    //or_reg(dh, SADC_ENA, SADC_ENA_TSEN, 1);
    while(1)
    {
        if(read_reg(dh, SADC_STATE, 1) & SADC_STATE_TSRDY)
        {
            printf("%x\n", read_reg(dh, SADC_TSDAT, 4));
            or_reg(dh, SADC_CTRL, read_reg(dh, SADC_STATE, 1) & TS_MASK, 1);
        }
        
        tmp = read_reg(dh, GPIO_PXPIN(3), 4);
        if(tmp < 0)
            return tmp;
        if(tmp ^ MASK)
        {
            if(!(tmp & VOL_DOWN))
                printf("VOL_DOWN\t");
            if(!(tmp & VOL_UP))
                printf("VOL_UP\t");
            if(!(tmp & MENU))
                printf("MENU\t");
            if(!(tmp & OFF))
                printf("OFF\t");
            if(!(tmp & HOLD))
                printf("HOLD\t");
            printf("\n");
        }
    }    
    return 0;
}

unsigned int read_file(const char *name, unsigned char **buffer)
{
    FILE *fd;
    int len, n;
    
    fd = fopen(name, "rb");
    if (fd < 0)
    {
        fprintf(stderr, "[ERR]  Could not open %s\n", name);
        return 0;
    }
    
    len = filesize(fd);
    
    *buffer = (unsigned char*)malloc(len);
    if (*buffer == NULL)
    {
        fprintf(stderr, "[ERR]  Could not allocate memory.\n");
        fclose(fd);
        return 0;
    }
    
    n = fread(*buffer, 1, len, fd);
    if (n != len)
    {
        fprintf(stderr, "[ERR]  Short read.\n");
        fclose(fd);
        return 0;
    }
    fclose(fd);
    
    return len;
}
#define _GET_CPU fprintf(stderr, "[INFO] GET_CPU_INFO:"); \
                 GET_CPU_INFO(cpu); \
                 cpu[8] = 0; \
                 fprintf(stderr, " %s\n", cpu);
#define _SET_ADDR(a) fprintf(stderr, "[INFO] Set address to 0x%x...", a); \
                     SEND_COMMAND(VR_SET_DATA_ADDRESS, a); \
                     fprintf(stderr, " Done!\n");
#define _SEND_FILE(a) fsize = read_file(a, &buffer); \
                      fprintf(stderr, "[INFO] Sending file %s: %d bytes...", a, fsize); \
                      SEND_DATA(buffer, fsize); \
                      free(buffer); \
                      fprintf(stderr, " Done!\n");
#define _VERIFY_DATA(a,c) fprintf(stderr, "[INFO] Verifying data (%s)...", a); \
                          fsize = read_file(a, &buffer); \
                          buffer2 = (unsigned char*)malloc(fsize); \
                          SEND_COMMAND(VR_SET_DATA_ADDRESS, c); \
                          SEND_COMMAND(VR_SET_DATA_LENGTH,  fsize); \
                          GET_DATA(buffer2, fsize); \
                          if(memcmp(buffer, buffer2, fsize) != 0) \
                              fprintf(stderr, "\n[WARN] Sent data isn't the same as received data...\n"); \
                          else \
                              fprintf(stderr, " Done!\n"); \
                          free(buffer); \
                          free(buffer2);
#define _STAGE1(a)     fprintf(stderr, "[INFO] Stage 1 at 0x%x\n", a); \
                       SEND_COMMAND(VR_PROGRAM_START1, a);
#define _STAGE2(a)     fprintf(stderr, "[INFO] Stage 2 at 0x%x\n", a); \
                       SEND_COMMAND(VR_PROGRAM_START2, a);
#define _FLUSH         fprintf(stderr, "[INFO] Flushing caches...\n"); \
                       SEND_COMMAND(VR_FLUSH_CACHES, 0);
#ifdef _WIN32
 #define _SLEEP(x) Sleep(x*1000);
#else
 #define _SLEEP(x) sleep(x);
#endif
int mimic_of(usb_dev_handle *dh)
{
    int err, fsize;
    unsigned char *buffer, *buffer2;
    char cpu[8];
    
    fprintf(stderr, "[INFO] Start!\n");
    _GET_CPU;
    _SET_ADDR(0x8000 << 16);
    _SEND_FILE("1.bin");
    _GET_CPU;
    _VERIFY_DATA("1.bin", 0x8000 << 16);
    _STAGE1(0x8000 << 16);
    _SLEEP(3);
    _VERIFY_DATA("2.bin", 0xB3020060);
    _GET_CPU;
    _GET_CPU;
    _FLUSH;
    _GET_CPU;
    _GET_CPU;
    _SET_ADDR(0x8000 << 16);
    _SEND_FILE("3.bin");
    _GET_CPU;
    _VERIFY_DATA("3.bin", 0x8000 << 16);
    _GET_CPU;
    _FLUSH;
    _GET_CPU;
    _GET_CPU;
    _SET_ADDR(0x80D0 << 16);
    _SEND_FILE("4.bin");
    _GET_CPU;
    _VERIFY_DATA("4.bin", 0x80D0 << 16);
    _GET_CPU;
    _FLUSH;
    _GET_CPU;
    _GET_CPU;
    _SET_ADDR(0x80E0 << 16);
    _SEND_FILE("5.bin");
    _GET_CPU;
    _VERIFY_DATA("5.bin", 0x80E0 << 16);
    _GET_CPU;
    _FLUSH;
    _GET_CPU;
    _GET_CPU;
    _SET_ADDR(0x80004000);
    _SEND_FILE("6.bin");
    _GET_CPU;
    _VERIFY_DATA("6.bin", 0x80004000);
    _GET_CPU;
    _FLUSH;
    _GET_CPU;
    _GET_CPU;
    _SET_ADDR(0x80FD << 16);
    _SEND_FILE("7.bin");
    _GET_CPU;
    _VERIFY_DATA("7.bin", 0x80FD << 16);
    _GET_CPU;
    _FLUSH;
    _GET_CPU;
    _STAGE2(0x80FD0004);
    _VERIFY_DATA("8.bin", 0x80004004);
    _VERIFY_DATA("9.bin", 0x80004008);
    _SLEEP(2);
    _GET_CPU;
    _SET_ADDR(0x80E0 << 16);
    _SEND_FILE("10.bin");
    _GET_CPU;
    _VERIFY_DATA("10.bin", 0x80E0 << 16);
    _GET_CPU;
    _FLUSH;
    _GET_CPU;
    _STAGE2(0x80e00008);
    fprintf(stderr, "[INFO] Done!\n");
    return 0;
}

void jzconnect(int address, unsigned char* buf, int len, int func)
{
    struct usb_bus *bus;
    struct usb_device *tmp_dev;
    struct usb_device *dev = NULL;
    usb_dev_handle *dh;
    int err;

    fprintf(stderr,"[INFO] Searching for device...\n");
 
    usb_init();
    if(usb_find_busses() < 0)
    {
        fprintf(stderr, "[ERR]  Could not find any USB busses.\n");
        return;
    }

    if (usb_find_devices() < 0)
    {
        fprintf(stderr, "[ERR]  USB devices not found(nor hubs!).\n");
        return;
    }

    for (bus = usb_get_busses(); bus; bus = bus->next)
    {
        for (tmp_dev = bus->devices; tmp_dev; tmp_dev = tmp_dev->next)
        {
            //printf("Found Vendor %04x Product %04x\n",tmp_dev->descriptor.idVendor, tmp_dev->descriptor.idProduct);
            if (tmp_dev->descriptor.idVendor == VID &&
                tmp_dev->descriptor.idProduct == PID)
            {
                dev = tmp_dev;
                goto found;

            }
        }
    }

    if (dev == NULL)
    {
        fprintf(stderr, "[ERR]  Device not found.\n");
        fprintf(stderr, "[ERR]  Ensure your device is in USB boot mode and run usbtool again.\n");
        return;
    }

found:
    if ( (dh = usb_open(dev)) == NULL)
    {
        fprintf(stderr,"[ERR]  Unable to open device.\n");
        return;
    }
    
    err = usb_set_configuration(dh, 1);

    if (err < 0)
    {
        fprintf(stderr, "[ERR]  usb_set_configuration failed (%d, %s)\n", err, usb_strerror());
        usb_close(dh);
        return;
    }
    
    /* "must be called" written in the libusb documentation */
    err = usb_claim_interface(dh, 0);
    if (err < 0)
    {
        fprintf(stderr, "[ERR]  Unable to claim interface (%d, %s)\n", err, usb_strerror());
        usb_close(dh);
        return;
    }

    fprintf(stderr,"[INFO] Found device, uploading application.\n");

    /* Now we can transfer the application to the device. */ 

    switch(func)
    {
        case 1:
        case 5:
            err = upload_app(dh, address, buf, len, (func == 5));
        break;
        case 2:
            err = read_data(dh, address, buf, len);
        break;
        case 3:
            err = test_device(dh);
        break;
        case 4:
            err = probe_device(dh);
        break;
        case 6:
            err = mimic_of(dh);
        break;
    }
    
    /* release claimed interface */
    usb_release_interface(dh, 0);

    usb_close(dh);
}

void print_usage(void)
{
#ifdef _WIN32
    fprintf(stderr, "Usage: usbtool.exe [CMD] [FILE] [ADDRESS] [LEN]\n");
#else
    fprintf(stderr, "Usage: usbtool [CMD] [FILE] [ADDRESS] [LEN]\n");
#endif
    fprintf(stderr, "\t[ADDRESS] has to be in 0xHEXADECIMAL format\n");
    fprintf(stderr, "\t[CMD]:\n\t\t1 -> upload file to specified address and boot from it\n\t\t2 -> read data from [ADDRESS] with length [LEN] to [FILE]\n");
    fprintf(stderr, "\t\t3 -> read device status\n\t\t4 -> probe keys (only Onda VX747)\n");
    fprintf(stderr, "\t\t5 -> same as 1 but do a stage 2 boot\n\t\t6 -> mimic OF fw recovery\n");
#ifdef _WIN32
    fprintf(stderr, "\nExample:\n\t usbtool.exe 1 fw.bin 0x80000000");
    fprintf(stderr, "\n\t usbtool.exe 2 save.bin 0x81000000 1024");
#else
    fprintf(stderr, "\nExample:\n\t usbtool 1 fw.bin 0x80000000");
    fprintf(stderr, "\n\t usbtool 2 save.bin 0x81000000 1024");
#endif
}

int main(int argc, char* argv[])
{
    unsigned char* buf;
    int n, len, address, cmd=0;
    FILE* fd;

    fprintf(stderr, "USBtool v" VERSION " - (C) 2008 Maurus Cuelenaere\n");
    fprintf(stderr, "This is free software; see the source for copying conditions.  There is NO\n");
    fprintf(stderr, "warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n\n");
    
    if(argc > 1)
        sscanf(argv[1], "%d", &cmd);
    switch(cmd)
    {
        case 5:
        case 1:
            if (strcmp(argv[3], "-1") == 0)
                address = 0x80000000;
            else
            {
                if (sscanf(argv[3], "0x%x", &address) <= 0)
                {
                    print_usage();
                    return -1;
                }
            }
            
            fd = fopen(argv[2], "rb");
            if (fd < 0)
            {
                fprintf(stderr, "[ERR]  Could not open %s\n", argv[2]);
                return 4;
            }
            
            len = filesize(fd);

            if (len > MAX_FIRMWARESIZE)
            {
                fprintf(stderr, "[ERR]  Firmware file too big\n");
                fclose(fd);
                return 5;
            }
            
            buf = malloc(len);
            if (buf == NULL)
            {
                fprintf(stderr, "[ERR]  Could not allocate memory.\n");
                fclose(fd);
                return 6;
            }
            
            n = fread(buf, 1, len, fd);
            if (n != len)
            {
                fprintf(stderr, "[ERR]  Short read.\n");
                fclose(fd);
                return 7;
            }
            fclose(fd);
            
            fprintf(stderr, "[INFO] File size: %d bytes\n", n);
            
            jzconnect(address, buf, len, cmd);
        break;
        case 2:
            if (sscanf(argv[3], "0x%x", &address) <= 0)
            {
                print_usage();
                return -1;
            }
            
            fd = fopen(argv[2], "wb");
            if (fd < 0)
            {
                fprintf(stderr, "[ERR]  Could not open %s\n", argv[2]);
                return 4;
            }
            
            sscanf(argv[4], "%d", &len);
            
            buf = malloc(len);
            if (buf == NULL)
            {
                fprintf(stderr, "[ERR]  Could not allocate memory.\n");
                fclose(fd);
                return 6;
            }
            
            jzconnect(address, buf, len, 2);
            
            n = fwrite(buf, 1, len, fd);
            if (n != len)
            {
                fprintf(stderr, "[ERR]  Short write.\n");
                fclose(fd);
                return 7;
            }
            fclose(fd);
        break;
        case 3:
        case 4:
        case 6:
            jzconnect(address, NULL, 0, cmd);
        break;
        default:
            print_usage();
            return 1;
        break;
    }

    return 0;
}
