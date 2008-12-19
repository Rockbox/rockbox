/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2008 by Mark Arigo
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

#include <stdlib.h>
#include "config.h"
#include "cpu.h"
#include "system.h"
#include "kernel.h"
#include "system.h"

#define LOGF_ENABLE
#include "logf.h"

/* Driver for the Synaptics Touchpad based on the "Synaptics Modular Embedded
   Protocol: 3-Wire Interface Specification" documentation */

#if defined(MROBE_100)
#define ACK     (GPIOD_INPUT_VAL & 0x1)
#define ACK_HI  GPIOD_OUTPUT_VAL |=  0x1
#define ACK_LO  GPIOD_OUTPUT_VAL &= ~0x1

#define CLK     ((GPIOD_INPUT_VAL & 0x2) >> 1)
#define CLK_HI  GPIOD_OUTPUT_VAL |=  0x2
#define CLK_LO  GPIOD_OUTPUT_VAL &= ~0x2

#define DATA    ((GPIOD_INPUT_VAL & 0x4) >> 2)
#define DATA_HI GPIOD_OUTPUT_EN |= 0x4; GPIOD_OUTPUT_VAL |=  0x4
#define DATA_LO GPIOD_OUTPUT_EN |= 0x4; GPIOD_OUTPUT_VAL &= ~0x4
#define DATA_CL GPIOD_OUTPUT_EN &= ~0x4

#elif defined(PHILIPS_HDD1630)
#define ACK     (GPIOD_INPUT_VAL & 0x80)
#define ACK_HI  GPIOD_OUTPUT_VAL |=  0x80
#define ACK_LO  GPIOD_OUTPUT_VAL &= ~0x80

#define CLK     ((GPIOA_INPUT_VAL & 0x20) >> 5)
#define CLK_HI  GPIOA_OUTPUT_VAL |=  0x20
#define CLK_LO  GPIOA_OUTPUT_VAL &= ~0x20

#define DATA    ((GPIOA_INPUT_VAL & 0x10) >> 4)
#define DATA_HI GPIOA_OUTPUT_EN |= 0x10; GPIOA_OUTPUT_VAL |=  0x10
#define DATA_LO GPIOA_OUTPUT_EN |= 0x10; GPIOA_OUTPUT_VAL &= ~0x10
#define DATA_CL GPIOA_OUTPUT_EN &= ~0x10
#endif

#define LO 0
#define HI 1

#define READ_RETRY      8
#define READ_ERROR     -1

#define MEP_HELLO_HEADER 0x19
#define MEP_HELLO_ID     0x1

#define MEP_READ  0x1
#define MEP_WRITE 0x3

static int syn_wait_clk_change(unsigned int val)
{
    int i;

    for (i = 0; i < 10000; i++)
    {
        if (CLK == val)
            return 1;
    }

    return 0;
}

static inline int syn_get_data(void)
{
    DATA_CL;
    return DATA;
}

static void syn_wait_guest_flush(void)
{
    /* Flush receiving (flushee) state:
       handshake until DATA goes high during P3 stage */
    if (CLK == LO)
    {
        ACK_HI;                  /* P1 -> P2 */
        syn_wait_clk_change(HI); /* P2 -> P3 */
    }

    while (syn_get_data() == LO)
    {
        ACK_HI;                  /* P3 -> P0 */
        syn_wait_clk_change(LO); /* P0 -> P1 */
        ACK_LO;                  /* P1 -> P2 */
        syn_wait_clk_change(HI); /* P2 -> P3 */
    }

    /* Continue handshaking until back to P0 */
    ACK_HI;                      /* P3 -> P0 */
}

static void syn_flush(void)
{
    int i;

    logf("syn_flush...");

    /* Flusher holds DATA low for at least 36 handshake cycles */
    DATA_LO;

    for (i = 0; i < 36; i++)
    {
        syn_wait_clk_change(LO); /* P0 -> P1 */
        ACK_LO;                  /* P1 -> P2 */
        syn_wait_clk_change(HI); /* P2 -> P3 */
        ACK_HI;                  /* P3 -> P0 */
    }

    /* Raise DATA in P1 stage */
    syn_wait_clk_change(LO); /* P0 -> P1 */
    DATA_HI;

    /* After a flush, the flushing device enters a flush-receiving (flushee)
       state */
    syn_wait_guest_flush();
}

