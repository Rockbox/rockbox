/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2014 Michael Sparmann
 * Copyright (C) 2014 by Marcin Bukat
 * Copyright (C) 2016 by Cástor Muñoz
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
#ifndef __USB_DESIGNWARE_H__
#define __USB_DESIGNWARE_H__

#include <inttypes.h>
#include "config.h"

#ifndef REG32_PTR_T
#define REG32_PTR_T volatile uint32_t *
#endif

/* Global registers */
#define DWC_GOTGCTL         (*((REG32_PTR_T)(OTGBASE + 0x00)))
#define DWC_GOTGINT         (*((REG32_PTR_T)(OTGBASE + 0x04)))
#define DWC_GAHBCFG         (*((REG32_PTR_T)(OTGBASE + 0x08)))
    #define PTXFELVL        (1<<8)
    #define TXFELVL         (1<<7)
    #define DMAEN           (1<<5)
    #define HBSTLEN(x)      ((x)<<1)
        #define HBSTLEN_SINGLE    0
        #define HBSTLEN_INCR      1
        #define HBSTLEN_INCR4     3
        #define HBSTLEN_INCR8     5
        #define HBSTLEN_INCR16    7
    #define GINT            (1<<0)

#define DWC_GUSBCFG         (*((REG32_PTR_T)(OTGBASE + 0x0c)))
    #define FDMOD           (1<<30)
    #define TRDT(x)         ((x)<<10)
    #define DDRSEL          (1<<7)
    #define PHSEL           (1<<6)
    #define FSINTF          (1<<5)
    #define ULPI_UTMI_SEL   (1<<4)
    #define PHYIF16         (1<<3)

#define DWC_GRSTCTL         (*((REG32_PTR_T)(OTGBASE + 0x10)))
    #define AHBIDL          (1<<31)
    #define TXFNUM(x)       ((x)<<6)
    #define TXFFLSH         (1<<5)
    #define RXFFLSH         (1<<4)
    #define CSRST           (1<<0)

#define DWC_GINTSTS         (*((REG32_PTR_T)(OTGBASE + 0x14)))
    #define WKUINT          (1<<31)
    #define SRQINT          (1<<30)
    #define DISCINT         (1<<29)
    #define CIDSCHG         (1<<28)
    #define PTXFE           (1<<26)
    #define HCINT           (1<<25)
    #define HPRTINT         (1<<24)
    #define FETSUSP         (1<<22)
    #define IPXFR           (1<<21)
    #define IISOIXFR        (1<<20)
    #define OEPINT          (1<<19)
    #define IEPINT          (1<<18)
    #define EPMIS           (1<<17)
    #define EOPF            (1<<15)
    #define ISOODPR         (1<<14)
    #define ENUMDNE         (1<<13)
    #define USBRST          (1<<12)
    #define USBSUSP         (1<<11)
    #define ESUSP           (1<<10)
    #define GOUTNAKEFF      (1<<7)
    #define GINAKEFF        (1<<6)
    #define NPTXFE          (1<<5)
    #define RXFLVL          (1<<4)
    #define SOF             (1<<3)
    #define OTGINT          (1<<2)
    #define MMIS            (1<<1)
    #define CMOD            (1<<0)

#define DWC_GINTMSK         (*((REG32_PTR_T)(OTGBASE + 0x18)))
#define DWC_GRXSTSR         (*((REG32_PTR_T)(OTGBASE + 0x1c)))
#define DWC_GRXSTSP         (*((REG32_PTR_T)(OTGBASE + 0x20)))
    #define PKTSTS_GLOBALOUTNAK     1
    #define PKTSTS_OUTRX            2
    #define PKTSTS_HCHIN            2
    #define PKTSTS_OUTDONE          3
    #define PKTSTS_HCHIN_XFER_COMP  3
    #define PKTSTS_SETUPDONE        4
    #define PKTSTS_DATATOGGLEERR    5
    #define PKTSTS_SETUPRX          6
    #define PKTSTS_HCHHALTED        7

