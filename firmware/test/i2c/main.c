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

unsigned char fliptable[] =
{
    0x00, 0x80, 0x40, 0xc0, 0x20, 0xa0, 0x60, 0xe0, 
    0x10, 0x90, 0x50, 0xd0, 0x30, 0xb0, 0x70, 0xf0, 
    0x08, 0x88, 0x48, 0xc8, 0x28, 0xa8, 0x68, 0xe8, 
    0x18, 0x98, 0x58, 0xd8, 0x38, 0xb8, 0x78, 0xf8, 
    0x04, 0x84, 0x44, 0xc4, 0x24, 0xa4, 0x64, 0xe4, 
    0x14, 0x94, 0x54, 0xd4, 0x34, 0xb4, 0x74, 0xf4, 
    0x0c, 0x8c, 0x4c, 0xcc, 0x2c, 0xac, 0x6c, 0xec, 
    0x1c, 0x9c, 0x5c, 0xdc, 0x3c, 0xbc, 0x7c, 0xfc, 
    0x02, 0x82, 0x42, 0xc2, 0x22, 0xa2, 0x62, 0xe2, 
    0x12, 0x92, 0x52, 0xd2, 0x32, 0xb2, 0x72, 0xf2, 
    0x0a, 0x8a, 0x4a, 0xca, 0x2a, 0xaa, 0x6a, 0xea, 
    0x1a, 0x9a, 0x5a, 0xda, 0x3a, 0xba, 0x7a, 0xfa, 
    0x06, 0x86, 0x46, 0xc6, 0x26, 0xa6, 0x66, 0xe6, 
    0x16, 0x96, 0x56, 0xd6, 0x36, 0xb6, 0x76, 0xf6, 
    0x0e, 0x8e, 0x4e, 0xce, 0x2e, 0xae, 0x6e, 0xee, 
    0x1e, 0x9e, 0x5e, 0xde, 0x3e, 0xbe, 0x7e, 0xfe, 
    0x01, 0x81, 0x41, 0xc1, 0x21, 0xa1, 0x61, 0xe1, 
    0x11, 0x91, 0x51, 0xd1, 0x31, 0xb1, 0x71, 0xf1, 
    0x09, 0x89, 0x49, 0xc9, 0x29, 0xa9, 0x69, 0xe9, 
    0x19, 0x99, 0x59, 0xd9, 0x39, 0xb9, 0x79, 0xf9, 
    0x05, 0x85, 0x45, 0xc5, 0x25, 0xa5, 0x65, 0xe5, 
    0x15, 0x95, 0x55, 0xd5, 0x35, 0xb5, 0x75, 0xf5, 
    0x0d, 0x8d, 0x4d, 0xcd, 0x2d, 0xad, 0x6d, 0xed, 
    0x1d, 0x9d, 0x5d, 0xdd, 0x3d, 0xbd, 0x7d, 0xfd, 
    0x03, 0x83, 0x43, 0xc3, 0x23, 0xa3, 0x63, 0xe3, 
    0x13, 0x93, 0x53, 0xd3, 0x33, 0xb3, 0x73, 0xf3, 
    0x0b, 0x8b, 0x4b, 0xcb, 0x2b, 0xab, 0x6b, 0xeb, 
    0x1b, 0x9b, 0x5b, 0xdb, 0x3b, 0xbb, 0x7b, 0xfb, 
    0x07, 0x87, 0x47, 0xc7, 0x27, 0xa7, 0x67, 0xe7, 
    0x17, 0x97, 0x57, 0xd7, 0x37, 0xb7, 0x77, 0xf7, 
    0x0f, 0x8f, 0x4f, 0xcf, 0x2f, 0xaf, 0x6f, 0xef, 
    0x1f, 0x9f, 0x5f, 0xdf, 0x3f, 0xbf, 0x7f, 0xff
};

extern unsigned char mp3data[];
extern int mp3datalen;

void setup_sci0(void)
{
    PBCR1 = (PBCR1 & 0xccff) | 0x0200;
    
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

    /* Set interrupt ITU2 and SCI0 priority to 0 */
    IPRD &= 0x0ff0;

    /* set IRQ6 and IRQ7 to edge detect */
//    ICR |= 0x03;

    /* set PB15 and PB14 to inputs */
    PBIOR &= 0x7fff;
    PBIOR &= 0xbfff;

    /* set IRQ6 prio 8 and IRQ7 prio 0 */
//    IPRB = ( IPRB & 0xff00 ) | 0x80;

    IPRB = 0;
    
    /* Enable Tx (only!) */
    SCR0 |= 0xa0;
}

int mas_tx_ready(void)
{
    return (SSR0 & SCI_TDRE);
}

void init_dma(void)
{
    SAR3 = (unsigned int) mp3data;
    DAR3 = 0xFFFFEC3;
    CHCR3 = 0x1500; /* Single address destination, TXI0 */
    DTCR3 = 64000;
    DMAOR = 0x0001; /* Enable DMA */
}

void start_dma(void)
{
    CHCR3 |= 1;
}

void stop_dma(void)
{
    CHCR3 &= ~1;
}


int main(void)
{
   char buf[40];
   char str[32];
   int i=0;
   int dma_on = 0;

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

   i=mas_readreg(0xed);
   if (i < 0) {
       debugf("Error - mas_readreg(ed) returned %d\n", i);
       while(1);
   }

   debugf("Register 0xed: %x\n", i);

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

   i = 0;

   init_dma();

   while(1)
   {
       /* Demand pin high? */
       if(PBDR & 0x4000)
       {
           start_dma();
#if 0
           /* More data to write? */
           if(i < mp3datalen)
           {
               /* Transmitter ready? */
               while(!mas_tx_ready()){};
               
                /* Write data into TDR and clear TDRE */
               TDR0 = fliptable[mp3data[i++]];
               SSR0 &= ~SCI_TDRE;
           }
#endif
       }
       else
       {
           stop_dma();
       }
   }
   
   while(1);
}
