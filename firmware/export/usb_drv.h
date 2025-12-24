/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 by Björn Stenberg
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
#ifndef _USB_DRV_H
#define _USB_DRV_H
#include "usb_ch9.h"
#include "kernel.h"

/** USB initialisation flow:
 * usb_init()
 *    -> usb_init_device()
 *       -> [soc specific one-time init]
 *       -> usb_drv_startup()
 * .....
 * [USB is plugged]
 * usb_enable(true)
 *    -> soc specific controller/clock init
 *    -> usb_core_init()
 *       -> usb_drv_init()
 *          -> usb_drv_int_enable(true)  [only if controller needs soc specific code for interrupt]
 *       -> for each usb driver, driver.init()
 * #ifdef USB_DETECT_BY_REQUEST
 *   [rockbox waits until first control request before proceeding]
 * #endif
 * [rockbox decides which usb drivers to enable, based on user preference and buttons]
 * -> if not exclusive mode, usb_attach()
 * -> if exclusive mode, usb_attach() call be called at any point starting from now
 *    (but after threads have acked usb mode and disk have been unmounted)
 * for each enabled driver
 *    -> driver.request_endpoints()
 *    -> driver.set_first_interface()
 * [usb controller/core start answering requests]
 * .....
 * [USB is unplugged]
 * usb_enable(false)
 *    -> usb_core_exit()
 *       -> for each enabled usb driver, driver.disconnect()
 *       -> usb_drv_exit()
 *          -> usb_drv_int_enable(false)  [ditto]
 *    -> soc specific controller/clock deinit */

/* endpoint allocation:
 * there are two ways to implement endpoint allocation.
 * 1. define usb_drv_ep_specs and usb_drv_ep_specs_flags in the driver.
 *    this is the simplest option when the types accepted by each endpoint are mutually independent.
 *    these variables are set only once during driver initialization and should not be modified afterward.
 * 2. define usb_drv_ep_alloc_ctx and implement usb_drv_ep_reset_alloc_ctx and usb_drv_ep_allocate in the driver.
 *    if the available endpoint types change based on allocation status,
 *    these functions can be overridden to allow the driver to track the endpoint state.
 * */

#ifndef usb_drv_ep_alloc_ctx
/* option 1 */
#define USB_ENDPOINT_TYPE_ANY  (-1)
#define USB_ENDPOINT_TYPE_NONE (-2)
struct usb_drv_ep_spec {
    int8_t type[2]; /* USB_ENDPOINT_TYPE_{ANY,NONE} USB_ENDPOINT_XFER_* */
};
extern struct usb_drv_ep_spec usb_drv_ep_specs[USB_NUM_ENDPOINTS];

#define USB_ENDPOINT_SPEC_FORCE_IO_TYPE_MATCH (1 << 0)
#define USB_ENDPOINT_SPEC_IO_EXCLUSIVE        (1 << 1)
extern uint8_t usb_drv_ep_specs_flags;

struct usb_drv_ep_alloc_ctx {
    int8_t type[USB_NUM_ENDPOINTS][2];
    int max_packet_size[USB_NUM_ENDPOINTS][2];
};
#else
/* option 2 */
void usb_drv_ep_reset_alloc_ctx(struct usb_drv_ep_alloc_ctx* ctx);
bool usb_drv_ep_allocate(struct usb_drv_ep_alloc_ctx* ctx, int ep, int type, int max_packet_size);
#endif

void usb_drv_ep_init(const struct usb_drv_ep_alloc_ctx* ctx, int ep);
void usb_drv_ep_deinit(const struct usb_drv_ep_alloc_ctx* ctx, int ep);

/* one-time initialisation of the USB driver */
void usb_drv_startup(void);
void usb_drv_int_enable(bool enable); /* Target implemented */
/* enable and initialise the USB controller */
void usb_drv_init(void);
/* stop and disable and the USB controller */
void usb_drv_exit(void);
void usb_drv_int(void); /* Call from target INT handler */
void usb_drv_stall(int endpoint, bool stall,bool in);
bool usb_drv_stalled(int endpoint,bool in);
int usb_drv_send(int endpoint, void* ptr, int length);
int usb_drv_send_nonblocking(int endpoint, void* ptr, int length);
int usb_drv_recv_blocking(int endpoint, void* ptr, int length);
int usb_drv_recv_nonblocking(int endpoint, void* ptr, int length);
void usb_drv_set_address(int address);
void usb_drv_reset_endpoint(int endpoint, bool send);
bool usb_drv_powered(void);
int usb_drv_port_speed(void);
void usb_drv_cancel_all_transfers(void);
void usb_drv_set_test_mode(int mode);
bool usb_drv_connected(void);
#ifdef USB_HAS_ISOCHRONOUS
/* returns the last received frame number (the 11-bit number contained in the last SOF):
 * - full-speed: the host sends one SOF every 1ms (so 1000 SOF/s)
 * - high-speed: the hosts sends one SOF every 125us but each consecutive 8 SOF have the same frame
 *   number
 * thus in all mode, the frame number can be interpreted as the current millisecond *in USB time*. */
int usb_drv_get_frame_number(void);
#endif

/* batched request api is for workloads that perform very high-frequency,
 * performance-sensitive isochronous transactions continuously.
 * this api allows multiple transaction requests to be buffered in udc driver.
 * so that it can maximize the controller's performance. */

/* called when usb system requires more buffer */
typedef void(*usb_drv_batch_get_more)(const void** ptr, size_t* len);

/* prepare request queue */
int usb_drv_batch_init(int ep, usb_drv_batch_get_more get_more);
/* destroy request queue */
int usb_drv_batch_deinit(void);
/* start processing */
int usb_drv_batch_start(void);
/* stop processing */
int usb_drv_batch_stop(void);

/* USB_STRING_INITIALIZER(u"Example String") */
#define USB_STRING_INITIALIZER(S) { \
    sizeof(struct usb_string_descriptor) + sizeof(S) - sizeof(*S), \
    USB_DT_STRING, \
    S \
}

#endif /* _USB_DRV_H */
