/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2005 by Linus Nielsen Feltzing
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
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
#include "hwcompat.h"
#include "logf.h"
#include "string.h"

/* TODO: merge all bit-banged I2C into a generic I2C driver */

#define USE_ASM

#define SDA             ( 0x00002000 & GPIO1_READ)
#define SDA_LO_OUT  or_l( 0x00002000, &GPIO1_ENABLE)
#define SDA_HI_IN  and_l(~0x00002000, &GPIO1_ENABLE)

#define SCL             ( 0x00001000 & GPIO_READ)
#define SCL_LO_OUT  or_l( 0x00001000, &GPIO_ENABLE)
#define SCL_HI_IN   and_l(~0x00001000, &GPIO_ENABLE); while(!SCL);

#define DELAY                           \
    asm (                               \
        "move.l  %[dly],%%d0 \n"        \
    "1:                      \n"        \
        "subq.l  #1,%%d0     \n"        \
        "bhi.s   1b          \n"        \
        : : [dly]"d"(i2c_delay) : "d0" );

static int i2c_delay IDATA_ATTR = 44;

void pcf50606_i2c_recalc_delay(int cpu_clock)
{
    i2c_delay = MAX(cpu_clock / (400000*2*3) - 7, 1);
}

static inline void pcf50606_i2c_start(void)
{
#ifdef USE_ASM
    asm (
        "not.l   %[sdab]             \n" /* SDA_HI_IN */
        "and.l   %[sdab],(8,%[gpi1]) \n"
        "not.l   %[sdab]             \n"

        "not.l   %[sclb]             \n" /* SCL_HI_IN */
        "and.l   %[sclb],(8,%[gpio]) \n"
        "not.l   %[sclb]             \n"
    "1:                              \n"
        "move.l  (%[gpio]),%%d0      \n"
        "btst.l  #12,%%d0            \n"
        "beq.s   1b                  \n"

        "move.l  %[dly],%%d0         \n" /* DELAY */
    "1:                              \n"
        "subq.l  #1,%%d0             \n"
        "bhi.s   1b                  \n"

        "or.l    %[sdab],(8,%[gpi1]) \n" /* SDA_LO_OUT */

        "move.l  %[dly],%%d0         \n" /* DELAY */
    "1:                              \n"
        "subq.l  #1,%%d0             \n"
        "bhi.s   1b                  \n"

        "or.l    %[sclb],(8,%[gpio]) \n" /* SCL_LO_OUT */
        : /* outputs */
        : /* inputs */
        [gpio]"a"(&GPIO_READ),
        [sclb]"d"(0x00001000),
        [gpi1]"a"(&GPIO1_READ),
        [sdab]"d"(0x00002000),
        [dly] "d"(i2c_delay)
        : /* clobbers */
        "d0"
    );
#else
    SDA_HI_IN;
    SCL_HI_IN;
    DELAY;
    SDA_LO_OUT;
    DELAY;
    SCL_LO_OUT;
#endif
}

static inline void pcf50606_i2c_stop(void)
{
#ifdef USE_ASM
    asm (
        "or.l    %[sdab],(8,%[gpi1]) \n" /* SDA_LO_OUT */

        "not.l   %[sclb]             \n" /* SCL_HI_IN */
        "and.l   %[sclb],(8,%[gpio]) \n"
        "not.l   %[sclb]             \n"
    "1:                              \n"
        "move.l  (%[gpio]),%%d0      \n"
        "btst.l  #12,%%d0            \n"
        "beq.s   1b                  \n"

        "move.l  %[dly],%%d0         \n" /* DELAY */
    "1:                              \n"
        "subq.l  #1,%%d0             \n"
        "bhi.s   1b                  \n"

        "not.l   %[sdab]             \n" /* SDA_HI_IN */
        "and.l   %[sdab],(8,%[gpi1]) \n"
        "not.l   %[sdab]             \n"
        : /* outputs */
        : /* inputs */
        [gpio]"a"(&GPIO_READ),
        [sclb]"d"(0x00001000),
        [gpi1]"a"(&GPIO1_READ),
        [sdab]"d"(0x00002000),
        [dly] "d"(i2c_delay)
        : /* clobbers */
        "d0"
    );
#else
    SDA_LO_OUT;
    SCL_HI_IN;
    DELAY;
    SDA_HI_IN;
#endif
}

