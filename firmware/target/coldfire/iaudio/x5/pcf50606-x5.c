/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 by Linus Nielsen Feltzing
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
#include "debug.h"
#include "string.h"
#include "generic_i2c.h"
#include "powermgmt.h"

#define USE_ASM

/* Data */
#define SDA_BITNUM          12              /* SDA1/RXD1/GPIO44 */
#define SDA_GPIO_READ       GPIO1_READ      /* MBAR2 + 0x0b0    */
#define SDA_GPIO_OUT        GPIO1_OUT       /* MBAR2 + 0x0b4    */
#define SDA_GPIO_ENABLE     GPIO1_ENABLE    /* MBAR2 + 0x0b8    */
#define SDA_GPIO_FUNCTION   GPIO1_FUNCTION  /* MBAR2 + 0x0bc    */

/* Clock */
#define SCL_BITNUM          10              /* SCL1/TXD1/GPIO10 */
#define SCL_GPIO_READ       GPIO_READ       /* MBAR2 + 0x000    */
#define SCL_GPIO_OUT        GPIO_OUT        /* MBAR2 + 0x004    */
#define SCL_GPIO_ENABLE     GPIO_ENABLE     /* MBAR2 + 0x008    */
#define SCL_GPIO_FUNCTION   GPIO_FUNCTION   /* MBAR2 + 0x00c    */

#define PCF50606_ADDR       0x10
#define SCL_BIT             (1ul << SCL_BITNUM)
#define SDA_BIT             (1ul << SDA_BITNUM)

#define SDA             ( SDA_BIT & SDA_GPIO_READ)
#define SDA_LO_OUT  or_l( SDA_BIT, &SDA_GPIO_ENABLE)
#define SDA_HI_IN  and_l(~SDA_BIT, &SDA_GPIO_ENABLE)

#define SCL             ( SCL_BIT & SCL_GPIO_READ)
#define SCL_LO_OUT  or_l( SCL_BIT, &SCL_GPIO_ENABLE)
#define SCL_HI_IN  and_l(~SCL_BIT, &SCL_GPIO_ENABLE); while(!SCL);

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

