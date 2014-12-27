/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2012 Lorenzo Miori
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
#include "logf.h"
#include "usb.h"
#include "usb_drv.h"
#include "usb_core.h"

// TODO cleanup of minivet (usb pin ID controller) ... refactor to another file?
// rename to actual chip name

static sMinivet_t rs = { .addr = 0xA, .value = 0 };
static int minivet_dev = -1;

void minivet_init(void)
{
    minivet_dev = open("/dev/minivet", O_RDWR);
    logf("MINIVET: %d", minivet_dev);
}

void minivet_close(void)
{
    if (minivet_dev >= 0) {
        close(minivet_dev);
    }
}

int minivet_ioctl(int request, sMinivet_t *param) {
    return ioctl(minivet_dev, request, param);
}

void usb_init_device(void)
{
    minivet_init();
    /* Start usb detection... */
    usb_start_monitoring();
}

void usb_attach(void)
{
    logf("USB: attached");
}

int usb_detect(void)
{
    /* Check if this register is set...DEVICE_1 */
    if (minivet_ioctl(IOCTL_MINIVET_READ_BYTE, &rs) >= 0) {
        return (rs.value == MINIVET_DEVICETYPE1_USB) ? USB_INSERTED : USB_EXTRACTED;
    }
    return USB_EXTRACTED;
}

void usb_enable(bool on)
{
    if (on)
    {
        logf("USB: connected");
    }
    else
    {
        logf("USB: disconnected");
    }
}


// from now on, everything will be placed in a usb-drv-gadgetfs.c file

void usb_drv_startup(void) {
    logf("USB INIT");
}
//void usb_drv_int_enable(bool enable); /* Target implemented */
void usb_drv_init(void) {
    
}
void usb_drv_exit(void) {
    
}
//void usb_drv_int(void); /* Call from target INT handler */
void usb_drv_stall(int endpoint, bool stall,bool in) {
    
}

bool usb_drv_stalled(int endpoint,bool in) {
    return true;
}

int usb_drv_send(int endpoint, void* ptr, int length) {
    return -1;
}

int usb_drv_send_nonblocking(int endpoint, void* ptr, int length) {
    return -1;
}

int usb_drv_recv(int endpoint, void* ptr, int length) {
    return -1;
}

void usb_drv_ack(struct usb_ctrlrequest* req) {
    
}

void usb_drv_set_address(int address) {
    
}

void usb_drv_reset_endpoint(int endpoint, bool send) {
    
}

bool usb_drv_powered(void) {
    return true;
}

int usb_drv_port_speed(void) {
    return 0;
}

void usb_drv_cancel_all_transfers(void) {
    
}

void usb_drv_set_test_mode(int mode) {
    
}

bool usb_drv_connected(void) {
    return true;
}

int usb_drv_request_endpoint(int type, int dir) {
    return -1;
}

void usb_drv_release_endpoint(int ep) {
    
}
