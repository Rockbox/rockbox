/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2004 by Linus Nielsen Feltzing
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#ifndef __MCF5249_H__
#define __MCF5249_H__

#define MBAR  0x40000000
#define MBAR2 0x80000000

#define SYSTEM_CTRL     (*(volatile unsigned char *)(MBAR + 0x000))
#define BUSMASTER_CTRL  (*(volatile unsigned char *)(MBAR + 0x00c))

#define IPR    (*(volatile unsigned long *)(MBAR + 0x040))
#define IMR    (*(volatile unsigned long *)(MBAR + 0x044))
#define ICR0   (*(volatile unsigned long *)(MBAR + 0x04c))
#define ICR4   (*(volatile unsigned long *)(MBAR + 0x050))
#define ICR8   (*(volatile unsigned long *)(MBAR + 0x054))

#define CSAR0 (*(volatile unsigned long *)(MBAR + 0x080))
#define CSMR0 (*(volatile unsigned long *)(MBAR + 0x084))
#define CSCR0 (*(volatile unsigned long *)(MBAR + 0x088))
#define CSAR1 (*(volatile unsigned long *)(MBAR + 0x08c))
#define CSMR1 (*(volatile unsigned long *)(MBAR + 0x090))
#define CSCR1 (*(volatile unsigned long *)(MBAR + 0x094))
#define CSAR2 (*(volatile unsigned long *)(MBAR + 0x098))
#define CSMR2 (*(volatile unsigned long *)(MBAR + 0x09c))
#define CSCR2 (*(volatile unsigned long *)(MBAR + 0x0a0))
#define CSAR3 (*(volatile unsigned long *)(MBAR + 0x0a4))
#define CSMR3 (*(volatile unsigned long *)(MBAR + 0x0a8))
#define CSCR3 (*(volatile unsigned long *)(MBAR + 0x0ac))

#define DCR   (*(volatile unsigned short *)(MBAR + 0x100))
#define DACR0 (*(volatile unsigned long *)(MBAR + 0x108))
#define DMR0  (*(volatile unsigned long *)(MBAR + 0x10c))
#define DACR1 (*(volatile unsigned long *)(MBAR + 0x110))
#define DMR1  (*(volatile unsigned long *)(MBAR + 0x114))
#define TMR0  (*(volatile unsigned short *)(MBAR + 0x140))
#define TRR0  (*(volatile unsigned short *)(MBAR + 0x144))
#define TCR0  (*(volatile unsigned short *)(MBAR + 0x148))
#define TCN0  (*(volatile unsigned short *)(MBAR + 0x14c))
#define TER0  (*(volatile unsigned short *)(MBAR + 0x150))
#define TMR1  (*(volatile unsigned short *)(MBAR + 0x180))
#define TRR1  (*(volatile unsigned short *)(MBAR + 0x184))
#define TCR1  (*(volatile unsigned short *)(MBAR + 0x188))
#define TCN1  (*(volatile unsigned short *)(MBAR + 0x18c))
#define TER1  (*(volatile unsigned short *)(MBAR + 0x190))

#define UMR10  (*(volatile unsigned char *)(MBAR + 0x1c0))
#define UMR20  (*(volatile unsigned char *)(MBAR + 0x1c0))
#define USR0   (*(volatile unsigned char *)(MBAR + 0x1c4))
#define UCSR0  (*(volatile unsigned char *)(MBAR + 0x1c4))
#define UCR0   (*(volatile unsigned char *)(MBAR + 0x1c8))
#define URB0   (*(volatile unsigned char *)(MBAR + 0x1cc))
#define UTB0   (*(volatile unsigned char *)(MBAR + 0x1cc))
#define UIPCR0 (*(volatile unsigned char *)(MBAR + 0x1d0))
#define UACR0  (*(volatile unsigned char *)(MBAR + 0x1d0))
#define UISR0  (*(volatile unsigned char *)(MBAR + 0x1d4))
#define UIMR0  (*(volatile unsigned char *)(MBAR + 0x1d4))
#define UBG10  (*(volatile unsigned char *)(MBAR + 0x1d8))
#define UBG20  (*(volatile unsigned char *)(MBAR + 0x1dc))
#define UIVR0  (*(volatile unsigned char *)(MBAR + 0x1f0))
#define UIP0   (*(volatile unsigned char *)(MBAR + 0x1f4))
#define UOP10  (*(volatile unsigned char *)(MBAR + 0x1f8))
#define UOP00  (*(volatile unsigned char *)(MBAR + 0x1fc))

