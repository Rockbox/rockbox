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

#define USB_DEVICE                  (USB_BASE + 0x0800)   /** USB Device base address */

/**
 * Core Global Registers
 */
#define USB_GOTGCTL     (*(volatile unsigned long *)(USB_BASE + 0x000)) /** OTG Control and Status Register */
#define USB_GOTGINT     (*(volatile unsigned long *)(USB_BASE + 0x004)) /** OTG Interrupt Register */
#define USB_GAHBCFG     (*(volatile unsigned long *)(USB_BASE + 0x008)) /** Core AHB Configuration Register */
#define USB_GUSBCFG     (*(volatile unsigned long *)(USB_BASE + 0x00C)) /** Core USB Configuration Register */
#define USB_GRSTCTL     (*(volatile unsigned long *)(USB_BASE + 0x010)) /** Core Reset Register */
#define USB_GINTSTS     (*(volatile unsigned long *)(USB_BASE + 0x014)) /** Core Interrupt Register */
#define USB_GINTMSK     (*(volatile unsigned long *)(USB_BASE + 0x018)) /** Core Interrupt Mask Register */
#define USB_GRXSTSR     (*(volatile unsigned long *)(USB_BASE + 0x01C)) /** Receive Status Debug Read Register (Read Only) */
#define USB_GRXSTSP     (*(volatile unsigned long *)(USB_BASE + 0x020)) /** Receive Status Read /Pop Register (Read Only) */
#define USB_GRXFSIZ     (*(volatile unsigned long *)(USB_BASE + 0x024)) /** Receive FIFO Size Register */
#define USB_GNPTXFSIZ   (*(volatile unsigned long *)(USB_BASE + 0x028)) /** Periodic Transmit FIFO Size Register */
#define USB_GNPTXSTS    (*(volatile unsigned long *)(USB_BASE + 0x02C)) /** Non-Periodic Transmit FIFO/Queue Status Register */
#define USB_GI2CCTL     (*(volatile unsigned long *)(USB_BASE + 0x030)) /** I2C Access Register */
#define USB_GPVNDCTL    (*(volatile unsigned long *)(USB_BASE + 0x034)) /** PHY Vendor Control Register */
#define USB_GGPIO       (*(volatile unsigned long *)(USB_BASE + 0x038)) /** General Purpose Input/Output Register */
#define USB_GUID        (*(volatile unsigned long *)(USB_BASE + 0x03C)) /** User ID Register */
#define USB_GSNPSID     (*(volatile unsigned long *)(USB_BASE + 0x040)) /** Synopsys ID Register */
#define USB_GHWCFG1     (*(volatile unsigned long *)(USB_BASE + 0x044)) /** User HW Config1 Register */
#define USB_GHWCFG2     (*(volatile unsigned long *)(USB_BASE + 0x048)) /** User HW Config2 Register */
#define USB_GHWCFG3     (*(volatile unsigned long *)(USB_BASE + 0x04C)) /** User HW Config3 Register */
#define USB_GHWCFG4     (*(volatile unsigned long *)(USB_BASE + 0x050)) /** User HW Config4 Register */

/* 1<=ep<=15, don't use ep=0 !!! */
/** Device IN Endpoint Transmit FIFO (ep) Size Register */
#define USB_DIEPTXFSIZ(ep)  (*(volatile unsigned long *)(USB_BASE + 0x100 + 4 * (ep)))

/** Build the content of a FIFO size register like USB_DIEPTXFSIZ(i) and USB_GNPTXFSIZ*/
#define USB_MAKE_FIFOSIZE_DATA(startadr, depth) \
    (((startadr) & 0xffff) | ((depth) << 16))

/** Retrieve fifo size for such registers */
#define USB_GET_FIFOSIZE_DEPTH(data) \
    ((data) >> 16)

/** Retrieve fifo start address for such registers */
#define USB_GET_FIFOSIZE_START_ADR(data) \
    ((data) & 0xffff)

#define USB_GRSTCTL_csftrst                 (1 << 0) /** Core soft reset */
#define USB_GRSTCTL_hsftrst                 (1 << 1) /** Hclk soft reset */
#define USB_GRSTCTL_intknqflsh              (1 << 3) /** In Token Sequence Learning Queue Flush */
#define USB_GRSTCTL_rxfflsh_flush           (1 << 4) /** RxFIFO Flush */
#define USB_GRSTCTL_txfflsh_flush           (1 << 5) /** TxFIFO Flush */
#define USB_GRSTCTL_txfnum_bit_pos          6 /** TxFIFO Number */
#define USB_GRSTCTL_txfnum_bits             (0x1f << 6)
#define USB_GRSTCTL_ahbidle                 (1 << 31) /** AHB idle state*/