static int syn_send_data(int *data, int len)
{
    int i, bit;
    int parity = 0;

    logf("syn_send_data...");

    /* 1. Lower DATA line to issue a request-to-send to guest */
    DATA_LO;

    /* 2. Wait for guest to lower CLK */
    syn_wait_clk_change(LO);

    /* 3. Lower ACK (with DATA still low) */
    ACK_LO;

    /* 4. Wait for guest to raise CLK */
    syn_wait_clk_change(HI);

    /* 5. Send data */
    for (i = 0; i < len; i++)
    {
       logf("  sending byte: %d", data[i]);

        bit = 0;
        while (bit < 8)
        {
            /* 5a. Drive data low if bit is 0, or high if bit is 1 */
            if (data[i] & (1 << bit))
            {
                DATA_HI;
                parity++;
            }
            else
            {
                DATA_LO;
            }
            bit++;

            /* 5b. Invert ACK to indicate that the data bit is ready */
            ACK_HI;

            /* 5c. Wait for guest to invert CLK */
            syn_wait_clk_change(LO);

            /* Repeat for next bit */
            if (data[i] & (1 << bit))
            {
                DATA_HI;
                parity++;
            }
            else
            {
                DATA_LO;
            }
            bit++;

            ACK_LO;

            syn_wait_clk_change(HI);
        }
    }

    /* 7. Transmission termination sequence: */
    /* 7a. Host may put parity bit on DATA. Hosts that do not generate
           parity should set DATA high. Parity is 1 if there's an odd
           number of '1' bits, or 0 if there's an even number of '1' bits. */
    parity = parity % 2;
    if (parity)
    {
        DATA_HI;
    }
    else
    {
        DATA_LO;
    }
    logf("  send parity = %d", parity);

    /* 7b. Raise ACK to indicate that the optional parity bit is ready */
    ACK_HI;

    /* 7c. Guest lowers CLK */
    syn_wait_clk_change(LO);

    /* 7d. Pull DATA high (if parity bit was 0) */
    DATA_HI;

    /* 7e. Lower ACK to indicate that the stop bit is ready */
    ACK_LO;

    /* 7f. Guest raises CLK */
    syn_wait_clk_change(HI);

    /* 7g. If DATA is low, guest is flushing this transfer. Host should
           enter the flushee state. */
    if (syn_get_data() == LO)
    {
        logf("  module flushing");

        syn_wait_guest_flush();
        return -1;
    }

    /* 7h. Host raises ACK and the link enters the idle state */
    ACK_HI;

    return len;
}

static int syn_read_data(int *data, int data_len)
{
    int i, len, bit, parity, tmp;
    int *data_ptr;

    logf("syn_read_data...");

    /* 1. Guest drives CLK low */
    if (CLK != LO)
        return 0;

    /* 1a. If the host is willing to receive a packet it lowers ACK */
    ACK_LO;

    /* 2. Guest may issue a request-to-send by lowering DATA. If the
          guest decides not to transmit a packet, it may abort the
          transmission by not lowering DATA. */

    /* 3. The guest raises CLK */
    syn_wait_clk_change(HI);

    /* 4. If the guest is still driving DATA low, the transfer is commited
          to occur. Otherwise, the transfer is aborted. In either case, 
          the host raises ACK. */
    if (syn_get_data() == HI)
    {
        logf("  read abort");

        ACK_HI;
        return READ_ERROR;
    }
    else
    {
        ACK_HI;
    }

    /* 5. Read the incoming data packet */
    i = 0;
    len = 0;
    parity = 0;
    while (i <= len)
    {
        bit = 0;

        if (i < data_len)
            data_ptr = &data[i];
        else
            data_ptr = &tmp;

        *data_ptr = 0;
        while (bit < 8)
        {
            /* 5b. Guset inverts CLK to indicate that data is ready */
            syn_wait_clk_change(LO);

            /* 5d. Read the data bit from DATA */
            if (syn_get_data() == HI)
            {
                *data_ptr |= (1 << bit);
                parity++;
            }
            bit++;

            /* 5e. Invert ACK to indicate that data has been read */
            ACK_LO;

            /* Repeat for next bit */
            syn_wait_clk_change(HI);

            if (syn_get_data() == HI)
            {
                *data_ptr |= (1 << bit);
                parity++;
            }
            bit++;

            ACK_HI;
        }

        /* First byte is the packet header */
        if (i == 0)
        {
            /* Format control (bit 3) should be 1 */
            if (*data_ptr & 0x8)
            {
                /* Packet length is bits 0:2 */
                len = *data_ptr & 0x7;
                logf("  packet length = %d", len);
            }
            else
            {
                logf("  invalid format ctrl bit");
                return READ_ERROR;
            }
        }

        i++;
    }

    /* 7. Transmission termination cycle */
    /* 7a. The guest generates a parity bit on DATA */
    /* 7b. The host waits for guest to lower CLK */
    syn_wait_clk_change(LO);

    /* 7c. The host verifies the parity bit is correct */
    parity = parity % 2;
    logf("  parity check: %d / %d", syn_get_data(), parity);

    /* TODO: parity error handling */

    /* 7d. The host lowers ACK */
    ACK_LO;

    /* 7e. The host waits for the guest to raise CLK indicating 
           that the stop bit is ready */
    syn_wait_clk_change(HI);

    /* 7f. The host reads DATA and verifies that it is 1 */
    if (syn_get_data() == LO)
    {
        logf("  framing error");

        ACK_HI;
        return READ_ERROR;
    }

    ACK_HI;

    return len;
}

