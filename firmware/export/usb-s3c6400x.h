/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2009 Michael Sparmann
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
#ifndef USB_S3C6400X_H
#define USB_S3C6400X_H


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


/*** OTG PHY CONTROL REGISTERS ***/
#define OPHYPWR     (*((uint32_t volatile*)(PHYBASE + 0x000)))
#define OPHYCLK     (*((uint32_t volatile*)(PHYBASE + 0x004)))
#define ORSTCON     (*((uint32_t volatile*)(PHYBASE + 0x008)))
#define OPHYUNK3    (*((uint32_t volatile*)(PHYBASE + 0x018)))
#define OPHYUNK1    (*((uint32_t volatile*)(PHYBASE + 0x01c)))
#define OPHYUNK2    (*((uint32_t volatile*)(PHYBASE + 0x044)))

/*** OTG LINK CORE REGISTERS ***/
/* Core Global Registers */

/** OTG Control and Status Register */
#define GOTGCTL     (*((uint32_t volatile*)(OTGBASE + 0x000)))

/** OTG Interrupt Register */
#define GOTGINT     (*((uint32_t volatile*)(OTGBASE + 0x004)))

/** Core AHB Configuration Register */
#define GAHBCFG     (*((uint32_t volatile*)(OTGBASE + 0x008)))
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
#define GUSBCFG     (*((uint32_t volatile*)(OTGBASE + 0x00C)))
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
#define GRSTCTL     (*((uint32_t volatile*)(OTGBASE + 0x010)))
#define GRSTCTL_csftrst         (1 << 0) /** Core soft reset */
#define GRSTCTL_hsftrst         (1 << 1) /** Hclk soft reset */
#define GRSTCTL_intknqflsh      (1 << 3) /** In Token Sequence Learning Queue Flush */
#define GRSTCTL_rxfflsh_flush   (1 << 4) /** RxFIFO Flush */
#define GRSTCTL_txfflsh_flush   (1 << 5) /** TxFIFO Flush */
#define GRSTCTL_txfnum_bitp     6 /** TxFIFO Number */
#define GRSTCTL_txfnum_bits     0x1f
#define GRSTCTL_ahbidle         (1 << 31) /** AHB idle state*/

/** Core Interrupt Register */
#define GINTSTS     (*((uint32_t volatile*)(OTGBASE + 0x014)))
/* NOTE: GINTSTS bits are the same as in GINTMSK plus this one */
#define GINTSTS_curmode     (1 << 0) /** Current mode, 0 for device */

/** Core Interrupt Mask Register */
#define GINTMSK     (*((uint32_t volatile*)(OTGBASE + 0x018)))
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
#define GRXSTSR     (*((uint32_t volatile*)(OTGBASE + 0x01C)))

/** Receive Status Read /Pop Register (Read Only) */
#define GRXSTSP     (*((uint32_t volatile*)(OTGBASE + 0x020)))

/** Receive FIFO Size Register */
#define GRXFSIZ     (*((uint32_t volatile*)(OTGBASE + 0x024)))

/** Periodic Transmit FIFO Size Register */
#define GNPTXFSIZ   (*((uint32_t volatile*)(OTGBASE + 0x028)))
#define MAKE_FIFOSIZE_DATA(depth) ((depth) | ((depth) << 16))

/** Non-Periodic Transmit FIFO/Queue Status Register */
#define GNPTXSTS    (*((uint32_t volatile*)(OTGBASE + 0x02C)))

/** Device IN Endpoint Transmit FIFO (ep) Size Register */
/* 1<=ep<=15, don't use ep=0 !!! */
#define HPTXFSIZ    (*((uint32_t volatile*)(OTGBASE + 0x100)))
#define DPTXFSIZ(x) (*((uint32_t volatile*)(OTGBASE + 0x100 + 4 * (x))))

/*** HOST MODE REGISTERS ***/
/* Host Global Registers */
#define HCFG        (*((uint32_t volatile*)(OTGBASE + 0x400)))
#define HFIR        (*((uint32_t volatile*)(OTGBASE + 0x404)))
#define HFNUM       (*((uint32_t volatile*)(OTGBASE + 0x408)))
#define HPTXSTS     (*((uint32_t volatile*)(OTGBASE + 0x410)))
#define HAINT       (*((uint32_t volatile*)(OTGBASE + 0x414)))
#define HAINTMSK    (*((uint32_t volatile*)(OTGBASE + 0x418)))