#define DWC_GRXFSIZ         (*((REG32_PTR_T)(OTGBASE + 0x24)))
#ifdef USB_DW_SHARED_FIFO
#define DWC_GNPTXFSIZ       (*((REG32_PTR_T)(OTGBASE + 0x28)))
#else
#define DWC_TX0FSIZ         (*((REG32_PTR_T)(OTGBASE + 0x28)))
#endif
#define DWC_GNPTXSTS        (*((REG32_PTR_T)(OTGBASE + 0x2c)))
#define DWC_GI2CCTL         (*((REG32_PTR_T)(OTGBASE + 0x30)))
/* reserved */
#define DWC_GCCFG           (*((REG32_PTR_T)(OTGBASE + 0x38)))
    #define NOVBUSSENS      (1<<21)
    #define SOFOUTEN        (1<<20)
    #define VBUSBSEN        (1<<19)
    #define VBUSASEN        (1<<18)
    #define I2CPADEN        (1<<17)
    #define PWRDWN          (1<<16)

#define DWC_CID             (*((REG32_PTR_T)(OTGBASE + 0x3c)))
#define DWC_GSNPSID         (*((REG32_PTR_T)(OTGBASE + 0x40)))
#define DWC_GHWCFG1         (*((REG32_PTR_T)(OTGBASE + 0x44)))
#define DWC_GHWCFG2         (*((REG32_PTR_T)(OTGBASE + 0x48)))
#define DWC_GHWCFG3         (*((REG32_PTR_T)(OTGBASE + 0x4c)))
#define DWC_GHWCFG4         (*((REG32_PTR_T)(OTGBASE + 0x50)))
#define DWC_GLPMCFG         (*((REG32_PTR_T)(OTGBASE + 0x54)))
#define DWC_HPTXFSIZ        (*((REG32_PTR_T)(OTGBASE + 0x100)))
#define DWC_DIEPTXF(x)      (*((REG32_PTR_T)(OTGBASE + 0x104 + 4*(x)))) /*0..15*/

/* Host mode registers */
#define DWC_HCFG            (*((REG32_PTR_T)(OTGBASE + 0x400)))
#define DWC_HFIR            (*((REG32_PTR_T)(OTGBASE + 0x404)))
#define DWC_HFNUM           (*((REG32_PTR_T)(OTGBASE + 0x408)))
/* reserved */
#define DWC_HPTXSTS         (*((REG32_PTR_T)(OTGBASE + 0x410)))
#define DWC_HAINT           (*((REG32_PTR_T)(OTGBASE + 0x414)))
#define DWC_HAINTMSK        (*((REG32_PTR_T)(OTGBASE + 0x418)))
#define DWC_HPRT            (*((REG32_PTR_T)(OTGBASE + 0x440)))
#define DWC_HCCHAR(x)       (*((REG32_PTR_T)(OTGBASE + 0x500 + 0x20*(x))))
#define DWC_HCSPLT(x)       (*((REG32_PTR_T)(OTGBASE + 0x504 + 0x20*(x))))
#define DWC_HCINT(x)        (*((REG32_PTR_T)(OTGBASE + 0x508 + 0x20*(x))))
#define DWC_HCINTMSK(x)     (*((REG32_PTR_T)(OTGBASE + 0x50c + 0x20*(x))))
#define DWC_HCTSIZ(x)       (*((REG32_PTR_T)(OTGBASE + 0x510 + 0x20*(x))))
#define DWC_HCDMA(x)        (*((REG32_PTR_T)(OTGBASE + 0x514 + 0x20*(x))))

/* Device mode registers */
#define DWC_DCFG            (*((REG32_PTR_T)(OTGBASE + 0x800)))
    #define EPMISCNT(x)     ((x)<<18)
    #define DAD(x)          ((x)<<4)
    #define NZLSOHSK        (1<<2)

