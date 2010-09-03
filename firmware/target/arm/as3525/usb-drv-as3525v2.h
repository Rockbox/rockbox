/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright © 2010 Amaury Pouly
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#ifndef __USB_DRV_AS3525v2_H__
#define __USB_DRV_AS3525v2_H__

#include "as3525v2.h"

/* All multi-bit fields in the driver use the following convention.
 * If the register name is NAME, then there is one define NAME_bitp
 * which holds the bit position and one define NAME_bits which holds
 * a mask of the bits within the register (after shift).
 * These macros allow easy access and construction of such fields */
/* Usage:
 * - extract(reg_name,field_name)
 *   extract a field of the register
 * - bitm(reg_name,field_name)
 *   build a bitmask for the field
 */
#define extract(reg_name, field_name) \
    ((reg_name >> reg_name##_##field_name##_bitp) & reg_name##_##field_name##_bits)

#define bitm(reg_name, field_name) \
    (reg_name##_##field_name##_bits << reg_name##_##field_name##_bitp)

#define USB_DEVICE                  (USB_BASE + 0x0800)   /** USB Device base address */

/**
 * Core Global Registers
 */
#define BASE_REG(offset) (*(volatile unsigned long *)(USB_BASE + offset))

/** OTG Control and Status Register */
#define GOTGCTL     BASE_REG(0x000)

/** OTG Interrupt Register */
#define GOTGINT     BASE_REG(0x004)

/** Core AHB Configuration Register */
#define GAHBCFG                         BASE_REG(0x008)
#define GAHBCFG_glblintrmsk             (1 << 0) /** Global interrupt mask */
#define GAHBCFG_hburstlen_bitp          1
#define GAHBCFG_hburstlen_bits          0xf
#define GAHBCFG_INT_DMA_BURST_SINGLE    0
#define GAHBCFG_INT_DMA_BURST_INCR      1 /** note: the linux patch has several other value, this is one picked for internal dma */
#define GAHBCFG_INT_DMA_BURST_INCR4     3
#define GAHBCFG_INT_DMA_BURST_INCR8     5
#define GAHBCFG_INT_DMA_BURST_INCR16    7
#define GAHBCFG_dma_enable              (1 << 5) /** Enable DMA */

/** Core USB Configuration Register */
#define GUSBCFG                         BASE_REG(0x00C)
#define GUSBCFG_toutcal_bitp            0
#define GUSBCFG_toutcal_bits            0x7
#define GUSBCFG_phy_if                  (1 << 3) /** select utmi bus width ? */
#define GUSBCFG_ulpi_utmi_sel           (1 << 4) /** select ulpi:1 or utmi:0 */
#define GUSBCFG_fsintf                  (1 << 5)
#define GUSBCFG_physel                  (1 << 6)
#define GUSBCFG_ddrsel                  (1 << 7)
#define GUSBCFG_srpcap                  (1 << 8)
#define GUSBCFG_hnpcapp                 (1 << 9)
#define GUSBCFG_usbtrdtim_bitp          10
#define GUSBCFG_usbtrdtim_bits          0xf
#define GUSBCFG_nptxfrwnden             (1 << 14)
#define GUSBCFG_phylpwrclksel           (1 << 15)
#define GUSBCFG_otgutmifssel            (1 << 16)
#define GUSBCFG_ulpi_fsls               (1 << 17)
#define GUSBCFG_ulpi_auto_res           (1 << 18)
#define GUSBCFG_ulpi_clk_sus_m          (1 << 19)
#define GUSBCFG_ulpi_ext_vbus_drv       (1 << 20)
#define GUSBCFG_ulpi_int_vbus_indicator (1 << 21)
#define GUSBCFG_term_sel_dl_pulse       (1 << 22)
#define GUSBCFG_force_host_mode         (1 << 29)
#define GUSBCFG_force_device_mode       (1 << 30)
#define GUSBCFG_corrupt_tx_packet       (1 << 31)

/** Core Reset Register */
#define GRSTCTL                 BASE_REG(0x010)
#define GRSTCTL_csftrst         (1 << 0) /** Core soft reset */
#define GRSTCTL_hsftrst         (1 << 1) /** Hclk soft reset */
#define GRSTCTL_intknqflsh      (1 << 3) /** In Token Sequence Learning Queue Flush */
#define GRSTCTL_rxfflsh_flush   (1 << 4) /** RxFIFO Flush */
#define GRSTCTL_txfflsh_flush   (1 << 5) /** TxFIFO Flush */
#define GRSTCTL_txfnum_bitp     6 /** TxFIFO Number */
#define GRSTCTL_txfnum_bits     0x1f
#define GRSTCTL_ahbidle         (1 << 31) /** AHB idle state*/

/** Core Interrupt Register */
#define GINTSTS             BASE_REG(0x014)
/* NOTE: GINTSTS bits are the same as in GINTMSK plus this one */
#define GINTSTS_curmode     (1 << 0) /** Current mode, 0 for device */

/** Core Interrupt Mask Register */
#define GINTMSK                 BASE_REG(0x018)
#define GINTMSK_modemismatch    (1 << 1) /** mode mismatch ? */
#define GINTMSK_otgintr         (1 << 2)
#define GINTMSK_sofintr         (1 << 3)
#define GINTMSK_rxstsqlvl       (1 << 4)
#define GINTMSK_nptxfempty      (1 << 5) /** Non-periodic TX fifo empty ? */
#define GINTMSK_ginnakeff       (1 << 6)
#define GINTMSK_goutnakeff      (1 << 7)
#define GINTMSK_i2cintr         (1 << 9)
#define GINTMSK_erlysuspend     (1 << 10)
#define GINTMSK_usbsuspend      (1 << 11) /** USB suspend */
#define GINTMSK_usbreset        (1 << 12) /** USB reset */
#define GINTMSK_enumdone        (1 << 13) /** Enumeration done */
#define GINTMSK_isooutdrop      (1 << 14)
#define GINTMSK_eopframe        (1 << 15)
#define GINTMSK_epmismatch      (1 << 17) /** endpoint mismatch ? */
#define GINTMSK_inepintr        (1 << 18) /** in pending ? */
#define GINTMSK_outepintr       (1 << 19) /** out pending ? */
#define GINTMSK_incomplisoin    (1 << 20) /** ISP in complete ? */
#define GINTMSK_incomplisoout   (1 << 21) /** ISO out complete ? */
#define GINTMSK_portintr        (1 << 24) /** Port status change ? */
#define GINTMSK_hcintr          (1 << 25)
#define GINTMSK_ptxfempty       (1 << 26) /** Periodic TX fifof empty ? */
#define GINTMSK_conidstschng    (1 << 28) 
#define GINTMSK_disconnect      (1 << 29) /** Disconnect */
#define GINTMSK_sessreqintr     (1 << 30) /** Session request */
#define GINTMSK_wkupintr        (1 << 31) /** Wake up */

/** Receive Status Debug Read Register (Read Only) */
#define GRXSTSR     BASE_REG(0x01C)

/** Receive Status Read /Pop Register (Read Only) */
#define GRXSTSP     BASE_REG(0x020)

/** Receive FIFO Size Register */
#define GRXFSIZ     BASE_REG(0x024)

/** Periodic Transmit FIFO Size Register */
#define GNPTXFSIZ   BASE_REG(0x028)

/** Non-Periodic Transmit FIFO/Queue Status Register */
#define GNPTXSTS    BASE_REG(0x02C)

/** I2C Access Register */
#define GI2CCTL     BASE_REG(0x030)

/** PHY Vendor Control Register */
#define GPVNDCTL    BASE_REG(0x034)

/** General Purpose Input/Output Register */
#define GGPIO       BASE_REG(0x038)

/** User ID Register */
#define GUID        BASE_REG(0x03C)

/** Synopsys ID Register */
#define GSNPSID     BASE_REG(0x040)

/** User HW Config1 Register */
#define GHWCFG1                 BASE_REG(0x044)
#define GHWCFG1_epdir_bitp(ep)  (2 * (ep))
#define GHWCFG1_epdir_bits      0x3
#define GHWCFG1_EPDIR_BIDIR     0
#define GHWCFG1_EPDIR_IN        1
#define GHWCFG1_EPDIR_OUT       2

/** User HW Config2 Register */
#define GHWCFG2                     BASE_REG(0x048)
#define GHWCFG2_arch_bitp           3 /** Architecture */
#define GHWCFG2_arch_bits           0x3
#define GHWCFG2_hs_phy_type_bitp    6 /** High speed PHY type */
#define GHWCFG2_hs_phy_type_bits    0x3
#define GHWCFG2_fs_phy_type_bitp    8 /** Full speed PHY type */
#define GHWCFG2_fs_phy_type_bits    0x3
#define GHWCFG2_num_ep_bitp         10 /** Number of endpoints */
#define GHWCFG2_num_ep_bits         0xf
#define GHWCFG2_dyn_fifo            (1 << 19) /** Dynamic FIFO */
/* For GHWCFG2_HS_PHY_TYPE and GHWCFG2_FS_PHY_TYPE */
#define GHWCFG2_PHY_TYPE_UNSUPPORTED        0
#define GHWCFG2_PHY_TYPE_UTMI               1
#define GHWCFG2_ARCH_INTERNAL_DMA           2

/** User HW Config3 Register */
#define GHWCFG3                 BASE_REG(0x04C)
#define GHWCFG3_dfifo_len_bitp  16 /** Total fifo size */
#define GHWCFG3_dfifo_len_bits  0xffff

/** User HW Config4 Register */
#define GHWCFG4                             BASE_REG(0x050)
#define GHWCFG4_utmi_phy_data_width_bitp    14 /** UTMI+ data bus width */
#define GHWCFG4_utmi_phy_data_width_bits    0x3
#define GHWCFG4_ded_fifo_en                 (1 << 25) /** Dedicated Tx FIFOs */
#define GHWCFG4_num_in_ep_bitp              26 /** Number of IN endpoints */
#define GHWCFG4_num_in_ep_bits              0xf

/* 1<=ep<=15, don't use ep=0 !!! */
/** Device IN Endpoint Transmit FIFO (ep) Size Register */
#define DIEPTXFSIZ(ep)  BASE_REG(0x100 + 4 * (ep))

/** Build the content of a FIFO size register like DIEPTXFSIZ(i) and GNPTXFSIZ*/
#define MAKE_FIFOSIZE_DATA(startadr, depth) \
    (((startadr) & 0xffff) | ((depth) << 16))
/** Retrieve fifo size for such registers */
#define GET_FIFOSIZE_DEPTH(data) \
    ((data) >> 16)
/** Retrieve fifo start address for such registers */
#define GET_FIFOSIZE_START_ADR(data) \
    ((data) & 0xffff)

/**
 * Device Registers Base Addresses
 */
#define DEV_REG(offset) (*(volatile unsigned long *)(USB_DEVICE + offset))

/** Device Configuration Register */
#define DCFG                    DEV_REG(0x00)
#define DCFG_devspd_bitp        0 /** Device Speed */
#define DCFG_devspd_bits        0x3
#define DCFG_devspd_hs_phy_hs   0 /** High speed PHY running at high speed */
#define DCFG_devspd_hs_phy_fs   1 /** High speed PHY running at full speed */
#define DCFG_nzstsouthshk       (1 << 2) /** Non Zero Length Status OUT Handshake */
#define DCFG_devadr_bitp        4 /** Device Address */
#define DCFG_devadr_bits        0x7f
#define DCFG_perfrint_bitp      11 /** Periodic Frame Interval */
#define DCFG_perfrint_bits      0x3
#define DCFG_FRAME_INTERVAL_80  0
#define DCFG_FRAME_INTERVAL_85  1
#define DCFG_FRAME_INTERVAL_90  2
#define DCFG_FRAME_INTERVAL_95  3

/** Device Control Register */
#define DCTL                DEV_REG(0x04)
#define DCTL_rmtwkupsig     (1 << 0) /** Remote Wakeup */
#define DCTL_sftdiscon      (1 << 1) /** Soft Disconnect */
#define DCTL_gnpinnaksts    (1 << 2) /** Global Non-Periodic IN NAK Status */
#define DCTL_goutnaksts     (1 << 3) /** Global OUT NAK Status */
#define DCTL_tstctl_bitp    4 /** Test Control */
#define DCTL_tstctl_bits    0x7
#define DCTL_sgnpinnak      (1 << 7) /** Set Global Non-Periodic IN NAK */
#define DCTL_cgnpinnak      (1 << 8) /** Clear Global Non-Periodic IN NAK */
#define DCTL_sgoutnak       (1 << 9) /** Set Global OUT NAK */
#define DCTL_cgoutnak       (1 << 10) /** Clear Global OUT NAK */
#define DCTL_pwronprgdone   (1 << 11) /** Power on Program Done ? */

/** Device Status Register */
#define DSTS                DEV_REG(0x08) 
#define DSTS_suspsts        (1 << 0) /** Suspend status */
#define DSTS_enumspd_bitp   1 /** Enumerated speed */
#define DSTS_enumspd_bits   0x3
#define DSTS_ENUMSPD_HS_PHY_30MHZ_OR_60MHZ 0
#define DSTS_ENUMSPD_FS_PHY_30MHZ_OR_60MHZ 1
#define DSTS_ENUMSPD_LS_PHY_6MHZ           2
#define DSTS_ENUMSPD_FS_PHY_48MHZ          3
#define DSTS_errticerr      (1 << 3) /** Erratic errors ? */
#define DSTS_soffn_bitp     8 /** Frame or Microframe Number of the received SOF */
#define DSTS_soffn_bits     0x3fff

/** Device IN Endpoint Common Interrupt Mask Register */
#define DIEPMSK                 DEV_REG(0x10) 
/* the following apply to DIEPMSK and DIEPINT */
#define DIEPINT_xfercompl       (1 << 0) /** Transfer complete */
#define DIEPINT_epdisabled      (1 << 1) /** Endpoint disabled */
#define DIEPINT_ahberr          (1 << 2) /** AHB error */
#define DIEPINT_timeout         (1 << 3) /** Timeout handshake (non-iso TX) */
#define DIEPINT_intktxfemp      (1 << 4) /** IN token received with tx fifo empty */
#define DIEPINT_intknepmis      (1 << 5) /** IN token received with ep mismatch */
#define DIEPINT_inepnakeff      (1 << 6) /** IN endpoint NAK effective */
#define DIEPINT_emptyintr       (1 << 7) /** linux doc broken on this, empty fifo ? */
#define DIEPINT_txfifoundrn     (1 << 8) /** linux doc void on this, tx fifo underrun ? */

/** Device OUT Endpoint Common Interrupt Mask Register */
#define DOEPMSK                 DEV_REG(0x14)
/* the following apply to DOEPMSK and DOEPINT */
#define DOEPINT_xfercompl       (1 << 0) /** Transfer complete */
#define DOEPINT_epdisabled      (1 << 1) /** Endpoint disabled */
#define DOEPINT_ahberr          (1 << 2) /** AHB error */
#define DOEPINT_setup           (1 << 3) /** Setup Phase Done (control EPs)*/

/** Device All Endpoints Interrupt Register */
#define DAINT               DEV_REG(0x18)
/* valid for DAINT and DAINTMSK, for 0<=ep<=15 */
#define DAINT_IN_EP(i)      (1 << (i))
#define DAINT_OUT_EP(i)     (1 << ((i) + 16))

/** Device Endpoints Interrupt Mask Register */
#define DAINTMSK    DEV_REG(0x1C)

/** Device IN Token Sequence Learning Queue Read Register 1 */
#define DTKNQR1     DEV_REG(0x20)

/** Device IN Token Sequence Learning Queue Register 2 */
#define DTKNQR2     DEV_REG(0x24)

/** Device IN Token Queue Pop register */
#define DTKNQP      DEV_REG(0x28)

/* fixme: those registers are not present in registers.h but are in dwc_otgh_regs.h.
 *        the previous registers exists but has a different name :( */
/** Device VBUS discharge register*/
#define DVBUSDIS    DEV_REG(0x28)

/** Device VBUS pulse register */
#define DVBUSPULSE  DEV_REG(0x2C)

/** Device IN Token Queue Read Register 3 (RO) */
#define DTKNQR3     DEV_REG(0x30)

/** Device Thresholding control register */
#define DTHRCTL                     DEV_REG(0x30)
#define DTHRCTL_non_iso_thr_en      (1 << 0)
#define DTHRCTL_iso_thr_en          (1 << 1)
#define DTHRCTL_tx_thr_len_bitp     2
#define DTHRCTL_tx_thr_len_bits     0x1FF
#define DTHRCTL_rx_thr_en           (1 << 16)
#define DTHRCTL_rx_thr_len_bitp     17
#define DTHRCTL_rx_thr_len_bits     0x1FF

/** Device IN Token Queue Read Register 4 (RO) */
#define DTKNQR4     DEV_REG(0x34)

/** Device IN EPs empty Inr. Mask Register */
#define FFEMPTYMSK  DEV_REG(0x34) 

#define PCGCCTL         BASE_REG(0xE00) /** Power and Clock Gating Control Register */

/** Device IN Endpoint (ep) Control Register */
#define DIEPCTL(ep)     DEV_REG(0x100 + (ep) * 0x20)
/** Device OUT Endpoint (ep) Control Register */
#define DOEPCTL(ep)     DEV_REG(0x300 + (ep) * 0x20)

/** Maximum Packet Size
 * IN/OUT EPn
 * IN/OUT EP0 - 2 bits
 *  2'b00: 64 Bytes
 *  2'b01: 32
 *  2'b10: 16
 *  2'b11: 8 */
#define DEPCTL_mps_bitp         0
#define DEPCTL_mps_bits         0x7ff
#define DEPCTL_MPS_64           0
#define DEPCTL_MPS_32           1
#define DEPCTL_MPS_16           2
#define DEPCTL_MPS_8            3
/** Next Endpoint
 * IN EPn/IN EP0
 * OUT EPn/OUT EP0 - reserved */
#define DEPCTL_nextep_bitp      11
#define DEPCTL_nextep_bits      0xf
#define DEPCTL_usbactep         (1 << 15) /** USB Active Endpoint */
/** Endpoint DPID (INTR/Bulk IN and OUT endpoints)
 * This field contains the PID of the packet going to
 * be received or transmitted on this endpoint. The
 * application should program the PID of the first
 * packet going to be received or transmitted on this
 * endpoint , after the endpoint is
 * activated. Application use the SetD1PID and
 * SetD0PID fields of this register to program either
 * D0 or D1 PID.
 *
 * The encoding for this field is
 * - 0: D0
 * - 1: D1
 */
#define DEPCTL_dpid             (1 << 16)
#define DEPCTL_naksts           (1 << 17) /** NAK Status */
/** Endpoint Type
 *  2'b00: Control
 *  2'b01: Isochronous
 *  2'b10: Bulk
 *  2'b11: Interrupt */
#define DEPCTL_eptype_bitp      18
#define DEPCTL_eptype_bits      0x3
/** Snoop Mode
 * OUT EPn/OUT EP0
 * IN EPn/IN EP0 - reserved */
#define DEPCTL_snp              (1 << 20)
#define DEPCTL_stall            (1 << 21) /** Stall Handshake */
/** Tx Fifo Number
 * IN EPn/IN EP0
 * OUT EPn/OUT EP0 - reserved */
#define DEPCTL_txfnum_bitp      22
#define DEPCTL_txfnum_bits      0xf

#define DEPCTL_cnak             (1 << 26) /** Clear NAK */
#define DEPCTL_snak             (1 << 27) /** Set NAK */
/** Set DATA0 PID (INTR/Bulk IN and OUT endpoints)
 * Writing to this field sets the Endpoint DPID (DPID)
 * field in this register to DATA0. Set Even
 * (micro)frame (SetEvenFr) (ISO IN and OUT Endpoints)
 * Writing to this field sets the Even/Odd
 * (micro)frame (EO_FrNum) field to even (micro)
 * frame.
 */
#define DEPCTL_setd0pid         (1 << 28)
/** Set DATA1 PID (INTR/Bulk IN and OUT endpoints)
 * Writing to this field sets the Endpoint DPID (DPID)
 * field in this register to DATA1 Set Odd
 * (micro)frame (SetOddFr) (ISO IN and OUT Endpoints)
 * Writing to this field sets the Even/Odd
 * (micro)frame (EO_FrNum) field to odd (micro) frame.
 */
#define DEPCTL_setd1pid         (1 << 29)
#define DEPCTL_epdis            (1 << 30) /** Endpoint disable */
#define DEPCTL_epena            (1 << 31) /** Endpoint enable */

/** Device IN Endpoint (ep) Transfer Size Register */
#define DIEPTSIZ(ep)    DEV_REG(0x100 + (ep) * 0x20 + 0x10)
/** Device OUT Endpoint (ep) Transfer Size Register */
#define DOEPTSIZ(ep)    DEV_REG(0x300 + (ep) * 0x20 + 0x10)

/* valid for any D{I,O}EPTSIZi with 1<=i<=15, NOT for i=0 ! */
#define DEPTSIZ_xfersize_bitp   0 /** Transfer Size */
#define DEPTSIZ_xfersize_bits   0x7ffff
#define DEPTSIZ_pkcnt_bitp      19 /** Packet Count */
#define DEPTSIZ_pkcnt_bits      0x3ff
#define DEPTSIZ_mc_bitp         29 /** Multi Count - Periodic IN endpoints */
#define DEPTSIZ_mc_bits         0x3

/* idem but for i=0 */
#define DEPTSIZ0_xfersize_bitp  0 /** Transfer Size */
#define DEPTSIZ0_xfersize_bits  0x7f
#define DEPTSIZ0_pkcnt_bitp     19 /** Packet Count */
#define DEPTSIZ0_pkcnt_bits     0x3
#define DEPTSIZ0_supcnt_bitp    29 /** Setup Packet Count (DOEPTSIZ0 Only) */
#define DEPTSIZ0_supcnt_bits    0x3

/** Device IN Endpoint (ep) Interrupt Register */
#define DIEPINT(ep)     DEV_REG(0x100 + (ep) * 0x20 + 0x8)
/** Device IN Endpoint (ep) DMA Address Register */
#define DIEPDMA(ep)     DEV_REG(0x100 + (ep) * 0x20 + 0x14)
/** Device IN Endpoint (ep) Transmit FIFO Status Register */
#define DTXFSTS(ep)     DEV_REG(0x100 + (ep) * 0x20 + 0x18)

/** Device OUT Endpoint (ep) Frame number Register */
#define DOEPFN(ep)      DEV_REG(0x300 + (ep) * 0x20 + 0x4)
/** Device Endpoint (ep) Interrupt Register */
#define DOEPINT(ep)     DEV_REG(0x300 + (ep) * 0x20 + 0x8)
/** Device Endpoint (ep) DMA Address Register */
#define DOEPDMA(ep)     DEV_REG(0x300 + (ep) * 0x20 + 0x14)

/**
 * Parameters
 */

/* Number of IN/OUT endpoints */
#define NUM_IN_EP           3
#define NUM_OUT_EP          2

/* List of IN enpoints */
#define IN_EP_LIST          1, 3, 5
#define OUT_EP_LIST         2, 4

#endif /* __USB_DRV_AS3525v2_H__ */