/* Host Port Control and Status Registers */
#define HPRT        (*((uint32_t volatile*)(OTGBASE + 0x440)))

/* Host Channel-Specific Registers */
#define HCCHAR(x)   (*((uint32_t volatile*)(OTGBASE + 0x500 + 0x20 * (x))))
#define HCSPLT(x)   (*((uint32_t volatile*)(OTGBASE + 0x504 + 0x20 * (x))))
#define HCINT(x)    (*((uint32_t volatile*)(OTGBASE + 0x508 + 0x20 * (x))))
#define HCINTMSK(x) (*((uint32_t volatile*)(OTGBASE + 0x50C + 0x20 * (x))))
#define HCTSIZ(x)   (*((uint32_t volatile*)(OTGBASE + 0x510 + 0x20 * (x))))
#define HCDMA(x)    (*((uint32_t volatile*)(OTGBASE + 0x514 + 0x20 * (x))))
#define HCCHAR0     (*((uint32_t volatile*)(OTGBASE + 0x500)))
#define HCSPLT0     (*((uint32_t volatile*)(OTGBASE + 0x504)))
#define HCINT0      (*((uint32_t volatile*)(OTGBASE + 0x508)))
#define HCINTMSK0   (*((uint32_t volatile*)(OTGBASE + 0x50C)))
#define HCTSIZ0     (*((uint32_t volatile*)(OTGBASE + 0x510)))
#define HCDMA0      (*((uint32_t volatile*)(OTGBASE + 0x514)))
#define HCCHAR1     (*((uint32_t volatile*)(OTGBASE + 0x520)))
#define HCSPLT1     (*((uint32_t volatile*)(OTGBASE + 0x524)))
#define HCINT1      (*((uint32_t volatile*)(OTGBASE + 0x528)))
#define HCINTMSK1   (*((uint32_t volatile*)(OTGBASE + 0x52C)))
#define HCTSIZ1     (*((uint32_t volatile*)(OTGBASE + 0x530)))
#define HCDMA1      (*((uint32_t volatile*)(OTGBASE + 0x534)))
#define HCCHAR2     (*((uint32_t volatile*)(OTGBASE + 0x540)))
#define HCSPLT2     (*((uint32_t volatile*)(OTGBASE + 0x544)))
#define HCINT2      (*((uint32_t volatile*)(OTGBASE + 0x548)))
#define HCINTMSK2   (*((uint32_t volatile*)(OTGBASE + 0x54C)))
#define HCTSIZ2     (*((uint32_t volatile*)(OTGBASE + 0x550)))
#define HCDMA2      (*((uint32_t volatile*)(OTGBASE + 0x554)))
#define HCCHAR3     (*((uint32_t volatile*)(OTGBASE + 0x560)))
#define HCSPLT3     (*((uint32_t volatile*)(OTGBASE + 0x564)))
#define HCINT3      (*((uint32_t volatile*)(OTGBASE + 0x568)))
#define HCINTMSK3   (*((uint32_t volatile*)(OTGBASE + 0x56C)))
#define HCTSIZ3     (*((uint32_t volatile*)(OTGBASE + 0x570)))
#define HCDMA3      (*((uint32_t volatile*)(OTGBASE + 0x574)))
#define HCCHAR4     (*((uint32_t volatile*)(OTGBASE + 0x580)))
#define HCSPLT4     (*((uint32_t volatile*)(OTGBASE + 0x584)))
#define HCINT4      (*((uint32_t volatile*)(OTGBASE + 0x588)))
#define HCINTMSK4   (*((uint32_t volatile*)(OTGBASE + 0x58C)))
#define HCTSIZ4     (*((uint32_t volatile*)(OTGBASE + 0x590)))
#define HCDMA4      (*((uint32_t volatile*)(OTGBASE + 0x594)))
#define HCCHAR5     (*((uint32_t volatile*)(OTGBASE + 0x5A0)))
#define HCSPLT5     (*((uint32_t volatile*)(OTGBASE + 0x5A4)))
#define HCINT5      (*((uint32_t volatile*)(OTGBASE + 0x5A8)))
#define HCINTMSK5   (*((uint32_t volatile*)(OTGBASE + 0x5AC)))
#define HCTSIZ5     (*((uint32_t volatile*)(OTGBASE + 0x5B0)))
#define HCDMA5      (*((uint32_t volatile*)(OTGBASE + 0x5B4)))
#define HCCHAR6     (*((uint32_t volatile*)(OTGBASE + 0x5C0)))
#define HCSPLT6     (*((uint32_t volatile*)(OTGBASE + 0x5C4)))
#define HCINT6      (*((uint32_t volatile*)(OTGBASE + 0x5C8)))
#define HCINTMSK6   (*((uint32_t volatile*)(OTGBASE + 0x5CC)))
#define HCTSIZ6     (*((uint32_t volatile*)(OTGBASE + 0x5D0)))
#define HCDMA6      (*((uint32_t volatile*)(OTGBASE + 0x5D4)))
#define HCCHAR7     (*((uint32_t volatile*)(OTGBASE + 0x5E0)))
#define HCSPLT7     (*((uint32_t volatile*)(OTGBASE + 0x5E4)))
#define HCINT7      (*((uint32_t volatile*)(OTGBASE + 0x5E8)))
#define HCINTMSK7   (*((uint32_t volatile*)(OTGBASE + 0x5EC)))
#define HCTSIZ7     (*((uint32_t volatile*)(OTGBASE + 0x5F0)))
#define HCDMA7      (*((uint32_t volatile*)(OTGBASE + 0x5F4)))
#define HCCHAR8     (*((uint32_t volatile*)(OTGBASE + 0x600)))
#define HCSPLT8     (*((uint32_t volatile*)(OTGBASE + 0x604)))
#define HCINT8      (*((uint32_t volatile*)(OTGBASE + 0x608)))
#define HCINTMSK8   (*((uint32_t volatile*)(OTGBASE + 0x60C)))
#define HCTSIZ8     (*((uint32_t volatile*)(OTGBASE + 0x610)))
#define HCDMA8      (*((uint32_t volatile*)(OTGBASE + 0x614)))
#define HCCHAR9     (*((uint32_t volatile*)(OTGBASE + 0x620)))
#define HCSPLT9     (*((uint32_t volatile*)(OTGBASE + 0x624)))
#define HCINT9      (*((uint32_t volatile*)(OTGBASE + 0x628)))
#define HCINTMSK9   (*((uint32_t volatile*)(OTGBASE + 0x62C)))
#define HCTSIZ9     (*((uint32_t volatile*)(OTGBASE + 0x630)))
#define HCDMA9      (*((uint32_t volatile*)(OTGBASE + 0x634)))
#define HCCHAR10    (*((uint32_t volatile*)(OTGBASE + 0x640)))
#define HCSPLT10    (*((uint32_t volatile*)(OTGBASE + 0x644)))
#define HCINT10     (*((uint32_t volatile*)(OTGBASE + 0x648)))
#define HCINTMSK10  (*((uint32_t volatile*)(OTGBASE + 0x64C)))
#define HCTSIZ10    (*((uint32_t volatile*)(OTGBASE + 0x650)))
#define HCDMA10     (*((uint32_t volatile*)(OTGBASE + 0x654)))
#define HCCHAR11    (*((uint32_t volatile*)(OTGBASE + 0x660)))
#define HCSPLT11    (*((uint32_t volatile*)(OTGBASE + 0x664)))
#define HCINT11     (*((uint32_t volatile*)(OTGBASE + 0x668)))
#define HCINTMSK11  (*((uint32_t volatile*)(OTGBASE + 0x66C)))
#define HCTSIZ11    (*((uint32_t volatile*)(OTGBASE + 0x670)))
#define HCDMA11     (*((uint32_t volatile*)(OTGBASE + 0x674)))
#define HCCHAR12    (*((uint32_t volatile*)(OTGBASE + 0x680)))
#define HCSPLT12    (*((uint32_t volatile*)(OTGBASE + 0x684)))
#define HCINT12     (*((uint32_t volatile*)(OTGBASE + 0x688)))
#define HCINTMSK12  (*((uint32_t volatile*)(OTGBASE + 0x68C)))
#define HCTSIZ12    (*((uint32_t volatile*)(OTGBASE + 0x690)))
#define HCDMA12     (*((uint32_t volatile*)(OTGBASE + 0x694)))
#define HCCHAR13    (*((uint32_t volatile*)(OTGBASE + 0x6A0)))
#define HCSPLT13    (*((uint32_t volatile*)(OTGBASE + 0x6A4)))
#define HCINT13     (*((uint32_t volatile*)(OTGBASE + 0x6A8)))
#define HCINTMSK13  (*((uint32_t volatile*)(OTGBASE + 0x6AC)))
#define HCTSIZ13    (*((uint32_t volatile*)(OTGBASE + 0x6B0)))
#define HCDMA13     (*((uint32_t volatile*)(OTGBASE + 0x6B4)))
#define HCCHAR14    (*((uint32_t volatile*)(OTGBASE + 0x6C0)))
#define HCSPLT14    (*((uint32_t volatile*)(OTGBASE + 0x6C4)))
#define HCINT14     (*((uint32_t volatile*)(OTGBASE + 0x6C8)))
#define HCINTMSK14  (*((uint32_t volatile*)(OTGBASE + 0x6CC)))
#define HCTSIZ14    (*((uint32_t volatile*)(OTGBASE + 0x6D0)))
#define HCDMA14     (*((uint32_t volatile*)(OTGBASE + 0x6D4)))
#define HCCHAR15    (*((uint32_t volatile*)(OTGBASE + 0x6E0)))
#define HCSPLT15    (*((uint32_t volatile*)(OTGBASE + 0x6E4)))
#define HCINT15     (*((uint32_t volatile*)(OTGBASE + 0x6E8)))
#define HCINTMSK15  (*((uint32_t volatile*)(OTGBASE + 0x6EC)))
#define HCTSIZ15    (*((uint32_t volatile*)(OTGBASE + 0x6F0)))
#define HCDMA15     (*((uint32_t volatile*)(OTGBASE + 0x6F4)))

