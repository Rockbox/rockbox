/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id:  $
 *
 * Copyright (C) 2007 by Björn Stenberg
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#ifndef USB_CORE_H
#define USB_CORE_H

#ifndef BOOTLOADER
#define USB_THREAD

#ifdef USE_ROCKBOX_USB
//#define USB_SERIAL
//#define USB_BENCHMARK
#define USB_STORAGE

#else
#define USB_CHARGING_ONLY

#endif /* USE_ROCKBOX_USB */
#else
#define USB_CHARGING_ONLY
#endif /* BOOTLOADER */

#include "usb_ch9.h"

#if defined(CPU_PP)
#define USB_IRAM_ORIGIN   ((unsigned char *)0x4000c000)
#define USB_IRAM_SIZE     ((size_t)0xc000)
#endif


enum {
	USB_CORE_QUIT,
	USB_CORE_TRANSFER_COMPLETION
};

/* endpoints */
enum {
	EP_CONTROL = 0,
#ifdef USB_STORAGE
	EP_MASS_STORAGE,
#endif
#ifdef USB_SERIAL
	EP_SERIAL,
#endif
#ifdef USB_CHARGING_ONLY
	EP_CHARGING_ONLY,
#endif
#ifdef USB_BENCHMARK
	EP_BENCHMARK,
#endif
	NUM_ENDPOINTS
};


/* queue events */
#define USB_TRANSFER_COMPLETE 1

struct queue_msg {
    int endpoint;
    int length;
    void* data;
};

extern int usb_max_pkt_size;

void usb_core_init(void);
void usb_core_exit(void);
void usb_core_control_request(struct usb_ctrlrequest* req);
void usb_core_transfer_complete(int endpoint, bool in, int status, int length);
void usb_core_bus_reset(void);
bool usb_core_data_connection(void);

#endif

