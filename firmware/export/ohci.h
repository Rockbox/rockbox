/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * OHCI data structures and registers
 *
 * Copyright (C) 2009 by Frank Gevaerts
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

#define OHCI_REVISION           (*(volatile unsigned int *)(OHCI_BASE+0x00))
#define OHCI_CONTROL            (*(volatile unsigned int *)(OHCI_BASE+0x04))
#define OHCI_COMMAND_STATUS     (*(volatile unsigned int *)(OHCI_BASE+0x08))
#define OHCI_INTERRUPT_STATUS   (*(volatile unsigned int *)(OHCI_BASE+0x0C))
#define OHCI_INTERRUPT_ENABLE   (*(volatile unsigned int *)(OHCI_BASE+0x10))
#define OHCI_INTERRUPT_DISABLE  (*(volatile unsigned int *)(OHCI_BASE+0x14))
#define OHCI_HCCA               (*(volatile unsigned int *)(OHCI_BASE+0x18))
#define OHCI_PERIOD_CURRENT_ED  (*(volatile unsigned int *)(OHCI_BASE+0x1C))
#define OHCI_CONTROL_HEAD_ED    (*(volatile unsigned int *)(OHCI_BASE+0x20))
#define OHCI_CONTROL_CURRENT_ED (*(volatile unsigned int *)(OHCI_BASE+0x24))
#define OHCI_BULK_HEAD_ED       (*(volatile unsigned int *)(OHCI_BASE+0x28))
#define OHCI_BULK_CURRENT_ED    (*(volatile unsigned int *)(OHCI_BASE+0x2C))
#define OHCI_DONE_HEAD          (*(volatile unsigned int *)(OHCI_BASE+0x30))
#define OHCI_FM_INTERVAL        (*(volatile unsigned int *)(OHCI_BASE+0x34))
#define OHCI_FM_REMAINING       (*(volatile unsigned int *)(OHCI_BASE+0x38))
#define OHCI_FM_NUMBER          (*(volatile unsigned int *)(OHCI_BASE+0x3C))
#define OHCI_PERIODIC_START     (*(volatile unsigned int *)(OHCI_BASE+0x40))
#define OHCI_LS_THRESHOLD       (*(volatile unsigned int *)(OHCI_BASE+0x44))
#define OHCI_RH_DESCRIPTOR_A    (*(volatile unsigned int *)(OHCI_BASE+0x48))
#define OHCI_RH_DESCRIPTOR_B    (*(volatile unsigned int *)(OHCI_BASE+0x4C))
#define OHCI_RH_STATUS          (*(volatile unsigned int *)(OHCI_BASE+0x50))
#define OHCI_RH_PORT_STATUS_1   (*(volatile unsigned int *)(OHCI_BASE+0x54))
#define OHCI_RH_PORT_STATUS_2   (*(volatile unsigned int *)(OHCI_BASE+0x58))

/* Transfer Descriptor */
struct ohci_td
{
    uint32_t td_config;
    void *current_buffer_pointer;
    struct ohci_td *nextTD;
    void *buffer_end;
};

/* Endpoint Descriptor */
struct ohci_ed
{
    uint32_t ep_config;
    struct ohci_td *tail;
    struct ohct_td *head;
    struct ohci_ed *nextED;
};

/* Host Controller Communications Area */
struct ohci_hcca
{
    struct ohci_ed (*interrupt_ed_table)[32];
    unsigned short frame_number;
    unsigned short pad1;
    uint32_t done_head;
    unsigned char reserved[116];
};