/*** DEVICE MODE REGISTERS ***/
/* Device Global Registers */

/** Device Configuration Register */
#define DCFG        (*((uint32_t volatile*)(OTGBASE + 0x800)))
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
#define DCTL        (*((uint32_t volatile*)(OTGBASE + 0x804)))
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
#define DSTS        (*((uint32_t volatile*)(OTGBASE + 0x808)))
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
#define DIEPMSK     (*((uint32_t volatile*)(OTGBASE + 0x810)))
/* the following apply to DEPMSK and DEPINT */
#define DEPINT_xfercompl       (1 << 0) /** Transfer complete */
#define DEPINT_epdisabled      (1 << 1) /** Endpoint disabled */
#define DEPINT_ahberr          (1 << 2) /** AHB error */
#define DIEPINT_timeout         (1 << 3) /** Timeout handshake (non-iso TX) */
#define DOEPINT_setup           (1 << 3) /** Setup Phase Done (control EPs)*/
#define DIEPINT_intktxfemp      (1 << 4) /** IN token received with tx fifo empty */
#define DIEPINT_intknepmis      (1 << 5) /** IN token received with ep mismatch */
#define DIEPINT_inepnakeff      (1 << 6) /** IN endpoint NAK effective */
#define DIEPINT_emptyintr       (1 << 7) /** linux doc broken on this, empty fifo ? */
#define DIEPINT_txfifoundrn     (1 << 8) /** linux doc void on this, tx fifo underrun ? */

