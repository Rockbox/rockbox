/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 by Barry Wardell
 *
 * ARC OTG USB device driver based on code from the Linux Target Image Builder
 * from Freescale - http://www.bitshrine.org/ and 
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
#ifndef __ARCOTG_UDC_H
#define __ARCOTG_UDC_H

#include "cpu.h"

#define ETIMEDOUT               1

#define USB_MAX_ENDPOINTS       8
#define USB_MAX_PIPES           (USB_MAX_ENDPOINTS*2)
#define USB_MAX_CTRL_PAYLOAD    64

/* USB DR device mode registers (Little Endian) */
/* Identification registers */
#define UDC_ID                  (*(volatile unsigned int *)(USB_BASE+0x000))
#define UDC_HWGENERAL           (*(volatile unsigned int *)(USB_BASE+0x004))
#define UDC_HWHOST              (*(volatile unsigned int *)(USB_BASE+0x008))
#define UDC_HWTXBUF             (*(volatile unsigned int *)(USB_BASE+0x010))
#define UDC_HWRXBUF             (*(volatile unsigned int *)(USB_BASE+0x014))

/* Capability registers */
#define UDC_CAPLENGTH           (*(volatile unsigned char *)(USB_BASE+0x100))   /* Capability Register Length */
#define UDC_HCIVERSION          (*(volatile unsigned short *)(USB_BASE+0x102))  /* Host Controller Interface Version */
#define UDC_HCSPARAMS           (*(volatile unsigned int *)(USB_BASE+0x104))    /* Host Controller Structual Parameters */
#define UDC_HCCPARAMS           (*(volatile unsigned int *)(USB_BASE+0x108))    /* Host Controller Capability Parameters */
#define UDC_DCIVERSION          (*(volatile unsigned short *)(USB_BASE+0x120))  /* Device Controller Interface Version */
#define UDC_DCCPARAMS           (*(volatile unsigned int *)(USB_BASE+0x124))    /* Device Controller Capability Parameters */

/* Operation registers */
#define UDC_USBCMD              (*(volatile unsigned int *)(USB_BASE+0x140))    /* USB Command Register */
#define UDC_USBSTS              (*(volatile unsigned int *)(USB_BASE+0x144))    /* USB Status Register */
#define UDC_USBINTR             (*(volatile unsigned int *)(USB_BASE+0x148))    /* USB Interrupt Enable Register */
#define UDC_FRINDEX             (*(volatile unsigned int *)(USB_BASE+0x14c))    /* Frame Index Register */
#define UDC_DEVICEADDR          (*(volatile unsigned int *)(USB_BASE+0x154))    /* Device Address */
#define UDC_ENDPOINTLISTADDR    (*(volatile unsigned int *)(USB_BASE+0x158))    /* Endpoint List Address Register */
#define UDC_BURSTSIZE           (*(volatile unsigned int *)(USB_BASE+0x160))    /* Master Interface Data Burst Size Register */
#define UDC_TXFILLTUNING        (*(volatile unsigned int *)(USB_BASE+0x164))    /* Transmit FIFO Tuning Controls Register */
#define UDC_ULPIVIEW            (*(volatile unsigned int *)(USB_BASE+0x170))    
#define UDC_CFGFLAG             (*(volatile unsigned int *)(USB_BASE+0x180))    /* Configure Flag Register */
#define UDC_PORTSC1             (*(volatile unsigned int *)(USB_BASE+0x184))    /* Port 1 Status and Control Register */
#define UDC_OTGSC               (*(volatile unsigned int *)(USB_BASE+0x1a4))    /* On-The-Go Status and Control */
#define UDC_USBMODE             (*(volatile unsigned int *)(USB_BASE+0x1a8))    /* USB Mode Register */
#define UDC_ENDPTSETUPSTAT      (*(volatile unsigned int *)(USB_BASE+0x1ac))    /* Endpoint Setup Status Register */
#define UDC_ENDPTPRIME          (*(volatile unsigned int *)(USB_BASE+0x1b0))    /* Endpoint Initialization Register */
#define UDC_ENDPTFLUSH          (*(volatile unsigned int *)(USB_BASE+0x1b4))    /* Endpoint Flush Register */
#define UDC_ENDPTSTAT           (*(volatile unsigned int *)(USB_BASE+0x1b8))    /* Endpoint Status Register */
#define UDC_ENDPTCOMPLETE       (*(volatile unsigned int *)(USB_BASE+0x1bc))    /* Endpoint Complete Register */
#define UDC_ENDPTCTRL0          (*(volatile unsigned int *)(USB_BASE+0x1c0))    /* Endpoint 0 Control Register */
#define UDC_ENDPTCTRL1          (*(volatile unsigned int *)(USB_BASE+0x1c4))    /* Endpoint 1 Control Register */
#define UDC_ENDPTCTRL2          (*(volatile unsigned int *)(USB_BASE+0x1c8))    /* Endpoint 2 Control Register */
#define UDC_ENDPTCTRL3          (*(volatile unsigned int *)(USB_BASE+0x1cc))    /* Endpoint 3 Control Register */
#define UDC_ENDPTCTRL4          (*(volatile unsigned int *)(USB_BASE+0x1d0))    /* Endpoint 4 Control Register */
#define UDC_ENDPTCTRL5          (*(volatile unsigned int *)(USB_BASE+0x1d4))    /* Endpoint 5 Control Register */
#define UDC_ENDPTCTRL6          (*(volatile unsigned int *)(USB_BASE+0x1d8))    /* Endpoint 6 Control Register */
#define UDC_ENDPTCTRL7          (*(volatile unsigned int *)(USB_BASE+0x1dc))    /* Endpoint 7 Control Register */
#define UDC_ENDPTCTRL(x)        (*(volatile unsigned int *)(USB_BASE+0x1c0+4*(x)))      /* Endpoint X Control Register */

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

/* Externally used functions */
int dr_controller_setup(void);
void dr_controller_run(void);
void dr_controller_stop(void);

#endif                /* __ARCOTG_UDC_H */
