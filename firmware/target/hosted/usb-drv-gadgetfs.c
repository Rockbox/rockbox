/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2014 - 2015 Lorenzo Miori
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
#include "config.h"
#include "stdbool.h"
#include "system.h"
#include "kernel.h"
#include "usb.h"
#include "sc900776.h"
#include "unistd.h"
#include "fcntl.h"
#include "stdio.h"
#include "sys/ioctl.h"
#include "stdlib.h"

#define LOGF_ENABLE

#include "logf.h"
#include "usb.h"
#include "usb_drv.h"
#include "usb_core.h"

#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <memory.h>

#include <signal.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/poll.h>

#include <asm/byteorder.h>

/* TODO make this to be used from kernel (usb/gadgetfs.h)
 the only problem is the conflicting ch9 definitions...
 */
#include "gadgetfs.h"

/*
 
#### Some random descriptors for testing #######
 
 */


#define    STRINGID_MFGR        1
#define    STRINGID_PRODUCT    2
#define    STRINGID_SERIAL        3
#define    STRINGID_CONFIG        4
#define    STRINGID_INTERFACE    5

static struct usb_interface_descriptor
source_sink_intf = {
    .bLength =        sizeof source_sink_intf,
    .bDescriptorType =    USB_DT_INTERFACE,

    .bInterfaceClass =    USB_CLASS_VENDOR_SPEC,
    .iInterface =        STRINGID_INTERFACE,
};

/* Full speed configurations are used for full-speed only devices as
 * well as dual-speed ones (the only kind with high speed support).
 */

static struct usb_endpoint_descriptor
fs_source_desc = {
    .bLength =        USB_DT_ENDPOINT_SIZE,
    .bDescriptorType =    USB_DT_ENDPOINT,

    .bmAttributes =        USB_ENDPOINT_XFER_BULK,
    /* NOTE some controllers may need FS bulk max packet size
     * to be smaller.  it would be a chip-specific option.
     */
    .wMaxPacketSize =    __constant_cpu_to_le16 (64),
};

static struct usb_endpoint_descriptor
fs_sink_desc = {
    .bLength =        USB_DT_ENDPOINT_SIZE,
    .bDescriptorType =    USB_DT_ENDPOINT,

    .bmAttributes =        USB_ENDPOINT_XFER_BULK,
    .wMaxPacketSize =    __constant_cpu_to_le16 (64),
};

/* some devices can handle other status packet sizes */
#define STATUS_MAXPACKET    8
#define    LOG2_STATUS_POLL_MSEC    3

static struct usb_endpoint_descriptor
fs_status_desc = {
    .bLength =        USB_DT_ENDPOINT_SIZE,
    .bDescriptorType =    USB_DT_ENDPOINT,

    .bmAttributes =        USB_ENDPOINT_XFER_INT,
    .wMaxPacketSize =    __constant_cpu_to_le16 (STATUS_MAXPACKET),
    .bInterval =        (1 << LOG2_STATUS_POLL_MSEC),
};

static const struct usb_endpoint_descriptor *fs_eps [3] = {
    &fs_source_desc,
    &fs_sink_desc,
    &fs_status_desc,
};


/* High speed configurations are used only in addition to a full-speed
 * ones ... since all high speed devices support full speed configs.
 * Of course, not all hardware supports high speed configurations.
 */

static struct usb_endpoint_descriptor
hs_source_desc = {
    .bLength =        USB_DT_ENDPOINT_SIZE,
    .bDescriptorType =    USB_DT_ENDPOINT,

    .bmAttributes =        USB_ENDPOINT_XFER_BULK,
    .wMaxPacketSize =    __constant_cpu_to_le16 (512),
};

static struct usb_endpoint_descriptor
hs_sink_desc = {
    .bLength =        USB_DT_ENDPOINT_SIZE,
    .bDescriptorType =    USB_DT_ENDPOINT,

    .bmAttributes =        USB_ENDPOINT_XFER_BULK,
    .wMaxPacketSize =    __constant_cpu_to_le16 (512),
    .bInterval =        1,
};