/** Device OUT Endpoint Common Interrupt Mask Register */
#define DOEPMSK     (*((uint32_t volatile*)(OTGBASE + 0x814)))

/** Device All Endpoints Interrupt Register */
#define DAINT       (*((uint32_t volatile*)(OTGBASE + 0x818)))
/* valid for DAINT and DAINTMSK, for 0<=ep<=15 */
#define DAINT_IN_EP(i)      (1 << (i))
#define DAINT_OUT_EP(i)     (1 << ((i) + 16))

/** Device Endpoints Interrupt Mask Register */
#define DAINTMSK    (*((uint32_t volatile*)(OTGBASE + 0x81C)))

/** Device IN Token Sequence Learning Queue Read Register 1 */
#define DTKNQR1     (*((uint32_t volatile*)(OTGBASE + 0x820)))

/** Device IN Token Sequence Learning Queue Register 2 */
#define DTKNQR2     (*((uint32_t volatile*)(OTGBASE + 0x824)))

/* fixme: those registers are not present in registers.h but are in dwc_otgh_regs.h.
 *        the previous registers exists but has a different name :( */
/** Device VBUS discharge register*/
#define DVBUSDIS    (*((uint32_t volatile*)(OTGBASE + 0x828)))

/** Device VBUS pulse register */
#define DVBUSPULSE  (*((uint32_t volatile*)(OTGBASE + 0x82C)))

