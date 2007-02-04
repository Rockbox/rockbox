/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id: $
 *
 * Copyright (C) 2007 by Barry Wardell
 *
 * i.MX31 driver based on code from the Linux Target Image Builder from
 * Freescale - http://www.bitshrine.org/ and 
 * http://www.bitshrine.org/gpp/linux-2.6.16-mx31-usb-2.patch
 * Adapted for Rockbox in January 2007
 * Original file: drivers/usb/gadget/arcotg_udc.h
 *
 * Copyright 2004-2006 Freescale Semiconductor, Inc. All Rights Reserved.
 *
 * Based on mpc-udc.h
 * Author: Li Yang (leoli@freescale.com)
 *         Jiang Bo (Tanya.jiang@freescale.com)
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

/*
 * Freescale USB device/endpoint management registers
 */
#ifndef __MX31_H
#define __MX31_H

/* Register addresses - from Freescale i.MX31 reference manual */
/* The PortalPlayer USB controller usec base address 0xc5000000 */
#define USB_BASE                0xc5000000

/* OTG */
#define UOG_ID                  (*(volatile unsigned int *)(USB_BASE+0x000))
#define UOG_HWGENERAL           (*(volatile unsigned int *)(USB_BASE+0x004))
#define UOG_HWHOST              (*(volatile unsigned int *)(USB_BASE+0x008))
#define UOG_HWTXBUF             (*(volatile unsigned int *)(USB_BASE+0x010))
#define UOG_HWRXBUF             (*(volatile unsigned int *)(USB_BASE+0x014))
#define UOG_CAPLENGTH           (*(volatile unsigned char *)(USB_BASE+0x100))
#define UOG_HCIVERSION          (*(volatile unsigned short *)(USB_BASE+0x102))
#define UOG_HCSPARAMS           (*(volatile unsigned int *)(USB_BASE+0x104))
#define UOG_HCCPARAMS           (*(volatile unsigned int *)(USB_BASE+0x108))
#define UOG_DCIVERSION          (*(volatile unsigned short *)(USB_BASE+0x120))
#define UOG_DCCPARAMS           (*(volatile unsigned int *)(USB_BASE+0x124))
#define UOG_USBCMD              (*(volatile unsigned int *)(USB_BASE+0x140))
#define UOG_USBSTS              (*(volatile unsigned int *)(USB_BASE+0x144))
#define UOG_USBINTR             (*(volatile unsigned int *)(USB_BASE+0x148))
#define UOG_FRINDEX             (*(volatile unsigned int *)(USB_BASE+0x14c))
#define UOG_PERIODICLISTBASE    (*(volatile unsigned int *)(USB_BASE+0x154))
#define UOG_ASYNCLISTADDR       (*(volatile unsigned int *)(USB_BASE+0x158))
#define UOG_BURSTSIZE           (*(volatile unsigned int *)(USB_BASE+0x160))
#define UOG_TXFILLTUNING        (*(volatile unsigned int *)(USB_BASE+0x164))
#define UOG_ULPIVIEW            (*(volatile unsigned int *)(USB_BASE+0x170))
#define UOG_CFGFLAG             (*(volatile unsigned int *)(USB_BASE+0x180))
#define UOG_PORTSC1             (*(volatile unsigned int *)(USB_BASE+0x184))
/*#define UOG_PORTSC2             (*(volatile unsigned int *)(USB_BASE+0x188))
#define UOG_PORTSC3             (*(volatile unsigned int *)(USB_BASE+0x18c))
#define UOG_PORTSC4             (*(volatile unsigned int *)(USB_BASE+0x190))
#define UOG_PORTSC5             (*(volatile unsigned int *)(USB_BASE+0x194))
#define UOG_PORTSC6             (*(volatile unsigned int *)(USB_BASE+0x198))
#define UOG_PORTSC7             (*(volatile unsigned int *)(USB_BASE+0x19c))
#define UOG_PORTSC8             (*(volatile unsigned int *)(USB_BASE+0x1a0))*/
#define UOG_OTGSC               (*(volatile unsigned int *)(USB_BASE+0x1a4))
#define UOG_USBMODE             (*(volatile unsigned int *)(USB_BASE+0x1a8))
#define UOG_ENDPTSETUPSTAT      (*(volatile unsigned int *)(USB_BASE+0x1ac))
#define UOG_ENDPTPRIME          (*(volatile unsigned int *)(USB_BASE+0x1b0))
#define UOG_ENDPTFLUSH          (*(volatile unsigned int *)(USB_BASE+0x1b4))
#define UOG_ENDPTSTAT           (*(volatile unsigned int *)(USB_BASE+0x1b8))
#define UOG_ENDPTCOMPLETE       (*(volatile unsigned int *)(USB_BASE+0x1bc))
#define ENDPTCRTL0              (*(volatile unsigned int *)(USB_BASE+0x1c0))
#define ENDPTCRTL1              (*(volatile unsigned int *)(USB_BASE+0x1c4))
#define ENDPTCRTL2              (*(volatile unsigned int *)(USB_BASE+0x1c8))
#define ENDPTCRTL3              (*(volatile unsigned int *)(USB_BASE+0x1cc))
#define ENDPTCRTL4              (*(volatile unsigned int *)(USB_BASE+0x1d0))
#define ENDPTCRTL5              (*(volatile unsigned int *)(USB_BASE+0x1d4))
#define ENDPTCRTL6              (*(volatile unsigned int *)(USB_BASE+0x1d8))
#define ENDPTCRTL7              (*(volatile unsigned int *)(USB_BASE+0x1dc))
/*#define ENDPTCRTL8              (*(volatile unsigned int *)(USB_BASE+0x1e0))
#define ENDPTCRTL9              (*(volatile unsigned int *)(USB_BASE+0x1e4))
#define ENDPTCRTL10             (*(volatile unsigned int *)(USB_BASE+0x1e8))
#define ENDPTCRTL11             (*(volatile unsigned int *)(USB_BASE+0x1ec))
#define ENDPTCRTL12             (*(volatile unsigned int *)(USB_BASE+0x1f0))
#define ENDPTCRTL13             (*(volatile unsigned int *)(USB_BASE+0x1f4))
#define ENDPTCRTL14             (*(volatile unsigned int *)(USB_BASE+0x1f8))
#define ENDPTCRTL15             (*(volatile unsigned int *)(USB_BASE+0x1fc))*/

