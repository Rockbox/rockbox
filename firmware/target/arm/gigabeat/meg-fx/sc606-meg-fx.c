#include "config.h"
#include "cpu.h"
#include <stdbool.h>
#include "kernel.h"
#include "system.h"
#include "hwcompat.h"
#include "logf.h"
#include "debug.h"
#include "string.h"

#define SLAVE_ADDRESS 0xCC

#define SDA_LO     GPHDAT &= ~(1 << 9)
#define SDA_HI     GPHDAT |= (1 << 9)
#define SDA_INPUT  GPHCON &= ~(3 << 18)
#define SDA_OUTPUT GPHCON |= (1 << 18)
#define SDA        GPHDAT & (1 << 9)

#define SCL_LO     GPHDAT &= ~(1 << 10)
#define SCL_HI     GPHDAT |= (1 << 10)
#define SCL_INPUT  GPHCON &= ~(3 << 20)
#define SCL_OUTPUT GPHCON |= (1 << 20)
#define SCL        GPHDAT & (1 << 10)

#define SCL_SDA_HI    GPHDAT |= (3 << 9)

/* arbitrary delay loop */
#define DELAY   do { int _x; for(_x=0;_x<2000;_x++);} while (0)

void sc606_i2c_start(void)
{
    SCL_SDA_HI;
    DELAY;
    SDA_LO;
    DELAY;
    SCL_LO;
}

void sc606_i2c_restart(void)
{
    SCL_SDA_HI;
    DELAY;
    SDA_LO;
    DELAY;
    SCL_LO;
}

void sc606_i2c_stop(void)
{
    SDA_LO;
    DELAY;
    SCL_HI;
    DELAY;
    SDA_HI;
    DELAY;
}

void sc606_i2c_ack(void)
{

    SDA_LO;
    SCL_HI;
    DELAY;
    SCL_LO;
}

int sc606_i2c_getack(void)
{
    int ret = 0;

    /* Don't need a delay since this follows a data bit with a delay on the end */
    SDA_INPUT;   /* And set to input */
    SCL_HI;
    DELAY;

    if (SDA) /* ack failed */
        ret = 1;

    DELAY;
    SCL_LO;
    DELAY;
    SDA_OUTPUT;
    return ret;
}

int sc606_i2c_outb(unsigned char byte)
{
    int i;

    /* clock out each bit, MSB first */
    for (i = 0x80; i; i >>= 1) {

       if (i & byte) {
           SDA_HI;
       } else {
           SDA_LO;
       }

       DELAY;
       SCL_HI;
       DELAY;
       SCL_LO;
       DELAY;
    }

    return sc606_i2c_getack();
}

unsigned char sc606_i2c_inb(void)
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

int sc606_write(unsigned char reg, unsigned char data)
{
    int x = 0;

    sc606_i2c_start();
    x += sc606_i2c_outb(SLAVE_ADDRESS);
    x += 0x10 * sc606_i2c_outb(reg);
    sc606_i2c_start();
/*    sc606_i2c_restart(); */
    x += 0x100 * sc606_i2c_outb(SLAVE_ADDRESS);
    x += 0x1000 *sc606_i2c_outb(data);
    sc606_i2c_stop();

    return x;
}

int sc606_read(unsigned char reg, unsigned char* data)
{
    int x = 0;

    sc606_i2c_start();
    sc606_i2c_outb(SLAVE_ADDRESS);
    sc606_i2c_outb(reg);
    sc606_i2c_restart();
    sc606_i2c_outb(SLAVE_ADDRESS | 1);
    *data = sc606_i2c_inb();
    sc606_i2c_stop();

    return x;
}

void sc606_init(void)
{
    /* Set GPB2 (EN) to 1 */
    GPBCON = (GPBCON & ~(3<<4)) | 1<<4;

    /* Turn enable line on */
    GPBDAT |= 1<<2;
    /* OFF GPBDAT &= ~(1 << 2); */

    /* About 400us - needs 350us */
    DELAY;
    DELAY;
    DELAY;
    DELAY;
    DELAY;

    /* Set GPH9 (SDA) and GPH10 (SCL) to 1 */
    GPHUP &= ~(3<<9);
    GPHCON = (GPHCON & ~(0xF<<18)) | 5<<18;
}