#define USB_GHWCFG1_IN_EP(ep)               ((USB_GHWCFG1 >> ((ep) *2)) & 0x1) /** 1 if EP(ep) has in cap */
#define USB_GHWCFG1_OUT_EP(ep)              ((USB_GHWCFG1 >> ((ep) *2 + 1)) & 0x1)/** 1 if EP(ep) has out cap */

#define USB_GHWCFG2_ARCH                    ((USB_GHWCFG2 >> 3) & 0x3) /** Architecture */
#define USB_GHWCFG2_HS_PHY_TYPE             ((USB_GHWCFG2 >> 6) & 0x3) /** High speed PHY type */
#define USB_GHWCFG2_FS_PHY_TYPE             ((USB_GHWCFG2 >> 8) & 0x3) /** Full speed PHY type */
#define USB_GHWCFG2_NUM_EP                  ((USB_GHWCFG2 >> 10) & 0xf) /** Number of endpoints */
#define USB_GHWCFG2_DYN_FIFO                ((USB_GHWCFG2 >> 19) & 0x1) /** Dynamic FIFO */

/* For USB_GHWCFG2_HS_PHY_TYPE and USB_GHWCFG2_SS_PHY_TYPE */
#define USB_PHY_TYPE_UNSUPPORTED            0
#define USB_PHY_TYPE_UTMI                   1
#define USB_INT_DMA_ARCH                    2

#define USB_GHWCFG3_DFIFO_LEN               (USB_GHWCFG3 >> 16) /** Total fifo size */

#define USB_GHWCFG4_UTMI_PHY_DATA_WIDTH     ((USB_GHWCFG4 >> 14) & 0x3) /** UTMI+ data bus width (format is unsure) */
#define USB_GHWCFG4_DED_FIFO_EN             ((USB_GHWCFG4 >> 25) & 0x1) /** Dedicated Tx FIFOs */
#define USB_GHWCFG4_NUM_IN_EP               ((USB_GHWCFG4 >> 26) & 0xf) /** Number of IN endpoints */

#define USB_GUSBCFG_toutcal_bit_pos         0
#define USB_GUSBCFG_toutcal_bits            (0x7 << USB_GUSBCFG_toutcal_bit_pos)
#define USB_GUSBCFG_phy_if                  (1 << 3) /** select utmi bus width ? */
#define USB_GUSBCFG_ulpi_utmi_sel           (1 << 4) /** select ulpi:1 or utmi:0 */
#define USB_GUSBCFG_fsintf                  (1 << 5)
#define USB_GUSBCFG_physel                  (1 << 6)
#define USB_GUSBCFG_ddrsel                  (1 << 7)
#define USB_GUSBCFG_srpcap                  (1 << 8)
#define USB_GUSBCFG_hnpcapp                 (1 << 9)
#define USB_GUSBCFG_usbtrdtim_bit_pos       10
#define USB_GUSBCFG_usbtrdtim_bits          (0xf << USB_GUSBCFG_usbtrdtim_bit_pos)
#define USB_GUSBCFG_nptxfrwnden             (1 << 14)
#define USB_GUSBCFG_phylpwrclksel           (1 << 15)
#define USB_GUSBCFG_otgutmifssel            (1 << 16)
#define USB_GUSBCFG_ulpi_fsls               (1 << 17)
#define USB_GUSBCFG_ulpi_auto_res           (1 << 18)
#define USB_GUSBCFG_ulpi_clk_sus_m          (1 << 19)
#define USB_GUSBCFG_ulpi_ext_vbus_drv       (1 << 20)
#define USB_GUSBCFG_ulpi_int_vbus_indicator (1 << 21)
#define USB_GUSBCFG_term_sel_dl_pulse       (1 << 22)
#define USB_GUSBCFG_force_host_mode         (1 << 29)
#define USB_GUSBCFG_force_device_mode       (1 << 30)
#define USB_GUSBCFG_corrupt_tx_packet       (1 << 31)

#define USB_GAHBCFG_glblintrmsk             (1 << 0) /** Global interrupt mask */
#define USB_GAHBCFG_hburstlen_bit_pos       1
#define USB_GAHBCFG_INT_DMA_BURST_INCR      1 /** note: the linux patch has several other value, this is one picked for internal dma */
#define USB_GAHBCFG_dma_enable              (1 << 5) /** Enable DMA */