static inline void pcf50606_i2c_ack(bool ack)
{
#ifdef USE_ASM
    asm (
        "tst.b   %[ack]              \n" /* if (!ack) */
        "bne.s   1f                  \n"

        "not.l   %[sdab]             \n" /*   SDA_HI_IN */
        "and.l   %[sdab],(8,%[gpi1]) \n"
        "not.l   %[sdab]             \n"
        ".word   0x51fb              \n" /* trapf.l : else */
    "1:                              \n"
        "or.l    %[sdab],(8,%[gpi1]) \n" /*   SDA_LO_OUT */

        "not.l   %[sclb]             \n" /* SCL_HI_IN */
        "and.l   %[sclb],(8,%[gpio]) \n"
        "not.l   %[sclb]             \n"
    "1:                              \n"
        "move.l  (%[gpio]),%%d0      \n"
        "btst.l  #12,%%d0            \n"
        "beq.s   1b                  \n"

        "move.l  %[dly],%%d0         \n" /* DELAY */
    "1:                              \n"
        "subq.l  #1,%%d0             \n"
        "bhi.s   1b                  \n"

        "or.l    %[sclb],(8,%[gpio]) \n" /* SCL_LO_OUT */
        : /* outputs */
        : /* inputs */
        [gpio]"a"(&GPIO_READ),
        [sclb]"d"(0x00001000),
        [gpi1]"a"(&GPIO1_READ),
        [sdab]"d"(0x00002000),
        [dly] "d"(i2c_delay),
        [ack] "d"(ack)
        : /* clobbers */
        "d0"
    );
#else
    if(ack)
        SDA_LO_OUT;
    else
        SDA_HI_IN;

    SCL_HI_IN;

    DELAY;
    SCL_LO_OUT;
#endif
}

static inline bool pcf50606_i2c_getack(void)
{
    bool ret;

#ifdef USE_ASM
    asm (
        "not.l   %[sdab]             \n" /* SDA_HI_IN */
        "and.l   %[sdab],(8,%[gpi1]) \n"
        "not.l   %[sdab]             \n"

        "move.l  %[dly],%%d0         \n" /* DELAY */
    "1:                              \n"
        "subq.l  #1,%%d0             \n"
        "bhi.s   1b                  \n"

        "not.l   %[sclb]             \n" /* SCL_HI_IN */
        "and.l   %[sclb],(8,%[gpio]) \n"
        "not.l   %[sclb]             \n"
    "1:                              \n"
        "move.l  (%[gpio]),%%d0      \n"
        "btst.l  #12,%%d0            \n"
        "beq.s   1b                  \n"

        "move.l  (%[gpi1]),%%d0      \n" /* ret = !SDA */
        "btst.l  #13,%%d0            \n"
        "seq.b   %[ret]              \n"

        "or.l    %[sclb],(8,%[gpio]) \n" /* SCL_LO_OUT */

        "move.l  %[dly],%%d0         \n" /* DELAY */
    "1:                              \n"
        "subq.l  #1,%%d0             \n"
        "bhi.s   1b                  \n"
        : /* outputs */
        [ret]"=&r"(ret)
        : /* inputs */
        [gpio]"a"(&GPIO_READ),
        [sclb]"d"(0x00001000),
        [gpi1]"a"(&GPIO1_READ),
        [sdab]"d"(0x00002000),
        [dly] "d"(i2c_delay)
        : /* clobbers */
        "d0"
    );
#else
    SDA_HI_IN;
    DELAY;
    SCL_HI_IN;

    ret = !SDA;

    SCL_LO_OUT;
    DELAY;
#endif

    return ret;
}