#define UMR11  (*(volatile unsigned char *)(MBAR + 0x200))
#define UMR21  (*(volatile unsigned char *)(MBAR + 0x200))
#define USR1   (*(volatile unsigned char *)(MBAR + 0x204))
#define UCSR1  (*(volatile unsigned char *)(MBAR + 0x204))
#define UCR1   (*(volatile unsigned char *)(MBAR + 0x208))
#define URB1   (*(volatile unsigned char *)(MBAR + 0x20c))
#define UTB1   (*(volatile unsigned char *)(MBAR + 0x20c))
#define UIPCR1 (*(volatile unsigned char *)(MBAR + 0x210))
#define UACR1  (*(volatile unsigned char *)(MBAR + 0x210))
#define UISR1  (*(volatile unsigned char *)(MBAR + 0x214))
#define UIMR1  (*(volatile unsigned char *)(MBAR + 0x214))
#define UBG11  (*(volatile unsigned char *)(MBAR + 0x218))
#define UBG21  (*(volatile unsigned char *)(MBAR + 0x21c))
#define UIVR1  (*(volatile unsigned char *)(MBAR + 0x230))
#define UIP1   (*(volatile unsigned char *)(MBAR + 0x234))
#define UOP11  (*(volatile unsigned char *)(MBAR + 0x238))
#define UOP01  (*(volatile unsigned char *)(MBAR + 0x23c))

#define MADR   (*(volatile unsigned char *)(MBAR + 0x280))
#define MFDR   (*(volatile unsigned char *)(MBAR + 0x284))
#define MBCR   (*(volatile unsigned char *)(MBAR + 0x288))
#define MBSR   (*(volatile unsigned char *)(MBAR + 0x28c))
#define MBDR   (*(volatile unsigned char *)(MBAR + 0x290))

#define SAR0   (*(volatile unsigned long *)(MBAR + 0x300))
#define DAR0   (*(volatile unsigned long *)(MBAR + 0x304))
#define DCR0   (*(volatile unsigned long *)(MBAR + 0x308))
#define BCR0   (*(volatile unsigned long *)(MBAR + 0x30c))
#define DSR0   (*(volatile unsigned char *)(MBAR + 0x310))
#define DIVR0  (*(volatile unsigned char *)(MBAR + 0x314))

#define SAR1   (*(volatile unsigned long *)(MBAR + 0x340))
#define DAR1   (*(volatile unsigned long *)(MBAR + 0x344))
#define DCR1   (*(volatile unsigned long *)(MBAR + 0x348))
#define BCR1   (*(volatile unsigned long *)(MBAR + 0x34c))
#define DSR1   (*(volatile unsigned char *)(MBAR + 0x350))
#define DIVR1  (*(volatile unsigned char *)(MBAR + 0x354))

#define SAR2   (*(volatile unsigned long *)(MBAR + 0x380))
#define DAR2   (*(volatile unsigned long *)(MBAR + 0x384))
#define DCR2   (*(volatile unsigned long *)(MBAR + 0x388))
#define BCR2   (*(volatile unsigned long *)(MBAR + 0x38c))
#define DSR2   (*(volatile unsigned char *)(MBAR + 0x390))
#define DIVR2  (*(volatile unsigned char *)(MBAR + 0x394))

#define SAR3   (*(volatile unsigned long *)(MBAR + 0x3c0))
#define DAR3   (*(volatile unsigned long *)(MBAR + 0x3c4))
#define DCR3   (*(volatile unsigned long *)(MBAR + 0x3c8))
#define BCR3   (*(volatile unsigned long *)(MBAR + 0x3cc))
#define DSR3   (*(volatile unsigned char *)(MBAR + 0x3d0))
#define DIVR3  (*(volatile unsigned char *)(MBAR + 0x3d4))

#define QSPIMR    (*(volatile unsigned short *)(MBAR + 0x400))
#define QSPIQDLYR (*(volatile unsigned short *)(MBAR + 0x404))
#define QSPIQWR   (*(volatile unsigned short *)(MBAR + 0x408))
#define QSPIQIR   (*(volatile unsigned short *)(MBAR + 0x40c))
#define QSPIQAR   (*(volatile unsigned short *)(MBAR + 0x410))
#define QIR       (*(volatile unsigned short *)(MBAR + 0x414))

#define GPIO_READ   (*(volatile unsigned long *)(MBAR2 + 0x000))
#define GPIO_OUT    (*(volatile unsigned long *)(MBAR2 + 0x004))
#define GPIO_ENABLE (*(volatile unsigned long *)(MBAR2 + 0x008))
#define GPIO_FUNCTION (*(volatile unsigned long *)(MBAR2 + 0x00c))