inline void pcf50606_i2c_start(void)
{
#ifdef USE_ASM
    asm (
        "not.l   %[sdab]              \n" /* SDA_HI_IN */
        "and.l   %[sdab],(8,%[sdard]) \n"
        "not.l   %[sdab]              \n"

        "not.l   %[sclb]              \n" /* SCL_HI_IN */
        "and.l   %[sclb],(8,%[sclrd]) \n"
        "not.l   %[sclb]              \n"
    "1:                               \n"
        "move.l  (%[sclrd]),%%d0      \n"
        "btst.l  %[sclbnum], %%d0     \n"
        "beq.s   1b                   \n"

        "move.l  %[dly],%%d0          \n" /* DELAY */
    "1:                               \n"
        "subq.l  #1,%%d0              \n"
        "bhi.s   1b                   \n"

        "or.l    %[sdab],(8,%[sdard]) \n" /* SDA_LO_OUT */

        "move.l  %[dly],%%d0          \n" /* DELAY */
    "1:                               \n"
        "subq.l  #1,%%d0              \n"
        "bhi.s   1b                   \n"

        "or.l    %[sclb],(8,%[sclrd]) \n" /* SCL_LO_OUT */
        : /* outputs */
        : /* inputs */
        [sclrd]   "a"(&SCL_GPIO_READ),
        [sclb]    "d"(SCL_BIT),
        [sclbnum] "i"(SCL_BITNUM),
        [sdard]   "a"(&SDA_GPIO_READ),
        [sdab]    "d"(SDA_BIT),
        [dly]     "d"(i2c_delay)
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

inline void pcf50606_i2c_stop(void)
{
#ifdef USE_ASM
    asm (
        "or.l    %[sdab],(8,%[sdard]) \n" /* SDA_LO_OUT */

        "not.l   %[sclb]              \n" /* SCL_HI_IN */
        "and.l   %[sclb],(8,%[sclrd]) \n"
        "not.l   %[sclb]              \n"
    "1:                               \n"
        "move.l  (%[sclrd]),%%d0      \n"
        "btst.l  %[sclbnum],%%d0      \n"
        "beq.s   1b                   \n"

        "move.l  %[dly],%%d0          \n" /* DELAY */
    "1:                               \n"
        "subq.l  #1,%%d0              \n"
        "bhi.s   1b                   \n"

        "not.l   %[sdab]              \n" /* SDA_HI_IN */
        "and.l   %[sdab],(8,%[sdard]) \n"
        "not.l   %[sdab]              \n"
        : /* outputs */
        : /* inputs */
        [sclrd]   "a"(&SCL_GPIO_READ),
        [sclb]    "d"(SCL_BIT),
        [sclbnum] "i"(SCL_BITNUM),
        [sdard]   "a"(&SDA_GPIO_READ),
        [sdab]    "d"(SDA_BIT),
        [dly]     "d"(i2c_delay)
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

inline void pcf50606_i2c_ack(bool ack)
{
#ifdef USE_ASM
    asm (
        "tst.b   %[ack]               \n" /* if (!ack) */
        "bne.s   1f                   \n"

        "not.l   %[sdab]              \n" /*   SDA_HI_IN */
        "and.l   %[sdab],(8,%[sdard]) \n"
        "not.l   %[sdab]              \n"
        ".word   0x51fb               \n" /* trapf.l : else */
    "1:                               \n"
        "or.l    %[sdab],(8,%[sdard]) \n" /*   SDA_LO_OUT */

        "not.l   %[sclb]              \n" /* SCL_HI_IN */
        "and.l   %[sclb],(8,%[sclrd]) \n"
        "not.l   %[sclb]              \n"
    "1:                               \n"
        "move.l  (%[sclrd]),%%d0      \n"
        "btst.l  %[sclbnum],%%d0      \n"
        "beq.s   1b                   \n"

        "move.l  %[dly],%%d0          \n" /* DELAY */
    "1:                               \n"
        "subq.l  #1,%%d0              \n"
        "bhi.s   1b                   \n"

        "or.l    %[sclb],(8,%[sclrd]) \n" /* SCL_LO_OUT */
        : /* outputs */
        : /* inputs */
        [sclrd]   "a"(&SCL_GPIO_READ),
        [sclb]    "d"(SCL_BIT),
        [sclbnum] "i"(SCL_BITNUM),
        [sdard]   "a"(&SDA_GPIO_READ),
        [sdab]    "d"(SDA_BIT),
        [dly]     "d"(i2c_delay),
        [ack]     "d"(ack)
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

inline bool pcf50606_i2c_getack(void)
{
    bool ret;

#ifdef USE_ASM
    asm (
        "not.l   %[sdab]              \n" /* SDA_HI_IN */
        "and.l   %[sdab],(8,%[sdard]) \n"
        "not.l   %[sdab]              \n"

        "move.l  %[dly],%%d0          \n" /* DELAY */
    "1:                               \n"
        "subq.l  #1,%%d0              \n"
        "bhi.s   1b                   \n"

        "not.l   %[sclb]              \n" /* SCL_HI_IN */
        "and.l   %[sclb],(8,%[sclrd]) \n"
        "not.l   %[sclb]              \n"
    "1:                               \n"
        "move.l  (%[sclrd]),%%d0      \n"
        "btst.l  %[sclbnum],%%d0      \n"
        "beq.s   1b                   \n"

        "move.l  (%[sdard]),%%d0      \n" /* ret = !SDA */
        "btst.l  %[sdabnum],%%d0      \n"
        "seq.b   %[ret]               \n"

        "or.l    %[sclb],(8,%[sclrd]) \n" /* SCL_LO_OUT */

        "move.l  %[dly],%%d0          \n" /* DELAY */
    "1:                               \n"
        "subq.l  #1,%%d0              \n"
        "bhi.s   1b                   \n"
        : /* outputs */
        [ret]   "=&d"(ret)
        : /* inputs */
        [sclrd]   "a"(&SCL_GPIO_READ),
        [sclb]    "d"(SCL_BIT),
        [sclbnum] "i"(SCL_BITNUM),
        [sdard]   "a"(&SDA_GPIO_READ),
        [sdab]    "d"(SDA_BIT),
        [sdabnum] "i"(SDA_BITNUM),
        [dly]     "d"(i2c_delay)
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

void pcf50606_i2c_outb(unsigned char byte)
{
#ifdef USE_ASM
    asm volatile (
        "moveq.l #24,%%d0             \n" /* byte <<= 24 */
        "lsl.l   %%d0,%[byte]         \n"
        "moveq.l #8,%%d1              \n" /* i = 8 */

    "2:                               \n" /* do */
        "lsl.l   #1,%[byte]           \n" /* if ((byte <<= 1) carry) */
        "bcc.s   1f                   \n"

        "not.l   %[sdab]              \n" /*   SDA_HI_IN */
        "and.l   %[sdab],(8,%[sdard]) \n"
        "not.l   %[sdab]              \n"
        ".word   0x51fb               \n" /* trapf.l; else */
    "1:                               \n"
        "or.l    %[sdab],(8,%[sdard]) \n" /*   SDA_LO_OUT */

        "move.l  %[dly],%%d0          \n" /* DELAY */
    "1:                               \n"
        "subq.l  #1,%%d0              \n"
        "bhi.s   1b                   \n"

        "not.l   %[sclb]              \n" /* SCL_HI_IN */
        "and.l   %[sclb],(8,%[sclrd]) \n"
        "not.l   %[sclb]              \n"
    "1:                               \n"
        "move.l  (%[sclrd]),%%d0      \n"
        "btst.l  %[sclbnum],%%d0      \n"
        "beq.s   1b                   \n"

        "move.l  %[dly],%%d0          \n" /* DELAY */
    "1:                               \n"
        "subq.l  #1,%%d0              \n"
        "bhi.s   1b                   \n"

        "or.l    %[sclb],(8,%[sclrd]) \n" /* SCL_LO_OUT */

        "subq.l  #1,%%d1              \n" /* i-- */
        "bne.s   2b                   \n" /* while (i != 0) */
        : /* outputs */
        [byte]    "+d"(byte)
        : /* inputs */
        [sclrd]   "a"(&SCL_GPIO_READ),
        [sclb]    "d"(SCL_BIT),
        [sclbnum] "i"(SCL_BITNUM),
        [sdard]   "a"(&SDA_GPIO_READ),
        [sdab]    "d"(SDA_BIT),
        [dly]     "d"(i2c_delay)
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

unsigned char pcf50606_i2c_inb(bool ack)
{
    unsigned char byte = 0;

#ifdef USE_ASM
    asm (
        "not.l   %[sdab]              \n" /* SDA_HI_IN */
        "and.l   %[sdab],(8,%[sdard]) \n"
        "not.l   %[sdab]              \n"

        "moveq.l #8,%%d1              \n" /* i = 8 */
        "clr.l   %[byte]              \n" /* byte = 0 */

    "2:                               \n" /* do */
        "not.l   %[sclb]              \n" /* SCL_HI_IN */
        "and.l   %[sclb],(8,%[sclrd]) \n"
        "not.l   %[sclb]              \n"
    "1:                               \n"
        "move.l  (%[sclrd]),%%d0      \n"
        "btst.l  %[sclbnum],%%d0      \n"
        "beq.s   1b                   \n"

        "move.l  %[dly],%%d0          \n" /* DELAY */
    "1:                               \n"
        "subq.l  #1,%%d0              \n"
        "bhi.s   1b                   \n"

        "lsl.l   #1,%[byte]           \n" /* byte <<= 1 */
        "move.l  (%[sdard]),%%d0      \n" /* if (SDA) */
        "btst.l  %[sdabnum],%%d0      \n"
        "beq.s   1f                   \n"
        "addq.l  #1,%[byte]           \n" /*   byte++ */
    "1:                               \n"

        "or.l    %[sclb],(8,%[sclrd]) \n" /* SCL_LO_OUT */

        "move.l  %[dly],%%d0          \n" /* DELAY */
    "1:                               \n"
        "subq.l  #1,%%d0              \n"
        "bhi.s   1b                   \n"

        "subq.l  #1,%%d1              \n" /* i-- */
        "bne.s   2b                   \n" /* while (i != 0) */
        : /* outputs */
        [byte]    "=&d"(byte)
        : /* inputs */
        [sclrd]   "a"(&SCL_GPIO_READ),
        [sclb]    "d"(SCL_BIT),
        [sclbnum] "i"(SCL_BITNUM),
        [sdard]   "a"(&SDA_GPIO_READ),
        [sdab]    "d"(SDA_BIT),
        [sdabnum] "i"(SDA_BITNUM),
        [dly]     "d"(i2c_delay)
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
    if (pcf50606_i2c_write(PCF50606_ADDR, obuf, 1) >= 0)
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
    if (pcf50606_i2c_write(PCF50606_ADDR, obuf, 1) >= 0)
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
   on a running X5, and verified by disassembling the original firmware */
static void set_voltages(void)
{
    static const unsigned char buf[4] =
    {
        0xf4,   /* IOREGC = 2.9V, ON in all states */
        0xf0,   /* D1REGC = 2.5V, ON in all states */
        0xf6,   /* D2REGC = 3.1V, ON in all states */
        0xf4,   /* D3REGC = 2.9V, ON in all states */
    };

    pcf50606_write_multiple(0x23, buf, 4);
}

static void init_pmu_interrupts(void)
{
    /* inital data is interrupt masks */
    unsigned char data[3] =
    {
        ~0x04, /* unmask ONKEY1S */
        ~0x00,
        ~0x00
    };

    /* make sure GPI0 interrupt is off before unmasking anything */
    and_l(~0xf, &INTPRI5);  /* INT32 - Priority 0 (Off) */

    /* unmask the PMU interrupts we want to service */
    pcf50606_write_multiple(0x05, data, 3);
    /* clear INT1-3 as these are left set after standby */
    pcf50606_read_multiple(0x02, data, 3);

    /* Set to read pcf50606 INT but keep GPI0 off until init completes */
    and_l(~0x00000001, &GPIO_ENABLE);
    or_l(0x00000001, &GPIO_FUNCTION);
    or_l(0x00000100, &GPIO_INT_EN);     /* GPI0 H-L */
}

static inline void enable_pmu_interrupts(void)
{
    or_l(0x3, &INTPRI5); /* INT32 - Priority 3 */
}

void pcf50606_init(void)
{
    /* Bit banged I2C */
    or_l(SDA_BIT, &SDA_GPIO_FUNCTION);
    or_l(SCL_BIT, &SCL_GPIO_FUNCTION);
    and_l(~SDA_BIT, &SDA_GPIO_OUT);
    and_l(~SCL_BIT, &SCL_GPIO_OUT);
    and_l(~SDA_BIT, &SDA_GPIO_ENABLE);
    and_l(~SCL_BIT, &SCL_GPIO_ENABLE);

    /* initialize pmu interrupts but don't service them yet */
    init_pmu_interrupts();
    set_voltages();

    pcf50606_write(0x39, 0x00); /* GPOOD0 = green led OFF */
    pcf50606_write(0x3a, 0x00); /* GPOOD1 = red led OFF */

    pcf50606_write(0x35, 0x11); /* Backlight PWM = 512Hz, 8/16, Active */
#ifdef BOOTLOADER
    /* Backlight starts OFF in bootloader */
    pcf50606_write(0x38, 0x80); /* Backlight OFF, GPO1INV=1, GPO1ACT=011 */
#else
    /* Keep backlight on when changing to firmware */
    pcf50606_write(0x38, 0xb0); /* Backlight ON, GPO1INV=1, GPO1ACT=011 */
#endif

    /* allow GPI0 interrupts from PMU now */
    enable_pmu_interrupts();
}

void pcf50606_reset_timeout(void)
{
    int level = set_irq_level(HIGHEST_IRQ_LEVEL);
    pcf50606_write(0x08, pcf50606_read(0x08) | 0x02); /* OOCC1 - TOTRST=1 */
    set_irq_level(level);
}

/* Handles interrupts generated by the pcf50606 */
void GPI0(void) __attribute__ ((interrupt_handler, section(".text")));
void GPI0(void)
{
    char read[3]; /* 0 = INT1, 1 = INT2, 2 = INT3 */

    /* clear pending interrupts from pcf50606 */
    pcf50606_read_multiple(0x02, read, 3);

    if (read[0] & 0x04)
    {
        /* ONKEY1S */
        if (GPIO_READ & 0x02000000)
            sys_poweroff();             /* main ONKEY */
        else
            pcf50606_reset_timeout();   /* remote ONKEY */
    }

    /* Clear pending GPI0 interrupts */
    or_l(0x00000100, &GPIO_INT_CLEAR);
}
