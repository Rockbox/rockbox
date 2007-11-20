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
#include "stdbool.h"

#include "bootimg.h"

#define VERSION "0.2"

/* USB IDs for Manufacturing Mode */
#define E200R_VENDORID    0x0781
#define E200R_PRODUCTID   0x0720

#define E200R_BULK_TO     1
#define TOUT              5000
#define MAX_TRANSFER      64       /* Number of bytes to send in one write */

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

int upload_app(usb_dev_handle* dh)
{
    char buf[4];
    int err;
    int tosend;
    char* p = (char*)bootimg;
    int bytesleft = LEN_bootimg;

    /* Write the data length */

    put_int32le(LEN_bootimg, buf);

    err = usb_bulk_write(dh, E200R_BULK_TO, buf, 4, TOUT);

    if (err < 0)
    {
        fprintf(stderr,"[ERR]  Error writing data length\n");
        fprintf(stderr,"[ERR]  Bulk write error (%d, %s)\n", err, strerror(-err));
        return -1;
    }

    /* Now send the data, MAX_TRANSFER bytes at a time. */

    while (bytesleft > 0)
    {
        tosend = MAX(MAX_TRANSFER, bytesleft);

        err = usb_bulk_write(dh, E200R_BULK_TO, p, tosend, TOUT);

        if (err < 0)
        {
            fprintf(stderr,"[ERR]  Error writing data\n");
            fprintf(stderr,"[ERR]  Bulk write error (%d, %s)\n", err, strerror(-err));
            return -1;
        }

        p += tosend;
        bytesleft -= tosend;
    }

    return 0;
}


/* The main function */

void do_patching(void)
{
    struct usb_bus *busses;
    struct usb_bus *bus;
    struct usb_device *tmp_dev;
    struct usb_device *dev = NULL;
    usb_dev_handle *dh;
    int err;

    fprintf(stderr,"[INFO] Searching for E200R\n");
 
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
            if (tmp_dev->descriptor.idVendor == E200R_VENDORID &&
                tmp_dev->descriptor.idProduct == E200R_PRODUCTID ) {

                dev = tmp_dev;
                goto found;

            }
        }
    }

    if (dev == NULL) {
        fprintf(stderr, "[ERR]  E200R device not found.\n");
        fprintf(stderr, "[ERR]  Ensure your E200R is in manufacturing mode and run e200rpatcher again.\n");
        return;
    }

found:
    if ( (dh = usb_open(dev)) == NULL) {
        fprintf(stderr,"[ERR]  Unable to open E200R device.\n");
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

    fprintf(stderr,"[INFO] Found E200R, uploading patching application.\n");

    /* Now we can transfer the application to the device. */ 

    if (upload_app(dh) < 0)
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
void print_usage(void)
{
    fprintf(stderr,"Usage: e200rpatcher [options]\n");
    fprintf(stderr,"Options:\n");
    fprintf(stderr,"  -s,  --silent\t\tDont display instructions\n");
}

int main(int argc, char* argv[])
{
    char input[4];
    int silent = 0;
    int i;
    
    /* check args */
    if ((argc > 1) && ((strcmp(argv[1],"-h")==0) || (strcmp(argv[1],"--help")==0))) {
        print_usage();
        return 1;
    }
    for (i=1;i<argc;i++)
    {
        if (!strcmp(argv[i], "--silent") || !strcmp(argv[i], "-s"))
            silent = 1;
    }

    printf("e200rpatcher v" VERSION " - (C) 2007 Jonathan Gordon & Dave Chapman\n");
    printf("This is free software; see the source for copying conditions.  There is NO\n");
    printf("warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n\n");

    if (!silent)
    {
        printf("Attach your E200R in \"manufacturing mode\" as follows:\n");
        printf("   1) Power-off your E200R\n");
        printf("   2) Turn ON the lock/hold switch\n");
        printf("   3) Press and hold the SELECT button and whilst it is held down,\n");
        printf("      attach your E200R to your computer via USB\n");
        printf("   4) After attaching to USB, keep the SELECT button held for 10 seconds.\n");
        printf("\n");
        printf("NOTE: If your E200R starts in the normal Sansa firmware, you have\n");
        printf("      failed to enter manufacturing mode and should try again at step 1).\n\n");
    
        printf("[INFO] Press Enter to continue:");
        fgets(input, 4, stdin);
    }
    do_patching();

    if (!silent)
    {
        printf("[INFO] Press ENTER to exit: ");
        fgets(input, 4, stdin);
    }

    return 0;
}