static struct usb_endpoint_descriptor
hs_status_desc = {
    .bLength =        USB_DT_ENDPOINT_SIZE,
    .bDescriptorType =    USB_DT_ENDPOINT,

    .bmAttributes =        USB_ENDPOINT_XFER_INT,
    .wMaxPacketSize =    __constant_cpu_to_le16 (STATUS_MAXPACKET),
    .bInterval =        LOG2_STATUS_POLL_MSEC + 3,
};

static const struct usb_endpoint_descriptor *hs_eps [] = {
    &hs_source_desc,
    &hs_sink_desc,
    &hs_status_desc,
};

#ifndef USB_MAX_CURRENT
#define USB_MAX_CURRENT 500
#endif

/*-------------------------------------------------------------------------*/
/* USB protocol descriptors: */

static struct usb_device_descriptor __attribute__((aligned(2)))
                                          device_desc=
{
    .bLength            = sizeof(struct usb_device_descriptor),
    .bDescriptorType    = USB_DT_DEVICE,
#ifndef USB_NO_HIGH_SPEED
    .bcdUSB             = 0x0200,
#else
    .bcdUSB             = 0x0110,
#endif
    .bDeviceClass       = USB_CLASS_PER_INTERFACE,
    .bDeviceSubClass    = 0,
    .bDeviceProtocol    = 0,
    .bMaxPacketSize0    = 64,
    .idVendor           = USB_VENDOR_ID,
    .idProduct          = USB_PRODUCT_ID,
    .bcdDevice          = 0x0100,
    .iManufacturer      = 1,
    .iProduct           = 2,
    .iSerialNumber      = 3,
    .bNumConfigurations = 1
} ;

static struct usb_config_descriptor __attribute__((aligned(2)))
                                    config =
{
    .bLength             = sizeof(struct usb_config_descriptor),
    .bDescriptorType     = USB_DT_CONFIG,
    .wTotalLength        = 0, /* will be filled in later */
    .bNumInterfaces      = 1,
    .bConfigurationValue = 1,
    .iConfiguration      = 0,
    .bmAttributes        = USB_CONFIG_ATT_ONE | USB_CONFIG_ATT_SELFPOWER,
    .bMaxPower           = (USB_MAX_CURRENT + 1) / 2, /* In 2mA units */
};

static const struct usb_qualifier_descriptor __attribute__((aligned(2)))
                                             qualifier_descriptor =
{
    .bLength            = sizeof(struct usb_qualifier_descriptor),
    .bDescriptorType    = USB_DT_DEVICE_QUALIFIER,
    .bcdUSB             = 0x0200,
    .bDeviceClass       = 0,
    .bDeviceSubClass    = 0,
    .bDeviceProtocol    = 0,
    .bMaxPacketSize0    = 64,
    .bNumConfigurations = 1
};

static const struct usb_string_descriptor __attribute__((aligned(2)))
                                    usb_string_iManufacturer =
{
    24,
    USB_DT_STRING,
    {'R', 'o', 'c', 'k', 'b', 'o', 'x', '.', 'o', 'r', 'g'}
};

static const struct usb_string_descriptor __attribute__((aligned(2)))
                                    usb_string_iProduct =
{
    42,
    USB_DT_STRING,
    {'R', 'o', 'c', 'k', 'b', 'o', 'x', ' ',
     'm', 'e', 'd', 'i', 'a', ' ',
     'p', 'l', 'a', 'y', 'e', 'r'}
};

static struct usb_string_descriptor __attribute__((aligned(2)))
                                    usb_string_iSerial =
{
    84,
    USB_DT_STRING,
    {'0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0',
     '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0',
     '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0',
     '0', '0', '0', '0', '0', '0', '0', '0'}
};

/* Generic for all targets */

/* this is stringid #0: languages supported */
static const struct usb_string_descriptor __attribute__((aligned(2)))
                                    lang_descriptor =
{
    4,
    USB_DT_STRING,
    {0x0409} /* LANGID US English */
};

static const struct usb_string_descriptor* const usb_strings[] =
{
   &lang_descriptor,
   &usb_string_iManufacturer,
   &usb_string_iProduct,
   &usb_string_iSerial
};