/* NOTE: USB_GINTSTS bits are the same as in USB_GINTMSK plus the following one */
#define USB_GINTSTS_curmode                 (1 << 0) /** Current mode: 1 for host, 0 for device */

#define USB_GINTMSK_modemismatch            (1 << 1) /** mode mismatch ? */
#define USB_GINTMSK_otgintr                 (1 << 2)
#define USB_GINTMSK_sofintr                 (1 << 3)
#define USB_GINTMSK_rxstsqlvl               (1 << 4)
#define USB_GINTMSK_nptxfempty              (1 << 5) /** Non-periodic TX fifo empty ? */
#define USB_GINTMSK_ginnakeff               (1 << 6)
#define USB_GINTMSK_goutnakeff              (1 << 7)
#define USB_GINTMSK_i2cintr                 (1 << 9)
#define USB_GINTMSK_erlysuspend             (1 << 10)
#define USB_GINTMSK_usbsuspend              (1 << 11) /** USB suspend */
#define USB_GINTMSK_usbreset                (1 << 12) /** USB reset */
#define USB_GINTMSK_enumdone                (1 << 13) /** Enumeration done */
#define USB_GINTMSK_isooutdrop              (1 << 14)
#define USB_GINTMSK_eopframe                (1 << 15)
#define USB_GINTMSK_epmismatch              (1 << 17) /** endpoint mismatch ? */
#define USB_GINTMSK_inepintr                (1 << 18) /** in pending ? */
#define USB_GINTMSK_outepintr               (1 << 19) /** out pending ? */
#define USB_GINTMSK_incomplisoin            (1 << 20) /** ISP in complete ? */
#define USB_GINTMSK_incomplisoout           (1 << 21) /** ISO out complete ? */
#define USB_GINTMSK_portintr                (1 << 24) /** Port status change ? */
#define USB_GINTMSK_hcintr                  (1 << 25)
#define USB_GINTMSK_ptxfempty               (1 << 26) /** Periodic TX fifof empty ? */
#define USB_GINTMSK_conidstschng            (1 << 28) 
#define USB_GINTMSK_disconnect              (1 << 29) /** Disconnect */
#define USB_GINTMSK_sessreqintr             (1 << 30) /** Session request */
#define USB_GINTMSK_wkupintr                (1 << 31) /** Wake up */

/**
 * Device Registers Base Addresses
 */
#define USB_DCFG        (*(volatile unsigned long *)(USB_DEVICE + 0x00)) /** Device Configuration Register */
#define USB_DCTL        (*(volatile unsigned long *)(USB_DEVICE + 0x04)) /** Device Control Register */
#define USB_DSTS        (*(volatile unsigned long *)(USB_DEVICE + 0x08)) /** Device Status Register */
#define USB_DIEPMSK     (*(volatile unsigned long *)(USB_DEVICE + 0x10)) /** Device IN Endpoint Common Interrupt Mask Register */
#define USB_DOEPMSK     (*(volatile unsigned long *)(USB_DEVICE + 0x14)) /** Device OUT Endpoint Common Interrupt Mask Register */
#define USB_DAINT       (*(volatile unsigned long *)(USB_DEVICE + 0x18)) /** Device All Endpoints Interrupt Register */
#define USB_DAINTMSK    (*(volatile unsigned long *)(USB_DEVICE + 0x1C)) /** Device Endpoints Interrupt Mask Register */
#define USB_DTKNQR1     (*(volatile unsigned long *)(USB_DEVICE + 0x20)) /** Device IN Token Sequence Learning Queue Read Register 1 */
#define USB_DTKNQR2     (*(volatile unsigned long *)(USB_DEVICE + 0x24)) /** Device IN Token Sequence Learning Queue Register 2 */
#define USB_DTKNQP      (*(volatile unsigned long *)(USB_DEVICE + 0x28)) /** Device IN Token Queue Pop register */
/* fixme: those registers are not present in usb_registers.h but are in dwc_otgh_regs.h.
 *        the previous registers exists but has a different name :( */
