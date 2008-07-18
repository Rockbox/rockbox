/***************************************************************************
*             __________               __   ___.
*   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
*   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
*   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
*   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
*                     \/            \/     \/    \/            \/
* $Id$
*
* Copyright (C) 2007 by Greg White
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
#include "cpu.h"
#include <stdbool.h>
#include "kernel.h"
#include "system.h"
#include "logf.h"
#include "debug.h"
#include "string.h"
#include "sc606-meg-fx.h"

#define SLAVE_ADDRESS 0xCC

#define SDA_LO     (GPHDAT &= ~(1 << 9))
#define SDA_HI     (GPHDAT |= (1 << 9))
#define SDA_INPUT  (GPHCON &= ~(3 << 18))
#define SDA_OUTPUT (GPHCON |= (1 << 18))
#define SDA        (GPHDAT & (1 << 9))

#define SCL_LO     (GPHDAT &= ~(1 << 10))
#define SCL_HI     (GPHDAT |= (1 << 10))
#define SCL_INPUT  (GPHCON &= ~(3 << 20))
#define SCL_OUTPUT (GPHCON |= (1 << 20))
#define SCL        (GPHDAT & (1 << 10))

#define SCL_SDA_HI (GPHDAT |= (3 << 9))

/* The SC606 can clock at 400KHz: */
/* Clock period high is 600nS and low is 1300nS */
/* The high and low times are different enough to need different timings */
/* cycles delayed = 30 + 7 * loops */
/* 100MHz = 10nS per cycle: LO:1300nS=130:14  HI:600nS=60:9 */
/* 300MHz = 3.36nS per cycle: LO:1300nS=387:51  HI:600nS=179:21 */
#define DELAY_LO do{int x;for(x=51;x;x--);} while (0)
#define DELAY    do{int x;for(x=35;x;x--);} while (0)
#define DELAY_HI do{int x;for(x=21;x;x--);} while (0)



static void sc606_i2c_start(void)
{
    SCL_SDA_HI;
    DELAY;
    SDA_LO;
    DELAY;
    SCL_LO;
}

static void sc606_i2c_restart(void)
{
    SCL_SDA_HI;
    DELAY;
    SDA_LO;
    DELAY;
    SCL_LO;
}

static void sc606_i2c_stop(void)
{
    SDA_LO;
    SCL_HI;
    DELAY_HI;
    SDA_HI;
}

static void sc606_i2c_ack(void)
{

    SDA_LO;
    SCL_HI;
    DELAY_HI;
    SCL_LO;
}



static int sc606_i2c_getack(void)
{
    int ret;

    /* Don't need a delay since follows a data bit with a delay on the end */
    SDA_INPUT;   /* And set to input */
    DELAY;
    SCL_HI;

    ret = (SDA != 0);   /* ack failed if SDA is not low */
    DELAY_HI;

    SCL_LO;
    DELAY_LO;

    SDA_HI;
    SDA_OUTPUT;
    DELAY_LO;

    return ret;
}



static void sc606_i2c_outb(unsigned char byte)
{
    int i;

    /* clock out each bit, MSB first */
    for (i = 0x80; i; i >>= 1)
    {
        if (i & byte)
        {
            SDA_HI;
        }
        else
        {
            SDA_LO;
        }
        DELAY;

        SCL_HI;
        DELAY_HI;

        SCL_LO;
        DELAY_LO;
    }

    SDA_HI;

}



static unsigned char sc606_i2c_inb(void)
{
   int i;
   unsigned char byte = 0;

   SDA_INPUT;   /* And set to input */
   /* clock in each bit, MSB first */
   for (i = 0x80; i; i >>= 1) {
       SCL_HI;

       if (SDA)
           byte |= i;

       SCL_LO;
   }
   SDA_OUTPUT;

   sc606_i2c_ack();

   return byte;
}



/* returns number of acks that were bad */
int sc606_write(unsigned char reg, unsigned char data)
{
    int x;

    sc606_i2c_start();

    sc606_i2c_outb(SLAVE_ADDRESS);
    x = sc606_i2c_getack();

    sc606_i2c_outb(reg);
    x += sc606_i2c_getack();

    sc606_i2c_restart();

    sc606_i2c_outb(SLAVE_ADDRESS);
    x += sc606_i2c_getack();

    sc606_i2c_outb(data);
    x += sc606_i2c_getack();

    sc606_i2c_stop();

    return x;
}



int sc606_read(unsigned char reg, unsigned char* data)
{
    int x;

    sc606_i2c_start();
    sc606_i2c_outb(SLAVE_ADDRESS);
    x = sc606_i2c_getack();

    sc606_i2c_outb(reg);
    x += sc606_i2c_getack();

    sc606_i2c_restart();
    sc606_i2c_outb(SLAVE_ADDRESS | 1);
    x += sc606_i2c_getack();

    *data = sc606_i2c_inb();
    sc606_i2c_stop();

    return x;
}



void sc606_init(void)
{
    volatile int i;

    /* Set GPB2 (EN) to 1 */
    GPBCON = (GPBCON & ~(3<<4)) | 1<<4;

    /* Turn enable line on */
    GPBDAT |= 1<<2;
    /* OFF GPBDAT &= ~(1 << 2); */

    /* About 400us - needs 350us */
    for (i = 200; i; i--)
    {
        DELAY_LO;
    }

    /* Set GPH9 (SDA) and GPH10 (SCL) to 1 */
    GPHUP &= ~(3<<9);
    GPHCON = (GPHCON & ~(0xF<<18)) | 5<<18;
}

