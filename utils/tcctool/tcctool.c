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

#define VERSION "0.1"

#define MAX_FIRMWARESIZE   (2*1024*1024)   /* Arbitrary limit (for safety) */

struct device_t
{
    char* name;
    char* label;
    uint32_t loadaddr;
    uint32_t magic;
};

static struct device_t devices[] = 
{ 
    {"logikdax", "Logik DAX 1GB DAB/MP3 player", 0x20000000, 0x52e97410 },
    {"iaudio6", "iAudio 6", 0x20000000, 0x62e97010 },
    {"iaudio7", "iAudio 7", 0x20000000, 0x62e97010 }
};

#define NUM_DEVICES ((sizeof(devices) / sizeof(struct device_t)))

int find_device(char* devname)
{
    unsigned int i = 0;

    while ((i < NUM_DEVICES) && (strcmp(devices[i].name,devname)))
        i++;

    if (i==NUM_DEVICES)
        return -1;
    else
        return i;
}

void print_devices(void)
{
    unsigned int i;

    printf("Valid devices are:\n");
    for (i=0; i<NUM_DEVICES; i++)
    {
        printf("  %10s - %s\n",devices[i].name,devices[i].label);
    }
}

/* USB IDs for USB Boot Mode */
#define TCC_VENDORID    0x140e
#define TCC_PRODUCTID   0xb021

#define TCC_BULK_TO       1
#define TOUT              5000
#define PACKET_SIZE       64  /* Number of bytes to send in one write */

#ifndef MAX
#define MAX(a,b) (((a)>(b))?(a):(b))
#endif

static void put_int32le(uint32_t x, char* p)
{
    p[0] = x & 0xff;
    p[1] = (x >> 8) & 0xff;
    p[2] = (x >> 16) & 0xff;
    p[3] = (x >> 24) & 0xff;
}

int upload_app(usb_dev_handle* dh, int device, char* p, int len)
{
    char buf[PACKET_SIZE];
    int err;
    int i;

    /* Send the header - Destination address, length and magic value */
    memset(buf, 0, PACKET_SIZE);

    put_int32le(0xf0000000, buf);   /* Unknown - always the same */
    put_int32le(len / PACKET_SIZE, buf + 4);
    put_int32le(devices[device].loadaddr, buf + 8);
    put_int32le(devices[device].magic, buf + 12);

    err = usb_bulk_write(dh, TCC_BULK_TO, buf, PACKET_SIZE, TOUT);

    if (err < 0)
    {
        fprintf(stderr,"[ERR]  Error writing header\n");
        fprintf(stderr,"[ERR]  Bulk write error (%d, %s)\n", err, strerror(-err));
        return -1;
    }

    /* Now send the data, PACKET_SIZE bytes at a time. */

    for (i=0 ; i < (len / PACKET_SIZE) ; i++)
    {
        err = usb_bulk_write(dh, TCC_BULK_TO, p, PACKET_SIZE, TOUT);

        if (err < 0)
        {
            fprintf(stderr,"[ERR]  Error writing data\n");
            fprintf(stderr,"[ERR]  Bulk write error (%d, %s)\n", err, strerror(-err));
            return -1;
        }

        p += PACKET_SIZE;
    }

    return 0;
}


/* The main function */

void do_patching(int device, char* buf, int len)
{
    struct usb_bus *busses;
    struct usb_bus *bus;
    struct usb_device *tmp_dev;
    struct usb_device *dev = NULL;
    usb_dev_handle *dh;
    int err;

    fprintf(stderr,"[INFO] Searching for TCC device...\n");
 
    usb_init();
    if(usb_find_busses() < 0) {
        fprintf(stderr, "[ERR]  Could not find any USB busses.\n");
        return;
    }

    if (usb_find_devices() < 0) {
        fprintf(stderr, "[ERR]  USB devices not found(nor hubs!).\n");
        return;
    }

    /* C calling convention, it's not nice to use global stuff */
    busses = usb_get_busses();

    for (bus = busses; bus; bus = bus->next) {
        for (tmp_dev = bus->devices; tmp_dev; tmp_dev = tmp_dev->next) {
            //printf("Found Vendor %04x Product %04x\n",tmp_dev->descriptor.idVendor, tmp_dev->descriptor.idProduct);
            if (tmp_dev->descriptor.idVendor == TCC_VENDORID &&
                tmp_dev->descriptor.idProduct == TCC_PRODUCTID ) {

                dev = tmp_dev;
                goto found;

            }
        }
    }

    if (dev == NULL) {
        fprintf(stderr, "[ERR]  TCC device not found.\n");
        fprintf(stderr, "[ERR]  Ensure your TCC device is in USB boot mode and run tcctool again.\n");
        return;
    }

found:
    if ( (dh = usb_open(dev)) == NULL) {
        fprintf(stderr,"[ERR]  Unable to open TCC device.\n");
        return;
    }

    err = usb_set_configuration(dh, 1);

    if (err < 0)  {
        fprintf(stderr, "[ERR]  usb_set_configuration failed (%d)\n", err);
        usb_close(dh);
        return;
    }

    /* "must be called" written in the libusb documentation */
    err = usb_claim_interface(dh, dev->config->interface->altsetting->bInterfaceNumber);
    if (err < 0) {
        fprintf(stderr, "[ERR]  Unable to claim interface (%d)\n", err);
        usb_close(dh);
        return;
    }

    fprintf(stderr,"[INFO] Found TCC device, uploading application.\n");

    /* Now we can transfer the application to the device. */ 

    if (upload_app(dh, device, buf, len) < 0)
    {
        fprintf(stderr,"[ERR]  Upload of application failed.\n");
    }
    else
    {
        fprintf(stderr,"[INFO] Patching application uploaded successfully!\n");
    }

    /* release claimed interface */
    usb_release_interface(dh, dev->config->interface->altsetting->bInterfaceNumber);

    usb_close(dh);
}

off_t filesize(int fd) {
    struct stat buf;

    if (fstat(fd,&buf) < 0) {
        perror("[ERR]  Checking filesize of input file");
        return -1;
    } else {
        return(buf.st_size);
    }
}

void print_usage(void)
{
    printf("Usage: tcctool -d devicename firmware.bin\n");
}

int main(int argc, char* argv[])
{
    char* buf;
    int n,len;
    int fd;
    int device;

    printf("tcctool v" VERSION " - (C) 2007 Dave Chapman\n");
    printf("This is free software; see the source for copying conditions.  There is NO\n");
    printf("warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n\n");

    if (argc != 4)
    {
        print_usage();
        return 1;
    }

    if (strcmp(argv[1],"-d"))
    {
        print_usage();
        return 2;
    }

    device = find_device(argv[2]);

    if (device < 0)
    {
        printf("[ERR]  Unknown device \"%s\"\n",argv[2]);
        print_devices();
        return 3;
    }

    printf("[INFO] Using device \"%s\"\n",devices[device].label);
    fd = open(argv[3], O_RDONLY);
    if (fd < 0)
    {
        printf("[ERR]  Could not open %s\n", argv[3]);
        return 4;
    }

    len = filesize(fd);

    if (len > MAX_FIRMWARESIZE)
    {
        printf("[ERR]  Firmware file too big\n");
        close(fd);
        return 5;
    }

    buf = malloc(len);
    if (buf == NULL)
    {
        printf("[ERR]  Could not allocate memory.\n");
        close(fd);
        return 6;
    }
    
    n = read(fd, buf, len);
    if (n != len)
    {
        printf("[ERR]  Short read.\n");
        close(fd);
        return 7;
    }
    close(fd);

    do_patching(device, buf, len);

    return 0;
}