/* Host 1 */
#define UH1_ID                  (*(volatile unsigned int *)(USB_BASE+0x200))
#define UH1_HWGENERAL           (*(volatile unsigned int *)(USB_BASE+0x204))
#define UH1_HWHOST              (*(volatile unsigned int *)(USB_BASE+0x208))
#define UH1_HWTXBUF             (*(volatile unsigned int *)(USB_BASE+0x210))
#define UH1_HWRXBUF             (*(volatile unsigned int *)(USB_BASE+0x214))
#define UH1_CAPLENGTH           (*(volatile unsigned int *)(USB_BASE+0x300))
#define UH1_HCIVERSION          (*(volatile unsigned int *)(USB_BASE+0x302))
#define UH1_HCSPARAMS           (*(volatile unsigned int *)(USB_BASE+0x304))
#define UH1_HCCPARAMS           (*(volatile unsigned int *)(USB_BASE+0x308))
#define UH1_USBCMD              (*(volatile unsigned int *)(USB_BASE+0x340))
#define UH1_USBSTS              (*(volatile unsigned int *)(USB_BASE+0x344))
#define UH1_USBINTR             (*(volatile unsigned int *)(USB_BASE+0x348))
#define UH1_FRINDEX             (*(volatile unsigned int *)(USB_BASE+0x34c))
#define UH1_PERIODICLISTBASE    (*(volatile unsigned int *)(USB_BASE+0x354))
#define UH1_ASYNCLISTADDR       (*(volatile unsigned int *)(USB_BASE+0x358))
#define UH1_BURSTSIZE           (*(volatile unsigned int *)(USB_BASE+0x360))
#define UH1_TXFILLTUNING        (*(volatile unsigned int *)(USB_BASE+0x364))
#define UH1_PORTSC1             (*(volatile unsigned int *)(USB_BASE+0x384))
#define UH1_USBMODE             (*(volatile unsigned int *)(USB_BASE+0x3a8))

