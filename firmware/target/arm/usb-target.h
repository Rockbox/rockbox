/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 by Barry Wardelll
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

/* The PortalPlayer USB controller is compatible with Freescale i.MX31 */

/* Register addresses */
/* OTG */
#define UOG_ID                  (*(volatile unsigned long *)(0xc5000000))
#define UOG_HWGENERAL           (*(volatile unsigned long *)(0xc5000004))
#define UOG_HWHOST              (*(volatile unsigned long *)(0xc5000008))
#define UOG_HWTXBUF             (*(volatile unsigned long *)(0xc5000010))
#define UOG_HWRXBUF             (*(volatile unsigned long *)(0xc5000014))
#define UOG_CAPLENGTH           (*(volatile unsigned char *)(0xc5000100))
#define UOG_HCIVERSION          (*(volatile unsigned short *)(0xc5000102))
#define UOG_HCSPARAMS           (*(volatile unsigned long *)(0xc5000104))
#define UOG_HCCPARAMS           (*(volatile unsigned long *)(0xc5000108))
#define UOG_DCIVERSION          (*(volatile unsigned short *)(0xc5000120))
#define UOG_DCCPARAMS           (*(volatile unsigned long *)(0xc5000124))
#define UOG_USBCMD              (*(volatile unsigned long *)(0xc5000140))
#define UOG_USBSTS              (*(volatile unsigned long *)(0xc5000144))
#define UOG_USBINTR             (*(volatile unsigned long *)(0xc5000148))
#define UOG_FRINDEX             (*(volatile unsigned long *)(0xc500014c))
#define UOG_PERIODICLISTBASE    (*(volatile unsigned long *)(0xc5000154))
#define UOG_ASYNCLISTADDR       (*(volatile unsigned long *)(0xc5000158))
#define UOG_BURSTSIZE           (*(volatile unsigned long *)(0xc5000160))
#define UOG_TXFILLTUNING        (*(volatile unsigned long *)(0xc5000164))
#define UOG_ULPIVIEW            (*(volatile unsigned long *)(0xc5000170))
#define UOG_CFGFLAG             (*(volatile unsigned long *)(0xc5000180))
#define UOG_PORTSC1             (*(volatile unsigned long *)(0xc5000184))
/*#define UOG_PORTSC2             (*(volatile unsigned long *)(0xc5000188))
#define UOG_PORTSC3             (*(volatile unsigned long *)(0xc500018c))
#define UOG_PORTSC4             (*(volatile unsigned long *)(0xc5000190))
#define UOG_PORTSC5             (*(volatile unsigned long *)(0xc5000194))
#define UOG_PORTSC6             (*(volatile unsigned long *)(0xc5000198))
#define UOG_PORTSC7             (*(volatile unsigned long *)(0xc500019c))
#define UOG_PORTSC8             (*(volatile unsigned long *)(0xc50001a0))*/
#define UOG_OTGSC               (*(volatile unsigned long *)(0xc50001a4))
#define UOG_USBMODE             (*(volatile unsigned long *)(0xc50001a8))
#define UOG_ENDPTSETUPSTAT      (*(volatile unsigned long *)(0xc50001ac))
#define UOG_ENDPTPRIME          (*(volatile unsigned long *)(0xc50001b0))
#define UOG_ENDPTFLUSH          (*(volatile unsigned long *)(0xc50001b4))
#define UOG_ENDPTSTAT           (*(volatile unsigned long *)(0xc50001b8))
#define UOG_ENDPTCOMPLETE       (*(volatile unsigned long *)(0xc50001bc))
#define ENDPTCRTL0              (*(volatile unsigned long *)(0xc50001c0))
#define ENDPTCRTL1              (*(volatile unsigned long *)(0xc50001c4))
#define ENDPTCRTL2              (*(volatile unsigned long *)(0xc50001c8))
#define ENDPTCRTL3              (*(volatile unsigned long *)(0xc50001cc))
#define ENDPTCRTL4              (*(volatile unsigned long *)(0xc50001d0))
#define ENDPTCRTL5              (*(volatile unsigned long *)(0xc50001d4))
#define ENDPTCRTL6              (*(volatile unsigned long *)(0xc50001d8))
#define ENDPTCRTL7              (*(volatile unsigned long *)(0xc50001dc))
/*#define ENDPTCRTL8              (*(volatile unsigned long *)(0xc50001e0))
#define ENDPTCRTL9              (*(volatile unsigned long *)(0xc50001e4))
#define ENDPTCRTL10             (*(volatile unsigned long *)(0xc50001e8))
#define ENDPTCRTL11             (*(volatile unsigned long *)(0xc50001ec))
#define ENDPTCRTL12             (*(volatile unsigned long *)(0xc50001f0))
#define ENDPTCRTL13             (*(volatile unsigned long *)(0xc50001f4))
#define ENDPTCRTL14             (*(volatile unsigned long *)(0xc50001f8))
#define ENDPTCRTL15             (*(volatile unsigned long *)(0xc50001fc))*/

