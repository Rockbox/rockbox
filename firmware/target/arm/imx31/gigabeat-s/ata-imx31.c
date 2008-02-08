/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 by Will Robertson
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include "config.h"
#include "cpu.h"
#include "kernel.h"
#include "thread.h"
#include "system.h"
#include "power.h"
#include "panic.h"
#include "pcf50606.h"
#include "ata-target.h"

void ata_reset(void)
{
    ATA_INTF_CONTROL &= ~ATA_ATA_RST;
    sleep(1);
    ATA_INTF_CONTROL |= ATA_ATA_RST;
    sleep(1);

    while (!(ATA_INTERRUPT_PENDING & ATA_CONTROLLER_IDLE));
}

/* This function is called before enabling the USB bus */
void ata_enable(bool on)
{
    (void)on;
}

bool ata_is_coldstart(void)
{
    return true;
}

unsigned long get_pll(bool serial) {
    unsigned long mfi, mfn, mfd, pdf, ref_clk;
    unsigned long reg = 0, ccmr;
    unsigned long long temp;
    unsigned int prcs;

    ccmr = CLKCTL_CCMR;
    prcs = (ccmr & 0x6) >> 1;
    if(prcs == 0x1) {
        ref_clk = 32768 * 1024;
    } else {
        ref_clk = 27000000;
    }

    if(serial) {
        reg = CLKCTL_SPCTL;
    } else {
        if((ccmr & 0x8) == 0)
            return ref_clk;
        if((ccmr & 0x80) != 0)
            return ref_clk;
        reg = CLKCTL_MPCTL;
    }
    pdf = (reg & (0x7 << 26)) >> 26;
    mfd = (reg & (0x3FF << 16)) >> 16;
    mfi = (reg & (0xF << 10)) >> 10;
    mfi = (mfi <= 5) ? 5 : mfi;
    mfn = (reg & 0x3FF);

    if(mfn < 0x200) {
        temp = (unsigned long long)2 *ref_clk * mfn;
        temp /= (mfd + 1);
        temp = (unsigned long long)2 *ref_clk * mfi + temp;
        temp /= (pdf + 1);
    } else {
        temp = (unsigned long long)2 *ref_clk * (0x400 - mfn);
        temp /= (mfd + 1);
        temp = (unsigned long long)2 *ref_clk * mfi - temp;
        temp /= (pdf + 1);

    }
    return (unsigned long)temp;
}

unsigned long get_ata_clock(void) {
    unsigned long pll, ret_val, hclk, max_pdf, ipg_pdf, mcu_pdf;

    max_pdf = (CLKCTL_PDR0 & (0x7 << 3)) >> 3;
    ipg_pdf = (CLKCTL_PDR0 & (0x3 << 6)) >> 6;
    mcu_pdf = (CLKCTL_PDR0 & 0x7);
    if((CLKCTL_PMCR0 & 0xC0000000 ) == 0) {
        pll = get_pll(true);
    } else {
        pll = get_pll(false);
    }
    hclk = pll/(max_pdf + 1);
    ret_val = hclk / (ipg_pdf + 1);

    return ret_val;
}

void ata_device_init(void)
{
    ATA_INTF_CONTROL |= ATA_ATA_RST; /* Make sure we're not in reset mode */

    while (!(ATA_INTERRUPT_PENDING & ATA_CONTROLLER_IDLE));

    /* Setup the timing for PIO mode */
    int T = 1000 * 1000 * 1000 / get_ata_clock();
    ATA_TIME_OFF = 3;
    ATA_TIME_ON = 3;

    ATA_TIME_1 = (T + 70)/T;
    ATA_TIME_2W = (T + 290)/T;
    ATA_TIME_2R = (T + 290)/T;
    ATA_TIME_AX = (T + 50)/T;
    ATA_TIME_PIO_RDX = 1;
    ATA_TIME_4 = (T + 30)/T;
    ATA_TIME_9 = (T + 20)/T;
}

#if !defined(BOOTLOADER)
void copy_write_sectors(const unsigned char* buf, int wordcount)
{
    (void)buf; (void)wordcount;
}
#endif
