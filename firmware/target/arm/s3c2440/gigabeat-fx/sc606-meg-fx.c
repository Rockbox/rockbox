#include "config.h"
#include "cpu.h"
#include "kernel.h"
#include "system.h"
#include "logf.h"
#include "debug.h"
#include "string.h"

#define SLAVE_ADDRESS 0xCC

#define USE_ASM

/* This I2C driver tristates the outputs instead of driving logic high's */
#define SDA        (GPHDAT & (1 << 9))
#define SDA_LO_OUT (GPHCON |= (1 << 18));(GPHDAT &= ~(1 << 9));
#define SDA_HI_IN  (GPHCON &= ~(3 << 18))

#define SCL        (GPHDAT & (1 << 10))
#define SCL_LO_OUT (GPHCON |= (1 << 20));(GPHDAT &= ~(1 << 10));
#define SCL_HI_IN  (GPHCON &= ~(3 << 20));while(!SCL);

/* The SC606 can clock at 400KHz:
 * Clock period high is 600nS and low is 1300nS
 * The high and low times are different enough to need different timings
 * cycles delayed = 2 + 4 * loops
 * 100MHz = 10nS per cycle: LO:1300nS=130:33  HI:600nS=60:15
 * 300MHz = 3.33nS per cycle:
 *          LO:1300nS=394:99
 *          HI:600nS=182:21 
 *          MID(50/50):950(1900/2)ns=288:72
 */

#ifdef USE_ASM

#define DELAY_LO    99
#define DELAY_MID   72
#define DELAY_HI    46
/* This delay loop takes 4 cycles/loop to execute plus 2 for setup */
#define DELAY(dly)                             \
    asm volatile(   "mov  r0,%0     \n"        \
                    "1:             \n"        \
                    "subs  r0,r0,#1  \n"        \
                    "bhi  1b         \n"        \
    : : "r"((dly)) : "r0" );
    
#else

#define DELAY_LO    51
#define DELAY_MID   35
#define DELAY_HI    21
#define DELAY(dly) do{int x;for(x=(dly);x;x--);} while (0)

#endif

static void sc606_i2c_start(void)
{
    SDA_HI_IN;
    SCL_HI_IN;
    DELAY(DELAY_MID);
    SDA_LO_OUT;
    DELAY(DELAY_MID);
    SCL_LO_OUT;
}

static void sc606_i2c_stop(void)
{
    SDA_LO_OUT;
    SCL_HI_IN;
    DELAY(DELAY_HI);
    SDA_HI_IN;
}

static void sc606_i2c_ack(void)
{
    SDA_HI_IN;
    SCL_HI_IN;

    DELAY(DELAY_HI);
    SCL_LO_OUT;
}

static bool sc606_i2c_getack(void)
{
    bool ret;

    SDA_HI_IN;
    DELAY(DELAY_MID);
    SCL_HI_IN;

    ret = !SDA;

    SCL_LO_OUT;
    DELAY(DELAY_LO);

    return ret;
}

static void sc606_i2c_outb(unsigned char byte)
{
    int i;

    /* clock out each bit, MSB first */
    for ( i=0x80; i; i>>=1 )
    {
        if ( i & byte )
            SDA_HI_IN;
        else
            SDA_LO_OUT;
        DELAY(DELAY_MID);
        SCL_HI_IN;
        DELAY(DELAY_HI);
        SCL_LO_OUT;
    }
}

static unsigned char sc606_i2c_inb(void)
{
    int i;
    unsigned char byte = 0;

    /* clock in each bit, MSB first */
    SDA_HI_IN;
    for ( i=0x80; i; i>>=1 )
    {
        SCL_HI_IN;
        DELAY(DELAY_HI);
        if ( SDA )
            byte |= i;
        SCL_LO_OUT;
        DELAY(DELAY_LO);
    }

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

    sc606_i2c_start();

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

    sc606_i2c_start();
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
        DELAY(DELAY_LO);

    /* Set GPH9 (SDA) and GPH10 (SCL) to 1 */
    GPHUP &= ~(3<<9);
    SCL_HI_IN;
    SDA_HI_IN;
}