int syn_read_device(int *data, int len)
{
    int i;
    int ret = READ_ERROR;

    for (i = 0; i < READ_RETRY; i++)
    {
        if (syn_wait_clk_change(LO))
        {
            /* module is sending data */
            ret = syn_read_data(data, len);
            if (ret != READ_ERROR)
                return ret;

            syn_flush();
        }
        else
        {
            /* module is idle */
            return 0;
        }
    }

    return ret;
}

static int syn_reset(void)
{
    int val, id;
    int data[2];

    logf("syn_reset...");

    /* reset module 0 */
    val = (0 << 4) | (1 << 3) | 0;
    syn_send_data(&val, 1);

    val = syn_read_device(data, 2);
    if (val == 1)
    {
        val = data[0] & 0xff;      /* packet header */
        id = (data[1] >> 4) & 0xf; /* packet id */
        if ((val == MEP_HELLO_HEADER) && (id == MEP_HELLO_ID))
        {
            logf("  module 0 reset");
            return 1;
        }
    }

    logf("  reset failed");
    return 0;
}

int syn_init(void)
{
    syn_flush();
    return syn_reset();
}

#ifdef ROCKBOX_HAS_LOGF
void syn_info(void)
{
    int i, val;
    int data[8];

    logf("syn_info...");

    /* module base info */
    logf("module base info:");
    data[0] = MEP_READ;
    data[1] = 0x80;
    syn_send_data(data, 2);
    val = syn_read_device(data, 8);
    if (val > 0)
    {
        for (i = 0; i < 8; i++)
            logf("  data[%d] = 0x%02x", i, data[i]);
    }
    
    /* module product info */
    logf("module product info:");
    data[0] = MEP_READ;
    data[1] = 0x81;
    syn_send_data(data, 2);
    val = syn_read_device(data, 8);
    if (val > 0)
    {
        for (i = 0; i < 8; i++)
            logf("  data[%d] = 0x%02x", i, data[i]);
    }

    /* module serialization */
    logf("module serialization:");
    data[0] = MEP_READ;
    data[1] = 0x82;
    syn_send_data(data, 2);
    val = syn_read_device(data, 8);
    if (val > 0)
    {
        for (i = 0; i < 8; i++)
            logf("  data[%d] = 0x%02x", i, data[i]);
    }

    /* 1-D sensor info */
    logf("1-d sensor info:");
    data[0] = MEP_READ;
    data[1] = 0x80 + 0x20;
    syn_send_data(data, 2);
    val = syn_read_device(data, 8);
    if (val > 0)
    {
        for (i = 0; i < 8; i++)
            logf("  data[%d] = 0x%02x", i, data[i]);
    }
}
#endif