static char *
build_config (char *cp, const struct usb_endpoint_descriptor **ep)
{
    struct usb_config_descriptor *c;
    int i;

    c = (struct usb_config_descriptor *) cp;

    memcpy (cp, &config, config.bLength);
    cp += config.bLength;
    memcpy (cp, &source_sink_intf, source_sink_intf.bLength);
    cp += source_sink_intf.bLength;

    for (i = 0; i < source_sink_intf.bNumEndpoints; i++) {
        memcpy (cp, ep [i], USB_DT_ENDPOINT_SIZE);
        cp += USB_DT_ENDPOINT_SIZE;
    }
    c->wTotalLength = __cpu_to_le16 (cp - (char *) c);
    return cp;
}


static int write_descriptors (int fd)
{
    char        buf [4096], *cp = &buf [0];
    int        status;

    *(__u32 *)cp = 0;    /* tag for this format */
    cp += 4;

    /* write full then high speed configs */
    cp = build_config (cp, fs_eps);
    if (1)//HIGHSPEED)
        cp = build_config (cp, hs_eps);

    /* and device descriptor at the end */
    memcpy(cp, &device_desc, sizeof device_desc);
    cp += sizeof device_desc;

    status = write(fd, &buf [0], cp - &buf [0]);
    if (status < 0) {
        logf("write dev descriptors");
        close (fd);
        return status;
    } else if (status != (cp - buf)) {
        logf("dev init, wrote %d expected %d",
                status, cp - buf);
        close (fd);
        return -EIO;
    }
    return fd;
}

/**
 * 
 * END OF RANDOM DESCRIPTORS
 * 
 */

// to be placed in the SIMULATOR config file
//#define DUMMY_HCD

// TODO for the moment I suggest to keep these values here
#if defined(SAMSUNG_YPR0)
#define DEVNAME "/dev/gadget/fsl-usb2-udc"
#elif defined(DUMMY_HCD)
#define DEVNAME "/dev/gadget/dummy_udc"
#endif

int fd = -1;

typedef struct usb_endpoint
{
    bool allocated[2];
    int fd[2];
    int stall[2];
    short type[2];
    short max_pkt_size[2];
} usb_endpoint_t;
static usb_endpoint_t endpoints[USB_NUM_ENDPOINTS];

/* control thread, handles main event loop  */

#define    NEVENT        5

static enum usb_device_speed current_speed;
int connected = 0;

static const char* speed (enum usb_device_speed s)
{
    switch (s) {
        case USB_SPEED_LOW:
            return "low speed";
        case USB_SPEED_FULL:
            return "full speed";
        case USB_SPEED_HIGH:
            return "high speed";
        default:
            return "UNKNOWN speed";
    }
}

/** INTERNAL FUNCTIONS **/

#define XFER_DIR_STR(dir) ((dir) ? "IN" : "OUT")
#define XFER_TYPE_STR(type) \
    ((type) == USB_ENDPOINT_XFER_CONTROL ? "CTRL" : \
     ((type) == USB_ENDPOINT_XFER_ISOC ? "ISOC" : \
      ((type) == USB_ENDPOINT_XFER_BULK ? "BULK" : \
       ((type) == USB_ENDPOINT_XFER_INT ? "INTR" : "INVL"))))

static void log_ep(int ep_num, int ep_dir, char* prefix)
{
    usb_endpoint_t* endpoint = &endpoints[ep_num];

    logf("%s: ep%d %s %s %d", prefix, ep_num, XFER_DIR_STR(ep_dir),
            XFER_TYPE_STR(endpoint->type[ep_dir]),
            endpoint->max_pkt_size[ep_dir]);
}

void reset_endpoints(void) {
    // TODO do some meaningful init, if needed
    memset(&endpoints, 0, sizeof(endpoints));
}

static int ep0_thread_stack[DEFAULT_STACK_SIZE*2];

