/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Linus Nielsen Feltzing
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include "i2c.h"
#include "mas.h"
#include "sh7034.h"
#include "debug.h"

extern char mp3data[];
extern int mp3datalen;

unsigned char *mp3dataptr;

void setup_sci0(void)
{
    /* set PB12 to output */
    PBIOR |= 0x1000;

    /* Disable serial port */
    SCR0 = 0x00;

    /* Syncronous, 8N1, no prescale */
    SMR0 = 0x80;

    /* Set baudrate 1Mbit/s */
    BRR0 = 0x03;

    /* use SCK as serial clock output */
    SCR0 = 0x01;

    /* Clear FER and PER */
    SSR0 &= 0xe7;

    /* Set interrupt D priority to 0 */
//    IPRD &= 0x0ff0;

    /* set IRQ6 and IRQ7 to edge detect */
//    ICR |= 0x03;

    /* set PB15 and PB14 to inputs */
    PBIOR &= 0x7fff;
    PBIOR &= 0xbfff;

    /* set IRQ6 prio 8 and IRQ7 prio 0 */
//    IPRB = ( IPRB & 0xff00 ) | 0x80;

    IPRB = 0;
    
    /* Enable Tx (only!) */
    SCR0 = 0x20;
}

int mas_tx_ready(void)
{
    return (SSR0 & SCI_TDRE);
}


int main(void)
{
   char buf[40];
   char str[32];
   int i=0;

   /* Clear it all! */
    SSR1 &= ~(SCI_RDRF | SCI_ORER | SCI_PER | SCI_FER);

    /* This enables the serial Rx interrupt, to be able to exit into the
       debugger when you hit CTRL-C */
    SCR1 |= 0x40;
    SCR1 &= ~0x80;
    asm ("ldc\t%0,sr" : : "r"(0<<4));

    debugf("Olle: %d\n", 7);
    
   i2c_init();
   debugf("I2C Init done\n");
   i=mas_readmem(MAS_BANK_D1,0xff6,(unsigned long*)buf,2);
   if (i) {
       debugf("Error - mas_readmem() returned %d\n", i);
       while(1);
   }

   i = buf[0] | buf[1] << 8;
   debugf("MAS version: %x\n", i);
   i = buf[4] | buf[5] << 8;
   debugf("MAS revision: %x\n", i);

   i=mas_readmem(MAS_BANK_D1,0xff9,(unsigned long*)buf,7);
   if (i) {
       debugf("Error - mas_readmem() returned %d\n", i);
       while(1);
   }

   for(i = 0;i < 7;i++)
   {
       str[i*2+1] = buf[i*4];
       str[i*2] = buf[i*4+1];
   }
   str[i*2] = 0;
   debugf("Description: %s\n", str);

   i=mas_readreg(0xe6);
   if (i < 0) {
       debugf("Error - mas_readreg() returned %d\n", i);
       while(1);
   }

   debugf("Register 0xe6: %x\n", i);
   

   debugf("Writing register 0xaa\n");
   
   i=mas_writereg(0xaa, 0x1);
   if (i < 0) {
       debugf("Error - mas_writereg() returned %d\n", i);
       while(1);
   }

   i=mas_readreg(0xaa);
   if (i < 0) {
       debugf("Error - mas_readreg() returned %d\n", i);
       while(1);
   }

   debugf("Register 0xaa: %x\n", i);
   
   debugf("Writing register 0xaa again\n");
   
   i=mas_writereg(0xaa, 0);
   if (i < 0) {
       debugf("Error - mas_writereg() returned %d\n", i);
       while(1);
   }

   i=mas_readreg(0xaa);
   if (i < 0) {
       debugf("Error - mas_readreg() returned %d\n", i);
       while(1);
   }

   debugf("Register 0xaa: %x\n", i);

   i=mas_writereg(0x3b, 0x20);
   if (i < 0) {
       debugf("Error - mas_writereg() returned %d\n", i);
       while(1);
   }

   i = mas_run(1);
   if (i < 0) {
       debugf("Error - mas_run() returned %d\n", i);
       while(1);
   }

   
   setup_sci0();

   mp3dataptr = mp3data;
   i = 0;
   
   while(1)
   {
       if(PBDR & 0x4000)
       {
	   if(i < mp3datalen)
	   {
	       while(!mas_tx_ready()){};
	       /*
		* Write data into TDR and clear TDRE
		*/
	       TDR0 = mp3dataptr[i++];
	       SSR0 &= ~SCI_TDRE;
	   }
       }
       else
       {
	   debugf("MAS not ready\n");
       }
   }
   
   while(1);
}

extern const void stack(void);

const void* vectors[] __attribute__ ((section (".vectors"))) = 
{
   main,	/* Power-on reset */
   stack,	/* Power-on reset (stack pointer) */
   main,	/* Manual reset */
   stack	/* Manual reset (stack pointer) */
};