// FIXME : 2 names for the same reg?
/** Device IN Token Queue Read Register 3 (RO) */
/** Device Thresholding control register */
#define DTKNQR3     (*((uint32_t volatile*)(OTGBASE + 0x830)))
#define DTHRCTL     (*((uint32_t volatile*)(OTGBASE + 0x830)))
#define DTHRCTL_non_iso_thr_en      (1 << 0)
#define DTHRCTL_iso_thr_en          (1 << 1)
#define DTHRCTL_tx_thr_len_bitp     2
#define DTHRCTL_tx_thr_len_bits     0x1FF
#define DTHRCTL_rx_thr_en           (1 << 16)
#define DTHRCTL_rx_thr_len_bitp     17
#define DTHRCTL_rx_thr_len_bits     0x1FF

/** Device IN Token Queue Read Register 4 (RO) */
#define DTKNQR4     (*((uint32_t volatile*)(OTGBASE + 0x834)))

/* Device Logical Endpoint-Specific Registers */
#define DEPCTL(x, out)  (*((uint32_t volatile*)(OTGBASE + 0x900 + ((!!out) * 0x200) + 0x20 * (x))))
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

/** Device Endpoint (ep) Transfer Size Register */
#define DEPTSIZ(x, out) (*((uint32_t volatile*)(OTGBASE + 0x910 + (0x200 * (!!out)) + 0x20 * (x))))
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


/** Device Endpoint (ep) Control Register */
#define DEPINT(x,out)  (*((uint32_t volatile*)(OTGBASE + 0x908 + (0x200 * (!!out)) + 0x20 * (x))))

/** Device Endpoint (ep) DMA Address Register */
#define DEPDMA(x,out)  (*((const void* volatile*)(OTGBASE + 0x914 + (0x200 * (!!out)) + 0x20 * (x))))

#if 0 /* Those are present in as3525v2, not s5l870x */
/** Device IN Endpoint (ep) Transmit FIFO Status Register */
#define DTXFSTS(ep) (*((const void* volatile*)(OTGBASE + 0x918 + 0x20 * (x))))

/** Device OUT Endpoint (ep) Frame number Register */
#define DOEPFN(ep)  (*((const void* volatile*)(OTGBASE + 0xB04 + 0x20 * (x))))
#endif

/* Power and Clock Gating Register */
#define PCGCCTL     (*((uint32_t volatile*)(OTGBASE + 0xE00)))

/** User HW Config1 Register */
#define GHWCFG1                 (*((uint32_t volatile*)(OTGBASE + 0x044)))
#define GHWCFG1_epdir_bitp(ep)  (2 * (ep))
#define GHWCFG1_epdir_bits      0x3
#define GHWCFG1_EPDIR_BIDIR     0
#define GHWCFG1_EPDIR_IN        1
#define GHWCFG1_EPDIR_OUT       2

/** User HW Config2 Register */
#define GHWCFG2                     (*((uint32_t volatile*)(OTGBASE + 0x048)))
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
#define GHWCFG3                 (*((uint32_t volatile*)(OTGBASE + 0x04C)))
#define GHWCFG3_dfifo_len_bitp  16 /** Total fifo size */
#define GHWCFG3_dfifo_len_bits  0xffff

/** User HW Config4 Register */
#define GHWCFG4                             (*((uint32_t volatile*)(OTGBASE + 0x050)))
#define GHWCFG4_utmi_phy_data_width_bitp    14 /** UTMI+ data bus width */
#define GHWCFG4_utmi_phy_data_width_bits    0x3
#define GHWCFG4_ded_fifo_en                 (1 << 25) /** Dedicated Tx FIFOs */
#define GHWCFG4_num_in_ep_bitp              26 /** Number of IN endpoints */
#define GHWCFG4_num_in_ep_bits              0xf

#endif /* USB_S3C6400X_H */