#define DWC_DCTL            (*((REG32_PTR_T)(OTGBASE + 0x804)))
    #define POPRGDNE        (1<<11)
    #define CGONAK          (1<<10)
    #define SGONAK          (1<<9)
    #define CGINAK          (1<<8)
    #define SGINAK          (1<<7)
    #define TCTL(x)         ((x)<<4)
    #define GONSTS          (1<<3)
    #define GINSTS          (1<<2)
    #define SDIS            (1<<1)
    #define RWUSIG          (1<<0)

#define DWC_DSTS            (*((REG32_PTR_T)(OTGBASE + 0x808)))
/* reserved */
#define DWC_DIEPMSK         (*((REG32_PTR_T)(OTGBASE + 0x810)))
#define DWC_DOEPMSK         (*((REG32_PTR_T)(OTGBASE + 0x814)))
#define DWC_DAINT           (*((REG32_PTR_T)(OTGBASE + 0x818)))
#define DWC_DAINTMSK        (*((REG32_PTR_T)(OTGBASE + 0x81c)))
/* reserved */
#define DWC_DVBUSDIS        (*((REG32_PTR_T)(OTGBASE + 0x828)))
#define DWC_DVBUSPULSE      (*((REG32_PTR_T)(OTGBASE + 0x82c)))

#ifdef USB_DW_SHARED_FIFO
#define DWC_DTKNQR1         (*((REG32_PTR_T)(OTGBASE + 0x820)))
#define DWC_DTKNQR2         (*((REG32_PTR_T)(OTGBASE + 0x824)))
#define DWC_DTKNQR3         (*((REG32_PTR_T)(OTGBASE + 0x830)))
#define DWC_DTKNQR4         (*((REG32_PTR_T)(OTGBASE + 0x834)))

#else /* !USB_DW_SHARED_FIFO */
#define DWC_DTHRCTL         (*((REG32_PTR_T)(OTGBASE + 0x830)))
    #define ARPEN           (1<<27)
    #define RXTHRLEN(x)     ((x)<<17)
    #define RXTHREN         (1<<16)
    #define TXTHRLEN(x)     ((x)<<2)
    #define ISOTHREN        (1<<1)
    #define NONISOTHREN     (1<<0)

#define DWC_DIEPEMPMSK      (*((REG32_PTR_T)(OTGBASE + 0x834)))
#define DWC_DEACHINT        (*((REG32_PTR_T)(OTGBASE + 0x838)))
#define DWC_DEACHINTMSK     (*((REG32_PTR_T)(OTGBASE + 0x83c)))
#define DWC_DIEPEACHMSK(x)  (*((REG32_PTR_T)(OTGBASE + 0x840 + 4*(x))))
#define DWC_DOEPEACHMSK(x)  (*((REG32_PTR_T)(OTGBASE + 0x880 + 4*(x))))
#endif

#define DWC_DIEPCTL(x)      (*((REG32_PTR_T)(OTGBASE + 0x900 + 0x20*(x))))
#define DWC_DOEPCTL(x)      (*((REG32_PTR_T)(OTGBASE + 0xb00 + 0x20*(x))))
    #define EPENA           (1<<31)
    #define EPDIS           (1<<30)
    #define SD0PID          (1<<28)
    #define SNAK            (1<<27)
    #define CNAK            (1<<26)
    #define DTXFNUM(x)      ((x)<<22)
    #define STALL           (1<<21)
    #define EPTYP(x)        ((x)<<18)
        #define EPTYP_CONTROL       0
        #define EPTYP_ISOCHRONOUS   1
        #define EPTYP_BULK          2
        #define EPTYP_INTERRUPT     3
    #define NAKSTS          (1<<17)
    #define USBAEP          (1<<15)
    #define NEXTEP(x)       ((x)<<11)

