/*
	LPCUSB, an USB device driver for LPC microcontrollers	
	Copyright (C) 2006 Bertrik Sikken (bertrik@sikken.nl)

	This library is free software; you can redistribute it and/or
	modify it under the terms of the GNU Lesser General Public
	License as published by the Free Software Foundation; either
	version 2.1 of the License, or (at your option) any later version.

	This library is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
	Lesser General Public License for more details.

	You should have received a copy of the GNU Lesser General Public
	License along with this library; if not, write to the Free Software
	Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

/*
	Simple benchmarking application.
	
	It talks with the 'custom' device application on the LPC214x through
	libusb.

        2007-11-01: Some minor modifications by <bjorn@haxx.se>

*/

#include <stdio.h>
#include <unistd.h>
#include <sys/timeb.h>

#include "usb.h"

// types
#ifndef MIN
#define MIN(a,b)	((a)<(b)?(a):(b))
#endif
typedef unsigned int U32;
typedef unsigned char U8;

#define MAX_TIME 3000

static unsigned char abData[16384];

// USB device specific definitions
#define VENDOR_ID	0x0781
#define PRODUCT_ID	0x7450

#define	BM_REQUEST_TYPE		(2<<5)
#define BULK_IN_EP			0x82
#define BULK_OUT_EP			0x05

// this structure should match with the expectations of the 'custom' device!
typedef struct {
	U32		dwAddress;
	U32		dwLength;
} TMemoryCmd;


static struct usb_device * find_device(int iVendor, int iProduct)
{
	struct usb_bus	*usb_bus;
	struct usb_device *dev;	
	
	for (usb_bus = usb_get_busses(); usb_bus; usb_bus = usb_bus->next) {
		for (dev = usb_bus->devices; dev; dev = dev->next) {
			if ((dev->descriptor.idVendor == iVendor) && 
				(dev->descriptor.idProduct == iProduct)) {
				return dev;
			}
		}
	}
	return NULL;
}


static struct timeb start;

static void starttimer(void)
{
	ftime(&start);
}

static int stoptimer(void)
{
	struct timeb now;
	
	ftime(&now);
	return 1000 * (now.time - start.time) + now.millitm - start.millitm;
}


int main(void)
{
    const int blocksize[] = { 128, 512 };
	struct usb_device *dev;	
	struct usb_dev_handle *hdl;
	int i, j;
	U32 dwBlockSize, dwChunk, dwBytes;
	TMemoryCmd MemCmd;
	int iTimer;

	usb_init();
	usb_find_busses();
	usb_find_devices();
	
        for (i=0; i<sizeof abData/4; i++)
            ((unsigned int*)abData)[i] = i;

	dev = find_device(VENDOR_ID, PRODUCT_ID);
	if (dev == NULL) {
		fprintf(stderr, "device not found\n");
		return -1;
	}
	
	hdl = usb_open(dev);
	
	i = usb_set_configuration(hdl, 1);
	if (i < 0) {
		fprintf(stderr, "usb_set_configuration failed\n");
	}
	
	i = usb_claim_interface(hdl, 0);
	if (i < 0) {
		fprintf(stderr, "usb_claim_interface failed %d\n", i);
		return -1;
	}


	// read some data
	for (j = 0; j < 1; j++) {
                dwBlockSize = blocksize[j];
		fprintf(stderr, "Testing blocksize %5d\n", dwBlockSize);

#if 1
		fprintf(stderr, "* write:");
		// send a vendor request for a write
		MemCmd.dwAddress = 0;
		MemCmd.dwLength = 20 * 1024;
		i = usb_control_msg(hdl, BM_REQUEST_TYPE, 0x02, 20, 1024, NULL, 0, 1000);
		if (i < 0) {
			fprintf(stderr, "usb_control_msg failed %d\n", i);
                        break;
		}
		dwBytes = 0;
		starttimer();
		while (MemCmd.dwLength > 0) {
			dwChunk = MIN(dwBlockSize, MemCmd.dwLength);
			i = usb_bulk_write(hdl, 0x01, (char *)abData, dwChunk, 2000);
			if (i < 1) {
				fprintf(stderr, "usb_bulk_write failed %d\n", i);
				break;
			}
			MemCmd.dwLength -= dwChunk;
			dwBytes += dwBlockSize;
			if (stoptimer() > MAX_TIME) {
				break;
			}
                        ((unsigned int*)abData)[0]++;
                }
                if (i<0)
                    break;
		iTimer = stoptimer();
                if (iTimer)
                    fprintf(stderr, " %7d bytes in %d ms = %d kB/s\n", dwBytes, iTimer, dwBytes / iTimer);
		// stdout
		printf("%d,%d,%d\n", dwBlockSize, dwBytes, iTimer);
#endif
#if 1
		fprintf(stderr, "* read :");
		// send a vendor request for a read
		MemCmd.dwAddress = 0;
		MemCmd.dwLength = 20 * 1024;
		i = usb_control_msg(hdl, BM_REQUEST_TYPE, 0x01, 20, 1024, NULL, 0, 1000);
		if (i < 0) {
			fprintf(stderr, "usb_control_msg failed %d\n", i);
                        break;
		}
		dwBytes = 0;
		starttimer();
		while (MemCmd.dwLength > 0) {
			dwChunk = MIN(dwBlockSize, MemCmd.dwLength);
			i = usb_bulk_read(hdl, 0x82, (char *)abData, dwChunk, 2000);
			if (i < 1) {
				fprintf(stderr, "usb_bulk_read failed %d\n", i);
				break;
			}
			MemCmd.dwLength -= dwChunk;
			dwBytes += dwBlockSize;
			if (stoptimer() > MAX_TIME) {
				break;
			}
		}
                if (i<0)
                    break;
		iTimer = stoptimer();
                if (iTimer)
                    fprintf(stderr, " %7d bytes in %d ms = %d kB/s\n", dwBytes, iTimer, dwBytes / iTimer);
		// stdout
		printf("%d,%d,%d\n", dwBlockSize, dwBytes, iTimer);
#endif
	}


	usb_release_interface(hdl, 0);
	usb_close(hdl);

	return 0;
}