static void pcf50606_i2c_outb(unsigned char byte)
{
#ifdef USE_ASM
    asm volatile (
        "moveq.l #24,%%d0            \n" /* byte <<= 24 */
        "lsl.l   %%d0,%[byte]        \n"
        "moveq.l #8,%%d1             \n" /* i = 8 */
        
    "2:                              \n" /* do */
        "lsl.l   #1,%[byte]          \n" /* if ((byte <<= 1) carry) */
        "bcc.s   1f                  \n"

        "not.l   %[sdab]             \n" /*   SDA_HI_IN */
        "and.l   %[sdab],(8,%[gpi1]) \n"
        "not.l   %[sdab]             \n"
        ".word   0x51fb              \n" /* trapf.l; else */
    "1:                              \n"
        "or.l    %[sdab],(8,%[gpi1]) \n" /*   SDA_LO_OUT */

        "move.l  %[dly],%%d0         \n" /* DELAY */
    "1:                              \n"
        "subq.l  #1,%%d0             \n"
        "bhi.s   1b                  \n"

        "not.l   %[sclb]             \n" /* SCL_HI_IN */
        "and.l   %[sclb],(8,%[gpio]) \n"
        "not.l   %[sclb]             \n"
    "1:                              \n"
        "move.l  (%[gpio]),%%d0      \n"
        "btst.l  #12,%%d0            \n"
        "beq.s   1b                  \n"

        "move.l  %[dly],%%d0         \n" /* DELAY */
    "1:                              \n"
        "subq.l  #1,%%d0             \n"
        "bhi.s   1b                  \n"

        "or.l    %[sclb],(8,%[gpio]) \n" /* SCL_LO_OUT */

        "subq.l  #1,%%d1             \n" /* i-- */
        "bne.s   2b                  \n" /* while (i != 0) */
        : /* outputs */
        [byte]"+d"(byte)
        : /* inputs */
        [gpio]"a"(&GPIO_READ),
        [sclb]"d"(0x00001000),
        [gpi1]"a"(&GPIO1_READ),
        [sdab]"d"(0x00002000),
        [dly] "d"(i2c_delay)
        : /* clobbers */
        "d0", "d1"
    );
#else
    int i;

    /* clock out each bit, MSB first */
    for ( i=0x80; i; i>>=1 )   
    {
        if ( i & byte )
            SDA_HI_IN;
        else
            SDA_LO_OUT;
        DELAY;
        SCL_HI_IN;
        DELAY;
        SCL_LO_OUT;
    }
#endif
}

static unsigned char pcf50606_i2c_inb(bool ack)
{
    unsigned char byte = 0;

#ifdef USE_ASM
    asm (
        "not.l   %[sdab]             \n" /* SDA_HI_IN */
        "and.l   %[sdab],(8,%[gpi1]) \n"
        "not.l   %[sdab]             \n"

        "moveq.l #8,%%d1             \n" /* i = 8 */
        "clr.l   %[byte]             \n" /* byte = 0 */
        
    "2:                              \n" /* do */
        "not.l   %[sclb]             \n" /* SCL_HI_IN */
        "and.l   %[sclb],(8,%[gpio]) \n"
        "not.l   %[sclb]             \n"
    "1:                              \n"
        "move.l  (%[gpio]),%%d0      \n"
        "btst.l  #12,%%d0            \n"
        "beq.s   1b                  \n"

        "move.l  %[dly],%%d0         \n" /* DELAY */
    "1:                              \n"
        "subq.l  #1,%%d0             \n"
        "bhi.s   1b                  \n"

        "lsl.l   #1,%[byte]          \n" /* byte <<= 1 */
        "move.l  (%[gpi1]),%%d0      \n" /* if (SDA) */
        "btst.l  #13,%%d0            \n"
        "beq.s   1f                  \n"
        "addq.l  #1,%[byte]          \n" /*   byte++ */
    "1:                              \n"

        "or.l    %[sclb],(8,%[gpio]) \n" /* SCL_LO_OUT */
    
        "move.l  %[dly],%%d0         \n" /* DELAY */
    "1:                              \n"
        "subq.l  #1,%%d0             \n"
        "bhi.s   1b                  \n"

        "subq.l  #1,%%d1             \n" /* i-- */
        "bne.s   2b                  \n" /* while (i != 0) */
        : /* outputs */
        [byte]"=&d"(byte)
        : /* inputs */
        [gpio]"a"(&GPIO_READ),
        [sclb]"d"(0x00001000),
        [gpi1]"a"(&GPIO1_READ),
        [sdab]"d"(0x00002000),
        [dly] "d"(i2c_delay)
        : /* clobbers */
        "d0", "d1"
    );
#else
    int i;

    /* clock in each bit, MSB first */
    SDA_HI_IN;
    for ( i=0x80; i; i>>=1 )
    {
        SCL_HI_IN;
        DELAY;
        if ( SDA )
            byte |= i;
        SCL_LO_OUT;
        DELAY;
    }
#endif

    pcf50606_i2c_ack(ack);
   
    return byte;
}

