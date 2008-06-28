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
#include "button.h"
#include "kernel.h"
#include "backlight.h"
#include "backlight-target.h"
#include "system.h"

#define LOGF_ENABLE
#include "logf.h"

static int int_btn = BUTTON_NONE;

#ifndef BOOTLOADER
/* Driver for the Synaptics Touchpad based on the "Synaptics Modular Embedded
   Protocol: 3-Wire Interface Specification" documentation */

#define ACK     (GPIOD_INPUT_VAL & 0x1)
#define ACK_HI  GPIOD_OUTPUT_VAL |=  0x1
#define ACK_LO  GPIOD_OUTPUT_VAL &= ~0x1

#define CLK     ((GPIOD_INPUT_VAL & 0x2) >> 1)
#define CLK_HI  GPIOD_OUTPUT_VAL |=  0x2
#define CLK_LO  GPIOD_OUTPUT_VAL &= ~0x2

#define DATA    ((GPIOD_INPUT_VAL & 0x4) >> 2)
#define DATA_HI GPIOD_OUTPUT_EN |= 0x4; GPIOD_OUTPUT_VAL |=  0x4
#define DATA_LO GPIOD_OUTPUT_EN |= 0x4; GPIOD_OUTPUT_VAL &= ~0x4

#define LO 0
#define HI 1

#define STATUS_READY    1
#define READ_RETRY      8
#define READ_ERROR     -1

#define HELLO_HEADER    0x19
#define HELLO_ID        0x1
#define BUTTONS_HEADER  0x1a
#define BUTTONS_ID      0x9
#define ABSOLUTE_HEADER 0x0b

#define MEP_READ 0x1
#define MEP_WRITE 0x3

static int syn_status = 0;

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
    GPIOD_OUTPUT_EN &= ~0x4;
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
#if defined(LOGF_ENABLE)
    logf("syn_flush...");
#endif
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
#if defined(LOGF_ENABLE)
    logf("syn_send_data...");
#endif
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
#if defined(LOGF_ENABLE)
       logf("  sending byte: %d", data[i]);
#endif
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
#if defined(LOGF_ENABLE)    
    logf("  send parity = %d", parity);
#endif   
    if (parity)
    {
        DATA_HI;
    }
    else
    {
        DATA_LO;
    }

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
#if defined(LOGF_ENABLE)
        logf("  module flushing");
#endif     
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
#if defined(LOGF_ENABLE)
    logf("syn_read_data...");
#endif
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
#if defined(LOGF_ENABLE)
                logf("  packet length = %d", len);
#endif
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
#if defined(LOGF_ENABLE)
    logf("  parity check: %d / %d", syn_get_data(), parity);
#endif
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

static int syn_read_device(int *data, int len)
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
#if defined(LOGF_ENABLE)
    logf("syn_reset...");
#endif
    /* reset module 0 */
    val = (0 << 4) | (1 << 3) | 0;
    syn_send_data(&val, 1);

    val = syn_read_device(data, 2);
    if (val == 1)
    {
        val = data[0] & 0xff;      /* packet header */
        id = (data[1] >> 4) & 0xf; /* packet id */
        if ((val == HELLO_HEADER) && (id == HELLO_ID))
        {
            logf("  module 0 reset");
            return 1;
        }
    }

    logf("  reset failed");
    return 0;
}

#if defined(ROCKBOX_HAS_LOGF) && defined(LOGF_ENABLE)
static void syn_info(void)
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

