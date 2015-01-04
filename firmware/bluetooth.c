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

#include "button.h"
#include "font.h"
#include "action.h"
#include "list.h"
#include "bluetooth.h"
#include "bluetooth_hw.h"
#include "bluetooth_hci.h"
#include "bluetooth_transport.h"
#include "string.h"
#include "stdio.h"
#include "stdbool.h"

#define LOGF_ENABLE
#include "logf.h"
#include "errno.h"

int init = 0;

// TODO temporary testing ;)
void bt_init(void)
{
    /* Start the hardware layer */
    bluetooth_hw_init();
    /* Start the transport layer */
    transport_init();
    bluetooth_hw_power(true);
    sleep(HZ/4);
    bluetooth_hw_reset(true);
    sleep(HZ/4);
    /* Start the HCI layer */
    bcm20xx_init();
}


/* Write Command Packet
 * ogf -> OpCode Group Field
 * ocf -> OpCode Command Field
 * parameters -> pointer to struct
 * len -> length of parameters
 * 
 * Returns the number of bytes written. -1 on failure
 */
int hci_write_command_pkt(int ogf, int ocf, unsigned char* parameters, int len)
{
    /* 5.4.1 HCI Command Packet Max size (255 data + 4 header) */
    char buf[259];
    char* ptr = buf;
//#ifdef BT_DBG
    char sbuf[128];
    sbuf[0] = '\0';
//#endif

    /* Force param length to be zero if NULL... */
    if (parameters == NULL)
    {
        len = 0;
    }

    /* Type of packet */
    *ptr = HCI_COMMAND_PKT;
    ptr++;
    /* (packed) Opcode */
    *ptr = ocf & 0x03ff;
    ptr++;
    *ptr = (ogf << 10) >> 8;
    ptr++;
    /* Extra parameters */
    *ptr = len;
    ptr++;
    for (int i = 0; i < len; i++)
    {
        *ptr = *(parameters + i);
        ptr++;
    }

    logf("**HCI WRITE COMMAND**");
    logf("OPCODE CMD: OGF %02X OCF %02X", ogf, ocf);
    for (int i = 0; i < 4; i++)
    {
        sprintf(sbuf, "%s%s%02X", sbuf, (i > 0) ? ":" : "", buf[i]);
    }
    logf("%s", sbuf);

    if (len != 0)
    {
        sprintf(sbuf, "RAW PARAMS: ");
        for (int i = 4; i < len + 4; i++)
        {
            sprintf(sbuf, "%s%s%02X", sbuf, (i > 0) ? ":" : "", buf[i]);
        }
        logf("%s", sbuf);
    }

    return transport_write(buf, ptr - buf);
}

// TODO THIS BELOW (read_event) COMES FROM BLUEZ MOMENTARILY, BEFORE MY OWN IMPLEMENTATION

/*
 * Read an HCI event from the given file descriptor.
 */
int hci_read_event(unsigned char* buf, int size)
{
    int remain, r;
    int count = 0;

    if (size <= 0)
        return -1;

    /* The first byte identifies the packet type. For HCI event packets, it
     * should be 0x04, so we read until we get to the 0x04. */
    while (1) {
        r = transport_read(buf, 1);
        if (r <= 0)
            return -1;
        if (buf[0] == 0x04)
            break;
    }
    count++;

    /* The next two bytes are the event code and parameter total length. */
    while (count < 3) {
        r = transport_read(buf + count, 3 - count);
        if (r <= 0)
            return -1;
        count += r;
    }

    /* Now we read the parameters. */
    if (buf[2] < (size - 3))
        remain = buf[2];
    else
        remain = size - 3;

    while ((count - 3) < remain) {
        r = transport_read(buf + count, remain - (count - 3));
        if (r <= 0)
            return -1;
        count += r;
    }

    return count;
}

/** Bluetooth HCI debug screen **/

// TBD current BT name, address, visibility, nearby devices etc

/** Bluetooth controller debug screen **/

// TBD, actually will belong to the controller driver itself...

/** Bluetooth Transport debug screen **/

// TBD, serial/usb/spi connection state, speed, tx/rx rate etc

/** Bluetooth HAL debug screen **/

static const char* bluetooth_hw_dgb_entry(int selected_item, void* data,
                                    char* buffer, size_t buffer_len)
{
    (void)data;
    switch(selected_item) {
        case 0:
            snprintf(buffer, buffer_len, "HW Power: %s",
                     bluetooth_hw_is_powered() ? "ON" : "OFF");
            break;
        case 1:
            snprintf(buffer, buffer_len, "HW Reset: %s",
                     bluetooth_hw_is_reset() ? "ON" : "OFF");
            break;
        case 2:
            snprintf(buffer, buffer_len, "HW Awake: %s",
                     bluetooth_hw_is_awake() ? "ON" : "OFF");
            break;
        case 3:
            snprintf(buffer, buffer_len, "BT INIT");
            break;
        case 4:
            snprintf(buffer, buffer_len, "BCM INIT");
            break;
        default:
            break;
    }
    return buffer;
}

static void bluetooth_hw_dbg_action(int item)
{
    switch(item) {
        case 0:
            bluetooth_hw_power(!bluetooth_hw_is_powered());
            break;
        case 1:
            bluetooth_hw_reset(!bluetooth_hw_is_reset());
            break;
        case 2:
            bluetooth_hw_awake(!bluetooth_hw_is_awake());
            break;
        case 3:
            bt_init();
            break;
        case 4:
            bcm20xx_init();
            break;
         default:
            break;
    }
}

bool bluetooth_hw_dbg_screen(void)
{
    struct gui_synclist lists;
    int action;
    gui_synclist_init(&lists, bluetooth_hw_dgb_entry, NULL, false, 1, NULL);
    gui_synclist_set_title(&lists, "Bluetooth debug", NOICON);
    gui_synclist_set_icon_callback(&lists, NULL);
    gui_synclist_set_color_callback(&lists, NULL);
    gui_synclist_set_nb_items(&lists, 5);

    if (init == 0)
    {
        bt_init();
        init = 1;
    }
    
    while(1)
    {
        gui_synclist_draw(&lists);
        list_do_action(CONTEXT_STD, HZ, &lists, &action, LIST_WRAP_UNLESS_HELD);

        if(action == ACTION_STD_CANCEL)
            return false;

        if(action == ACTION_STD_OK) {
            int item = gui_synclist_get_sel_pos(&lists);
            bluetooth_hw_dbg_action(item);
        }

        yield();
    }

    return true;
}
