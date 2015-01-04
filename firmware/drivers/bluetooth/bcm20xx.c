/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2015 Lorenzo Miori
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

#include "bluetooth.h"
#include "bluetooth_transport.h" // TODO violation of the rule! hci should have everything needed (upper layer)
#include "bluetooth_hci.h"
#include "bcm20xx.h" /* vendor commands */
#include "panic.h"
#include "stddef.h"
#include "string.h"

#define LOGF_ENABLE
#include "logf.h"

char current_bdaddr[6];

int bcm20xx_init(void)
{
    int scount, count;
    /* 255 max data + 4 header */
    char buf[259];

    scount = hci_write_command_pkt(OGF_HOST_CTL, OCF_RESET, NULL, 0);
    count = hci_read_event(buf, 4);
    if (count < 0)
    {
        logf("Unable to send HCI_OP_RESET %d-%d", scount, count);
    }

    scount = hci_write_command_pkt(OGF_INFO_PARAM, OCF_READ_LOCAL_VERSION, NULL, 0);
    count = hci_read_event(buf, 4);
    if (count < 0)
    {
        logf("Unable to send HCI_OP_READ_LOCAL_VERSION %d-%d", scount, count);
    }

    scount = hci_write_command_pkt(OGF_INFO_PARAM, OCF_READ_LOCAL_COMMANDS, NULL, 0);
    count = hci_read_event(buf, 4);
    if (count < 0)
    {
        logf("Unable to send HCI_OP_READ_LOCAL_COMMANDS %d-%d", scount, count);
    }

    bcm20xx_speed(115200); // TODO hardcoded for testing

}

int bcm20xx_close(void)
{
    
}

int bcm20xx_reset(void)
{
    return bcm20xx_init();
}

int bcm20xx_set_bdaddr(char* bdaddr[6])
{
    int count;
    /* 255 max data + 4 header */
    char buf[259];

    memcpy(&current_bdaddr, bdaddr, 6);
    hci_write_command_pkt(OGF_VENDOR_CMD, HCI_VENDOR_SET_BDADDR, (char*)bdaddr, 6);
    count = hci_read_event(buf, 10);
    if (count < 0)
    {
        logf("Unable to send HCI_VENDOR_SET_BDADDR %d", count);
    }
}

int bcm20xx_get_bdaddr(char* bdaddr[6])
{
    // TODO is there a command to read the bt addr?
//    return &current_bdaddr;
}

//#ifdef SAMSUNGYPR1
// yep we did it, let's see if there is a better way ...
// the target might define something like nice_pointer_to_be_freed bcm20xx_patchram_provide(void)
// which every target can define. The goal is to read the patchram directly from the OF, patch it
// and free.

//#endif

#ifdef SAMSUNGYPR1
void bcm20xx_patchram_provide(char* buffer, char* len)
{
    
}
#else
void bcm20xx_patchram_provide(char* buffer, char* len)
{
    (void)buffer;
    (void)len;
}
#endif

static void bcm20xx_patchram_upload(void)
{
//    char buf[50*1024];
//    bcm20xx_patchram_provide(ptr, sizeof(buf));
    
    // do the magic
}

static void bcm20xx_speed(int speed)
{
    int count;
    /* 255 max data + 4 header */
    char buf[259];
    char data[2];

    /* Select the baud rate */
    switch (speed) {
    case 57600:
        data[0] = 0x00;
        data[1] = 0xe6;
        break;
    case 230400:
        data[0] = 0x22;
        data[1] = 0xfa;
        break;
    case 460800:
        data[0] = 0x22;
        data[1] = 0xfd;
        break;
    case 921600:
        data[0] = 0x55;
        data[1] = 0xff;
        break;
    default:
        /* Default is 115200 */
        data[0] = 0x00;
        data[1] = 0xf3;
        break;
    }

    hci_write_command_pkt(OGF_VENDOR_CMD, HCI_VENDOR_SET_BAUDRATE, data, 2);
    count = hci_read_event(buf, 6);
    if (count < 0)
    {
        logf("Unable to send HCI_OP_READ_LOCAL_COMMANDS %d", count);
    }
}

static void bcm20xx_pcm(int speed)
{
    
}