static void NORETURN_ATTR ep0_thread(void)
{
    int tmp;
    struct usb_gadgetfs_event event[NEVENT];
    int i, nevent;

    /* Use poll() to test that mechanism, to generate
     * activity timestamps, and to make it easier to
     * tweak this code to work without pthreads.  When
     * AIO is needed without pthreads, ep0 can be driven
     * instead using SIGIO.
     */
    struct pollfd        ep0_poll;
    ep0_poll.fd = fd;
    ep0_poll.events = POLLIN | POLLOUT | POLLHUP;

    while (1) {

        sleep(HZ/1);
        fflush(stdout);
        fflush(stderr);

        if (fd == -1) {
            continue;
        }

        /*
         The device doesn't read until someone putting descriptor information
         */

        // HACK TODO WARNING silly trick to use timeout, otherwise rb gets stuck :/
        // I don't see the reason: the blocking call is in its own thread...or?
        tmp = poll(&ep0_poll, 1, 1);

        tmp = read(fd, &event, sizeof event);
        if (tmp < 0) {
//             logf("error reading ep0");
             continue;
        }
        nevent = tmp / sizeof event[0];

        logf("nevent %d", nevent);

        if (nevent > NEVENT) {
            logf("nevent > NEVENT");
        }

        for (i = 0; i < nevent; i++) {
            switch (event [i].type) {
            case GADGETFS_NOP:
                logf("NOP");
                break;
            case GADGETFS_CONNECT:
                connected = 1;
                current_speed = event [i].u.speed;
                logf("CONNECT %s", speed (event [i].u.speed));
                break;
            case GADGETFS_SETUP:
                connected = 1;
//                 handle_control (fd, &event [i].u.setup); (struct usb_ctrlrequest*)
                logf("SETUP req=%d",&event[i].u.setup.bRequest);
                usb_core_control_request(&event[i].u.setup);
                break;
            case GADGETFS_DISCONNECT:
                connected = 0;
                current_speed = USB_SPEED_UNKNOWN;
                logf("DISCONNECT");
                break;
            case GADGETFS_SUSPEND:
                connected = 1;
                logf("SUSPEND");
                break;
            default:
                logf("* unhandled event %d",
                    event [i].type);
            }
        }
    }
}



/** END OF INTERNAL FUNCTIONS **/

void init_device(void)
{

    reset_endpoints();

    create_thread(ep0_thread, ep0_thread_stack, sizeof(ep0_thread_stack), 0,
        "ep0 thread" IF_PRIO(, PRIORITY_BACKGROUND) IF_COP(, CPU));

    /* Get rid of this, polling should be sufficient */
    sleep(8*HZ);

    logf(DEVNAME);
    fd = open(DEVNAME, O_RDWR);
    if (fd < 0) {
        logf("cannot open %s", DEVNAME);
    }

    endpoints[0].fd[DIR_OUT] = endpoints[0].fd[DIR_IN] = fd;
    endpoints[0].type[DIR_OUT] = endpoints[0].type[DIR_IN] = USB_ENDPOINT_XFER_CONTROL;

    logf("init_device finish");

//     struct usb_ctrlrequest fake_request;
//     memset(&fake_request, 0, sizeof(fake_request));
//      
//     usb_core_control_request(&fake_request);

    write_descriptors(fd);
    logf("descriptors written");

    return;
}

void close_device(void) {
    if (close(fd) < 0) {
        logf("close error");
    }
    /* It is important to set this */
    fd = -1;
    reset_endpoints();
}

/**
 * 
 * End of pure gadgetfs stuff
 * 
 */

void usb_drv_startup(void) {
    logf("USB INIT");
}

void usb_drv_init(void) {

//     if (chdir("/dev/gadget") < 0) {
//         logf("/dev/gadget is missing!");
//     }

    init_device();

}
void usb_drv_exit(void) {
    close_device();
}

void usb_drv_int(void) { /* Call from target INT handler */

}


void usb_drv_stall(int endpoint, bool stall, bool in)
{
    int ep_num = EP_NUM(endpoint);
    int tmp = 0;

    logf("%sstall %d", stall ? "" : "un", ep_num);

    /* gadgetfs does stall by issuing a "wrong" I/O call */
    if(in) {
        if (stall) {
            write(endpoints[ep_num].fd[DIR_OUT], &tmp, 0);
            endpoints[ep_num].stall[DIR_OUT] = 1;
        }
        else {
            endpoints[ep_num].stall[DIR_OUT] = 0;
        }
    }
    else {
        if (stall) {
            read(endpoints[ep_num].fd[DIR_IN], &tmp, 0);
            endpoints[ep_num].stall[DIR_IN] = 1;
        }
        else {
            endpoints[ep_num].stall[DIR_IN] = 0;
        }
    }
}