/* Host 2 */
#define UH2_ID                  (*(volatile unsigned int *)(USB_BASE+0x400))
#define UH2_HWGENERAL           (*(volatile unsigned int *)(USB_BASE+0x404))
#define UH2_HWHOST              (*(volatile unsigned int *)(USB_BASE+0x408))
#define UH2_HWTXBUF             (*(volatile unsigned int *)(USB_BASE+0x410))
#define UH2_HWRXBUF             (*(volatile unsigned int *)(USB_BASE+0x414))
#define UH2_CAPLENGTH           (*(volatile unsigned int *)(USB_BASE+0x500))
#define UH2_HCIVERSION          (*(volatile unsigned int *)(USB_BASE+0x502))
#define UH2_HCSPARAMS           (*(volatile unsigned int *)(USB_BASE+0x504))
#define UH2_HCCPARAMS           (*(volatile unsigned int *)(USB_BASE+0x508))
#define UH2_USBCMD              (*(volatile unsigned int *)(USB_BASE+0x540))
#define UH2_USBSTS              (*(volatile unsigned int *)(USB_BASE+0x544))
#define UH2_USBINTR             (*(volatile unsigned int *)(USB_BASE+0x548))
#define UH2_FRINDEX             (*(volatile unsigned int *)(USB_BASE+0x54c))
#define UH2_PERIODICLISTBASE    (*(volatile unsigned int *)(USB_BASE+0x554))
#define UH2_ASYNCLISTADDR       (*(volatile unsigned int *)(USB_BASE+0x558))
#define UH2_BURSTSIZE           (*(volatile unsigned int *)(USB_BASE+0x560))
#define UH2_TXFILLTUNING        (*(volatile unsigned int *)(USB_BASE+0x564))
#define UH2_ULPIVIEW            (*(volatile unsigned int *)(USB_BASE+0x570))
#define UH2_PORTSC1             (*(volatile unsigned int *)(USB_BASE+0x584))
#define UH2_USBMODE             (*(volatile unsigned int *)(USB_BASE+0x5a8))

/* General */
#define USB_CTRL                (*(volatile unsigned int *)(USB_BASE+0x600))
#define USB_OTG_MIRROR          (*(volatile unsigned int *)(USB_BASE+0x604))

/* Maximum values */
#define USB_MAX_ENDPOINTS       8
#define USB_MAX_PIPES           (USB_MAX_ENDPOINTS*2)
#define USB_MAX_CTRL_PAYLOAD    64

/* ep0 transfer state */
#define WAIT_FOR_SETUP          0
#define DATA_STATE_XMIT         1
#define DATA_STATE_NEED_ZLP     2
#define WAIT_FOR_OUT_STATUS     3
#define DATA_STATE_RECV         4

/* Frame Index Register Bit Masks */
#define  USB_FRINDEX_MASKS                      (0x3fff)

/* USB CMD  Register Bit Masks */
#define  USB_CMD_RUN_STOP                       (0x00000001)
#define  USB_CMD_CTRL_RESET                     (0x00000002)
#define  USB_CMD_PERIODIC_SCHEDULE_EN           (0x00000010)
#define  USB_CMD_ASYNC_SCHEDULE_EN              (0x00000020)
#define  USB_CMD_INT_AA_DOORBELL                (0x00000040)
#define  USB_CMD_ASP                            (0x00000300)
#define  USB_CMD_ASYNC_SCH_PARK_EN              (0x00000800)
#define  USB_CMD_SUTW                           (0x00002000)
#define  USB_CMD_ATDTW                          (0x00004000)
#define  USB_CMD_ITC                            (0x00FF0000)

/* bit 15,3,2 are frame list size */
#define  USB_CMD_FRAME_SIZE_1024                (0x00000000)
#define  USB_CMD_FRAME_SIZE_512                 (0x00000004)
#define  USB_CMD_FRAME_SIZE_256                 (0x00000008)
#define  USB_CMD_FRAME_SIZE_128                 (0x0000000C)
#define  USB_CMD_FRAME_SIZE_64                  (0x00008000)
#define  USB_CMD_FRAME_SIZE_32                  (0x00008004)
#define  USB_CMD_FRAME_SIZE_16                  (0x00008008)
#define  USB_CMD_FRAME_SIZE_8                   (0x0000800C)

