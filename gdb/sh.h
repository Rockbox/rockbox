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
#ifndef SH_H_INCLUDED
#define SH_H_INCLUDED

/* Support for Serial I/O using on chip uart */

#define SMR0 (*(volatile unsigned char *)(0x05FFFEC0)) /* Ch 0 serial mode  */
#define BRR0 (*(volatile unsigned char *)(0x05FFFEC1)) /* Ch 0 bit rate */
#define SCR0 (*(volatile unsigned char *)(0x05FFFEC2)) /* Ch 0 serial ctrl */
#define TDR0 (*(volatile unsigned char *)(0x05FFFEC3)) /* Ch 0 transmit data */
#define SSR0 (*(volatile unsigned char *)(0x05FFFEC4)) /* Ch 0 serial status */
#define RDR0 (*(volatile unsigned char *)(0x05FFFEC5)) /* Ch 0 receive data */

#define SMR1 (*(volatile unsigned char *)(0x05FFFEC8)) /* Ch 1 serial mode */
#define BRR1 (*(volatile unsigned char *)(0x05FFFEC9)) /* Ch 1 bit rate */
#define SCR1 (*(volatile unsigned char *)(0x05FFFECA)) /* Ch 1 serial ctrl */
#define TDR1 (*(volatile unsigned char *)(0x05FFFECB)) /* Ch 1 transmit data */
#define SSR1 (*(volatile unsigned char *)(0x05FFFECC)) /* Ch 1 serial status */
#define RDR1 (*(volatile unsigned char *)(0x05FFFECD)) /* Ch 1 receive data */

/*
 * Serial mode register bits
 */

#define SYNC_MODE 		0x80
#define SEVEN_BIT_DATA 		0x40
#define PARITY_ON		0x20
#define ODD_PARITY		0x10
#define STOP_BITS_2		0x08
#define ENABLE_MULTIP		0x04
#define PHI_64			0x03
#define PHI_16			0x02
#define PHI_4			0x01

/*
 * Serial control register bits
 */
#define SCI_TIE				0x80	/* Transmit interrupt enable */
#define SCI_RIE				0x40	/* Receive interrupt enable */
#define SCI_TE				0x20	/* Transmit enable */
#define SCI_RE				0x10	/* Receive enable */
#define SCI_MPIE			0x08	/* Multiprocessor interrupt enable */
#define SCI_TEIE			0x04	/* Transmit end interrupt enable */
#define SCI_CKE1			0x02	/* Clock enable 1 */
#define SCI_CKE0			0x01	/* Clock enable 0 */

/*
 * Serial status register bits
 */
#define SCI_TDRE			0x80	/* Transmit data register empty */
#define SCI_RDRF			0x40	/* Receive data register full */
#define SCI_ORER			0x20	/* Overrun error */
#define SCI_FER				0x10	/* Framing error */
#define SCI_PER				0x08	/* Parity error */
#define SCI_TEND			0x04	/* Transmit end */
#define SCI_MPB				0x02	/* Multiprocessor bit */
#define SCI_MPBT			0x01	/* Multiprocessor bit transfer */

/*
 * Port Registers
 */
#define PADR  (*(volatile unsigned short *)(0x5ffffc0)) /* Port A Data */
#define PAIOR (*(volatile unsigned short *)(0x5ffffc4)) /* Port A I/O */
#define PACR1 (*(volatile unsigned short *)(0x5ffffc8)) /* Port A ctrl 1 */
#define PACR2 (*(volatile unsigned short *)(0x5ffffca)) /* Port A ctrl 2 */

#define PBDR  (*(volatile unsigned short *)(0x5ffffc2)) /* Port B Data */
#define PBIOR (*(volatile unsigned short *)(0x5ffffc6)) /* Port B I/O */
#define PBCR1 (*(volatile unsigned short *)(0x5ffffcc)) /* Port B ctrl 1 */
#define PBCR2 (*(volatile unsigned short *)(0x5ffffce)) /* Port B ctrl 2 */

