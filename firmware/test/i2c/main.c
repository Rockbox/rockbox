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

int strlen(unsigned char* str)
{
   int i=0;
   while (*str++)
      i++;
   return i;
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
   debug("I2C Init done\n");
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