/* bit 9-8 are async schedule park mode count */
#define  USB_CMD_ASP_00                         (0x00000000)
#define  USB_CMD_ASP_01                         (0x00000100)
#define  USB_CMD_ASP_10                         (0x00000200)
#define  USB_CMD_ASP_11                         (0x00000300)
#define  USB_CMD_ASP_BIT_POS                    (8)

/* bit 23-16 are interrupt threshold control */
#define  USB_CMD_ITC_NO_THRESHOLD               (0x00000000)
#define  USB_CMD_ITC_1_MICRO_FRM                (0x00010000)
#define  USB_CMD_ITC_2_MICRO_FRM                (0x00020000)
#define  USB_CMD_ITC_4_MICRO_FRM                (0x00040000)
#define  USB_CMD_ITC_8_MICRO_FRM                (0x00080000)
#define  USB_CMD_ITC_16_MICRO_FRM               (0x00100000)
#define  USB_CMD_ITC_32_MICRO_FRM               (0x00200000)
#define  USB_CMD_ITC_64_MICRO_FRM               (0x00400000)
#define  USB_CMD_ITC_BIT_POS                    (16)

/* USB STS Register Bit Masks */
#define  USB_STS_INT                            (0x00000001)
#define  USB_STS_ERR                            (0x00000002)
#define  USB_STS_PORT_CHANGE                    (0x00000004)
#define  USB_STS_FRM_LST_ROLL                   (0x00000008)
#define  USB_STS_SYS_ERR                        (0x00000010)
#define  USB_STS_IAA                            (0x00000020)
#define  USB_STS_RESET                          (0x00000040)
#define  USB_STS_SOF                            (0x00000080)
#define  USB_STS_SUSPEND                        (0x00000100)
#define  USB_STS_HC_HALTED                      (0x00001000)
#define  USB_STS_RCL                            (0x00002000)
#define  USB_STS_PERIODIC_SCHEDULE              (0x00004000)
#define  USB_STS_ASYNC_SCHEDULE                 (0x00008000)

/* USB INTR Register Bit Masks */
#define  USB_INTR_INT_EN                        (0x00000001)
#define  USB_INTR_ERR_INT_EN                    (0x00000002)
#define  USB_INTR_PTC_DETECT_EN                 (0x00000004)
#define  USB_INTR_FRM_LST_ROLL_EN               (0x00000008)
#define  USB_INTR_SYS_ERR_EN                    (0x00000010)
#define  USB_INTR_ASYN_ADV_EN                   (0x00000020)
#define  USB_INTR_RESET_EN                      (0x00000040)
#define  USB_INTR_SOF_EN                        (0x00000080)
#define  USB_INTR_DEVICE_SUSPEND                (0x00000100)

/* Device Address bit masks */
#define  USB_DEVICE_ADDRESS_MASK                (0xFE000000)
#define  USB_DEVICE_ADDRESS_BIT_POS             (25)

/* endpoint list address bit masks */
#define  USB_EP_LIST_ADDRESS_MASK               (0xfffff800)

