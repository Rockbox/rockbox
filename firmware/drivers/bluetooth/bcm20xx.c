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

// this has to receive transport information (i.e. read/write perhaps)

#include "bluetooth_transport.h" // TODO violation of the rule! hci should have everything needed (upper layer)
#include "bluetooth_hci.h"
#include "bcm20xx.h" /* vendor commands */
#include "panic.h"

// struct my_fancy_transport {
//     read = **,
//     write = **,
//     init
//     etc
// }

// ... so that a driver remains cross-platform!

// => the goal is to avoid as many IFDEFS as possible :)

int bcm20xx_init(void)
{
    int count;
    char buf[512]; // TODO adjust me ;)

    hci_write(HCI_COMMAND_PKT, HCI_OP_RESET);
    count = hci_read_event(buf);
    if (count < 0)
    {
        panicf("Unable to send HCI_OP_RESET");
    }
    
    hci_write(HCI_COMMAND_PKT, HCI_OP_READ_LOCAL_VERSION);
    count = hci_read_event(buf);
    if (count < 0)
    {
        panicf("Unable to send HCI_OP_READ_LOCAL_VERSION");
    }
//     /* Read the local supported commands info */
//     memset(cmd, 0, sizeof(cmd));
//     memset(resp, 0, sizeof(resp));
//     cmd[0] = HCI_COMMAND_PKT;
//     cmd[1] = 0x02;
//     cmd[2] = 0x10;
//     cmd[3] = 0x00;
    
    hci_write(HCI_COMMAND_PKT, 0x0210);
    count = hci_read_event(buf);
    if (count < 0)
    {
        panicf("Unable to send HCI_OP_READ_LOCAL_COMMANDS");
    }

    /* Set the baud rate */
    char cmd[30];
    memset(cmd, 0, sizeof(cmd));
//    cmd[0] = HCI_COMMAND_PKT;
    cmd[1] = 0x18;
    cmd[2] = 0xfc;
    cmd[3] = 0x02;

    switch (u->speed) {
    case 57600:
        cmd[4] = 0x00;
        cmd[5] = 0xe6;
        break;
    case 230400:
        cmd[4] = 0x22;
        cmd[5] = 0xfa;
        break;
    case 460800:
        cmd[4] = 0x22;
        cmd[5] = 0xfd;
        break;
    case 921600:
        cmd[4] = 0x55;
        cmd[5] = 0xff;
        break;
    default:
        /* Default is 115200 */
        cmd[4] = 0x00;
        cmd[5] = 0xf3;
        break;
    }
    fprintf(stderr, "Baud rate parameters: DHBR=0x%2x,DLBR=0x%2x\n",
        cmd[4], cmd[5]);

    hci_write(HCI_COMMAND_PKT, cmd);
    count = hci_read_event(buf);
    if (count < 0)
    {
        panicf("Unable to send HCI_OP_READ_LOCAL_COMMANDS");
    }
}

int bcm20xx_close(void)
{
    
}

int bcm20xx_reset(void)
{
    return bcm20xx_init();
}

int bcm20xx_set_bdaddr(char[6] bdaddr)
{
    hci_write_data(HCI_COMMAND_PKT, HCI_VENDOR_SET_BDADDR, &bdaddr);
    count = hci_read_event(buf);
    if (count < 0)
    {
        panicf("Unable to send HCI_VENDOR_SET_BDADDR");
    }
}

int bcm20xx_get_bdaddr(char[6]* bdaddr)
{
    
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
    char buf[50*1024];
    bcm20xx_patchram_provide(&buf, sizeof(buf));
    
    // do the magic
}

static void bcm20xx_speed(int speed)
{
    
}

static void bcm20xx_pcm(int speed)
{
    
}

static void bcm20xx_speed(int speed)
{
    
}