#define CASCR (*(volatile unsigned short *)(0x5ffffee)) /* CAS strobe pin */

#define	PB15MD 		PB15MD1|PB14MD0
#define	PB14MD 		PB14MD1|PB14MD0
#define	PB13MD 		PB13MD1|PB13MD0
#define	PB12MD 		PB12MD1|PB12MD0
#define	PB11MD 		PB11MD1|PB11MD0
#define	PB10MD 		PB10MD1|PB10MD0
#define	PB9MD 		PB9MD1|PB9MD0
#define	PB8MD 		PB8MD1|PB8MD0

#define PB_TXD1 	PB11MD1
#define PB_RXD1 	PB10MD1
#define PB_TXD0 	PB9MD1
#define PB_RXD0 	PB8MD1

#define	PB7MD 	PB7MD1|PB7MD0
#define	PB6MD 	PB6MD1|PB6MD0
#define	PB5MD 	PB5MD1|PB5MD0
#define	PB4MD 	PB4MD1|PB4MD0
#define	PB3MD 	PB3MD1|PB3MD0
#define	PB2MD 	PB2MD1|PB2MD0
#define	PB1MD 	PB1MD1|PB1MD0
#define	PB0MD 	PB0MD1|PB0MD0

/* Bus state controller registers */
#define BCR (*(volatile unsigned short *)(0x5ffffa0)) /* Bus control */
#define WCR1 (*(volatile unsigned short *)(0x5ffffa2)) /* Wait state ctrl 1 */
#define WCR2 (*(volatile unsigned short *)(0x5ffffa4)) /* Wait state ctrl 2 */
#define WCR3 (*(volatile unsigned short *)(0x5ffffa6)) /* Wait state ctrl 3 */
#define DCR (*(volatile unsigned short *)(0x5ffffa8)) /* DRAM area ctrl */
#define PCR (*(volatile unsigned short *)(0x5ffffaa)) /* Parity control */
#define RCR (*(volatile unsigned short *)(0x5ffffae)) /* Refresh control */
#define RTCSR (*(volatile unsigned short *)(0x5ffffae)) /* Refresh timer
                                                           control/status */
#define RTCNT (*(volatile unsigned short *)(0x5ffffb0)) /* Refresh timer cnt */
#define RTCOR (*(volatile unsigned short *)(0x5ffffb2)) /* Refresh time
                                                           constant */

/* Interrupt controller registers */
#define IPRA (*(volatile unsigned short *)(0x5ffff84)) /* Priority A */
#define IPRB (*(volatile unsigned short *)(0x5ffff86)) /* Priority B */
#define IPRC (*(volatile unsigned short *)(0x5ffff88)) /* Priority C */
#define IPRD (*(volatile unsigned short *)(0x5ffff88)) /* Priority D */
#define IPRE (*(volatile unsigned short *)(0x5ffff8c)) /* Priority E */
#define ICR  (*(volatile unsigned short *)(0x5ffff8e)) /* Interrupt Control */

/* ITU registers */
#define TSTR  (*(volatile unsigned char *)(0x5ffff00)) /* Timer Start */
#define TSNC  (*(volatile unsigned char *)(0x5ffff01)) /* Timer Synchro */
#define TMDR  (*(volatile unsigned char *)(0x5ffff02)) /* Timer Mode */
#define TFCR  (*(volatile unsigned char *)(0x5ffff03)) /* Timer Function Ctrl */
#define TOCR  (*(volatile unsigned char *)(0x5ffff31)) /* Timer Output Ctrl */

#define TCR0  (*(volatile unsigned char *)(0x5ffff04)) /* Timer 0 Ctrl */
#define TIOR0 (*(volatile unsigned char *)(0x5ffff05)) /* Timer 0 I/O Ctrl */
#define TIER0 (*(volatile unsigned char *)(0x5ffff06)) /* Timer 0 Int Enable */
#define TSR0  (*(volatile unsigned char *)(0x5ffff07)) /* Timer 0 Status */
#define TCNT0 (*(volatile unsigned short *)(0x5ffff08)) /* Timer 0 Count */
#define GRA0  (*(volatile unsigned short *)(0x5ffff0a)) /* Timer 0 GRA */
#define GRB0  (*(volatile unsigned short *)(0x5ffff0c)) /* Timer 0 GRB */