/* PORTSCX  Register Bit Masks */
#define  PORTSCX_CURRENT_CONNECT_STATUS         (0x00000001)
#define  PORTSCX_CONNECT_STATUS_CHANGE          (0x00000002)
#define  PORTSCX_PORT_ENABLE                    (0x00000004)
#define  PORTSCX_PORT_EN_DIS_CHANGE             (0x00000008)
#define  PORTSCX_OVER_CURRENT_ACT               (0x00000010)
#define  PORTSCX_OVER_CURRENT_CHG               (0x00000020)
#define  PORTSCX_PORT_FORCE_RESUME              (0x00000040)
#define  PORTSCX_PORT_SUSPEND                   (0x00000080)
#define  PORTSCX_PORT_RESET                     (0x00000100)
#define  PORTSCX_LINE_STATUS_BITS               (0x00000C00)
#define  PORTSCX_PORT_POWER                     (0x00001000)
#define  PORTSCX_PORT_INDICTOR_CTRL             (0x0000C000)
#define  PORTSCX_PORT_TEST_CTRL                 (0x000F0000)
#define  PORTSCX_WAKE_ON_CONNECT_EN             (0x00100000)
#define  PORTSCX_WAKE_ON_CONNECT_DIS            (0x00200000)
#define  PORTSCX_WAKE_ON_OVER_CURRENT           (0x00400000)
#define  PORTSCX_PHY_LOW_POWER_SPD              (0x00800000)
#define  PORTSCX_PORT_FORCE_FULL_SPEED          (0x01000000)
#define  PORTSCX_PORT_SPEED_MASK                (0x0C000000)
#define  PORTSCX_PORT_WIDTH                     (0x10000000)
#define  PORTSCX_PHY_TYPE_SEL                   (0xC0000000)

/* bit 11-10 are line status */
#define  PORTSCX_LINE_STATUS_SE0                (0x00000000)
#define  PORTSCX_LINE_STATUS_JSTATE             (0x00000400)
#define  PORTSCX_LINE_STATUS_KSTATE             (0x00000800)
#define  PORTSCX_LINE_STATUS_UNDEF              (0x00000C00)
#define  PORTSCX_LINE_STATUS_BIT_POS            (10)

/* bit 15-14 are port indicator control */
#define  PORTSCX_PIC_OFF                        (0x00000000)
#define  PORTSCX_PIC_AMBER                      (0x00004000)
#define  PORTSCX_PIC_GREEN                      (0x00008000)
#define  PORTSCX_PIC_UNDEF                      (0x0000C000)
#define  PORTSCX_PIC_BIT_POS                    (14)

/* bit 19-16 are port test control */
#define  PORTSCX_PTC_DISABLE                    (0x00000000)
#define  PORTSCX_PTC_JSTATE                     (0x00010000)
#define  PORTSCX_PTC_KSTATE                     (0x00020000)
#define  PORTSCX_PTC_SEQNAK                     (0x00030000)
#define  PORTSCX_PTC_PACKET                     (0x00040000)
#define  PORTSCX_PTC_FORCE_EN                   (0x00050000)
#define  PORTSCX_PTC_BIT_POS                    (16)

/* bit 27-26 are port speed */
#define  PORTSCX_PORT_SPEED_FULL                (0x00000000)
#define  PORTSCX_PORT_SPEED_LOW                 (0x04000000)
#define  PORTSCX_PORT_SPEED_HIGH                (0x08000000)
#define  PORTSCX_PORT_SPEED_UNDEF               (0x0C000000)
#define  PORTSCX_SPEED_BIT_POS                  (26)

/* bit 28 is parallel transceiver width for UTMI interface */
#define  PORTSCX_PTW                            (0x10000000)
#define  PORTSCX_PTW_8BIT                       (0x00000000)
#define  PORTSCX_PTW_16BIT                      (0x10000000)

/* bit 31-30 are port transceiver select */
#define  PORTSCX_PTS_UTMI                       (0x00000000)
#define  PORTSCX_PTS_ULPI                       (0x80000000)
#define  PORTSCX_PTS_FSLS                       (0xC0000000)
#define  PORTSCX_PTS_BIT_POS                    (30)

/* USB MODE Register Bit Masks */
#define  USB_MODE_CTRL_MODE_IDLE                (0x00000000)
#define  USB_MODE_CTRL_MODE_DEVICE              (0x00000002)
#define  USB_MODE_CTRL_MODE_HOST                (0x00000003)
#define  USB_MODE_CTRL_MODE_RSV                 (0x00000001)
#define  USB_MODE_SETUP_LOCK_OFF                (0x00000008)
#define  USB_MODE_STREAM_DISABLE                (0x00000010)

/* Endpoint Flush Register */
#define  EPFLUSH_TX_OFFSET                      (0x00010000)
#define  EPFLUSH_RX_OFFSET                      (0x00000000)

/* Endpoint Setup Status bit masks */
#define  EP_SETUP_STATUS_MASK                   (0x0000003F)
#define  EP_SETUP_STATUS_EP0                    (0x00000001)