/* Host 1 */
#define UH1_ID                  (*(volatile unsigned long *)(0xc5000200))
#define UH1_HWGENERAL           (*(volatile unsigned long *)(0xc5000204))
#define UH1_HWHOST              (*(volatile unsigned long *)(0xc5000208))
#define UH1_HWTXBUF             (*(volatile unsigned long *)(0xc5000210))
#define UH1_HWRXBUF             (*(volatile unsigned long *)(0xc5000214))
#define UH1_CAPLENGTH           (*(volatile unsigned long *)(0xc5000300))
#define UH1_HCIVERSION          (*(volatile unsigned long *)(0xc5000302))
#define UH1_HCSPARAMS           (*(volatile unsigned long *)(0xc5000304))
#define UH1_HCCPARAMS           (*(volatile unsigned long *)(0xc5000308))
#define UH1_USBCMD              (*(volatile unsigned long *)(0xc5000340))
#define UH1_USBSTS              (*(volatile unsigned long *)(0xc5000344))
#define UH1_USBINTR             (*(volatile unsigned long *)(0xc5000348))
#define UH1_FRINDEX             (*(volatile unsigned long *)(0xc500034c))
#define UH1_PERIODICLISTBASE    (*(volatile unsigned long *)(0xc5000354))
#define UH1_ASYNCLISTADDR       (*(volatile unsigned long *)(0xc5000358))
#define UH1_BURSTSIZE           (*(volatile unsigned long *)(0xc5000360))
#define UH1_TXFILLTUNING        (*(volatile unsigned long *)(0xc5000364))
#define UH1_PORTSC1             (*(volatile unsigned long *)(0xc5000384))
#define UH1_USBMODE             (*(volatile unsigned long *)(0xc50003a8))

/* Host 2 */
#define UH2_ID                  (*(volatile unsigned long *)(0xc5000400))
#define UH2_HWGENERAL           (*(volatile unsigned long *)(0xc5000404))
#define UH2_HWHOST              (*(volatile unsigned long *)(0xc5000408))
#define UH2_HWTXBUF             (*(volatile unsigned long *)(0xc5000410))
#define UH2_HWRXBUF             (*(volatile unsigned long *)(0xc5000414))
#define UH2_CAPLENGTH           (*(volatile unsigned long *)(0xc5000500))
#define UH2_HCIVERSION          (*(volatile unsigned long *)(0xc5000502))
#define UH2_HCSPARAMS           (*(volatile unsigned long *)(0xc5000504))
#define UH2_HCCPARAMS           (*(volatile unsigned long *)(0xc5000508))
#define UH2_USBCMD              (*(volatile unsigned long *)(0xc5000540))
#define UH2_USBSTS              (*(volatile unsigned long *)(0xc5000544))
#define UH2_USBINTR             (*(volatile unsigned long *)(0xc5000548))
#define UH2_FRINDEX             (*(volatile unsigned long *)(0xc500054c))
#define UH2_PERIODICLISTBASE    (*(volatile unsigned long *)(0xc5000554))
#define UH2_ASYNCLISTADDR       (*(volatile unsigned long *)(0xc5000558))
#define UH2_BURSTSIZE           (*(volatile unsigned long *)(0xc5000560))
#define UH2_TXFILLTUNING        (*(volatile unsigned long *)(0xc5000564))
#define UH2_ULPIVIEW            (*(volatile unsigned long *)(0xc5000570))
#define UH2_PORTSC1             (*(volatile unsigned long *)(0xc5000584))
#define UH2_USBMODE             (*(volatile unsigned long *)(0xc50005a8))

/* General */
#define USB_CTRL                (*(volatile unsigned long *)(0xc5000600))
#define USB_OTG_MIRROR          (*(volatile unsigned long *)(0xc5000604))

void usb_init_device(void);

#endif