#define IIS1CONFIG (*(volatile unsigned long *)(MBAR2 + 0x010))
#define IIS2CONFIG (*(volatile unsigned long *)(MBAR2 + 0x014))
#define IIS3CONFIG (*(volatile unsigned long *)(MBAR2 + 0x018))
#define IIS4CONFIG (*(volatile unsigned long *)(MBAR2 + 0x01c))
#define EBU1CONFIG (*(volatile unsigned long *)(MBAR2 + 0x020))
#define EBU1RCVCCHANNEL1 (*(volatile unsigned long *)(MBAR2 + 0x024))
#define EBUTXCCHANNEL1   (*(volatile unsigned long *)(MBAR2 + 0x028))
#define EBUTXCCHANNEL2   (*(volatile unsigned long *)(MBAR2 + 0x02c))
#define DATAINCONTROL    (*(volatile unsigned long *)(MBAR2 + 0x030))
#define PDIR1_L  (*(volatile unsigned long *)(MBAR2 + 0x034))
#define PDIR3_L  (*(volatile unsigned long *)(MBAR2 + 0x044))
#define PDIR1_R  (*(volatile unsigned long *)(MBAR2 + 0x054))
#define PDIR3_R  (*(volatile unsigned long *)(MBAR2 + 0x064))
#define PDOR1_L  (*(volatile unsigned long *)(MBAR2 + 0x034))
#define PDOR1_R  (*(volatile unsigned long *)(MBAR2 + 0x044))
#define PDOR2_L  (*(volatile unsigned long *)(MBAR2 + 0x054))
#define PDOR2_R  (*(volatile unsigned long *)(MBAR2 + 0x064))
#define PDIR3    (*(volatile unsigned long *)(MBAR2 + 0x074))
#define PDOR3    (*(volatile unsigned long *)(MBAR2 + 0x074))
#define UCHANNELTRANSMIT  (*(volatile unsigned long *)(MBAR2 + 0x084))
#define U1CHANNELRECEIVE  (*(volatile unsigned long *)(MBAR2 + 0x088))
#define Q1CHANNELRECEIVE  (*(volatile unsigned long *)(MBAR2 + 0x08c))
#define CD_TEXT_CONTROL   (*(volatile unsigned char *)(MBAR2 + 0x092))
#define INTERRUPTEN       (*(volatile unsigned long *)(MBAR2 + 0x094))
#define INTERRUPTCLEAR    (*(volatile unsigned long *)(MBAR2 + 0x098))
#define INTERRUPTSTAT     (*(volatile unsigned long *)(MBAR2 + 0x098))
#define DMACONFIG         (*(volatile unsigned char *)(MBAR2 + 0x09f))
#define PHASECONFIG       (*(volatile unsigned char *)(MBAR2 + 0x0a3))
#define XTRIM             (*(volatile unsigned short *)(MBAR2 + 0x0a6))
#define FREQMEAS          (*(volatile unsigned long *)(MBAR2 + 0x0a8))
#define BLOCKCONTROL      (*(volatile unsigned short *)(MBAR2 + 0x0ca))
#define AUDIOGLOB         (*(volatile unsigned short *)(MBAR2 + 0x0ce))
#define EBU2CONFIG        (*(volatile unsigned long *)(MBAR2 + 0x0d0))
#define EBU2RCVCCHANNEL1  (*(volatile unsigned short *)(MBAR2 + 0x0d4))
#define U2CHANNELRECEIVE  (*(volatile unsigned long *)(MBAR2 + 0x0d8))
#define Q2CHANNELRECEIVE  (*(volatile unsigned long *)(MBAR2 + 0x0dc))

#define GPIO1_READ   (*(volatile unsigned long *)(MBAR2 + 0x0b0))
#define GPIO1_OUT    (*(volatile unsigned long *)(MBAR2 + 0x0b4))
#define GPIO1_ENABLE (*(volatile unsigned long *)(MBAR2 + 0x0b8))
#define GPIO1_FUNCTION (*(volatile unsigned long *)(MBAR2 + 0x0bc))
#define GPIO_INT_STAT  (*(volatile unsigned long *)(MBAR2 + 0x0c0))
#define GPIO_INT_CLEAR (*(volatile unsigned long *)(MBAR2 + 0x0c0))
#define GPIO_INT_EN    (*(volatile unsigned long *)(MBAR2 + 0x0c4))
#define INTERRUPTSTAT3  (*(volatile unsigned long *)(MBAR2 + 0x0e0))
#define INTERRUPTCLEAR3 (*(volatile unsigned long *)(MBAR2 + 0x0e0))
#define INTERRUPTEN3    (*(volatile unsigned long *)(MBAR2 + 0x0e4))
#define INTPRI1  (*(volatile unsigned long *)(MBAR2 + 0x140))
#define INTPRI2  (*(volatile unsigned long *)(MBAR2 + 0x144))
#define INTPRI3  (*(volatile unsigned long *)(MBAR2 + 0x148))
#define INTPRI4  (*(volatile unsigned long *)(MBAR2 + 0x14c))
#define INTPRI5  (*(volatile unsigned long *)(MBAR2 + 0x150))
#define INTPRI6  (*(volatile unsigned long *)(MBAR2 + 0x154))
#define INTPRI7  (*(volatile unsigned long *)(MBAR2 + 0x158))
#define INTPRI8  (*(volatile unsigned long *)(MBAR2 + 0x15c))
#define SPURVEC  (*(volatile unsigned char *)(MBAR2 + 0x167))
#define INTBASE  (*(volatile unsigned char *)(MBAR2 + 0x16b))
#define PLLCR      (*(volatile unsigned long *)(MBAR2 + 0x180))
#define DMAROUTE   (*(volatile unsigned long *)(MBAR2 + 0x188))
#define IDECONFIG1 (*(volatile unsigned long *)(MBAR2 + 0x18c))
#define IDECONFIG2 (*(volatile unsigned long *)(MBAR2 + 0x190))
#define IPERRORADR (*(volatile unsigned long *)(MBAR2 + 0x194))
#define EXTRAINT   (*(volatile unsigned long *)(MBAR2 + 0x198))