void button_init_device(void)
{
    /* enable touchpad leds */
    GPIOA_ENABLE     |= BUTTONLIGHT_ALL;
    GPIOA_OUTPUT_EN  |= BUTTONLIGHT_ALL;
    
    /* enable touchpad */
    GPO32_ENABLE     |=  0x40000000;
    GPO32_VAL        &= ~0x40000000;

    /* enable ACK, CLK, DATA lines */
    GPIOD_ENABLE     |=  (0x1 | 0x2 | 0x4);

    GPIOD_OUTPUT_EN  |=  0x1; /* ACK  */
    GPIOD_OUTPUT_VAL |=  0x1; /* high */

    GPIOD_OUTPUT_EN  &= ~0x2; /* CLK  */
    
    GPIOD_OUTPUT_EN  |=  0x4; /* DATA */
    GPIOD_OUTPUT_VAL |=  0x4; /* high */

    syn_flush();

    if (syn_reset())
    {
#if defined(ROCKBOX_HAS_LOGF) && defined(LOGF_ENABLE)
        syn_info();
#endif

        syn_status = STATUS_READY;

        /* enable interrupts */
        GPIOD_INT_LEV &= ~0x2;
        GPIOD_INT_CLR |=  0x2;
        GPIOD_INT_EN  |=  0x2;

        CPU_INT_EN    |= HI_MASK;
        CPU_HI_INT_EN |= GPIO0_MASK;
    }
}

/*
 * Button interrupt handler
 */
void button_int(void)
{
    int data[4];
    int val, id;

    int_btn = BUTTON_NONE;

    if (syn_status == STATUS_READY)
    {
        /* disable interrupt while we read the touchpad */
        GPIOD_INT_EN &= ~0x2;

        val = syn_read_device(data, 4);
        if (val > 0)
        {
            val = data[0] & 0xff;      /* packet header */
            id = (data[1] >> 4) & 0xf; /* packet id */
#if defined(LOGF_ENABLE)
            logf("button_read_device...");
            logf("  data[0] = 0x%08x", data[0]);
            logf("  data[1] = 0x%08x", data[1]);
            logf("  data[2] = 0x%08x", data[2]);
            logf("  data[3] = 0x%08x", data[3]);
#endif
            if ((val == BUTTONS_HEADER) && (id == BUTTONS_ID))
            {
                /* Buttons packet - touched one of the 5 "buttons" */
                if (data[1] & 0x1)
                    int_btn |= BUTTON_PLAY;
                if (data[1] & 0x2)
                    int_btn |= BUTTON_MENU;
                if (data[1] & 0x4)
                    int_btn |= BUTTON_LEFT;
                if (data[1] & 0x8)
                    int_btn |= BUTTON_DISPLAY;
                if (data[2] & 0x1)
                    int_btn |= BUTTON_RIGHT;

                /* An Absolute packet should follow which we ignore */
                val = syn_read_device(data, 4);
#if defined(LOGF_ENABLE)
                logf("  int_btn = 0x%04x", int_btn);
#endif            
            }
            else if (val == ABSOLUTE_HEADER)
            {
                /* Absolute packet - the finger is on the vertical strip.
                   Position ranges from 1-4095, with 1 at the bottom. */
                val = ((data[1] >> 4) << 8) | data[2]; /* position */
#if defined(LOGF_ENABLE)                
                logf(" pos %d", val);
                logf(" z %d", data[3]);
                logf(" finger %d", data[1] & 0x1);
                logf(" gesture %d", data[1] & 0x2);
                logf(" RelPosVld %d", data[1] & 0x4);
#endif                
                if(data[1] & 0x1)  /* if finger on touch strip */
                {
                    if ((val > 0) && (val <= 1365))
                        int_btn |= BUTTON_DOWN;
                    else if ((val > 1365) && (val <= 2730))
                        int_btn |= BUTTON_SELECT;
                    else if ((val > 2730) && (val <= 4095))
                        int_btn |= BUTTON_UP;
                }
            }
        }

        /* re-enable interrupts */
        GPIOD_INT_LEV &= ~0x2;
        GPIOD_INT_CLR |=  0x2;
        GPIOD_INT_EN  |=  0x2;
    }
}
#else
void button_init_device(void){}
#endif /* bootloader */

/*
 * Get button pressed from hardware
 */
int button_read_device(void)
{
    int btn = int_btn;

    if(button_hold())
        return BUTTON_NONE;
    
    if (~GPIOA_INPUT_VAL & 0x40)
        btn |= BUTTON_POWER;

    return btn;
}

bool button_hold(void)
{
    return (GPIOD_INPUT_VAL & 0x10) ? false : true;
}

bool headphones_inserted(void)
{
    return (GPIOD_INPUT_VAL & 0x80) ? false : true;
}
