/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2008 by Maurus Cuelenaere
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#ifndef USB_TARGET_H
#define USB_TARGET_H

#include "dm320.h"

/* General purpose memory region #2 */
#define ISP1583_IOBASE              0x60FFC000
#define ISP1583_H_OVERRIDE

#define ISP1583_INIT_ADDRESS        (*((volatile unsigned char*)(ISP1583_IOBASE+0x0))) //char
#define ISP1583_INIT_MODE           (*((volatile unsigned short*)(ISP1583_IOBASE+0xC*2)))
#define ISP1583_INIT_INTCONF        (*((volatile unsigned char*)(ISP1583_IOBASE+0x10*2))) //char
#define ISP1583_INIT_OTG            (*((volatile unsigned char*)(ISP1583_IOBASE+0x12*2))) //char
#define ISP1583_INIT_INTEN_A        (*((volatile unsigned short*)(ISP1583_IOBASE+0x14*2)))
#define ISP1583_INIT_INTEN_B        (*((volatile unsigned short*)(ISP1583_IOBASE+0x14*2+4)))
#define ISP1583_INIT_INTEN_READ     (unsigned long)( (ISP1583_INIT_INTEN_A & 0xFFFF) | ((ISP1583_INIT_INTEN_B & 0xFFFF) << 16) )
/* Data flow registers */
#define ISP1583_DFLOW_EPINDEX       (*((volatile unsigned char*)(ISP1583_IOBASE+0xC2*2))) //char
#define ISP1583_DFLOW_CTRLFUN       (*((volatile unsigned char*)(ISP1583_IOBASE+0x28*2))) //char
#define ISP1583_DFLOW_DATA          (*((volatile unsigned short*)(ISP1583_IOBASE+0x20*2)))
#define ISP1583_DFLOW_BUFLEN        (*((volatile unsigned short*)(ISP1583_IOBASE+0x1C*2)))
#define ISP1583_DFLOW_BUFSTAT       (*((volatile unsigned char*)(ISP1583_IOBASE+0x1E*2))) //char
#define ISP1583_DFLOW_MAXPKSZ       (*((volatile unsigned short*)(ISP1583_IOBASE+0x04*2)))
#define ISP1583_DFLOW_EPTYPE        (*((volatile unsigned short*)(ISP1583_IOBASE+0x08*2)))
/* DMA registers */
#define ISP1583_DMA_ENDPOINT        (*((volatile unsigned char*)(ISP1583_IOBASE+0x58*2)))
/* General registers */
#define ISP1583_GEN_INT_A           (*((volatile unsigned short*)(ISP1583_IOBASE+0x18*2)))
#define ISP1583_GEN_INT_B           (*((volatile unsigned short*)(ISP1583_IOBASE+0x18*2+4)))
#define ISP1583_GEN_INT_READ        (unsigned long)( (ISP1583_GEN_INT_A & 0xFFFF) | ((ISP1583_GEN_INT_B & 0xFFFF) << 16))
#define ISP1583_GEN_CHIPID_A        (*((volatile unsigned short*)(ISP1583_IOBASE+0x70*2)))
#define ISP1583_GEN_CHIPID_B        (*((volatile unsigned char*)(ISP1583_IOBASE+0x70*2+4))) //char
#define ISP1583_GEN_CHIPID          (unsigned long)( (ISP1583_GEN_CHIPID_A & 0xFFFF) | ((ISP1583_GEN_CHIPID_B & 0xFFFF) << 16) )
#define ISP1583_GEN_FRAMEN0         (*((volatile unsigned short*)(ISP1583_IOBASE+0x74*2)))
#define ISP1583_GEN_SCRATCH         (*((volatile unsigned short*)(ISP1583_IOBASE+0x78*2)))
#define ISP1583_GEN_UNLCKDEV        (*((volatile unsigned short*)(ISP1583_IOBASE+0x7C*2)))
#define ISP1583_GEN_TSTMOD          (*((volatile unsigned char*)(ISP1583_IOBASE+0x84*2))) //char

#define EN_INT_CPU_TARGET           IO_INTC_EINT1 |= INTR_EINT1_EXT7
#define DIS_INT_CPU_TARGET          IO_INTC_EINT1 &= ~INTR_EINT1_EXT7
#define INT_CONF_TARGET             0
//#define INT_CONF_TARGET             2
#define set_int_value(a,b,value)    a = value & 0xFFFF; \
                                    b = value >> 16;
                                    
                                    
#define ZVM_SPECIFIC                asm volatile( \
                                                 "LDR     R12, =0x50FFC000\n" \
                                                 "LDRH    R12, [R12]\n"  \
                                                 : : : "r12")
//#define ZVM_SPECIFIC

#define USE_IRAM

#include <stdbool.h>
int usb_detect(void);
void usb_init_device(void);
bool usb_drv_connected(void);

#endif