int pcf50606_i2c_write(int address, const unsigned char* buf, int count)
{
    int i,x=0;

    pcf50606_i2c_start();
    pcf50606_i2c_outb(address & 0xfe);
    if (pcf50606_i2c_getack())
    {
        for (i=0; i<count; i++)
        {
            pcf50606_i2c_outb(buf[i]);
            if (!pcf50606_i2c_getack())
            {
                x=-2;
                break;
            }
        }
    }
    else
    {
        logf("pcf50606_i2c_write() - no ack\n");
        x=-1;
    }
    return x;
}

int pcf50606_read_multiple(int address, unsigned char* buf, int count)
{
    int i=0;
    int ret = 0;
    unsigned char obuf[1];

    obuf[0] = address;

    /* send read command */
    if (pcf50606_i2c_write(0x10, obuf, 1) >= 0)
    {
        pcf50606_i2c_start();
        pcf50606_i2c_outb(0x11);
        if (pcf50606_i2c_getack())
        {
            for(i = 0;i < count-1;i++)
                buf[i] = pcf50606_i2c_inb(true);
            
            buf[i] = pcf50606_i2c_inb(false);
        }
        else
        {
            ret = -1;
        }
    }

    pcf50606_i2c_stop();

    return ret;
}

int pcf50606_read(int address)
{
    int ret;
    unsigned char c;

    ret = pcf50606_read_multiple(address, &c, 1);
    if(ret >= 0)
        return c;
    else
        return ret;
}

int pcf50606_write_multiple(int address, const unsigned char* buf, int count)
{
    unsigned char obuf[1];
    int i;
    int ret = 0;

    obuf[0] = address;
    
    /* send write command */
    if (pcf50606_i2c_write(0x10, obuf, 1) >= 0)
    {
        for (i=0; i<count; i++)
        {
            pcf50606_i2c_outb(buf[i]);
            if (!pcf50606_i2c_getack())
            {
                ret = -2;
                break;
            }
        }
    }
    else
    {
        ret = -1;
    }

    pcf50606_i2c_stop();
    return ret;
}

int pcf50606_write(int address, unsigned char val)
{
    return pcf50606_write_multiple(address, &val, 1);
}


/* These voltages were determined by measuring the output of the PCF50606
   on a running H300, and verified by disassembling the original firmware */
static void set_voltages(void)
{
    static const unsigned char buf[5] =
        {
            0xf4,   /* IOREGC = 2.9V, ON in all states */
            0xef,   /* D1REGC = 2.4V, ON in all states */
            0x18,   /* D2REGC = 3.3V, OFF in all states */
            0xf0,   /* D3REGC = 2.5V, ON in all states */
            0xef,   /* LPREGC1 = 2.4V, ON in all states */
        };
    
    pcf50606_write_multiple(0x23, buf, 5);
}

void pcf50606_init(void)
{
    /* Bit banged I2C */
    or_l(0x00002000, &GPIO1_FUNCTION);
    or_l(0x00001000, &GPIO_FUNCTION);
    and_l(~0x00002000, &GPIO1_OUT);
    and_l(~0x00001000, &GPIO_OUT);
    and_l(~0x00002000, &GPIO1_ENABLE);
    and_l(~0x00001000, &GPIO_ENABLE);

    set_voltages();

    pcf50606_write(0x08, 0x60); /* Wake on USB and charger insertion */
    pcf50606_write(0x09, 0x05); /* USB and ON key debounce: 14ms */
    pcf50606_write(0x29, 0x1C); /* Disable the unused MBC module */
    
    pcf50606_write(0x35, 0x13); /* Backlight PWM = 512Hz 50/50 */
    pcf50606_write(0x3a, 0x3b); /* PWM output on GPOOD1 */
}