#define USB_DVBUSDIS    (*(volatile unsigned long *)(USB_DEVICE + 0x28)) /** Device VBUS discharge register*/
#define USB_DVBUSPULSE  (*(volatile unsigned long *)(USB_DEVICE + 0x2C)) /** Device VBUS pulse register */
#define USB_DTKNQR3     (*(volatile unsigned long *)(USB_DEVICE + 0x30)) /** Device IN Token Queue Read Register 3 (RO) */
#define USB_DTHRCTL     (*(volatile unsigned long *)(USB_DEVICE + 0x30)) /** Device Thresholding control register */
#define USB_DTKNQR4     (*(volatile unsigned long *)(USB_DEVICE + 0x34)) /** Device IN Token Queue Read Register 4 (RO) */
#define USB_FFEMPTYMSK  (*(volatile unsigned long *)(USB_DEVICE + 0x34)) /** Device IN EPs empty Inr. Mask Register */

#define USB_DCTL_rmtwkupsig                 (1 << 0) /** Remote Wakeup */
#define USB_DCTL_sftdiscon                  (1 << 1) /** Soft Disconnect */
#define USB_DCTL_gnpinnaksts                (1 << 2) /** Global Non-Periodic IN NAK Status */
#define USB_DCTL_goutnaksts                 (1 << 3) /** Global OUT NAK Status */
#define USB_DCTL_tstctl_bit_pos             4 /** Test Control */
#define USB_DCTL_tstctl_bits                (0x7 << USB_DCTL_tstctl_bit_pos)
#define USB_DCTL_sgnpinnak                  (1 << 7) /** Set Global Non-Periodic IN NAK */
#define USB_DCTL_cgnpinnak                  (1 << 8) /** Clear Global Non-Periodic IN NAK */
#define USB_DCTL_sgoutnak                   (1 << 9) /** Set Global OUT NAK */
#define USB_DCTL_cgoutnak                   (1 << 10) /** Clear Global OUT NAK */
/* "documented" in usb_constants.h only */
#define USB_DCTL_pwronprgdone               (1 << 11) /** Power on Program Done ? */

#define USB_DCFG_devspd_bits                0x3 /** Device Speed */
#define USB_DCFG_devspd_hs_phy_hs           0 /** High speed PHY running at high speed */
#define USB_DCFG_devspd_hs_phy_fs           1 /** High speed PHY running at full speed */
#define USB_DCFG_nzstsouthshk               (1 << 2) /** Non Zero Length Status OUT Handshake */
#define USB_DCFG_devadr_bit_pos             4 /** Device Address */
#define USB_DCFG_devadr_bits                (0x7f << USB_DCFG_devadr_bit_pos)
#define USB_DCFG_perfrint_bit_pos           11 /** Periodic Frame Interval */
#define USB_DCFG_perfrint_bits              (0x3 << USB_DCFG_perfrint_bit_pos)
#define USB_DCFG_FRAME_INTERVAL_80          0
#define USB_DCFG_FRAME_INTERVAL_85          1
#define USB_DCFG_FRAME_INTERVAL_90          2
#define USB_DCFG_FRAME_INTERVAL_95          3

#define USB_DSTS_suspsts                    (1 << 0) /** Suspend status */
#define USB_DSTS_enumspd_bit_pos            1 /** Enumerated speed */
#define USB_DSTS_enumspd_bits               (0x3 << USB_DSTS_enumspd_bit_pos)
#define USB_DSTS_ENUMSPD_HS_PHY_30MHZ_OR_60MHZ 0
#define USB_DSTS_ENUMSPD_FS_PHY_30MHZ_OR_60MHZ 1
#define USB_DSTS_ENUMSPD_LS_PHY_6MHZ           2
#define USB_DSTS_ENUMSPD_FS_PHY_48MHZ          3
#define USB_DSTS_errticerr                  (1 << 3) /** Erratic errors ? */
#define USB_DSTS_soffn_bit_pos              7 /** Frame or Microframe Number of the received SOF */
#define USB_DSTS_soffn_bits                 (0x3fff << USB_DSTS_soffn_bit_pos)

#define USB_DTHRCTL_non_iso_thr_en          (1 << 0)
#define USB_DTHRCTL_iso_thr_en              (1 << 1)
#define USB_DTHRCTL_tx_thr_len_bit_pos      2
#define USB_DTHRCTL_tx_thr_len_bits         (0x1FF << USB_DTHRCTL_tx_thr_len_bit_pos)
#define USB_DTHRCTL_rx_thr_en               (1 << 16)
#define USB_DTHRCTL_rx_thr_len_bit_pos      17
#define USB_DTHRCTL_rx_thr_len_bits         (0x1FF << USB_DTHRCTL_rx_thr_len_bit_pos)