#define TCR1  (*(volatile unsigned char *)(0x5ffff0e)) /* Timer 1 Ctrl */
#define TIOR1 (*(volatile unsigned char *)(0x5ffff0f)) /* Timer 1 I/O Ctrl */
#define TIER1 (*(volatile unsigned char *)(0x5ffff10)) /* Timer 1 Int Enable */
#define TSR1  (*(volatile unsigned char *)(0x5ffff11)) /* Timer 1 Status */
#define TCNT1 (*(volatile unsigned short *)(0x5ffff12)) /* Timer 1 Count */
#define GRA1  (*(volatile unsigned short *)(0x5ffff14)) /* Timer 1 GRA */
#define GRB1  (*(volatile unsigned short *)(0x5ffff16)) /* Timer 1 GRB */

#define TCR2  (*(volatile unsigned char *)(0x5ffff18)) /* Timer 2 Ctrl */
#define TIOR2 (*(volatile unsigned char *)(0x5ffff19)) /* Timer 2 I/O Ctrl */
#define TIER2 (*(volatile unsigned char *)(0x5ffff1a)) /* Timer 2 Int Enable */
#define TSR2  (*(volatile unsigned char *)(0x5ffff1b)) /* Timer 2 Status */
#define TCNT2 (*(volatile unsigned short *)(0x5ffff1c)) /* Timer 2 Count */
#define GRA2  (*(volatile unsigned short *)(0x5ffff1e)) /* Timer 2 GRA */
#define GRB2  (*(volatile unsigned short *)(0x5ffff20)) /* Timer 2 GRB */

#define TCR3  (*(volatile unsigned char *)(0x5ffff22)) /* Timer 3 Ctrl */
#define TIOR3 (*(volatile unsigned char *)(0x5ffff23)) /* Timer 3 I/O Ctrl */
#define TIER3 (*(volatile unsigned char *)(0x5ffff24)) /* Timer 3 Int Enable */
#define TSR3  (*(volatile unsigned char *)(0x5ffff25)) /* Timer 3 Status */
#define TCNT3 (*(volatile unsigned short *)(0x5ffff26)) /* Timer 3 Count */
#define GRA3  (*(volatile unsigned short *)(0x5ffff28)) /* Timer 3 GRA */
#define GRB3  (*(volatile unsigned short *)(0x5ffff2a)) /* Timer 3 GRB */
#define BRA3  (*(volatile unsigned short *)(0x5ffff2c)) /* Timer 3 BRA */
#define BRB3  (*(volatile unsigned short *)(0x5ffff2e)) /* Timer 3 BRB */

#define TCR4  (*(volatile unsigned char *)(0x5ffff32)) /* Timer 4 Ctrl */
#define TIOR4 (*(volatile unsigned char *)(0x5ffff33)) /* Timer 4 I/O Ctrl */
#define TIER4 (*(volatile unsigned char *)(0x5ffff34)) /* Timer 4 Int Enable */
#define TSR4  (*(volatile unsigned char *)(0x5ffff35)) /* Timer 4 Status */
#define TCNT4 (*(volatile unsigned short *)(0x5ffff36)) /* Timer 4 Count */
#define GRA4  (*(volatile unsigned short *)(0x5ffff38)) /* Timer 4 GRA */
#define GRB4  (*(volatile unsigned short *)(0x5ffff3a)) /* Timer 4 GRB */
#define BRA4  (*(volatile unsigned short *)(0x5ffff3c)) /* Timer 4 BRA */
#define BRB4  (*(volatile unsigned short *)(0x5ffff3e)) /* Timer 4 BRB */

#endif
