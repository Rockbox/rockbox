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
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
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

#define VERSION "0.2"

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

#ifndef MAX
#define MAX(a,b) (((a)>(b))?(a):(b))
#endif

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

#define SEND_COMMAND(cmd, arg) err = usb_control_msg(dh, USB_ENDPOINT_OUT | USB_TYPE_VENDOR, cmd, arg>>16, arg&0xFFFF, NULL, 0, TOUT);\
                               if (err < 0) \
                               { \
                                   fprintf(stderr,"\n[ERR]  Error sending control message (%d, %s)\n", err, usb_strerror()); \
                                   return -1; \
                               }

#define GET_CPU_INFO(s)        err = usb_control_msg(dh, USB_ENDPOINT_IN | USB_TYPE_VENDOR, VR_GET_CPU_INFO, 0, 0, buf, 8, TOUT); \
                               if (err < 0) \
                               { \
                                   fprintf(stderr,"\n[ERR]  Error sending control message (%d, %s)\n", err, usb_strerror()); \
                                   return -1; \
                               }

#define SEND_DATA(ptr, size)   err = usb_bulk_write(dh, USB_ENDPOINT_OUT | EP_BULK_TO, ptr, size, TOUT); \
                               if (err != len)  \
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

int upload_app(usb_dev_handle* dh, int address, unsigned char* p, int len)
{
    int err;
    char buf[8];
    unsigned char* tmp_buf;

    fprintf(stderr, "[INFO] GET_CPU_INFO: ");
    GET_CPU_INFO(buf);
    buf[8] = 0;
    fprintf(stderr, "%s\n", buf);

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
    {
        fprintf(stderr, "\n[ERR]  Sent data isn't the same as received data...\n");
        free(tmp_buf);
        return -1;
    }
    free(tmp_buf);
    fprintf(stderr, " Done !\n");

    fprintf(stderr, "[INFO] Booting device...");
    SEND_COMMAND(VR_PROGRAM_START1, address);
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

unsigned int read_reg(usb_dev_handle* dh, int address)
{
    int err;
    unsigned char buf[4];
    
    SEND_COMMAND(VR_SET_DATA_ADDRESS, address);
    SEND_COMMAND(VR_SET_DATA_LENGTH, 4);
    GET_DATA(buf, 4);

    return (buf[3] << 24) | (buf[2] << 16) | (buf[1] << 8) | buf[0];
}

#define TEST(m) fprintf(stderr, "%s -> %x\n", #m, read_reg(dh, m));
int test_device(usb_dev_handle* dh)
{
    TEST(INTC_ISR);
    TEST(INTC_IMR);
    TEST(INTC_IMSR);
    TEST(INTC_IMCR);
    TEST(INTC_IPR);
    
    fprintf(stderr, "\n");
    TEST(RTC_RCR);
    TEST(RTC_RSR);
    TEST(RTC_RSAR);
    TEST(RTC_RGR);
    TEST(RTC_HCR);
    TEST(RTC_RCR);
    TEST(RTC_HWFCR);
    TEST(RTC_HRCR);
    TEST(RTC_HWCR);
    TEST(RTC_HWSR);
    
    fprintf(stderr, "\n");
    TEST(GPIO_PXPIN(0));
    TEST(GPIO_PXPIN(1));
    TEST(GPIO_PXPIN(2));
    TEST(GPIO_PXPIN(3));
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
            err = upload_app(dh, address, buf, len);
        break;
        case 2:
            err = read_data(dh, address, buf, len);
        break;
        case 3:
            err = test_device(dh);
        break;
    }
    
    /* release claimed interface */
    usb_release_interface(dh, 0);

    usb_close(dh);
}

int filesize(FILE* fd)
{
    int tmp;
    tmp = fseek(fd, 0, SEEK_END);
    fseek(fd, 0, SEEK_SET);
    return tmp;
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
    fprintf(stderr, "\t\t3 -> read device status\n");
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
        case 1:
            if (sscanf(argv[3], "0x%x", &address) <= 0)
            {
                print_usage();
                return -1;
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
            
            jzconnect(address, buf, len, 1);
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
            jzconnect(address, NULL, 0, 3);
        break;
        default:
            print_usage();
            return 1;
        break;
    }

    return 0;
}