/* 0<=ep<=15, you can use ep=0 */
/** Device IN Endpoint (ep) Control Register */
#define USB_DIEPCTL(ep)     (*(volatile unsigned long *)(USB_DEVICE + 0x100 + (ep) * 0x20))
/** Device IN Endpoint (ep) Interrupt Register */
#define USB_DIEPINT(ep)     (*(volatile unsigned long *)(USB_DEVICE + 0x100 + (ep) * 0x20 + 0x8))
/** Device IN Endpoint (ep) Transfer Size Register */
#define USB_DIEPTSIZ(ep)    (*(volatile unsigned long *)(USB_DEVICE + 0x100 + (ep) * 0x20 + 0x10))
/** Device IN Endpoint (ep) DMA Address Register */
#define USB_DIEPDMA(ep)     (*(volatile unsigned long *)(USB_DEVICE + 0x100 + (ep) * 0x20 + 0x14))
/** Device IN Endpoint (ep) Transmit FIFO Status Register */
#define USB_DTXFSTS(ep)     (*(volatile unsigned long *)(USB_DEVICE + 0x100 + (ep) * 0x20 + 0x18))

/* the following also apply to DIEPMSK */
#define USB_DIEPINT_xfercompl       (1 << 0) /** Transfer complete */
#define USB_DIEPINT_epdisabled      (1 << 1) /** Endpoint disabled */
#define USB_DIEPINT_ahberr          (1 << 2) /** AHB error */
#define USB_DIEPINT_timeout         (1 << 3) /** Timeout handshake (non-iso TX) */
#define USB_DIEPINT_intktxfemp      (1 << 4) /** IN token received with tx fifo empty */
#define USB_DIEPINT_intknepmis      (1 << 5) /** IN token received with ep mismatch */
#define USB_DIEPINT_inepnakeff      (1 << 6) /** IN endpoint NAK effective */
#define USB_DIEPINT_emptyintr       (1 << 7) /** linux doc broken on this, empty fifo ? */
#define USB_DIEPINT_txfifoundrn     (1 << 8) /** linux doc void on this, tx fifo underrun ? */

/* the following also apply to DOEPMSK */
#define USB_DOEPINT_xfercompl       (1 << 0) /** Transfer complete */
#define USB_DOEPINT_epdisabled      (1 << 1) /** Endpoint disabled */
#define USB_DOEPINT_ahberr          (1 << 2) /** AHB error */
#define USB_DOEPINT_setup           (1 << 3) /** Setup Phase Done (control EPs)*/

/* 0<=ep<=15, you can use ep=0 */
/** Device OUT Endpoint (ep) Control Register */
#define USB_DOEPCTL(ep)     (*(volatile unsigned long *)(USB_DEVICE + 0x300 + (ep) * 0x20))
/** Device OUT Endpoint (ep) Frame number Register */
#define USB_DOEPFN(ep)      (*(volatile unsigned long *)(USB_DEVICE + 0x300 + (ep) * 0x20 + 0x4))
/** Device Endpoint (ep) Interrupt Register */
#define USB_DOEPINT(ep)     (*(volatile unsigned long *)(USB_DEVICE + 0x300 + (ep) * 0x20 + 0x8))
/** Device OUT Endpoint (ep) Transfer Size Register */
#define USB_DOEPTSIZ(ep)    (*(volatile unsigned long *)(USB_DEVICE + 0x300 + (ep) * 0x20 + 0x10))
/** Device Endpoint (ep) DMA Address Register */
#define USB_DOEPDMA(ep)     (*(volatile unsigned long *)(USB_DEVICE + 0x300 + (ep) * 0x20 + 0x14))

#define USB_PCGCCTL         (*(volatile unsigned long *)(USB_BASE + 0xE00)) /** Power and Clock Gating Control Register */


/** Maximum Packet Size
 * IN/OUT EPn
 * IN/OUT EP0 - 2 bits
 *  2'b00: 64 Bytes
 *  2'b01: 32
 *  2'b10: 16
 *  2'b11: 8 */
#define USB_DEPCTL_mps_bits         0x7ff
#define USB_DEPCTL_mps_bit_pos      0
#define USB_DEPCTL_MPS_64           0
#define USB_DEPCTL_MPS_32           1
#define USB_DEPCTL_MPS_16           2
#define USB_DEPCTL_MPS_8            3
/** Next Endpoint
 * IN EPn/IN EP0
 * OUT EPn/OUT EP0 - reserved */