bool usb_drv_stalled(int endpoint, bool in) {

    int ep_num = EP_NUM(endpoint);

    if(in) {
        return endpoints[ep_num].stall[DIR_OUT];
    }
    else {
        return endpoints[ep_num].stall[DIR_IN];
    }
}

int usb_drv_send(int endpoint, void* ptr, int length) {

    int ep_num = EP_NUM(endpoint);

    logf("writing %d", ep_num);
    logf("*data*");
    void* dptr = ptr;
    write(1, dptr, length);

    if (write(endpoints[ep_num].fd[DIR_IN], ptr, length) < 0) {
        logf("error writing %d", ep_num);
        return -1;
    }

    return 0;
}

int usb_drv_send_nonblocking(int endpoint, void* ptr, int length) {
    // TODO not implemented, blocking :P
    return usb_drv_send(endpoint, ptr, length);
}

int usb_drv_recv(int endpoint, void* ptr, int length) {

    int ep_num = EP_NUM(endpoint);

    logf("receiving %d", ep_num);

    if (read(endpoints[ep_num].fd[DIR_OUT], ptr, length) != length) {
        logf("error reading %d", ep_num);
        return -1;
    }

    return 0;
}

void usb_drv_set_address(int address) {
    logf("set address %d", address);
}

void usb_drv_reset_endpoint(int endpoint, bool send) {
    int ep_num = EP_NUM(endpoint);
    int ep_dir = send ? DIR_OUT : DIR_IN;

    log_ep(ep_num, ep_dir, "reset");

    if (fsync(endpoints[ep_num].fd[ep_dir])) {
        logf("error reading %d", ep_num);
    }
}

bool usb_drv_powered(void) {
    return connected;
}

int usb_drv_port_speed(void) {

    logf("get port speed %d", current_speed);

    int rv = 0;

    switch (current_speed) {
        case USB_SPEED_HIGH:
            rv = 1;
            break;
        case USB_SPEED_FULL:
        default:
            rv = 0;
            break;
    }

    return rv;
}

void usb_drv_cancel_all_transfers(void) {
    
}

void usb_drv_set_test_mode(int mode) {
    
}

bool usb_drv_connected(void) {
    logf("connected");
    return connected;
}

int usb_drv_request_endpoint(int type, int dir) {
    
    logf("asking for endpoint %d", type);
    
    int ep_num, ep_dir;
    short ep_type;

    /* Safety */
    ep_dir = EP_DIR(dir);
    ep_type = type & USB_ENDPOINT_XFERTYPE_MASK;

    logf("req: %s %s", XFER_DIR_STR(ep_dir), XFER_TYPE_STR(ep_type));

    /* Find an available ep/dir pair */
    for (ep_num=1;ep_num<USB_NUM_ENDPOINTS;ep_num++) {
        usb_endpoint_t* endpoint=&endpoints[ep_num];
        int other_dir=(ep_dir ? 0:1);

        if (endpoint->allocated[ep_dir])
            continue;

        if (endpoint->allocated[other_dir] &&
                endpoint->type[other_dir] != ep_type) {
            logf("ep of different type!");
            continue;
        }


        endpoint->allocated[ep_dir] = 1;
        endpoint->type[ep_dir] = ep_type;
        endpoint->fd[ep_dir] = open("/dev/gadget/ep%d%s", ep_num, ep_dir ? "in" : "out");

        log_ep(ep_num, ep_dir, "add");
        return (ep_num | (dir & USB_ENDPOINT_DIR_MASK));
    }

// #define USB_ENDPOINT_XFER_CONTROL       0
// #define USB_ENDPOINT_XFER_ISOC          1
// #define USB_ENDPOINT_XFER_BULK          2
// #define USB_ENDPOINT_XFER_INT           3
    return -1;
}

void usb_drv_release_endpoint(int ep) {

    int ep_num = EP_NUM(ep);
    int ep_dir = EP_DIR(ep);

    log_ep(ep_num, ep_dir, "rel");
    endpoints[ep_num].allocated[ep_dir] = 0;

    // do we really need this?!
//     if (close(endpoint_state[ep_num].fd)) {
//         logf("error in closing endpoint %d", ep_num);
//     }
} 