#define DWC_DIEPINT(x)      (*((REG32_PTR_T)(OTGBASE + 0x908 + 0x20*(x))))
#define DWC_DOEPINT(x)      (*((REG32_PTR_T)(OTGBASE + 0xb08 + 0x20*(x))))
    #define TXFE            (1<<7)  /* IN */
    #define INEPNE          (1<<6)  /* IN */
    #define ITEPMIS         (1<<5)  /* IN */
    #define ITTXFE          (1<<4)  /* IN */
    #define OTEPDIS         (1<<4)  /* OUT */
    #define TOC             (1<<3)  /* control IN */
    #define STUP            (1<<3)  /* control OUT */
    #define AHBERR          (1<<2)
    #define EPDISD          (1<<1)
    #define XFRC            (1<<0)

#define DWC_DIEPTSIZ(x)     (*((REG32_PTR_T)(OTGBASE + 0x910 + 0x20*(x))))
#define DWC_DOEPTSIZ(x)     (*((REG32_PTR_T)(OTGBASE + 0xb10 + 0x20*(x))))
    #define MCCNT(x)        ((x)<<29)  /* IN */
    #define STUPCNT(x)      ((x)<<29)  /* control OUT */
    #define RXDPID(x)       ((x)<<29)  /* isochronous OUT */
    #define PKTCNT(x)       ((x)<<19)
    #define XFERSIZE(x)     ((x)<<0)

#define DWC_DIEPDMA(x)      (*((REG32_PTR_T)(OTGBASE + 0x914 + 0x20*(x))))
#define DWC_DOEPDMA(x)      (*((REG32_PTR_T)(OTGBASE + 0xb14 + 0x20*(x))))

#define DWC_DTXFSTS(x)      (*((REG32_PTR_T)(OTGBASE + 0x918 + 0x20*(x))))
#define DWC_PCGCCTL         (*((REG32_PTR_T)(OTGBASE + 0xe00)))
#define DWC_DFIFO(x)        (*((REG32_PTR_T)(OTGBASE + 0x1000 + 0x1000*(x))))

/* Device mode registers by (epnum,epdir), d==0 -> IN */
#define DWC_EPCTL(n,d)      (*((REG32_PTR_T)(OTGBASE + 0x900 + 0x200*(d) + 0x20*(n))))
#define DWC_EPINT(n,d)      (*((REG32_PTR_T)(OTGBASE + 0x908 + 0x200*(d) + 0x20*(n))))
#define DWC_EPTSIZ(n,d)     (*((REG32_PTR_T)(OTGBASE + 0x910 + 0x200*(d) + 0x20*(n))))
#define DWC_EPDMA(n,d)      (*((REG32_PTR_T)(OTGBASE + 0x914 + 0x200*(d) + 0x20*(n))))


/* HS PHY/interface configuration */
#define DWC_PHYTYPE_UTMI_8       (0)
#define DWC_PHYTYPE_UTMI_16      (PHYIF16)
#define DWC_PHYTYPE_ULPI_SDR     (ULPI_UTMI_SEL)
#define DWC_PHYTYPE_ULPI_DDR     (ULPI_UTMI_SEL|DDRSEL)

/* configure USB OTG capabilities on SoC */
struct usb_dw_config
{
    uint8_t phytype;  /* DWC_PHYTYPE_ */

    uint16_t rx_fifosz;
    uint16_t nptx_fifosz;
    uint16_t ptx_fifosz;
#ifdef USB_DW_SHARED_FIFO
    bool use_ptxfifo_as_plain_buffer;
#endif
#ifdef USB_DW_ARCH_SLAVE
    bool disable_double_buffering;
#else
    uint8_t ahb_burst_len;  /* HBSTLEN_ */
#ifndef USB_DW_SHARED_FIFO
    uint8_t ahb_threshold;
#endif
#endif
};

extern const struct usb_dw_config usb_dw_config;

extern void usb_dw_target_enable_clocks(void);
extern void usb_dw_target_disable_clocks(void);
extern void usb_dw_target_enable_irq(void);
extern void usb_dw_target_disable_irq(void);
extern void usb_dw_target_clear_irq(void);

#endif /* __USB_DESIGNWARE_H__ */