#define ADCONFIG   (*(volatile unsigned short *)(MBAR2 + 0x402))
#define ADVALUE    (*(volatile unsigned short *)(MBAR2 + 0x406))
#define MADR2      (*(volatile unsigned char *)(MBAR2 + 0x440))
#define MFDR2      (*(volatile unsigned char *)(MBAR2 + 0x444))
#define MBCR2      (*(volatile unsigned char *)(MBAR2 + 0x448))
#define MBSR2      (*(volatile unsigned char *)(MBAR2 + 0x44c))
#define MBDR2      (*(volatile unsigned char *)(MBAR2 + 0x450))

#define FLASHMEDIACONFIG   (*(volatile unsigned long *)(MBAR2 + 0x460))
#define FLASHMEDIACMD1     (*(volatile unsigned long *)(MBAR2 + 0x464))
#define FLASHMEDIACMD2     (*(volatile unsigned long *)(MBAR2 + 0x468))
#define FLASHMEDIADATA1    (*(volatile unsigned long *)(MBAR2 + 0x46c))
#define FLASHMEDIADATA2    (*(volatile unsigned long *)(MBAR2 + 0x470))
#define FLASHMEDIASTATUS   (*(volatile unsigned long *)(MBAR2 + 0x474))
#define FLASHMEDIAINTEN    (*(volatile unsigned long *)(MBAR2 + 0x478))
#define FLASHMEDIAINTSTAT  (*(volatile unsigned long *)(MBAR2 + 0x47c))
#define FLASHMEDIAINTCLEAR (*(volatile unsigned long *)(MBAR2 + 0x47c))

#define DEVICE_ID (*(volatile unsigned long *)(MBAR2 + 0x0ac))

/* DMA Registers ... */

#define O_SAR   0x00        /* Source Address                       */
#define O_DAR   0x04        /* Destination Address                  */
#define O_DCR   0x08        /* DMA Control Register                 */
#define O_BCR   0x0C        /* 16 or 24 bits depending on BCR24BIT  */
#define O_DSR   0x10        /* DMA Status Register                  */
#define O_IVR   0x14        /* Interrupt Vector Register            */ 

/* DMA Control Register bits */
#define DMA_INT         (1 << 31)       /* Enable Interrupts            */
#define DMA_EEXT        (1 << 30)       /* Enable peripherial request   */
#define DMA_CS          (1 << 29)       /* Cycle Steal                  */
#define DMA_AA          (1 << 28)       /* Auto-Align                   */
#define DMA_SINC        (1 << 22)       /* Source Increment             */
#define DMA_SSIZE(x)    (((x)&3) << 20) /* Size of source data          */
#define DMA_DINC        (1 << 19)       /* Destination Increment        */
#define DMA_DSIZE(x)    (((x)&3) << 17) /* Size of destination data     */
#define DMA_START       (1 << 16)       /* Start DMA transfer           */

#define DMA_SIZE_DWORD  0               /* 4 bytes  */
#define DMA_SIZE_BYTE   1               /* 1 byte   */
#define DMA_SIZE_WORD   2               /* 2 bytes  */
#define DMA_SIZE_LINE   3               /* 16 bytes */

/* DMA Status Register bits */
#define DMA_CE          (1 << 6)        /* Configuration Error          */
#define DMA_BES         (1 << 5)        /* Bus error on source          */
#define DMA_BED         (1 << 4)        /* Bus error on destination     */
#define DMA_REQ         (1 << 2)        /* Request pending              */
#define DMA_BSY         (1 << 1)        /* DMA channel busy             */
#define DMA_DONE        (1 << 0)        /* Transfer has completed       */

/* DMAROUTE config */
#define DMA0_REQ_AUDIO_1     0x80
#define DMA0_REQ_AUDIO_2     0x81

#endif