#define USB_DEPCTL_nextep_bit_pos   11
#define USB_DEPCTL_nextep_bits      (0xf << USB_DEPCTL_nextep_bit_pos)
#define USB_DEPCTL_usbactep         (1 << 15) /** USB Active Endpoint */
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
#define USB_DEPCTL_dpid             (1 << 16)
#define USB_DEPCTL_naksts           (1 << 17) /** NAK Status */
/** Endpoint Type
 *  2'b00: Control
 *  2'b01: Isochronous
 *  2'b10: Bulk
 *  2'b11: Interrupt */
#define USB_DEPCTL_eptype_bit_pos   18
#define USB_DEPCTL_eptype_bits      (0x3 << USB_DEPCTL_eptype_bit_pos)
/** Snoop Mode
 * OUT EPn/OUT EP0
 * IN EPn/IN EP0 - reserved */
#define USB_DEPCTL_snp              (1 << 20)
#define USB_DEPCTL_stall            (1 << 21) /** Stall Handshake */
/** Tx Fifo Number
 * IN EPn/IN EP0
 * OUT EPn/OUT EP0 - reserved */
#define USB_DEPCTL_txfnum_bit_pos   22
#define USB_DEPCTL_txfnum_bits      (0xf << USB_DEPCTL_txfnum_bit_pos)

#define USB_DEPCTL_cnak             (1 << 26) /** Clear NAK */
#define USB_DEPCTL_snak             (1 << 27) /** Set NAK */
/** Set DATA0 PID (INTR/Bulk IN and OUT endpoints)
 * Writing to this field sets the Endpoint DPID (DPID)
 * field in this register to DATA0. Set Even
 * (micro)frame (SetEvenFr) (ISO IN and OUT Endpoints)
 * Writing to this field sets the Even/Odd
 * (micro)frame (EO_FrNum) field to even (micro)
 * frame.
 */
#define USB_DEPCTL_setd0pid         (1 << 28)
/** Set DATA1 PID (INTR/Bulk IN and OUT endpoints)
 * Writing to this field sets the Endpoint DPID (DPID)
 * field in this register to DATA1 Set Odd
 * (micro)frame (SetOddFr) (ISO IN and OUT Endpoints)
 * Writing to this field sets the Even/Odd
 * (micro)frame (EO_FrNum) field to odd (micro) frame.
 */
#define USB_DEPCTL_setd1pid         (1 << 29)
#define USB_DEPCTL_epdis            (1 << 30) /** Endpoint disable */
#define USB_DEPCTL_epena            (1 << 31) /** Endpoint enable */


/* valid for any D{I,O}EPTSIZi with 1<=i<=15, NOT for i=0 ! */
#define USB_DEPTSIZ_xfersize_bits   0x7ffff /** Transfer Size */
#define USB_DEPTSIZ_pkcnt_bit_pos   19 /** Packet Count */
#define USB_DEPTSIZ_pkcnt_bits      (0x3ff << USB_DEPTSIZ_pkcnt_bit_pos)
#define USB_DEPTSIZ_mc_bit_pos      29 /** Multi Count - Periodic IN endpoints */
#define USB_DEPTSIZ_mc_bits         (0x3 << USB_DEPTSIZ_mc_bit_pos)

/* idem but for i=0 */
#define USB_DEPTSIZ0_xfersize_bits   0x7f /** Transfer Size */
#define USB_DEPTSIZ0_pkcnt_bit_pos   19 /** Packet Count */
#define USB_DEPTSIZ0_pkcnt_bits      (0x1 << USB_DEPTSIZ0_pkcnt_bit_pos)
#define USB_DEPTSIZ0_supcnt_bit_pos  29 /** Setup Packet Count (DOEPTSIZ0 Only) */
#define USB_DEPTSIZ0_supcnt_bits     (0x3 << USB_DEPTSIZ0_supcnt_bit_pos)

/* valid for USB_DAINT and USB_DAINTMSK, for 0<=ep<=15 */
#define USB_DAINT_IN_EP(i)      (1 << (i))
#define USB_DAINT_OUT_EP(i)     (1 << ((i) + 16))

/**
 * Parameters
 */
/* Number of IN/OUT endpoints */
#define USB_NUM_IN_EP           3
#define USB_NUM_OUT_EP          2

/* List of IN enpoints */
#define USB_IN_EP_LIST          1, 3, 5
#define USB_OUT_EP_LIST         2, 4

#endif /* __USB_DRV_AS3525v2_H__ */
