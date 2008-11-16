/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright © 2008 Rafaël Carré
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

/* ARM PrimeCell PL180 SD/MMC controller */

/* MCIStatus bits */

/* bits 10:0 can be cleared by a write in MCIClear */
#define MCI_CMD_CRC_FAIL    (1<<0)
#define MCI_DATA_CRC_FAIL   (1<<1)
#define MCI_CMD_TIMEOUT     (1<<2)
#define MCI_DATA_TIMEOUT    (1<<3)
#define MCI_TX_UNDERRUN     (1<<4)
#define MCI_RX_OVERRUN      (1<<5)
#define MCI_CMD_RESP_END    (1<<6)
#define MCI_CMD_SENT        (1<<7)
#define MCI_DATA_END        (1<<8)
#define MCI_START_BIT_ERR   (1<<9)
#define MCI_DATA_BLOCK_END  (1<<10)
/* bits 21:11 are only cleared by the hardware logic */
#define MCI_CMD_ACTIVE          (1<<11)
#define MCI_TX_ACTIVE           (1<<12)
#define MCI_RX_ACTIVE           (1<<13)
#define MCI_TX_FIFO_HALF_EMPTY  (1<<14)
#define MCI_RX_FIFO_HALF_FULL   (1<<15)
#define MCI_TX_FIFO_FULL        (1<<16)
#define MCI_RX_FIFO_FULL        (1<<17)
#define MCI_TX_FIFO_EMPTY       (1<<18)
#define MCI_RX_FIFO_EMPTY       (1<<19)
#define MCI_TX_DATA_AVAIL       (1<<20)
#define MCI_RX_DATA_AVAIL       (1<<21)


/* MCIPower bits */
#define MCI_POWER_OFF           0x0
/* 0x1 is reserved */
#define MCI_POWER_UP            0x2
#define MCI_POWER_ON            0x3

/* bits 5:2 are the voltage */

#define MCI_POWER_OPEN_DRAIN    (1<<6)
#define MCI_POWER_ROD           (1<<7)


/* MCIClock bits */
/* bits 7:0 are the clock divider */
#define MCI_CLOCK_ENABLE        (1<<8)
#define MCI_CLOCK_POWERSAVE     (1<<9)
#define MCI_CLOCK_BYPASS        (1<<10)
#define MCI_CLOCK_WIDEBUS       (1<<11)


/* MCICommand bits */
/* bits 5:0 are the command index */
#define MCI_COMMAND_RESPONSE        (1<<6)
#define MCI_COMMAND_LONG_RESPONSE   (1<<7)
#define MCI_COMMAND_INTERRUPT       (1<<8)
#define MCI_COMMAND_PENDING         (1<<9)
#define MCI_COMMAND_ENABLE          (1<<10)