/* ENDPOINTCTRLx  Register Bit Masks */
#define  EPCTRL_TX_ENABLE                       (0x00800000)
#define  EPCTRL_TX_DATA_TOGGLE_RST              (0x00400000)    /* Not EP0 */
#define  EPCTRL_TX_DATA_TOGGLE_INH              (0x00200000)    /* Not EP0 */
#define  EPCTRL_TX_TYPE                         (0x000C0000)
#define  EPCTRL_TX_DATA_SOURCE                  (0x00020000)    /* Not EP0 */
#define  EPCTRL_TX_EP_STALL                     (0x00010000)
#define  EPCTRL_RX_ENABLE                       (0x00000080)
#define  EPCTRL_RX_DATA_TOGGLE_RST              (0x00000040)    /* Not EP0 */
#define  EPCTRL_RX_DATA_TOGGLE_INH              (0x00000020)    /* Not EP0 */
#define  EPCTRL_RX_TYPE                         (0x0000000C)
#define  EPCTRL_RX_DATA_SINK                    (0x00000002)    /* Not EP0 */
#define  EPCTRL_RX_EP_STALL                     (0x00000001)

/* bit 19-18 and 3-2 are endpoint type */
#define  EPCTRL_EP_TYPE_CONTROL                 (0)
#define  EPCTRL_EP_TYPE_ISO                     (1)
#define  EPCTRL_EP_TYPE_BULK                    (2)
#define  EPCTRL_EP_TYPE_INTERRUPT               (3)
#define  EPCTRL_TX_EP_TYPE_SHIFT                (18)
#define  EPCTRL_RX_EP_TYPE_SHIFT                (2)

/* SNOOPn Register Bit Masks */
#define  SNOOP_ADDRESS_MASK                     (0xFFFFF000)
#define  SNOOP_SIZE_ZERO                        (0x00)    /* snooping disable */
#define  SNOOP_SIZE_4KB                         (0x0B)    /* 4KB snoop size */
#define  SNOOP_SIZE_8KB                         (0x0C)
#define  SNOOP_SIZE_16KB                        (0x0D)
#define  SNOOP_SIZE_32KB                        (0x0E)
#define  SNOOP_SIZE_64KB                        (0x0F)
#define  SNOOP_SIZE_128KB                       (0x10)
#define  SNOOP_SIZE_256KB                       (0x11)
#define  SNOOP_SIZE_512KB                       (0x12)
#define  SNOOP_SIZE_1MB                         (0x13)
#define  SNOOP_SIZE_2MB                         (0x14)
#define  SNOOP_SIZE_4MB                         (0x15)
#define  SNOOP_SIZE_8MB                         (0x16)
#define  SNOOP_SIZE_16MB                        (0x17)
#define  SNOOP_SIZE_32MB                        (0x18)
#define  SNOOP_SIZE_64MB                        (0x19)
#define  SNOOP_SIZE_128MB                       (0x1A)
#define  SNOOP_SIZE_256MB                       (0x1B)
#define  SNOOP_SIZE_512MB                       (0x1C)
#define  SNOOP_SIZE_1GB                         (0x1D)
#define  SNOOP_SIZE_2GB                         (0x1E)    /* 2GB snoop size */

/* pri_ctrl Register Bit Masks */
#define  PRI_CTRL_PRI_LVL1                      (0x0000000C)
#define  PRI_CTRL_PRI_LVL0                      (0x00000003)

/* si_ctrl Register Bit Masks */
#define  SI_CTRL_ERR_DISABLE                    (0x00000010)
#define  SI_CTRL_IDRC_DISABLE                   (0x00000008)
#define  SI_CTRL_RD_SAFE_EN                     (0x00000004)
#define  SI_CTRL_RD_PREFETCH_DISABLE            (0x00000002)
#define  SI_CTRL_RD_PREFEFETCH_VAL              (0x00000001)

/* control Register Bit Masks */
#define  USB_CTRL_IOENB                         (0x00000004)
#define  USB_CTRL_ULPI_INT0EN                   (0x00000001)

#endif                /* __MX31_H */
